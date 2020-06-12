// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "Templates/SharedPointer.h"
#include "Misc/ScopeLock.h"

namespace SD
{
	class FCancellationHandle;

	class FCancellablePromise : public TSharedFromThis<FCancellablePromise, ESPMode::ThreadSafe>
	{
		friend class FCancellationHandle;

	public:
		virtual ~FCancellablePromise() {}

	protected:
		virtual void Cancel() = 0;
	};

	using WeakSharedCancellablePromisePtr = TWeakPtr<FCancellablePromise, ESPMode::ThreadSafe>;
	using SharedCancellablePromiseRef = TSharedRef<FCancellablePromise, ESPMode::ThreadSafe>;
	using SharedCancellablePromisePtr = TSharedPtr<FCancellablePromise, ESPMode::ThreadSafe>;

	using WeakSharedCancellationHandlePtr = TWeakPtr<FCancellationHandle, ESPMode::ThreadSafe>;
	using SharedCancellationHandleRef = TSharedRef<FCancellationHandle, ESPMode::ThreadSafe>;
	using SharedCancellationHandlePtr = TSharedPtr<FCancellationHandle, ESPMode::ThreadSafe>;

	class FCancellationHandle : public TSharedFromThis<FCancellationHandle, ESPMode::ThreadSafe>
	{
	public:
		inline void AddPromise(const SharedCancellablePromiseRef& Promise)
		{
			FScopeLock Lock(&PromisesLock);

			if(!bCancelled)
			{
				PromisesToCancel.Push(WeakSharedCancellablePromisePtr(Promise));
			}
			else
			{
				Promise->Cancel();
			}
		}

		inline void Cancel()
		{
			FScopeLock Lock(&PromisesLock);
			bCancelled = true;

			for (auto& WeakPromise : PromisesToCancel)
			{
				if (SharedCancellablePromisePtr Promise = WeakPromise.Pin())
				{
					Promise->Cancel();
				}
			}

			PromisesToCancel.Empty();
		}

	private:
		FCriticalSection PromisesLock;
		TArray<WeakSharedCancellablePromisePtr> PromisesToCancel;
		bool bCancelled = false;
	};

	inline SharedCancellationHandleRef CreateCancellationHandle()
	{
		return MakeShared<FCancellationHandle, ESPMode::ThreadSafe>();
	}
}