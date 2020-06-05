#include "SDAutomationTest.h"
#include "Misc/CommandLine.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "UObject/UObjectGlobals.h"

#include "SDAutomationTestMessagingInterface.h"
#include "SDTestHarnessModule.h"

FSDAutomationTest::FSDAutomationTest(const FString& InName, const bool bInComplexTask)
	: FAutomationTestBase(InName, bInComplexTask)
	, WeakMessagingInterface(nullptr)
{}

void FSDAutomationTest::TestStringEqual(const FString& Description, const FString& A, const FString& B, ESearchCase::Type SearchCase)
{
	if (SearchCase == ESearchCase::CaseSensitive)
	{
		if (!A.Equals(B, SearchCase))
		{
			AddError(FString::Printf(TEXT("%s: The two strings are not equal (checking case).\n  Actual: '%s'\nExpected: '%s'"), *Description, *A, *B), 1);
		}
	}
	else
	{
		if (!A.Equals(B, SearchCase))
		{
			AddError(FString::Printf(TEXT("%s: The two strings are not equal (ignoring case).\n  Actual: '%s'\nExpected: '%s'"), *Description, *A, *B), 1);
		}
	}
}

UWorld* FSDAutomationTest::CreateWorld()
{
	if (TestWorld != nullptr)
	{
		AddWarning(TEXT("CreateWorld() called multiple times within the same test"));
		return TestWorld;
	}

	const bool InformEngineOfWorld = true;
	TestWorld = UWorld::CreateWorld(EWorldType::Game, InformEngineOfWorld, TEXT("FSDAutomationTest::World"));
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(TestWorld);

	if (TestWorld == nullptr)
	{
		AddError(TEXT("CreateWorld() failed"));
		return nullptr;
	}
	
	FURL URL;
	TestWorld->InitializeActorsForPlay(URL);

	// Setting this bool allows all actors and components to initialize properly using
	// the expected PostInitProperties() / BeginPlay() flow, without manual intervention.
	TestWorld->bBegunPlay = true;
	
	return TestWorld;
}

#if WITH_SDTEST_EXTENSIONS
void FSDAutomationTest::PreRunTest()
{
	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		MessagingInterfacePtr->WriteStartTestMessage(TestName, SD::AutomationTestMessaging::ECaptureStandardOutput::No);
	}
	
	FAutomationTestBase::PreRunTest();
}

void FSDAutomationTest::PostRunTest()
{
	if (TestWorld)
	{
		for (TActorIterator<AActor> Iter(TestWorld); Iter; ++Iter)
		{
			(*Iter)->Destroy();
		}

		const bool InformEngineOfWorld = true;
		GEngine->DestroyWorldContext(TestWorld);
		TestWorld->DestroyWorld(InformEngineOfWorld);
		TestWorld = nullptr;

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
}
#endif

void FSDAutomationTest::AddError(const FString& InError, int32 StackOffset /*= 0*/)
{
	if (IsLoggingMessages) { return; }
	RecursionGuard guard(this); 

	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		MessagingInterfacePtr->WriteTestOutputMessage(TestName, InError, SD::AutomationTestMessaging::EStandardMessageType::Error);
	}

	FAutomationTestBase::AddError(InError, StackOffset);
}

void FSDAutomationTest::AddErrorS(const FString& InError, const FString& InFilename, int32 InLineNumber)
{
	if (IsLoggingMessages) { return; }
	RecursionGuard guard(this);

	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		MessagingInterfacePtr->WriteTestOutputMessage(TestName, InError, SD::AutomationTestMessaging::EStandardMessageType::Error);
	}

	FAutomationTestBase::AddErrorS(InError, InFilename, InLineNumber);
}

void FSDAutomationTest::AddWarningS(const FString& InWarning, const FString& InFilename, int32 InLineNumber)
{
	if (IsLoggingMessages) { return; }
	RecursionGuard guard(this);

	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		MessagingInterfacePtr->WriteTestOutputMessage(TestName, InWarning, SD::AutomationTestMessaging::EStandardMessageType::Output);
	}

	FAutomationTestBase::AddWarningS(InWarning, InFilename, InLineNumber);
}

void FSDAutomationTest::AddWarning(const FString& InWarning, int32 StackOffset /*= 0*/)
{
	if (IsLoggingMessages) { return; }
	RecursionGuard guard(this);

	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		MessagingInterfacePtr->WriteTestOutputMessage(TestName, InWarning, SD::AutomationTestMessaging::EStandardMessageType::Output);
	}

	FAutomationTestBase::AddWarning(InWarning, StackOffset);
}

void FSDAutomationTest::AddInfo(const FString& InLogItem, int32 StackOffset /*= 0*/)
{
	if (IsLoggingMessages) { return; }
	RecursionGuard guard(this);

	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		MessagingInterfacePtr->WriteTestOutputMessage(TestName, InLogItem, SD::AutomationTestMessaging::EStandardMessageType::Output);
	}

	FAutomationTestBase::AddInfo(InLogItem, StackOffset);
}

#if WITH_SDTEST_EXTENSIONS
void FSDAutomationTest::SetSuccessState(bool bSuccessful)
{
	if (auto MessagingInterfacePtr = GetMessagingInterface().Pin())
	{
		if (!bSuccessful)
		{
			//SetSuccessState doesn't provide us with any context for the failure, so let's check our execution context.
			FString FailureMessage = TEXT("Test failed but no error found");
			FString FailureDetails = TEXT("");

			const TArray<FAutomationExecutionEntry>& ExecutionEntries = ExecutionInfo.GetEntries();
			const FAutomationExecutionEntry* FirstEntry = ExecutionEntries.FindByPredicate([&](const FAutomationExecutionEntry& InEntry) {
				return (InEntry.Event.Type == EAutomationEventType::Error);
			});

			if (FirstEntry != nullptr)
			{
				FailureMessage = FirstEntry->Event.Message;
				FailureDetails = FirstEntry->Event.Context;
			}

			MessagingInterfacePtr->WriteTestFailedMessage(TestName, FailureMessage, FailureDetails);
		}

		MessagingInterfacePtr->WriteEndTestMessage(TestName);
	}

	FAutomationTestBase::SetSuccessState(bSuccessful);
}
#endif

TWeakPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> FSDAutomationTest::GetMessagingInterface()
{
	if (!WeakMessagingInterface.IsValid())
	{
		if (FSDTestHarness* TestModule = FModuleManager::GetModulePtr<FSDTestHarness>("SDTestHarness"))
		{
			WeakMessagingInterface = TestModule->GetMessagingInterface();
		}
	}

	return WeakMessagingInterface;
}