// Copyright(c) Splash Damage. All rights reserved.
#pragma once
#include <atomic>

namespace SD
{
	namespace Errors
	{
		constexpr int32 ERROR_INVALID_ARGUMENT = 1;
	}

	enum class EFailMode
	{
		Full,
		Fast
	};

	template<typename F>
	auto Async(F&& Function, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions())
	{
		return FutureInitialisationDetails::CreateExpectedFuture(Forward<F>(Function), FutureOptions);
	}

	template<typename T>
	SD::TExpectedFuture<TArray<T>> WhenAll(const TArray<SD::TExpectedFuture<T>>& Futures, const EFailMode FailMode)
	{
		if (Futures.Num() == 0)
		{
			return MakeReadyFuture<TArray<T>>(TArray<T>());
		}

		const auto CounterRef = MakeShared<std::atomic<int32>, ESPMode::ThreadSafe>(Futures.Num());
		const auto PromiseRef = MakeShared<SD::TExpectedPromise<TArray<T>>, ESPMode::ThreadSafe>();
		const auto ValueRef = MakeShared<TArray<T>, ESPMode::ThreadSafe>();
		const auto FirstErrorRef = MakeShared<SD::TExpectedPromise<TArray<T>>, ESPMode::ThreadSafe>();
		for (const auto& Future : Futures)
		{
			Future.Then([CounterRef, PromiseRef, ValueRef, FirstErrorRef, FailMode] (const SD::TExpected<T>& Result)
				{
					--(CounterRef.Get());
					if (Result.IsCompleted())
					{
						ValueRef->Emplace(*Result);
					}
					else
					{
						FirstErrorRef->SetValue(Convert<TArray<T>, T>(Result, TArray<T>()));
					
						if (FailMode == EFailMode::Fast)
						{
							PromiseRef->SetValue(FirstErrorRef->GetFuture().Get());
						}
					}
			
					if (CounterRef.Get() == 0)
					{
						if (FirstErrorRef->IsSet() == false)
						{
							PromiseRef->SetValue(ValueRef.Get());
						}
						else
						{
							PromiseRef->SetValue(FirstErrorRef->GetFuture().Get());
						}

						//avoid broken promises
						PromiseRef->SetValue(TArray<>());
					}
				});
		}
		return PromiseRef->GetFuture();
	}

	template<typename T>
	SD::TExpectedFuture<TArray<T>> WhenAll(const TArray<SD::TExpectedFuture<T>>& Futures) { return WhenAll<T>(Futures, EFailMode::Full); }

	SDFUTUREEXTENSIONS_API SD::TExpectedFuture<void> WhenAll(const TArray<SD::TExpectedFuture<void>>& Futures, const EFailMode FailMode);
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
