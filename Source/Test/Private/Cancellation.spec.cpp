// Copyright 2020 Splash Damage, Ltd. - All Rights Reserved.

#include <CoreMinimal.h>
#include <FutureExtensions.h>

#include "Helpers/TestHelpers.h"


#if WITH_DEV_AUTOMATION_TESTS

/************************************************************************/
/* FUTURE CANCELLATION SPEC                                             */
/************************************************************************/

class FFutureTestSpec_Cancellation : public FFutureTestSpec
{
	GENERATE_SPEC(FFutureTestSpec_Cancellation, "FutureExtensions.Cancellation",
		EAutomationTestFlags::ProductFilter |
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ServerContext
	);

	bool bInitFunctionExecuted = false;
	bool bThenExecuted = false;

	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	FFutureTestSpec_Cancellation() : FFutureTestSpec()
	{
		DefaultTimeout = FTimespan::FromSeconds(1.5);
	}
};


void FFutureTestSpec_Cancellation::Define()
{
	BeforeEach([this]()
	{
		CancellationHandle = SD::CreateCancellationHandle();
		bInitFunctionExecuted = false;
		bThenExecuted = false;
	});

	LatentIt("Can deal with a cancel race condition", [this](const auto& Done)
	{
		SD::TExpectedFuture<int32> Future = SD::Async([]() {
			return SD::MakeReadyExpected(5);
		}, SD::FExpectedFutureOptions(CancellationHandle));

		CancellationHandle->Cancel();

		Future.Then([this, Done](SD::TExpected<int32> Expected)
		{
			//This is weird, but we *expect* cancellation to be a race condition. Cancellation is a best-effort process.
			//It may, or may not, cancel before the value is set. This test is here to make sure that we handle the race condition.
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled() || Expected.IsCompleted());
			Done.Execute();
		});
	});

	LatentIt("Can cancel a Future", [this](const auto& Done)
	{
		SD::TExpectedFuture<int32> Future = SD::Async([]()
		{
			//Force a sleep here to avoid the race condition described above - we want to ensure a cancel.
			FPlatformProcess::Sleep(1.0f);
			return SD::MakeReadyExpected(5);
		}, SD::FExpectedFutureOptions(CancellationHandle));

		CancellationHandle->Cancel();

		Future.Then([this, Done](SD::TExpected<int32> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			Done.Execute();
		});
	});

	LatentIt("Can cancel before the Future is set", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		SD::Async([]()
		{
			return SD::MakeReadyExpected(5);
		}, SD::FExpectedFutureOptions(CancellationHandle))
		.Then([this, Done](SD::TExpected<int32> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			Done.Execute();
		});
	});

	LatentIt("Expect Then is called after cancel", [this](const auto& Done)
	{
		SD::TExpectedFuture<void> Future = SD::Async([]()
		{
			//Force a sleep here to avoid the race condition described above - we want to ensure a cancel.
			FPlatformProcess::Sleep(1.0f);
			return SD::MakeReadyExpected(5);
		}, SD::FExpectedFutureOptions(CancellationHandle))
		.Then([this](SD::TExpected<int32> Expected)
		{
			bThenExecuted = true;
			return SD::Convert(Expected);
		});

		CancellationHandle->Cancel();

		Future.Then([this, Done](SD::TExpected<void> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			TestTrue("Expected-based continuation was called", bThenExecuted);
			Done.Execute();
		});
	});

	LatentIt("Cancelled Then wont be called", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		SD::Async([this]()
		{
			bInitFunctionExecuted = true;
			return SD::MakeReadyExpected(5);
		})
		.Then([this](SD::TExpected<int32> Expected)
		{
			bThenExecuted = true;
			return SD::Convert(Expected);
		}, SD::FExpectedFutureOptions(CancellationHandle))
		.Then([this, Done](SD::TExpected<void> Expected)
		{
			TestTrue("Expected result was cancelled or set", Expected.IsCancelled());
			TestTrue("Initial function was called", bInitFunctionExecuted);
			TestFalse("Expected-based continuation was called", bThenExecuted);
			Done.Execute();
		});
	});

	LatentIt("Expect Then is called after cancelled Then", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		SD::Async([]()
		{
			return SD::MakeReadyExpected(5);
		})
		.Then([this](SD::TExpected<int32> Expected)
		{
			return Expected;
		}, SD::FExpectedFutureOptions(CancellationHandle))
		.Then([](SD::TExpected<int32> Expected)
		{
			//Then expected-based continuation is called with "Cancelled" status
			return SD::MakeReadyExpected<bool>(Expected.IsCancelled());
		})
		.Then([this, Done](SD::TExpected<bool> Expected)
		{
			TestTrue("Expected is completed", Expected.IsCompleted());
			TestTrue("Then received was 'cancel' state", *Expected);
			Done.Execute();
		});
	});

	LatentIt("Then is not called with raw value after Cancellation", [this](const auto& Done)
	{
		CancellationHandle->Cancel();

		SD::Async([]()
		{
			return SD::MakeReadyExpected(5);
		})
		.Then([this](SD::TExpected<int32> Expected)
		{
			return Expected;
		}, SD::FExpectedFutureOptions(CancellationHandle))
		.Then([this](int32 Result)
		{
			bThenExecuted = true;
			return true;
		})
		.Then([this, Done](SD::TExpected<bool> Expected)
		{
			TestTrue("Expected is completed", Expected.IsCancelled());
			TestFalse("Then with raw value executed", bThenExecuted);
			Done.Execute();
		});
	});
}

#endif //WITH_DEV_AUTOMATION_TESTS
