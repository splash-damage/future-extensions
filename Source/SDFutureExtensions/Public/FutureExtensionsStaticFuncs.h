// Copyright(c) Splash Damage. All rights reserved.
#pragma once

namespace SD
{
	namespace Errors
	{
		constexpr int32 ERROR_INVALID_ARGUMENT = 1;
	}

	template<typename F>
	auto Async(F&& Function, const FExpectedFutureOptions& FutureOptions = FExpectedFutureOptions())
	{
		return FutureInitialisationDetails::CreateExpectedFuture(Forward<F>(Function), FutureOptions);
	}

	template<typename T>
	SD::TExpectedFuture<TArray<T>> WhenAll(const TArray<SD::TExpectedFuture<T>>& Futures)
	{
		if (Futures.Num() == 0)
		{
			return MakeReadyFuture<TArray<T>>(TArray<T>());
		}

		auto PromiseRef = MakeShared<SD::TExpectedPromise<TArray<T>>, ESPMode::ThreadSafe>();
		auto ValueRef = MakeShared<TArray<T>, ESPMode::ThreadSafe>();
		auto CounterRef = MakeShared<int32, ESPMode::ThreadSafe>(Futures.Num());
		for (auto& Future : Futures)
		{
			Future.Then([CounterRef, PromiseRef, ValueRef](const SD::TExpected<T>& Result)
				{
					if (Result.IsCompleted())
					{
						ValueRef->Emplace(Result.GetValue().GetValue());
						if (--(CounterRef.Get()) == 0)
						{
							PromiseRef->SetValue(ValueRef.Get());
						}
					}
					else if(Result.IsError())
					{
						PromiseRef->SetValue(Result.GetError().Get());
					}
					else if (Result.IsCancelled())
					{
						PromiseRef->SetValue(MakeCancelledExpected<TArray<T>>());
					}
				});
		}
		return PromiseRef->GetFuture();
	}

	SDFUTUREEXTENSIONS_API SD::TExpectedFuture<void> WhenAll(const TArray<SD::TExpectedFuture<void>>& Futures);

	template<typename T>
	SD::TExpectedFuture<T> WhenAny(const TArray<SD::TExpectedFuture<T>>& Futures)
	{
		if (Futures.Num() == 0)
		{
			return SD::MakeErrorFuture<T>(Error(Errors::ERROR_INVALID_ARGUMENT, TEXT("SD::WhenAny - Must have at least one element in the array.")));
		}
		auto PromiseRef = MakeShared<SD::TExpectedPromise<T>, ESPMode::ThreadSafe>();
		for (auto& Future : Futures)
		{
			Future.Then([PromiseRef](const SD::TExpected<T>& Result)
				{
					PromiseRef->SetValue(SD::TExpected<T>(Result));
				});
		}
		return PromiseRef->GetFuture();
	}
}
