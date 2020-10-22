// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "Async/Async.h"

//TODO: can be rewriten with C++17 `if constexpr`

namespace SD
{
	namespace FutureExtensionTaskGraph
	{
		using namespace FutureExtensionTypeTraits;

		namespace Details
		{
			template<typename OldType, typename NewType>
			void ConvertIncompleteExpectedToNewPromise(const TExpected<OldType>& From,
				TExpectedPromise<NewType>& ToSet)
			{
				TExpected<NewType> ConvertedExpected = ConvertIncomplete<NewType, OldType>(From);
				ToSet.SetValue(MoveTemp(ConvertedExpected));
			}

			template<typename OldType>
			void ConvertIncompleteExpectedToNewPromise(const TExpected<OldType>& From,
				TExpectedPromise<void>& ToSet)
			{
				TExpected<void> ConvertedExpected = ConvertIncomplete(From);
				ToSet.SetValue(MoveTemp(ConvertedExpected));
			}

			/*
			*	Executes an initial function that:
			*		- Takes no parameters
			*		- Returns TExpected<TExpectedFuture<PromiseType>>
			*
			*	Resolve the function to get the internal future, resolve it, and use the returned value to set the promise.
			*/
			template<typename Function, typename PromiseType,
				typename S = typename TInitialFunctorTypes<Function>::Specialization,
				typename S::ReturnsExpectedFuture::Type* = nullptr>
				void ExecuteInitialFunction(Function&& InitialFunction,
					TExpectedPromise<PromiseType>& InitialPromise)
			{
				auto Future = InitialFunction();

				Future.Then([p = MoveTemp(InitialPromise)](TExpected<PromiseType> Expected) mutable {
					p.SetValue(MoveTemp(Expected));
				});
			}

			/*
			*	Executes an initial function that:
			*		- Takes no parameters
			*		- Returns TExpected<PromiseType> or PromiseType
			*/
			template<typename Function, typename PromiseType,
				typename S = typename TInitialFunctorTypes<Function>::Specialization,
				typename S::ReturnsPlainValueOrExpected::Type* = nullptr>
				void ExecuteInitialFunction(Function&& InitialFunction,
					TExpectedPromise<PromiseType>& InitialPromise)
			{
				InitialPromise.SetValue(InitialFunction());
			}

			/*
			*	Executes an initial function that:
			*		- Takes no parameters
			*		- Returns void
			*/
			template<typename Function,
				typename S = typename TInitialFunctorTypes<Function>::Specialization,
				typename S::ReturnsVoid::Type* = nullptr>
				void ExecuteInitialFunction(Function&& InitialFunction,
					TExpectedPromise<void>& InitialPromise)
			{
				InitialFunction();
				InitialPromise.SetValue();
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of TExpected<FutureType>
			*		- Returns TExpectedFuture<TExpectedFuture<PromiseType>>
			*
			*	Always call the continuation, regardless of the state of PreviousFuture and 'unwrap' the returned future.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsExpectedFutureAndTakesExpected::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());

				ContinuationFunction(PreviousFuture.Get())
					.Then([p = MoveTemp(ContinuationPromise)](TExpected<PromiseType> Expected) mutable {
					p.SetValue(MoveTemp(Expected));
				});
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of FutureType
			*		- Returns TExpectedFuture<TExpectedFuture<PromiseType>>
			*
			*	Only call the continuation if PreviousFuture completed successfully, and 'unwrap' the returned future.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsExpectedFutureAndTakesValue::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				auto PrevExpected = PreviousFuture.Get();
				if (PrevExpected.IsCompleted())
				{
					ContinuationFunction(*PrevExpected)
						.Then([p = MoveTemp(ContinuationPromise)](TExpected<PromiseType> Expected) mutable {
						p.SetValue(MoveTemp(Expected));
					});
				}
				else
				{
					ConvertIncompleteExpectedToNewPromise(PrevExpected, ContinuationPromise);
				}
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of void
			*		- Returns TExpectedFuture<TExpectedFuture<PromiseType>>
			*
			*	Only call the continuation if PreviousFuture completed successfully, and 'unwrap' the returned future.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsExpectedFutureAndTakesVoid::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				auto PrevExpected = PreviousFuture.Get();
				if (PrevExpected.IsCompleted())
				{
					ContinuationFunction()
						.Then([p = MoveTemp(ContinuationPromise)](TExpected<PromiseType> Expected) mutable {
						p.SetValue(MoveTemp(Expected));
					});
				}
				else
				{
					ConvertIncompleteExpectedToNewPromise(PrevExpected, ContinuationPromise);
				}
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of TExpected<FutureType>
			*		- Returns TExpected<PromiseType> or PromiseType
			*
			*	Always call the continuation, regardless of the state of PreviousFuture.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsPlainValueOrExpectedAndTakesExpected::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				ContinuationPromise.SetValue(ContinuationFunction(PreviousFuture.Get()));
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of FutureType
			*		- Returns TExpected<PromiseType> or PromiseType
			*
			*	Only call the continuation if PreviousFuture completed successfully.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsPlainValueOrExpectedAndTakesValue::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				auto PrevExpected = PreviousFuture.Get();
				if (PrevExpected.IsCompleted())
				{
					ContinuationPromise.SetValue(ContinuationFunction(*PrevExpected));
				}
				else
				{
					ConvertIncompleteExpectedToNewPromise(PrevExpected, ContinuationPromise);
				}
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of void
			*		- Returns TExpected<PromiseType> or PromiseType
			*
			*	Only call the continuation if PreviousFuture completed successfully.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsPlainValueOrExpectedAndTakesVoid::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				auto PrevExpected = PreviousFuture.Get();
				if (PrevExpected.IsCompleted())
				{
					ContinuationPromise.SetValue(ContinuationFunction());
				}
				else
				{
					ConvertIncompleteExpectedToNewPromise(PrevExpected, ContinuationPromise);
				}
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of TExpected<FutureType>
			*		- Returns void
			*
			*	Always call the continuation, regardless of the state of PreviousFuture.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsVoidAndTakesExpected::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				ContinuationFunction(PreviousFuture.Get());
				ContinuationPromise.SetValue();
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of FutureType
			*		- Returns void
			*
			*	Only call the continuation if PreviousFuture completed successfully.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsVoidAndTakesValue::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				auto PrevExpected = PreviousFuture.Get();
				if (PrevExpected.IsCompleted())
				{
					ContinuationFunction(*PrevExpected);
					ContinuationPromise.SetValue();
				}
				else
				{
					ConvertIncompleteExpectedToNewPromise(PrevExpected, ContinuationPromise);
				}
			}

			/*
			*	Executes a continuation function that:
			*		- Takes a parameter of void
			*		- Returns void
			*
			*	Only call the continuation if PreviousFuture completed successfully.
			*/
			template<typename Function, typename FutureType, typename PromiseType,
				typename S = typename TContinuationFunctorTypes<Function, FutureType>::Specialization,
				typename S::ReturnsVoidAndTakesVoid::Type* = nullptr>
				void ExecuteContinuationFunction(Function&& ContinuationFunction,
					TExpectedFuture<FutureType>& PreviousFuture,
					TExpectedPromise<PromiseType>& ContinuationPromise)
			{
				check(PreviousFuture.IsReady());
				auto PrevExpected = PreviousFuture.Get();
				if (PrevExpected.IsCompleted())
				{
					ContinuationFunction();
					ContinuationPromise.SetValue();
				}
				else
				{
					ConvertIncompleteExpectedToNewPromise(PrevExpected, ContinuationPromise);
				}
			}
		}

		inline void TryAddPromiseToCancellationHandle(WeakSharedCancellationHandlePtr WeakHandle,
			const SharedCancellablePromiseRef& Promise)
		{
			if (WeakHandle.IsValid())
			{
				if (SharedCancellationHandlePtr CancellationHandle = WeakHandle.Pin())
				{
					CancellationHandle->AddPromise(Promise);
				}
			}
		}

		template<typename F, typename R>
		class TExpectedFutureInitTask : public FAsyncGraphTaskBase
		{
			using SharedPromiseRef = TSharedRef<TExpectedPromise<R>, ESPMode::ThreadSafe>;
		public:
			TExpectedFutureInitTask(F&& InFunc, const SharedPromiseRef& InPromise,
				WeakSharedCancellationHandlePtr WeakCancellationHandle);

			void DoTask(ENamedThreads::Type, const FGraphEventRef&);

			ENamedThreads::Type GetDesiredThread();

		private:

			SharedPromiseRef SharedPromise;
			F InitFunctor;
		};


		template<typename F, typename P, typename R>
		class TExpectedFutureContinuationTask : public FAsyncGraphTaskBase
		{
			using SharedPromiseRef = TSharedRef<TExpectedPromise<R>, ESPMode::ThreadSafe>;

		public:
			TExpectedFutureContinuationTask(F&& InFunction, const SharedPromiseRef& InPromise,
				const TExpectedFuture<P>& InPrevFuture,
				WeakSharedCancellationHandlePtr WeakCancellationHandle);

			void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent);

			ENamedThreads::Type GetDesiredThread();

		private:

			SharedPromiseRef SharedPromise;
			TExpectedFuture<P> PrevFuture;

			F ContinuationFunction;
		};

		template<typename F, typename R>
		TExpectedFutureInitTask<F, R>::TExpectedFutureInitTask(F&& InFunc,
			const TExpectedFutureInitTask<F, R>::SharedPromiseRef& InPromise,
			WeakSharedCancellationHandlePtr WeakCancellationHandle)
			: SharedPromise(InPromise)
			, InitFunctor(MoveTemp(InFunc))
		{
			TryAddPromiseToCancellationHandle(WeakCancellationHandle, SharedPromise);
		}

		template<typename F, typename R>
		void TExpectedFutureInitTask<F, R>::DoTask(ENamedThreads::Type, const FGraphEventRef&)
		{
			if (!SharedPromise->IsSet())
			{
				Details::ExecuteInitialFunction(MoveTemp(InitFunctor), *SharedPromise);
			}
		}

		template<typename F, typename R>
		ENamedThreads::Type TExpectedFutureInitTask<F, R>::GetDesiredThread()
		{
			return SharedPromise->GetExecutionDetails().ExecutionThread;
		}

		template<typename F, typename P, typename R>
		TExpectedFutureContinuationTask<F, P, R>::TExpectedFutureContinuationTask(F&& InFunction,
			const TExpectedFutureContinuationTask<F, P, R>::SharedPromiseRef& InPromise,
			const TExpectedFuture<P>& InPrevFuture,
			WeakSharedCancellationHandlePtr WeakCancellationHandle)
			: SharedPromise(InPromise)
			, PrevFuture(InPrevFuture)
			, ContinuationFunction(MoveTemp(InFunction))
		{
			TryAddPromiseToCancellationHandle(WeakCancellationHandle, SharedPromise);
		}

		template<typename F, typename P, typename R>
		void TExpectedFutureContinuationTask<F, P, R>::DoTask(ENamedThreads::Type, const FGraphEventRef&)
		{
			if (!SharedPromise->IsSet())
			{
				Details::ExecuteContinuationFunction(MoveTemp(ContinuationFunction), PrevFuture, *SharedPromise);
			}
		}

		template<typename F, typename P, typename R>
		ENamedThreads::Type TExpectedFutureContinuationTask<F, P, R>::GetDesiredThread()
		{
			return SharedPromise->GetExecutionDetails().ExecutionThread;
		}

		template<typename R>
		class TExpectedFutureQueuedWork : public IQueuedWork
		{
			using SharedPromiseRef = TSharedRef<TExpectedPromise<R>, ESPMode::ThreadSafe>;

		public:
			TExpectedFutureQueuedWork(const SharedPromiseRef& InPromise,
				WeakSharedCancellationHandlePtr WeakCancellationHandle);
			virtual ~TExpectedFutureQueuedWork() {}

		protected:
			virtual void DoWork() = 0;
			SharedPromiseRef GetSharedPromise() const;

		private:
			// Begin IQueuedWork override
			virtual void DoThreadedWork() final;
			virtual void Abandon() final;
			// End IQueuedWork override

			void Finalize();

		private:
			SharedPromiseRef SharedPromise;
			SharedCancellationHandlePtr CancellationHandle;
		};

		template<typename R>
		TSharedRef<TExpectedPromise<R>, ESPMode::ThreadSafe> TExpectedFutureQueuedWork<R>::GetSharedPromise() const
		{
			return SharedPromise;
		}

		template<typename R>
		void TExpectedFutureQueuedWork<R>::DoThreadedWork()
		{
			DoWork();
			Finalize();
		}

		template<typename R>
		TExpectedFutureQueuedWork<R>::TExpectedFutureQueuedWork(const SharedPromiseRef& InPromise,
			WeakSharedCancellationHandlePtr WeakCancellationHandle)
			: SharedPromise(InPromise)
		{
			//Task queued on a thread pool can be abandoned, which we conflate to cancellation.
			//This requires them to always have a valid cancellation handle that we can use in this case.
			//Create one if the promise doesn't already have one associated.
			CancellationHandle = WeakCancellationHandle.IsValid() ? WeakCancellationHandle.Pin() :
				CreateCancellationHandle();
			TryAddPromiseToCancellationHandle(CancellationHandle, SharedPromise);
		}

		template<typename R>
		void TExpectedFutureQueuedWork<R>::Abandon()
		{
			//Treat abandonment as a cancellation
			CancellationHandle->Cancel();
			Finalize();
		}

		template<typename R>
		void TExpectedFutureQueuedWork<R>::Finalize()
		{
			//Queued work is allocated when being added to the pool, so we need to delete it when we're finished.
			delete this;
		}

		template<typename F, typename R>
		class TExpectedFutureInitQueuedWork : public TExpectedFutureQueuedWork<R>
		{
			using SharedPromiseRef = TSharedRef<TExpectedPromise<R>, ESPMode::ThreadSafe>;
		public:
			TExpectedFutureInitQueuedWork(F&& InFunc, const SharedPromiseRef& InPromise,
				WeakSharedCancellationHandlePtr WeakCancellationHandle);

			virtual ~TExpectedFutureInitQueuedWork() {}

			// Begin TExpectedFutureQueuedWork override
			virtual void DoWork() final;
			// End TExpectedFutureQueuedWork override

		private:

			F InitFunctor;
		};

		template<typename F, typename R>
		TExpectedFutureInitQueuedWork<F, R>::TExpectedFutureInitQueuedWork(F&& InFunc,
			const SharedPromiseRef& InPromise,
			WeakSharedCancellationHandlePtr WeakCancellationHandle)
			: TExpectedFutureQueuedWork<R>(InPromise, WeakCancellationHandle)
			, InitFunctor(MoveTemp(InFunc))
		{}

		template<typename F, typename R>
		void TExpectedFutureInitQueuedWork<F, R>::DoWork()
		{
			TExpectedFutureInitQueuedWork::SharedPromiseRef Promise = TExpectedFutureQueuedWork<R>::GetSharedPromise();
			if (!Promise->IsSet())
			{
				Details::ExecuteInitialFunction(MoveTemp(InitFunctor), *Promise);
			}
		}

		template<typename F, typename P, typename R>
		class TExpectedFutureContinuationQueuedWork : public TExpectedFutureQueuedWork<R>
		{
			using SharedPromiseRef = TSharedRef<TExpectedPromise<R>, ESPMode::ThreadSafe>;

		public:
			TExpectedFutureContinuationQueuedWork(F&& InFunction, const SharedPromiseRef& InPromise,
				const TExpectedFuture<P>& InPrevFuture,
				WeakSharedCancellationHandlePtr WeakCancellationHandle);
			virtual ~TExpectedFutureContinuationQueuedWork() {}

			// Begin TExpectedFutureQueuedWork override
			virtual void DoWork() final;
			// End TExpectedFutureQueuedWork override

		private:

			TExpectedFuture<P> PrevFuture;
			F ContinuationFunction;
		};

		template<typename F, typename P, typename R>
		TExpectedFutureContinuationQueuedWork<F, P, R>::TExpectedFutureContinuationQueuedWork(F&& InFunction,
			const SharedPromiseRef& InPromise,
			const TExpectedFuture<P>& InPrevFuture,
			WeakSharedCancellationHandlePtr WeakCancellationHandle)
			: TExpectedFutureQueuedWork<R>(InPromise, WeakCancellationHandle)
			, PrevFuture(InPrevFuture)
			, ContinuationFunction(MoveTemp(InFunction))
		{}

		template<typename F, typename P, typename R>
		void TExpectedFutureContinuationQueuedWork<F, P, R>::DoWork()
		{
			TExpectedFutureContinuationQueuedWork::SharedPromiseRef Promise = TExpectedFutureQueuedWork<R>::GetSharedPromise();
			if (!Promise->IsSet())
			{
				//We're running on a thread pool, so we can just wait on our previous future.
				PrevFuture.Get();
				Details::ExecuteContinuationFunction(MoveTemp(ContinuationFunction), PrevFuture, *Promise);
			}
		}
	}
}
