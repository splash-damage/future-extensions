#pragma once

#include "FutureExtensions.h"
#include <atomic>

SD::TExpectedFuture<void> SD::WhenAll(const TArray<SD::TExpectedFuture<void>>& Futures, const EFailMode FailMode)
{
	if (Futures.Num() == 0)
	{
		return SD::MakeReadyFuture();
	}

	const auto CounterRef = MakeShared<std::atomic<int32>, ESPMode::ThreadSafe>(Futures.Num());
	const auto PromiseRef = MakeShared<SD::TExpectedPromise<void>, ESPMode::ThreadSafe>();
	const auto FirstErrorRef = MakeShared<SD::TExpectedPromise<void>, ESPMode::ThreadSafe>();
	for (auto& Future : Futures)
	{
		Future.Then([CounterRef, PromiseRef, FirstErrorRef, FailMode](const SD::TExpected<void>& Result)
			{
				--(CounterRef.Get());
				if (Result.IsCompleted() == false)
				{
					FirstErrorRef->SetValue(Convert<void>(Result));

					if (FailMode == EFailMode::Fast)
					{
						PromiseRef->SetValue(FirstErrorRef->GetFuture().Get());
					}
				}

				if (CounterRef.Get() == 0)
				{
					if (FirstErrorRef->IsSet() == false)
					{
						PromiseRef->SetValue();
					}
					else
					{
						PromiseRef->SetValue(FirstErrorRef->GetFuture().Get());
					}
					//avoid broken promises
					PromiseRef->SetValue();
				}
			});
	}

	return PromiseRef->GetFuture();
}

SD::TExpectedFuture<void> SD::WhenAll(const TArray<TExpectedFuture<void>>& Futures)
{
	return WhenAll(Futures, EFailMode::Full); 
}
