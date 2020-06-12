// Copyright 2020 Splash Damage, Ltd. - All Rights Reserved.

#include <CoreMinimal.h>
#include <FutureExtensions.h>

#include "Helpers/TestHelpers.h"


#if WITH_DEV_AUTOMATION_TESTS

/************************************************************************/
/* FUTURE BASIC SPEC                                                    */
/************************************************************************/

class FFutureTestSpec_Basic : public FFutureTestSpec
{
	GENERATE_SPEC(FFutureTestSpec_Basic, "FutureExtensions.Basic",
		EAutomationTestFlags::ProductFilter |
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ServerContext
	);

	static constexpr int32 ErrorContext = 0xbaadf00d;
	static constexpr int32 ErrorCode = 0xdeadbeef;

	bool bThenCalled = false;
	int32 CapturedInt = 0;


	FFutureTestSpec_Basic() : FFutureTestSpec()
	{
		DefaultTimeout = FTimespan::FromSeconds(0.2);
	}

	SD::TExpectedFuture<FString> AsyncAddTen(int32 Value)
	{
		return SD::Async([Value]()
		{
			return FString::Printf(TEXT("%i"), Value + 10);
		});
	}
};


void FFutureTestSpec_Basic::Define()
{
	LatentIt("Can run future", [this](const auto& Done)
	{
		SD::TExpectedFuture<void> VoidFuture = SD::MakeReadyFuture();
		TestTrue("Future is ready", VoidFuture.IsReady());

		VoidFuture.Then([this, Done](SD::TExpected<void> Expected)
		{
			TestTrue("Future is completed", Expected.IsCompleted());
			Done.Execute();
		});
	});

	LatentIt("Will run future on Then and not before", [this](const auto& Done)
	{
		SD::TExpectedFuture<int32> FirstFuture = SD::Async([]()
		{
			return 10;
		});

		FirstFuture.Then([this, Done](int32 Result)
		{
			TestEqual("Value", Result, 10);
			Done.Execute();
		});
	});

	LatentIt("Can pass expect directly to next step", [this](const auto& Done)
	{
		SD::Async([]()
		{
			return 10;
		})
		.Then([](SD::TExpected<int32> Expected)
		{
			//Pass initial result straight through
			return Expected;
		})
		.Then([this, Done](SD::TExpected<int32> Expected)
		{
			TestTrue("Expected was completed", Expected.IsCompleted());
			TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
			TestEqual("Value", *Expected, 10);
			Done.Execute();
		});
	});

	Describe("Raw value lambdas", [this]()
	{
		LatentIt("Can capture and return the value", [this](const auto& Done)
		{
			SD::MakeReadyFuture<int32>(13)
			.Then([this, Done](int32 Expected)
			{
				TestEqual("Value", Expected, 13);
				Done.Execute();
			});
		});

		LatentIt("Can concatenate Then", [this](const auto& Done)
		{
			SD::MakeReadyFuture<int32>(13)
			.Then([this](int32 Result)
			{
				TestEqual("Value on first Then", Result, 13);
				return Result + 7;
			})
			.Then([this, Done](int32 Result)
			{
				TestEqual("Value on second Then", Result, 20);
				Done.Execute();
			});
		});

		LatentIt("Can capture initial value and then concatenate void", [this](const auto& Done)
		{
			SD::MakeReadyFuture<int32>(13)
			.Then([this](int32 Result)
			{
				TestEqual("Value on first Then", Result, 13);
			})
			.Then([this, Done]()
			{
				Done.Execute();
			});
		});

		LatentIt("Can execute with no capture and then concatenate with capture", [this](const auto& Done)
		{
			SD::MakeReadyFuture()
			.Then([this]()
			{
				return 20;
			})
			.Then([this, Done](int32 Result)
			{
				TestEqual("Value on second Then", Result, 20);
				Done.Execute();
			});
		});
	});


	Describe("Expected value lambdas", [this]()
	{
		LatentIt("Can capture and return the value", [this](const auto& Done)
		{
			SD::MakeReadyFuture<int32>(13)
			.Then([this, Done](SD::TExpected<int32> Expected)
			{
				TestTrue("Future is completed", Expected.IsCompleted());
				TestEqual("Value", *Expected, 13);
				Done.Execute();
			});
		});

		LatentIt("Can concatenate Then", [this](const auto& Done)
		{
			SD::MakeReadyFuture<int32>(13)
			.Then([this](SD::TExpected<int32> Result)
			{
				TestEqual("Value on first Then", *Result, 13);
				return *Result + 7;
			})
			.Then([this, Done](SD::TExpected<int32> Result)
			{
				TestTrue("Future is completed", Result.IsCompleted());
				TestEqual("Value on second Then", *Result, 20);
				Done.Execute();
			});
		});

		LatentIt("Can capture initial value and then concatenate void", [this](const auto& Done)
		{
			SD::MakeReadyFuture<int32>(13)
			.Then([this](SD::TExpected<int32> Result)
			{
				TestEqual("Value on first Then", *Result, 13);
			})
			.Then([this, Done](SD::TExpected<void> Result)
			{
				TestTrue("Future is completed", Result.IsCompleted());
				Done.Execute();
			});
		});

		LatentIt("Can execute with no capture and then concatenate with capture", [this](const auto& Done)
		{
			SD::MakeReadyFuture()
			.Then([this]()
			{
				return 20;
			})
			.Then([this, Done](SD::TExpected<int32> Result)
			{
				TestTrue("Future is completed", Result.IsCompleted());
				TestEqual("Value on second Then", *Result, 20);
				Done.Execute();
			});
		});

		LatentIt("Can change future captured type", [this](const auto& Done)
		{
			SD::MakeReadyFuture().Then([]()
			{
				return SD::MakeReadyExpected(10);
			})
			.Then([this, Done](SD::TExpected<int32> Result)
			{
				TestTrue("Future is completed", Result.IsCompleted());
				TestEqual("Value on second Then", *Result, 10);
				Done.Execute();
			});
		});
	});

	LatentIt("Can return a TOptional", [this](const auto& Done)
	{
		SD::MakeReadyFuture<int32>(5)
		.Then([this, Done](SD::TExpected<int32> Expected)
		{
			TestTrue("Expected was completed", Expected.IsCompleted());
			TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
			TestEqual("Value", *Expected, 5);
			Done.Execute();
		});
	});

	Describe("Async", [this]()
	{
		LatentIt("Can execute Async without return", [this](const auto& Done)
		{
			CapturedInt = 0;
			SD::Async([this]()
			{
				CapturedInt = 23;
			})
			.Then([this, Done]()
			{
				TestEqual("Value", CapturedInt, 23);
				Done.Execute();
			});
		});

		LatentIt("Can execute Async with return", [this](const auto& Done)
		{
			SD::Async([]()
			{
				return 5;
			})
			.Then([this, Done](int32 Result)
			{
				TestEqual("Value", Result, 5);
				Done.Execute();
			});
		});

		LatentIt("Can unwrap Async inside Then", [this](const auto& Done)
		{
			SD::MakeReadyFuture()
			.Then([this]()
			{
				return AsyncAddTen(10);
			})
			.Then([this, Done](const FString& Result)
			{
				TestEqual("Value on second Then", Result, TEXT("20"));
				Done.Execute();
			});
		});

		LatentIt("Can unwrap inside an unwrapped Async inside Then", [this](const auto& Done)
		{
			SD::MakeReadyFuture()
			.Then([this]()
			{
				return SD::Async([this]()
				{
					return SD::MakeReadyExpected(5);
				});
			})
			.Then([this, Done](int32 Result)
			{
				TestEqual("Value on second Then", Result, 5);
				Done.Execute();
			});
		});

		LatentIt("Can unwrap Async inside Async", [this](const auto& Done)
		{
			SD::Async([this]()
			{
				return AsyncAddTen(20);
			})
			.Then([this, Done](const FString& Result)
			{
				TestEqual("Value on second Then", Result, TEXT("30"));
				Done.Execute();
			});
		});

		LatentIt("Captured lambda value can be changed", [this](const auto& Done)
		{
			CapturedInt = 2;
			SD::Async([this]()
			{
				CapturedInt = 5;
			})
			.Then([this, Done](SD::TExpected<void> Expected)
			{
				TestTrue("Expected was completed", Expected.IsCompleted());
				TestEqual("Captured value", CapturedInt, 5);
				Done.Execute();
			});
		});
	});

	Describe("Errors", [this]()
	{
		LatentIt("Can return an error and be received in expected then", [this](const auto& Done)
		{
			SD::Async([this]()
			{
				return SD::MakeErrorExpected<int32>(SD::Error(ErrorCode, ErrorContext, TEXT("Bad times!")));
			})
			.Then([this, Done](SD::TExpected<int32> Expected)
			{
				TestTrue("Result is an error", Expected.IsError());
				TestEqual("Error Code", Expected.GetError()->GetErrorCode(), ErrorCode);
				TestEqual("Error Context", Expected.GetError()->GetErrorContext(), ErrorContext);
				TestEqual("Error Message", *Expected.GetError()->GetErrorInfo(), TEXT("Bad times!"));
				Done.Execute();
			});
		});

		LatentIt("Value Then wont be called on error", [this](const auto& Done)
		{
			bThenCalled = false;

			SD::Async([this]()
			{
				return SD::MakeErrorExpected<int32>(SD::Error(ErrorCode, ErrorContext));
			})
			.Then([this](int32 Expected)
			{
				bThenCalled = true;
				return Expected;
			})
			.Then([this, Done](SD::TExpected<int32> Expected)
			{
				TestTrue("Result is an error", Expected.IsError());
				TestEqual("Error Code", Expected.GetError()->GetErrorCode(), ErrorCode);
				TestEqual("Error Context", Expected.GetError()->GetErrorContext(), ErrorContext);

				TestFalse("Then Called", bThenCalled);
				Done.Execute();
			});
		});

		LatentIt("Error can be passed through captured type change", [this](const auto& Done)
		{
			SD::Async([this]()
			{
				return SD::MakeErrorExpected<int32>(SD::Error(ErrorCode, ErrorContext));
			})
			.Then([this](int32 Expected)
			{
				return FString{};
			})
			.Then([this, Done](SD::TExpected<FString> Expected)
			{
				TestTrue("Result is an error", Expected.IsError());
				TestEqual("Error Code", Expected.GetError()->GetErrorCode(), ErrorCode);
				TestEqual("Error Context", Expected.GetError()->GetErrorContext(), ErrorContext);
				Done.Execute();
			});
		});

		LatentIt("Can handle error", [this](const auto& Done)
		{
			SD::Async([this]()
			{
				return SD::MakeErrorExpected<int32>(SD::Error(ErrorCode, ErrorContext, TEXT("Bad times!")));
			})
			.Then([this](SD::TExpected<int32> Expected)
			{
				if (Expected.IsError())
				{
					return *Expected.GetError()->GetErrorInfo();
				}
				return FString{};
			})
			.Then([this, Done](SD::TExpected<FString> Expected)
			{
				TestFalse("Result is an error", Expected.IsError());
				TestTrue("Result is completed", Expected.IsCompleted());
				TestEqual("Captured String", *Expected, TEXT("Bad times!"));
				Done.Execute();
			});
		});
	});
}

#endif //WITH_DEV_AUTOMATION_TESTS
