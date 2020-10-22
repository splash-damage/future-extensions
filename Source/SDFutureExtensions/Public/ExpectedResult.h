// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "Misc/Optional.h"
#include "Error.h"

namespace SD
{
	enum class EExpectedResultState : uint8
	{
		Incomplete,
		Completed,
		Cancelled,
		Error
	};
	
	class TExpectedBase
	{
	public:
		
		TExpectedBase()
			: State(EExpectedResultState::Incomplete)
		{}

		virtual ~TExpectedBase()
		{
			InternalError.Reset();
		}

		TExpectedBase(const TExpectedBase& InValue) = default;
		TExpectedBase(TExpectedBase&& InValue) = default;

		TExpectedBase& operator=(const TExpectedBase& InExpected) = default;
		TExpectedBase& operator=(TExpectedBase&& InValue) = default;

		TExpectedBase(const Error& InError)
			: State(EExpectedResultState::Error)
			, InternalError(MakeShareable(new Error(InError)))
		{}

		TExpectedBase(Error&& InError)
			: State(EExpectedResultState::Error)
			, InternalError(MakeShareable(new Error(InError)))
		{}

		TExpectedBase& operator=(const Error& InError)
		{
			InternalError = MakeShareable(new Error(InError));
			State = EExpectedResultState::Error;

			return *this;
		}

		TExpectedBase& operator=(Error&& InError)
		{
			InternalError = MakeShareable(new Error(InError));
			State = EExpectedResultState::Error;

			return *this;
		}

		bool IsCompleted() const
		{
			return State == EExpectedResultState::Completed;
		}

		bool IsError() const
		{
			return State == EExpectedResultState::Error;
		}

		bool IsCancelled() const
		{
			return State == EExpectedResultState::Cancelled;
		}

		EExpectedResultState GetState() const
		{
			return State;
		}

		TSharedRef<Error, ESPMode::ThreadSafe> GetError() const
		{
			checkf(InternalError.IsValid() && IsError(), TEXT("Called GetError() on a non-error state TExpected"));
			return InternalError.ToSharedRef();
		}

	protected:

		struct FCancelledExpectedConstructorTag
		{};

		explicit TExpectedBase(FCancelledExpectedConstructorTag)
			: State(EExpectedResultState::Cancelled)
		{}

		void SetComplete()
		{
			check(State == EExpectedResultState::Incomplete);

			InternalError.Reset();
			State = EExpectedResultState::Completed;
		}

		EExpectedResultState State = EExpectedResultState::Incomplete;
		TSharedPtr<Error, ESPMode::ThreadSafe> InternalError = nullptr;
	};

	template <typename R>
	class TExpected : public TExpectedBase
	{
	public:
		using ResultType = R;

		TExpected(const ResultType& InValue)
			: TExpectedBase()
			, Value(InValue)
		{
			SetComplete();
		}

		TExpected(ResultType&& InValue)
			: TExpectedBase()
			, Value(Forward<R>(InValue))
		{
			SetComplete();
		}

		~TExpected()
		{	
			Value.Reset();
		}

		TExpected() = default;
		TExpected(const TExpected& InValue) = default;
		TExpected(TExpected&& InValue) = default;

		TExpected& operator=(const TExpected& InValue) = default;
		TExpected& operator=(TExpected&& InValue) = default;

		TExpected& operator=(const ResultType& InValue)
		{
			Value = InValue;
			SetComplete();

			return *this;
		}

		TExpected& operator=(ResultType&& InValue)
		{
			Value = MoveTempIfPossible(InValue);
			SetComplete();

			return *this;
		}

		TExpected(const Error& InError)
			: TExpectedBase(InError)
		{}

		TExpected(Error&& InError)
			: TExpectedBase(InError)
		{}

		TExpected& operator=(const Error& InError)
		{
			TExpectedBase::operator=(InError);
			Value.Reset();
			
			return *this;
		}

		TExpected& operator=(Error&& InError)
		{
			TExpectedBase::operator=(InError);
			Value.Reset();

			return *this;
		}

		TOptional<ResultType>& GetValue()
		{
			checkf(IsCompleted(), TEXT("Called GetValue() on a non-completed state TExpected"));
			return Value;
		}

		const TOptional<ResultType>& GetValue() const
		{
			checkf(IsCompleted(), TEXT("Called GetValue() on a non-completed state TExpected"));
			return Value;
		}

		ResultType& operator*()
		{
			return GetValue().GetValue();
		}

		const ResultType& operator*() const
		{
			return GetValue().GetValue();
		}

		static TExpected<R> MakeCancelled()
		{
			return TExpected<R>(TExpectedBase::FCancelledExpectedConstructorTag());
		}

	private:

		explicit TExpected(TExpectedBase::FCancelledExpectedConstructorTag Tag)
			: TExpectedBase(Tag)
		{}

		TOptional<ResultType> Value;
	};

	struct FVoidExpectedConstructorTag
	{};

	template <>
	class TExpected<void> : public TExpectedBase
	{
	public:
		using ResultType = void;

		//Dummy value constructor overload to match TExpected<T> usage
		explicit TExpected(FVoidExpectedConstructorTag)
			: TExpectedBase()
		{
			SetComplete();
		}

		TExpected() = default;
		TExpected(const TExpected& InValue) = default;
		TExpected(TExpected&& InValue) = default;

		TExpected& operator=(const TExpected& InValue) = default;
		TExpected& operator=(TExpected&& InValue) = default;

		TExpected(const Error& InError)
			: TExpectedBase(InError)
		{}

		TExpected(Error&& InError)
			: TExpectedBase(InError)
		{}

		TExpected& operator=(const Error& InError)
		{
			TExpectedBase::operator=(InError);
			return *this;
		}

		TExpected& operator=(Error&& InError)
		{
			TExpectedBase::operator=(InError);
			return *this;
		}

		static TExpected<void> MakeCancelled()
		{
			return TExpected<void>(TExpectedBase::FCancelledExpectedConstructorTag());
		}

	private:

		explicit TExpected(TExpectedBase::FCancelledExpectedConstructorTag Tag)
			: TExpectedBase(Tag)
		{}
	};

	template <typename R>
	TExpected<R> MakeReadyExpected(R&& InValue)
	{
		return TExpected<R>(Forward<R>(InValue));
	}

	template <typename R>
	TExpected<R> MakeReadyExpected(const R& InValue)
	{
		return TExpected<R>(InValue);
	}

	template <typename R>
	TExpected<R> MakeErrorExpected(const Error& InError)
	{
		return TExpected<R>(InError);
	}

	template <typename R>
	TExpected<R> MakeErrorExpected(Error&& InError)
	{
		return TExpected<R>(Forward<Error>(InError));
	}

	template <typename R>
	TExpected<R> MakeCancelledExpected()
	{
		return TExpected<R>::MakeCancelled();
	}

	inline TExpected<void> MakeReadyExpected()
	{
		return TExpected<void>(FVoidExpectedConstructorTag());
	}

	inline TExpected<void> MakeErrorExpected(const Error& InError)
	{
		return TExpected<void>(InError);
	}

	inline TExpected<void> MakeErrorExpected(Error&& InError)
	{
		return TExpected<void>(Forward<Error>(InError));
	}

	inline TExpected<void> MakeCancelledExpected()
	{
		return TExpected<void>::MakeCancelled();
	}

	template <typename R, typename P>
	TExpected<R> Convert(const TExpected<P>& Other, R&& ConvertValue = R())
	{
		switch (Other.GetState())
		{
		case EExpectedResultState::Completed:
			return MakeReadyExpected<R>(Forward<R>(ConvertValue));
		case EExpectedResultState::Cancelled:
			return MakeCancelledExpected<R>();
		case EExpectedResultState::Error:
			return MakeErrorExpected<R>(*Other.GetError());
		case EExpectedResultState::Incomplete:
		default:
			return TExpected<R>();
		}
	}

	template <typename P>
	TExpected<void> Convert(const TExpected<P>& Other)
	{
		switch (Other.GetState())
		{
		case EExpectedResultState::Completed:
			return MakeReadyExpected();
		case EExpectedResultState::Cancelled:
			return MakeCancelledExpected();
		case EExpectedResultState::Error:
			return MakeErrorExpected(*Other.GetError());
		case EExpectedResultState::Incomplete:
		default:
			return TExpected<void>();
		}
	}

	template <typename R, typename P>
	TExpected<R> ConvertIncomplete(const TExpected<P>& Other)
	{
		switch (Other.GetState())
		{
		case EExpectedResultState::Cancelled:
			return MakeCancelledExpected<R>();
		case EExpectedResultState::Error:
			return MakeErrorExpected<R>(*Other.GetError());
		case EExpectedResultState::Completed:
			checkf(false, TEXT("This should only be called from incomplete TExpected"));
		case EExpectedResultState::Incomplete:
		default:
			return TExpected<R>();
		}
	}

	template <typename P>
	TExpected<void> ConvertIncomplete(const TExpected<P>& Other)
	{
		switch (Other.GetState())
		{
		case EExpectedResultState::Cancelled:
			return MakeCancelledExpected<void>();
		case EExpectedResultState::Error:
			return MakeErrorExpected<void>(*Other.GetError());
		case EExpectedResultState::Completed:
			checkf(false, TEXT("This should only be called from incomplete TExpected"));
		case EExpectedResultState::Incomplete:
		default:
			return TExpected<void>();
		}
	}

	template <typename R, typename O>
	TExpected<R> MakeErrorExpected(const TExpected<O>& InExpected)
	{
		return ConvertIncomplete<R>(InExpected);
	}

	template <typename O>
	TExpected<void> MakeErrorExpected(const TExpected<O>& InExpected)
	{
		return ConvertIncomplete<void>(InExpected);
	}
}
