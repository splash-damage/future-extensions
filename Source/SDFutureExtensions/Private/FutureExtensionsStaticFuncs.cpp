#pragma once

#include "FutureExtensions.h"
#include <atomic>

SD::TExpectedFuture<void> SD::WhenAll(const TArray<SD::TExpectedFuture<void>>& Futures, const EFailMode FailMode)
{
	if (Futures.Num() == 0)
	{
		return SD::MakeReadyFuture();
	}

	struct FSharedExpected
	{
		void Set(const TExpected<void>& Other) { Expected = Other; }
		TExpected<void> Expected;
	};

	const auto CounterRef = MakeShared<std::atomic<int32>, ESPMode::ThreadSafe>(Futures.Num());
	const auto PromiseRef = MakeShared<SD::TExpectedPromise<void>, ESPMode::ThreadSafe>();
	const auto FirstErrorRef = MakeShared<FSharedExpected, ESPMode::ThreadSafe>();
	for (auto& Future : Futures)
	{
		Future.Then([CounterRef, PromiseRef, FirstErrorRef, FailMode](const SD::TExpected<void>& Result)
			{
				--(CounterRef.Get());
				if (Result.IsCompleted() == false)
				{
					if (FirstErrorRef->Expected.GetState() == EExpectedResultState::Incomplete)
					{
						if (Result.IsError())
						{
							FirstErrorRef->Set(MakeErrorExpected<void>(Result.GetError().Get()));
						}
						else if (Result.IsCancelled())
						{
							FirstErrorRef->Set(MakeCancelledExpected<void>());
						}
					}

					if (FailMode == EFailMode::Fast)
					{
						//Copy the expected value here as other continuations may still use it.
						PromiseRef->SetValue(TExpected<void>(FirstErrorRef->Expected));
					}
				}

				if (CounterRef.Get() == 0)
				{
					if (FirstErrorRef->Expected.GetState() == EExpectedResultState::Incomplete)
					{
						PromiseRef->SetValue();
					}
					else
					{
						PromiseRef->SetValue(MoveTemp(FirstErrorRef->Expected));
					}
				}
			});
	}

	return PromiseRef->GetFuture();
}
