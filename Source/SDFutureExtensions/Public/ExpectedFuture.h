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

		template<typename F, typename P, typename R, typename TLifetimeMonitor>
		class TExpectedFutureContinuationTask;

		template<typename F, typename R>
		class TExpectedFutureInitQueuedWork;

		template<typename F, typename P, typename R, typename TLifetimeMonitor>
		class TExpectedFutureContinuationQueuedWork;
	}

	template <typename T>
	TExpectedFuture<T> MakeErrorFuture(Error&& InError);

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

	template<typename ResultType>
	class TExpectedPromiseState
	{
	public:
		TExpectedPromiseState(FutureExecutionDetails::FExecutionDetails InExecutionDetails)
			: CompletionTask(TGraphTask<FNullGraphTask>::CreateTask().ConstructAndHold(TStatId(), ENamedThreads::AnyThread))
			, ValueSetSync(0)
			, ExecutionDetails(MoveTemp(InExecutionDetails))
		{
		}

		void SetValue(const TExpected<ResultType>& Result)
		{
			if (FPlatformAtomics::InterlockedCompareExchange(&ValueSetSync, 1, 0) == 0)
			{
				Value = Result;
				FPlatformAtomics::InterlockedExchange(&ValueSetSync, 2);

				Trigger();
			}
		}

		void SetValue(TExpected<ResultType>&& Result)
		{
			if (FPlatformAtomics::InterlockedCompareExchange(&ValueSetSync, 1, 0) == 0)
			{
				Value = MoveTemp(Result);
				FPlatformAtomics::InterlockedExchange(&ValueSetSync, 2);

				Trigger();
			}
		}

		TExpected<ResultType> Get() const
		{
			return Value;
		}

		FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return ExecutionDetails;
		}

		bool IsSet() const
		{
			return FPlatformAtomics::AtomicRead(&ValueSetSync) == 2;
		}

		FGraphEventRef GetCompletionEvent() const
		{
			return CompletionTask->GetCompletionEvent();
		}

	private:
		void Trigger()
		{
			TArray<FBaseGraphTask*> EmptySubsequentsArray;
			CompletionTask->GetCompletionEvent()->DispatchSubsequents(EmptySubsequentsArray);
		}

		TGraphTask<FNullGraphTask>* CompletionTask;

		// By design, cancellation and valid value setting is a race - cancellation is always *best attempt*.
		// Trying to set a promise value that's already been set *should* just fail silently
		int8 ValueSetSync;

		FutureExecutionDetails::FExecutionDetails ExecutionDetails;

		TExpected<ResultType> Value;
	};

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
		template<typename T, typename Enabled = void>
		class TWeakRefType
		{
		public:
			TWeakRefType(T*)
			{
				static_assert(std::is_void<Enabled>::value == false, "Lifetime management requires object with managed lifetime (ie. subclass of UObject, TSharedFromThis<T>)");
			}
			bool Pin() const;
		};

		template<typename T>
		class TWeakRefType<T, typename std::enable_if<std::is_base_of<TSharedFromThis<T, ESPMode::ThreadSafe>, T>::value>::type> : TWeakPtr<T, ESPMode::ThreadSafe>
		{
		public:
			explicit TWeakRefType(T* Obj) : TWeakPtr<T, ESPMode::ThreadSafe>(GetWeakPtr(Obj)) {}
			static TWeakPtr<T, ESPMode::ThreadSafe> GetWeakPtr(T* Obj)
			{
				if (Obj)
				{
					return Obj->AsShared();
				}
				return TWeakPtr<T, ESPMode::ThreadSafe>();
			}
			TSharedPtr<T, ESPMode::ThreadSafe> Pin() const { return TWeakPtr<T, ESPMode::ThreadSafe>::Pin(); }
		};

		template<typename T>
		class TWeakRefType<T, typename std::enable_if<std::is_base_of<TSharedFromThis<T, ESPMode::NotThreadSafe>, T>::value>::type> : TWeakPtr<T, ESPMode::NotThreadSafe>
		{
		public:
			explicit TWeakRefType(T* Obj) : TWeakPtr<T, ESPMode::NotThreadSafe>(GetWeakPtr(Obj)) {}
			static TWeakPtr<T, ESPMode::NotThreadSafe> GetWeakPtr(T* Obj)
			{
				if (Obj)
				{
					return Obj->AsShared();
				}
				return TWeakPtr<T, ESPMode::NotThreadSafe>();
			}
			TSharedPtr<T, ESPMode::NotThreadSafe> Pin() const { return TWeakPtr<T, ESPMode::NotThreadSafe>::Pin(); }
		};

		template<typename T>
		class TWeakRefType<T, typename std::enable_if<std::is_base_of<UObject, T>::value>::type> : TWeakObjectPtr<T>
		{
		public:
			explicit TWeakRefType(T* Obj) : TWeakObjectPtr<T>(Obj) {}
			T* Pin() const { return TWeakObjectPtr<T>::Get(); }
		};

		template<typename T>
		class TLifetimeMonitor
		{
		public:
			explicit TLifetimeMonitor(T* Obj) : WeakRef(Obj) {}
			TLifetimeMonitor(const TLifetimeMonitor<T>& Other) : WeakRef(Other.WeakRef) {}

			auto Pin() const { return WeakRef.Pin(); }

		private:
			TWeakRefType<T> WeakRef;
		};

		template<>
		class TLifetimeMonitor<void>
		{
		public:
			constexpr bool Pin() const { return true; }
		};

		using namespace FutureExtensionTypeTraits;

		template<class F, class P, typename LifetimeMonitorType>
		auto ThenImpl(F&& Func, const TExpectedFuture<P>& PrevFuture, FGraphEventRef ContinuationTrigger,
						const SD::FExpectedFutureOptions& FutureOptions, LifetimeMonitorType LifetimeMonitor)
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
				using ContinuationWorkType = FutureExtensionTaskGraph::TExpectedFutureContinuationQueuedWork<F, P, UnwrappedReturnType, LifetimeMonitorType>;
				GThreadPool->AddQueuedWork(new ContinuationWorkType(Forward<F>(Func),
																	MoveTemp(Promise),
																	PrevFuture,
																	FutureOptions.GetCancellationTokenHandle(),
																	MoveTemp(LifetimeMonitor)));
			}
			else
			{
				using ContinuationTaskType = FutureExtensionTaskGraph::TExpectedFutureContinuationTask<F, P, UnwrappedReturnType, LifetimeMonitorType>;
				FGraphEventArray TriggerArray{ ContinuationTrigger };
				TGraphTask<ContinuationTaskType>::CreateTask(&TriggerArray)
					.ConstructAndDispatchWhenReady(Forward<F>(Func),
													MoveTemp(Promise),
													PrevFuture,
													FutureOptions.GetCancellationTokenHandle(),
													MoveTemp(LifetimeMonitor));
			}

			return Future;
		}
	}

	template<class R>
	class TExpectedFuture
	{
	public:
		using ResultType = R;
		using UnwrappedResultType = typename FutureExtensionTypeTraits::TUnwrap<R>::Type;
		using ExpectedResultType = TExpected<UnwrappedResultType>;

		TExpectedFuture(const TSharedRef<TExpectedPromiseState<ResultType>, ESPMode::ThreadSafe>& InPreviousPromise)
			: PreviousPromise(InPreviousPromise)
		{}

		TExpectedFuture(const TExpectedFuture<ResultType>& Other)
			: PreviousPromise(Other.PreviousPromise)
		{}

		TExpectedFuture<ResultType>& operator=(const TExpectedFuture<ResultType>& Other)
		{
			PreviousPromise = Other.PreviousPromise;
			return *this;
		}

		TExpectedFuture()
		{
		}

		TExpectedFuture(Error&& Error)
			: TExpectedFuture(MakeErrorFuture<R>(MoveTemp(Error)))
		{
		}

		TExpectedFuture(TExpectedFuture<ResultType>&& Other)
			: PreviousPromise(MoveTemp(Other.PreviousPromise))
		{
		}

		TExpectedFuture<ResultType>& operator=(TExpectedFuture<ResultType>&& Other)
		{
			PreviousPromise = MoveTemp(Other.PreviousPromise);
			return *this;
		}

		template<class F>
		auto Then(F&& Func, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions()) const
		{
			check(IsValid());
			return FutureContinuationDetails::ThenImpl(Forward<F>(Func), *this,
														PreviousPromise->GetCompletionEvent(), FutureOptions, FutureContinuationDetails::TLifetimeMonitor<void>());
		}

		template<class F, typename TOwnerType>
		auto Then(TOwnerType* Owner, F&& Func, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions()) const
		{
			check(IsValid());
			return FutureContinuationDetails::ThenImpl(Forward<F>(Func), *this,
														PreviousPromise->GetCompletionEvent(), FutureOptions, FutureContinuationDetails::TLifetimeMonitor<TOwnerType>(Owner));
		}

		bool IsReady() const
		{
			return IsValid() && PreviousPromise->IsSet();
		}

		ExpectedResultType Get() const
		{
			check(IsValid());
			return PreviousPromise->Get();
		}

		bool IsValid() const
		{
			return PreviousPromise.IsValid();
		}

		FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			check(IsValid());
			return PreviousPromise->GetExecutionDetails();
		}

	private:
		TSharedPtr<TExpectedPromiseState<ResultType>, ESPMode::ThreadSafe> PreviousPromise;
	};

	template <class R>
	class TExpectedPromise : public FCancellablePromise
	{
		using ExpectedResultType = TExpected<R>;

	public:
		TExpectedPromise(const FutureExecutionDetails::FExecutionDetails& InExecutionDetails =
							FutureExecutionDetails::FExecutionDetails())
			: State(MakeShared<TExpectedPromiseState<R>, ESPMode::ThreadSafe>(InExecutionDetails))
		{
		}

		TExpectedPromise(TExpectedPromise&& Other) = default;
		TExpectedPromise& operator=(TExpectedPromise&& Other) = default;

		TExpectedFuture<R> GetFuture()
		{
			return TExpectedFuture<R>(State);
		}

		bool IsSet() const
		{
			return State->IsSet();
		}

		void SetValue(const ExpectedResultType& Result)
		{
			State->SetValue(Result);
		}

		void SetValue(ExpectedResultType&& Result)
		{
			State->SetValue(Result);
		}

		void SetValue(R&& Result)
		{
			SetValue(SD::MakeReadyExpected(Forward<R>(Result)));
		}

		void SetValue(const R& Result)
		{
			SetValue(SD::MakeReadyExpected(Result));
		}

		FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return State->GetExecutionDetails();
		}

		void Cancel()
		{
			SetValue(SD::MakeCancelledExpected<R>());
		}

	private:
		TSharedRef<TExpectedPromiseState<R>, ESPMode::ThreadSafe> State;
	};

	template <>
	class TExpectedFuture<void>
	{
	public:
		using ResultType = void;
		using ExpectedResultType = TExpected<void>;

		TExpectedFuture(const TSharedRef<TExpectedPromiseState<void>, ESPMode::ThreadSafe>& InPreviousPromise)
			: PreviousPromise(InPreviousPromise)
		{}

		TExpectedFuture(const TExpectedFuture<void>& Other)
			: PreviousPromise(Other.PreviousPromise)
		{}

		TExpectedFuture<void>& operator=(const TExpectedFuture<void>& Other)
		{
			PreviousPromise = Other.PreviousPromise;
			return *this;
		}

		TExpectedFuture()
		{
		}

		TExpectedFuture(Error&& Error)
			: TExpectedFuture(MakeErrorFuture<void>(MoveTemp(Error)))
		{
		}

		TExpectedFuture(TExpectedFuture<ResultType>&& Other)
			: PreviousPromise(MoveTemp(Other.PreviousPromise))
		{}

		TExpectedFuture<ResultType>& operator=(TExpectedFuture<ResultType>&& Other)
		{
			PreviousPromise = MoveTemp(Other.PreviousPromise);
			return *this;
		}

		bool IsReady() const
		{
			return IsValid() && PreviousPromise->IsSet();
		}

		template<class F>
		auto Then(F&& Func, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions()) const
		{
			check(IsValid());
			return FutureContinuationDetails::ThenImpl(Forward<F>(Func), *this,
														PreviousPromise->GetCompletionEvent(), FutureOptions, FutureContinuationDetails::TLifetimeMonitor<void>());
		}

		template<class F, typename TOwnerType>
		auto Then(TOwnerType* Owner, F&& Func, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions()) const
		{
			check(IsValid());
			return FutureContinuationDetails::ThenImpl(Forward<F>(Func), *this,
														PreviousPromise->GetCompletionEvent(), FutureOptions, FutureContinuationDetails::TLifetimeMonitor<TOwnerType>(Owner));
		}

		ExpectedResultType Get() const
		{
			check(IsValid());
			return PreviousPromise->Get();
		}

		bool IsValid() const
		{
			return PreviousPromise.IsValid();
		}

		FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			check(IsValid());
			return PreviousPromise->GetExecutionDetails();
		}

	private:
		TSharedPtr<TExpectedPromiseState<void>, ESPMode::ThreadSafe> PreviousPromise;
	};

	template<>
	class TExpectedPromise<void> : public FCancellablePromise
	{
	public:

		using ExpectedResultType = TExpected<void>;

		TExpectedPromise(const FutureExecutionDetails::FExecutionDetails& InExecutionDetails =
							FutureExecutionDetails::FExecutionDetails())
			: State(MakeShared<TExpectedPromiseState<void>, ESPMode::ThreadSafe>(InExecutionDetails))
		{
		}

		TExpectedPromise(TExpectedPromise&& Other) = default;
		TExpectedPromise& operator=(TExpectedPromise&& Other) = default;

		TExpectedFuture<void> GetFuture()
		{
			return TExpectedFuture<void>(State);
		}

		bool IsSet() const
		{
			return State->IsSet();
		}

		void SetValue(ExpectedResultType&& InResult)
		{
			State->SetValue(InResult);
		}

		void SetValue()
		{
			SetValue(SD::MakeReadyExpected());
		}

		FutureExecutionDetails::FExecutionDetails GetExecutionDetails() const
		{
			return State->GetExecutionDetails();
		}

		void Cancel()
		{
			SetValue(SD::MakeCancelledExpected());
		}

	private:
		TSharedRef<TExpectedPromiseState<void>, ESPMode::ThreadSafe> State;
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

	template <typename T>
	TExpectedFuture<T> MakeErrorFuture(Error&& InError)
	{
		TExpectedPromise<T> ErrorPromise = TExpectedPromise<T>();
		ErrorPromise.SetValue(MakeErrorExpected<T>(MoveTemp(InError)));
		return ErrorPromise.GetFuture();
	}
}
