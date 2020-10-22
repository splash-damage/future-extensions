// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "CancellationHandle.h"
#include "Async/TaskGraphInterfaces.h"
#include "FutureLogging.h"

namespace SD
{
	enum class EExpectedFutureExecutionPolicy
	{
		//Specifies that the function body associated with this future should be run on whatever
		//thread it is being scheduled from using EAsyncExecution::TaskGraph
		Current,

		//Specifies that the function body associated with this future should be run on whatever
		//thread the antecedent future was scheduled on. If there's no antecedent, defaults to
		//Current.
		Inline,

		//Specifies that the function body associated with this future should be run on a specific
		//thread (as described by ENamedThreads) using EAsyncExecution::TaskGraph.
		NamedThread,

		//Specifies that the function body associated with this future should be run on the thread
		//pool as provided by EAsyncExecution::ThreadPool
		ThreadPool
	};

	class FExpectedFutureOptionsBuilder;

	class FExpectedFutureOptions
	{
		struct Properties;

	public:
		FExpectedFutureOptions() = default;

		FExpectedFutureOptions(const SharedCancellationHandleRef& InCancellationHandle);
		FExpectedFutureOptions(const EExpectedFutureExecutionPolicy InExecutionPolicy);
		FExpectedFutureOptions(const ENamedThreads::Type InDesiredExecutionThread);

		WeakSharedCancellationHandlePtr GetCancellationTokenHandle() const;
		EExpectedFutureExecutionPolicy GetExecutionPolicy() const;
		ENamedThreads::Type GetDesiredExecutionThread() const;

	private:
		friend class FExpectedFutureOptionsBuilder;

		FExpectedFutureOptions(const FExpectedFutureOptions::Properties& InProperties);

		struct Properties
		{
			WeakSharedCancellationHandlePtr CancellationTokenHandle = nullptr;
			EExpectedFutureExecutionPolicy ExecutionPolicy = EExpectedFutureExecutionPolicy::Current;
			ENamedThreads::Type DesiredExecutionThread = ENamedThreads::UnusedAnchor;

			void Sanitize();
		};

		Properties OptionsProperties;
	};

	class FExpectedFutureOptionsBuilder
	{
	public:
		FExpectedFutureOptionsBuilder() = default;

		FExpectedFutureOptionsBuilder& SetCancellationTokenHandle(const SharedCancellationHandleRef& InCancellationHandle);
		FExpectedFutureOptionsBuilder& SetExecutionPolicy(const EExpectedFutureExecutionPolicy InExecutionPolicy);
		FExpectedFutureOptionsBuilder& SetDesiredExecutionThread(const ENamedThreads::Type InNamedThread);

		FExpectedFutureOptions Build();

	private:
		FExpectedFutureOptions::Properties OptionsProperties;
	};

	inline FExpectedFutureOptionsBuilder&
		FExpectedFutureOptionsBuilder::SetCancellationTokenHandle(const SharedCancellationHandleRef& InCancellationHandle)
	{
		OptionsProperties.CancellationTokenHandle = InCancellationHandle;
		return *this;
	}

	inline FExpectedFutureOptionsBuilder&
		FExpectedFutureOptionsBuilder::SetExecutionPolicy(const EExpectedFutureExecutionPolicy InExecutionPolicy)
	{
		OptionsProperties.ExecutionPolicy = InExecutionPolicy;
		return *this;
	}

	inline FExpectedFutureOptionsBuilder&
		FExpectedFutureOptionsBuilder::SetDesiredExecutionThread(const ENamedThreads::Type InNamedThread)
	{
		OptionsProperties.ExecutionPolicy = EExpectedFutureExecutionPolicy::NamedThread;
		OptionsProperties.DesiredExecutionThread = InNamedThread;
		return *this;
	}

	inline FExpectedFutureOptions FExpectedFutureOptionsBuilder::Build()
	{
		OptionsProperties.Sanitize();
		return FExpectedFutureOptions(OptionsProperties);
	}

	inline FExpectedFutureOptions::FExpectedFutureOptions(const SharedCancellationHandleRef& InCancellationHandle)
	{
		OptionsProperties.CancellationTokenHandle = InCancellationHandle;
		OptionsProperties.Sanitize();
	}

	inline FExpectedFutureOptions::FExpectedFutureOptions(const EExpectedFutureExecutionPolicy InExecutionPolicy)
	{
		OptionsProperties.ExecutionPolicy = InExecutionPolicy;
		OptionsProperties.Sanitize();
	}

	inline FExpectedFutureOptions::FExpectedFutureOptions(const ENamedThreads::Type InDesiredExecutionThread)
	{
		OptionsProperties.ExecutionPolicy = EExpectedFutureExecutionPolicy::NamedThread;
		OptionsProperties.DesiredExecutionThread = InDesiredExecutionThread;
		OptionsProperties.Sanitize();
	}

	inline FExpectedFutureOptions::FExpectedFutureOptions(const FExpectedFutureOptions::Properties& InProperties)
	{
		OptionsProperties = InProperties;
	}

	inline WeakSharedCancellationHandlePtr FExpectedFutureOptions::GetCancellationTokenHandle() const
	{
		return OptionsProperties.CancellationTokenHandle;
	}

	inline EExpectedFutureExecutionPolicy FExpectedFutureOptions::GetExecutionPolicy() const
	{
		return OptionsProperties.ExecutionPolicy;
	}

	inline ENamedThreads::Type FExpectedFutureOptions::GetDesiredExecutionThread() const
	{
		return OptionsProperties.DesiredExecutionThread;
	}


	inline void FExpectedFutureOptions::Properties::Sanitize()
	{
		if (ExecutionPolicy == EExpectedFutureExecutionPolicy::NamedThread &&
			DesiredExecutionThread == ENamedThreads::UnusedAnchor)
		{
			ensureMsgf(false, TEXT("NamedThread execution policy specified but no NamedThread specified."));
			UE_LOG(LogFutureExtensions, Error, TEXT("Setting execution policy to Current as no NamedThread specified"));
			ExecutionPolicy = EExpectedFutureExecutionPolicy::Current;
		}
	}
}