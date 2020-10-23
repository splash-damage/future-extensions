// Copyright(c) Splash Damage. All rights reserved.
#pragma once
#include <atomic>

#include "ExpectedResult.h"
#include "ExpectedFuture.h"
#include "SDFutureExtensions/Private/FutureExtensionTaskGraph.h"

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

		const auto SetPromise = [FirstErrorRef, PromiseRef]()
		{
			FirstErrorRef->GetFuture().Then([PromiseRef](SD::TExpected<TArray<T>> Result) {PromiseRef->SetValue(MoveTemp(Result)); });
		};

		if (FailMode == EFailMode::Fast)
		{
			SetPromise();
		}

		for (const auto& Future : Futures)
		{
			Future.Then([CounterRef, ValueRef, FirstErrorRef, SetPromise, FailMode](const SD::TExpected<T>& Result)
				{
					if (Result.IsCompleted())
					{
						ValueRef->Emplace(*Result);
					}
					else
					{
						FirstErrorRef->SetValue(Convert<TArray<T>, T>(Result, TArray<T>()));
					}

					if (--(CounterRef.Get()) == 0)
					{
						FirstErrorRef->SetValue(ValueRef.Get());

						if (FailMode != EFailMode::Fast)
						{
							SetPromise();
						}
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

	SDFUTUREEXTENSIONS_API TExpectedFuture<void> WaitAsync(const float DelayInSeconds);
}
