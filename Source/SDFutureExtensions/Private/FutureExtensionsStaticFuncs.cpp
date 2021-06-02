#include "FutureExtensionsStaticFuncs.h"
#include <atomic>

#include "Containers/Ticker.h"

SD::TExpectedFuture<void> SD::WhenAll(const TArray<SD::TExpectedFuture<void>>& Futures, const EFailMode FailMode)
{
	if (Futures.Num() == 0)
	{
		return SD::MakeReadyFuture();
	}

	const auto CounterRef = MakeShared<std::atomic<int32>, ESPMode::ThreadSafe>(Futures.Num());
	const auto PromiseRef = MakeShared<SD::TExpectedPromise<void>, ESPMode::ThreadSafe>();
	const auto FirstErrorRef = MakeShared<SD::TExpectedPromise<void>, ESPMode::ThreadSafe>();

	const auto SetPromise = [FirstErrorRef, PromiseRef]()
	{
		FirstErrorRef->GetFuture().Then([PromiseRef](SD::TExpected<void> Result) {PromiseRef->SetValue(MoveTemp(Result)); });
	};

	if (FailMode == EFailMode::Fast)
	{
		SetPromise();
	}

	for (const auto& Future : Futures)
	{
		Future.Then([CounterRef, FirstErrorRef, SetPromise, FailMode](const SD::TExpected<void>& Result)
			{
				if (Result.IsCompleted() == false)
				{
					FirstErrorRef->SetValue(SD::TExpected<void>(Result));
				}

				if (--(CounterRef.Get()) == 0)
				{
					FirstErrorRef->SetValue();

					if (FailMode != EFailMode::Fast)
					{
						SetPromise();
					}
				}
			});
	}
	return PromiseRef->GetFuture();
}

SD::TExpectedFuture<void> SD::WhenAll(const TArray<TExpectedFuture<void>>& Futures)
{
	return WhenAll(Futures, EFailMode::Full);
}

SD::TExpectedFuture<void> SD::WaitAsync(const float DelayInSeconds)
{
	const TSharedRef<TExpectedPromise<void>> Promise = MakeShared<TExpectedPromise<void>>();

	FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([Promise](const float Delta)
	{
		Promise->SetValue();

		// false = don't need to execute again
		return false;
	}), DelayInSeconds);

	return Promise->GetFuture();
}
