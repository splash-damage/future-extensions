// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "Async/Future.h"
#include "SDFutureExtensions/Private/FutureExtensionsTypeTraits.h"
#include "Templates/AreTypesEqual.h"
#include "Async/TaskGraphInterfaces.h"
#include "Misc/QueuedThreadPool.h"
#include "ExpectedResult.h"
#include "ExpectedFutureOptions.h"

namespace SD
{
	//Forward declarations
	template <class ResultType>
	class TExpectedFuture;

	template<>
	class TExpectedFuture<void>;

	template <class ResultType>
	class TExpectedPromise;

	template<>
	class TExpectedPromise<void>;

	namespace FutureExtensionTaskGraph
	{
		template<typename F, typename R>
		class TExpectedFutureInitTask;

		template<typename F, typename P, typename R>
		class TExpectedFutureContinuationTask;

		template<typename F, typename R>
		class TExpectedFutureInitQueuedWork;

		template<typename F, typename P, typename R>
		class TExpectedFutureContinuationQueuedWork;
	}
	//
	namespace FutureExecutionDetails
	{
		struct FExecutionDetails
		{
			FExecutionDetails() = default;

			FExecutionDetails(EExpectedFutureExecutionPolicy InPolicy, ENamedThreads::Type InThread)
				: ExecutionPolicy(InPolicy)
				, ExecutionThread(InThread)
			{}

			EExpectedFutureExecutionPolicy ExecutionPolicy = EExpectedFutureExecutionPolicy::Current;
			ENamedThreads::Type	ExecutionThread = ENamedThreads::AnyThread;
		};

		inline FExecutionDetails GetExecutionDetails(const FExpectedFutureOptions& FutureOptions)
		{
			if (FutureOptions.GetExecutionPolicy() == EExpectedFutureExecutionPolicy::ThreadPool)
			{
				return FExecutionDetails(EExpectedFutureExecutionPolicy::ThreadPool, ENamedThreads::AnyThread);
			}
			else
			{
				switch (FutureOptions.GetExecutionPolicy())
				{
				case EExpectedFutureExecutionPolicy::NamedThread:
					return FExecutionDetails(EExpectedFutureExecutionPolicy::NamedThread,
						FutureOptions.GetDesiredExecutionThread());
				//There is no antecedent future so inline is equivalent to current
				case EExpectedFutureExecutionPolicy::Inline:
				case EExpectedFutureExecutionPolicy::Current:
				default:
					return FExecutionDetails(EExpectedFutureExecutionPolicy::Current,
						FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
				}
			}
		}

		template<typename P>
		FExecutionDetails GetExecutionDetails(const FExpectedFutureOptions& FutureOptions,
												const TExpectedFuture<P>& AntecedentFuture)
		{
			if (FutureOptions.GetExecutionPolicy() == EExpectedFutureExecutionPolicy::Inline)
			{
				return AntecedentFuture.GetExecutionDetails();
			}
			else
			{
				return GetExecutionDetails(FutureOptions);
			}
		}
	}

	namespace FutureState
	{
		template< typename R>
		class TExpectedFutureState
		{
		public:
			TExpectedFutureState(TFuture<TExpected<R>>&& Future, const FutureExecutionDetails::FExecutionDetails& Execution)
				: InternalFuture(MoveTemp(Future))
				, ExecutionDetails(Execution)
			{}

			TFuture<TExpected<R>> InternalFuture;
			FutureExecutionDetails::FExecutionDetails ExecutionDetails;
		};

		template< typename R>
		TSharedRef<TExpectedFutureState<R>, ESPMode::ThreadSafe> Allocate(TFuture<TExpected<R>> Future, const FutureExecutionDetails::FExecutionDetails& Execution)
		{
			return MakeShared<TExpectedFutureState<R>, ESPMode::ThreadSafe>(MoveTemp(Future), Execution);
		}
	}


	namespace FutureInitialisationDetails
	{
		using namespace FutureExtensionTypeTraits;

		template<class F>
		auto CreateExpectedFuture(F&& Function, const FExpectedFutureOptions& FutureOptions)
		{
			using InitialFunctorTypes = TInitialFunctorTypes<F>;

			using UnwrappedReturnType = typename TUnwrap<typename InitialFunctorTypes::ReturnType>::Type;
			using SharedPromiseRef = TSharedRef<TExpectedPromise<UnwrappedReturnType>, ESPMode::ThreadSafe>;

			const FutureExecutionDetails::FExecutionDetails ExecutionDetails =
					FutureExecutionDetails::GetExecutionDetails(FutureOptions);

			SharedPromiseRef Promise =
				MakeShared<TExpectedPromise<UnwrappedReturnType>, ESPMode::ThreadSafe>(ExecutionDetails);
			TExpectedFuture<UnwrappedReturnType> Future = Promise->GetFuture();

			if (ExecutionDetails.ExecutionPolicy == EExpectedFutureExecutionPolicy::ThreadPool)
			{
				using InitWorkType = FutureExtensionTaskGraph::TExpectedFutureInitQueuedWork<F, UnwrappedReturnType>;
				GThreadPool->AddQueuedWork(new InitWorkType(MoveTemp(Function), Promise, FutureOptions.GetCancellationTokenHandle()));
			}
			else
			{
				using InitTaskType = FutureExtensionTaskGraph::TExpectedFutureInitTask<F, UnwrappedReturnType>;
				TGraphTask<InitTaskType>::CreateTask()
					.ConstructAndDispatchWhenReady(MoveTemp(Function), Promise, FutureOptions.GetCancellationTokenHandle());
			}

			return Future;
		}
	}

	namespace FutureContinuationDetails
	{
		using namespace FutureExtensionTypeTraits;

		template<class F, class P>
		auto ThenImpl(F&& Func, const TExpectedFuture<P>& PrevFuture, FGraphEventRef ContinuationTrigger,
						const SD::FExpectedFutureOptions& FutureOptions)
		{
			check(PrevFuture.IsValid());

			using ContinuationFunctorTypes = TContinuationFunctorTypes<F, P>;

			using UnwrappedReturnType = typename TUnwrap<typename ContinuationFunctorTypes::ReturnType>::Type;
			using SharedPromiseRef = TSharedRef<TExpectedPromise<UnwrappedReturnType>, ESPMode::ThreadSafe>;

			const FutureExecutionDetails::FExecutionDetails ExecutionDetails =
					FutureExecutionDetails::GetExecutionDetails(FutureOptions, PrevFuture);

			SharedPromiseRef Promise = MakeShared<TExpectedPromise<UnwrappedReturnType>, ESPMode::ThreadSafe>(ExecutionDetails);
			TExpectedFuture<UnwrappedReturnType> Future = Promise->GetFuture();

			if (ExecutionDetails.ExecutionPolicy == EExpectedFutureExecutionPolicy::ThreadPool)
			{
				using ContinuationWorkType = FutureExtensionTaskGraph::TExpectedFutureContinuationQueuedWork<F, P, UnwrappedReturnType>;
				GThreadPool->AddQueuedWork(new ContinuationWorkType(Forward<F>(Func),
																	MoveTemp(Promise),
																	PrevFuture,
																	FutureOptions.GetCancellationTokenHandle()));
			}
			else
			{
				using ContinuationTaskType = FutureExtensionTaskGraph::TExpectedFutureContinuationTask<F, P, UnwrappedReturnType>;
				FGraphEventArray TriggerArray{ ContinuationTrigger };
				TGraphTask<ContinuationTaskType>::CreateTask(&TriggerArray)
					.ConstructAndDispatchWhenReady(Forward<F>(Func),
													MoveTemp(Promise),
													PrevFuture,
													FutureOptions.GetCancellationTokenHandle());
			}

			return Future;
		}

		inline TFunction<void()> GetTriggerFunc(TGraphTask<FNullGraphTask>* WithTask)
		{
			return TFunction<void()>([CompletionEvent = WithTask->GetCompletionEvent()]() {
				TArray<FBaseGraphTask*> EmptySubsequentsArray = TArray<FBaseGraphTask*>();
				CompletionEvent->DispatchSubsequents(EmptySubsequentsArray);
			});
		}
	}

	class TExpectedFutureBase
	{
	public:
		TExpectedFutureBase(FGraphEventRef InPromiseCompletionEventRef)
			: PromiseCompletionEventRef(InPromiseCompletionEventRef)
		{}

	protected:
		FGraphEventRef PromiseCompletionEventRef;
	};

	template <class R>
	class TExpectedFuture : public TExpectedFutureBase
	{
	public:
		using ResultType = R;
		using UnwrappedResultType = typename FutureExtensionTypeTraits::TUnwrap<R>::Type;
		using ExpectedResultType = TExpected<UnwrappedResultType>;

		TExpectedFuture(TFuture<ExpectedResultType>&& Future, FGraphEventRef InPromiseCompletionEventRef,
						const FutureExecutionDetails::FExecutionDetails& InExecutionDetails)
			: TExpectedFutureBase(InPromiseCompletionEventRef)
			, SharedState(FutureState::Allocate<ResultType>(MoveTemp(Future), InExecutionDetails))
		{}

		TExpectedFuture(const TExpectedFuture<ResultType>& Other)
			: TExpectedFutureBase(Other.PromiseCompletionEventRef)
			, SharedState(Other.SharedState)
		{}

		TExpectedFuture<ResultType>& operator=(const TExpectedFuture<ResultType>& Other)
		{
			PromiseCompletionEventRef = Other.PromiseCompletionEventRef;
			SharedState = Other.SharedState;
			return *this;
		}

		TExpectedFuture()
			: TExpectedFutureBase(FGraphEventRef())
			, SharedState(FutureState::Allocate<ResultType>(TFuture<ExpectedResultType>(), FutureExecutionDetails::FExecutionDetails()))
		{}

		TExpectedFuture(TExpectedFuture<ResultType>&& Other)
			: TExpectedFutureBase(MoveTemp(Other.PromiseCompletionEventRef))
			, SharedState(MoveTemp(Other.SharedState))
		{}

		TExpectedFuture<ResultType>& operator=(TExpectedFuture<ResultType>&& Other)
		{
			PromiseCompletionEventRef = MoveTemp(Other.PromiseCompletionEventRef);
			SharedState = MoveTemp(Other.SharedState);
			return *this;
		}

		template<class F>
		auto Then(F&& Func, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions()) const
		{
			check(IsValid());
			return FutureContinuationDetails::ThenImpl(Forward<F>(Func), *this,
														PromiseCompletionEventRef, FutureOptions);
		}

		bool IsReady() const
		{
			return SharedState->InternalFuture.IsReady();
		}

		ExpectedResultType Get() const
		{
			return SharedState->InternalFuture.Get();
		}

		bool IsValid() const
		{
			return PromiseCompletionEventRef.IsValid() && SharedState->InternalFuture.IsValid();
		}

		const FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return SharedState->ExecutionDetails;
		}

	private:

		//Construction from UE4 TFuture is not allowed
		TExpectedFuture(const TFuture<ResultType>&) = delete;

		TSharedRef<FutureState::TExpectedFutureState<ResultType>, ESPMode::ThreadSafe> SharedState;
	};

	template <class R>
	class TExpectedPromise : public FCancellablePromise
	{
		using ExpectedResultType = TExpected<R>;

	public:

		TExpectedPromise(const FutureExecutionDetails::FExecutionDetails& InExecutionDetails =
							FutureExecutionDetails::FExecutionDetails())
			: CompletionTask(TGraphTask<FNullGraphTask>::CreateTask().ConstructAndHold(TStatId(), ENamedThreads::AnyThread))
			, InternalPromise(FutureContinuationDetails::GetTriggerFunc(CompletionTask))
			, ValueSetSync(0)
			, ExecutionDetails(InExecutionDetails)
		{}

		TExpectedPromise(TExpectedPromise&& Other) = default;

		TExpectedPromise& operator=(TExpectedPromise&& Other)
		{
			InternalPromise = MoveTemp(Other.InternalPromise);
			return *this;
		}

		TExpectedFuture<R> GetFuture()
		{
			return TExpectedFuture<R>(InternalPromise.GetFuture(),
										CompletionTask->GetCompletionEvent(),
										ExecutionDetails);
		}

		bool IsSet() const
		{
			return FPlatformAtomics::AtomicRead(&ValueSetSync) == 1;
		}

		void SetValue(ExpectedResultType&& Result)
		{
			if (FPlatformAtomics::InterlockedExchange(&ValueSetSync, 1) == 0)
			{
				InternalPromise.SetValue(Forward<ExpectedResultType>(Result));
			}
		}

		void SetValue(R&& Result)
		{
			SetValue(SD::MakeReadyExpected(Forward<R>(Result)));
		}

		void SetValue(const R& Result)
		{
			SetValue(SD::MakeReadyExpected(Result));
		}

		const FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return ExecutionDetails;
		}

	protected:

		void Cancel()
		{
			SetValue(SD::MakeCancelledExpected<R>());
		}

	private:

		TGraphTask<FNullGraphTask>* CompletionTask;
		TPromise<ExpectedResultType> InternalPromise;

		//By design, cancellation and valid value setting is a race - cancellation is always *best attempt*.
		//Trying to set a promise value that's already been set *should* just fail silently, but unfortunately
		//TPromise asserts. We need to use our own synchronisation to make sure we don't fire this assert.
		int8 ValueSetSync;

		FutureExecutionDetails::FExecutionDetails ExecutionDetails;
	};

	template <>
	class TExpectedFuture<void> : public TExpectedFutureBase
	{
	public:
		using ResultType = void;
		using ExpectedResultType = TExpected<void>;

		TExpectedFuture(TFuture<ExpectedResultType>&& Future, FGraphEventRef InPromiseCompletionEventRef,
						const FutureExecutionDetails::FExecutionDetails& InExecutionDetails)
			: TExpectedFutureBase(InPromiseCompletionEventRef)
			, SharedState(FutureState::Allocate<void>(MoveTemp(Future), InExecutionDetails))
		{}

		TExpectedFuture(const TExpectedFuture<void>& Other)
			: TExpectedFutureBase(Other.PromiseCompletionEventRef)
			, SharedState(Other.SharedState)
		{}

		TExpectedFuture<void>& operator=(const TExpectedFuture<void>& Other)
		{
			PromiseCompletionEventRef = Other.PromiseCompletionEventRef;
			SharedState = Other.SharedState;
			return *this;
		}

		TExpectedFuture()
			: TExpectedFutureBase(FGraphEventRef())
			, SharedState(FutureState::Allocate<ResultType>(TFuture<ExpectedResultType>(), FutureExecutionDetails::FExecutionDetails()))
		{}

		TExpectedFuture(TExpectedFuture<ResultType>&& Other)
			: TExpectedFutureBase(MoveTemp(Other.PromiseCompletionEventRef))
			, SharedState(MoveTemp(Other.SharedState))
		{}

		TExpectedFuture<ResultType>& operator=(TExpectedFuture<ResultType>&& Other)
		{
			PromiseCompletionEventRef = MoveTemp(Other.PromiseCompletionEventRef);
			SharedState = MoveTemp(Other.SharedState);
			return *this;
		}

		bool IsReady() const
		{
			return SharedState->InternalFuture.IsReady();
		}

		template<class F>
		auto Then(F&& Func, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions()) const
		{
			check(IsValid());
			return FutureContinuationDetails::ThenImpl(Forward<F>(Func), *this,
														PromiseCompletionEventRef, FutureOptions);
		}

		ExpectedResultType Get() const
		{
			return SharedState->InternalFuture.Get();
		}

		bool IsValid() const
		{
			return PromiseCompletionEventRef.IsValid() && SharedState->InternalFuture.IsValid();
		}

		const FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return SharedState->ExecutionDetails;
		}

	private:
		TExpectedFuture(const TFuture<void>& Future) = delete;

		TSharedRef<FutureState::TExpectedFutureState<void>, ESPMode::ThreadSafe> SharedState;
	};

	template <>
	class TExpectedPromise<void> : public FCancellablePromise
	{
	public:

		using ExpectedResultType = TExpected<void>;

		TExpectedPromise(const FutureExecutionDetails::FExecutionDetails& InExecutionDetails =
							FutureExecutionDetails::FExecutionDetails())
			: CompletionTask(TGraphTask<FNullGraphTask>::CreateTask().ConstructAndHold(TStatId(), ENamedThreads::AnyThread))
			, InternalPromise(FutureContinuationDetails::GetTriggerFunc(CompletionTask))
			, ValueSetSync(0)
			, ExecutionDetails(InExecutionDetails)
		{}

		TExpectedPromise(TExpectedPromise&& Other) = default;

		TExpectedPromise& operator=(TExpectedPromise&& Other)
		{
			InternalPromise = MoveTemp(Other.InternalPromise);
			return *this;
		}

		TExpectedFuture<void> GetFuture()
		{
			return TExpectedFuture<void>(InternalPromise.GetFuture(),
											CompletionTask->GetCompletionEvent(),
											ExecutionDetails);
		}

		bool IsSet() const
		{
			return FPlatformAtomics::AtomicRead(&ValueSetSync) == 1;
		}

		void SetValue(ExpectedResultType&& InResult)
		{
			if (FPlatformAtomics::InterlockedExchange(&ValueSetSync, 1) == 0)
			{
				InternalPromise.SetValue(Forward<ExpectedResultType>(InResult));
			}
		}

		void SetValue()
		{
			SetValue(SD::MakeReadyExpected());
		}

		const FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return ExecutionDetails;
		}

	protected:

		void Cancel()
		{
			SetValue(SD::MakeCancelledExpected());
		}

	private:

		TGraphTask<FNullGraphTask>* CompletionTask;
		TPromise<ExpectedResultType> InternalPromise;

		int8 ValueSetSync;

		FutureExecutionDetails::FExecutionDetails ExecutionDetails;
	};

	template <class T>
	TExpectedFuture<T> MakeReadyFuture(T&& InValue)
	{
		TExpectedPromise<T> ValuePromise = TExpectedPromise<T>();
		ValuePromise.SetValue(SD::MakeReadyExpected<T>(Forward<T>(InValue)));
		return ValuePromise.GetFuture();
	}

	template <class T>
	TExpectedFuture<T> MakeReadyFuture(TExpected<T>&& InExpected)
	{
		TExpectedPromise<T> ValuePromise = TExpectedPromise<T>();
		ValuePromise.SetValue(Forward<TExpected<T>>(InExpected));
		return ValuePromise.GetFuture();
	}

	inline TExpectedFuture<void> MakeReadyFuture()
	{
		TExpectedPromise<void> ValuePromise = TExpectedPromise<void>();
		ValuePromise.SetValue();
		return ValuePromise.GetFuture();
	}

	template <typename T, typename R, typename TEnableIf<TIsSame<T, R>::Value>::Type* = nullptr>
	TExpectedFuture<T> MakeReadyFutureFromExpected(const TExpected<R>& InExpected)
	{
		TExpectedPromise<T> Promise = TExpectedPromise<T>();
		Promise.SetValue(InExpected);
		return Promise.GetFuture();
	}

	template <typename T, typename R, typename TEnableIf<!TIsSame<T, R>::Value>::Type* = nullptr>
	TExpectedFuture<T> MakeReadyFutureFromExpected(const TExpected<R>& InExpected)
	{
		return MakeReadyFuture<T>(Convert<T>(InExpected));
	}

	template <typename T, typename R>
	TExpectedFuture<T> MakeErrorFuture(const TExpected<R>& InExpected)
	{
		TExpectedPromise<T> ErrorPromise = TExpectedPromise<T>();
		ErrorPromise.SetValue(ConvertIncomplete<T>(InExpected));
		return ErrorPromise.GetFuture();
	}

	template <typename T>
	TExpectedFuture<T> MakeErrorFuture(const TExpected<T>& InExpected)
	{
		TExpectedPromise<T> ErrorPromise = TExpectedPromise<T>();
		ErrorPromise.SetValue(InExpected);
		return ErrorPromise.GetFuture();
	}

	template <typename T>
	TExpectedFuture<T> MakeErrorFuture(TExpected<T>&& InExpected)
	{
		TExpectedPromise<T> ErrorPromise = TExpectedPromise<T>();
		ErrorPromise.SetValue(Forward<TExpected<T>>(InExpected));
		return ErrorPromise.GetFuture();
	}

	template <typename T>
	TExpectedFuture<T> MakeErrorFuture(const Error& InError)
	{
		TExpectedPromise<T> ErrorPromise = TExpectedPromise<T>();
		ErrorPromise.SetValue(MakeErrorExpected<T>(InError));
		return ErrorPromise.GetFuture();
	}
}
