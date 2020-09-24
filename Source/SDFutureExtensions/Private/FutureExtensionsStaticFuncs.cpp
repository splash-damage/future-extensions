#pragma once

#include "FutureExtensions.h"

SD::TExpectedFuture<void> SD::WhenAll(const TArray<SD::TExpectedFuture<void>>& Futures)
{
	if (Futures.Num() == 0)
	{
		return SD::MakeReadyFuture();
	}

	auto PromiseRef = MakeShared<SD::TExpectedPromise<void>, ESPMode::ThreadSafe>();
	auto CounterRef = MakeShared<int32>(Futures.Num());
	for (auto& Future : Futures)
	{
		Future.Then([CounterRef, PromiseRef](SD::TExpected<void> Result)
			{
				if (Result.IsCompleted())
				{
					if (--(CounterRef.Get()) == 0)
					{
						PromiseRef->SetValue();
					}
				}
				else
				{
					PromiseRef->SetValue(MoveTemp(Result));
				}
			});
	}

	return PromiseRef->GetFuture();
}
