// Copyright 2019 Splash Damage, Ltd. - All Rights Reserved.
#pragma once
#include "Misc/AutomationTest.h"


namespace SD { namespace AutomationTestMessaging { class ITestMessagingInterface; } }

class SDTESTHARNESS_API FSDAutomationTest : public FAutomationTestBase
{
public:
	FSDAutomationTest(const FString& InName, const bool bInComplexTask);
	void TestStringEqual(const FString& Description, const FString& A, const FString& B, ESearchCase::Type SearchType = ESearchCase::CaseSensitive);

	template<typename EnumType>
	void TestEnumEqual(const FString& Description, EnumType A, EnumType B);

protected:
	UWorld* CreateWorld();

	// Begin FAutomationTestBase override
#if WITH_SDTEST_EXTENSIONS
	virtual void PreRunTest() override;
	virtual void PostRunTest() override;
#endif
	virtual void AddError(const FString& InError, int32 StackOffset = 0) override;
	virtual void AddErrorS(const FString& InError, const FString& InFilename, int32 InLineNumber) override;
	virtual void AddWarning(const FString& InWarning, int32 StackOffset = 0) override;
	virtual void AddWarningS(const FString& InWarning, const FString& InFilename, int32 InLineNumber) override;
	// End FAutomationTestBase override

private:
	// Begin FAutomationTestBase override
	virtual void AddInfo(const FString& InLogItem, int32 StackOffset = 0) override;
#if WITH_SDTEST_EXTENSIONS
	virtual void SetSuccessState(bool bSuccessful) override;
#endif
	// End FAutomationTestBase override

	TWeakPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> GetMessagingInterface();

	UWorld* TestWorld = nullptr;
	
	class RecursionGuard
	{
		FSDAutomationTest* const Test;
	public: 
		RecursionGuard(FSDAutomationTest* const test): Test(test) { Test->IsLoggingMessages = true;  }
		~RecursionGuard() { Test->IsLoggingMessages = false; }
	};
	bool IsLoggingMessages = false; 

	TWeakPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> WeakMessagingInterface;
};

template<typename EnumType>
inline void FSDAutomationTest::TestEnumEqual(const FString& Description, EnumType A, EnumType B)
{
	if (A != B)
	{
		TEnumAsByte<EnumType> EnumA = A;
		TEnumAsByte<EnumType> EnumB = B;
		AddError(
			FString::Printf(
				TEXT("%s: The two enum values are not equal.\nActual: '%s'\nExpected: '%s'"),
				*Description,
				*UEnum::GetValueAsString(EnumA.GetValue()),
				*UEnum::GetValueAsString(EnumB.GetValue())
			),
		1);
	}
}