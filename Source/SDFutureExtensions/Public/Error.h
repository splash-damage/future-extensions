// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "Templates/SharedPointer.h"

namespace SD
{
	class Error
	{
	public:
		using ErrorStringPtr = TSharedPtr<FString, ESPMode::ThreadSafe>;

		Error(int32 InErrorCode)
			: ErrorCode(InErrorCode)
		{}

		Error(int32 InErrorCode, int32 InErrorContext)
			: ErrorCode(InErrorCode)
			, ErrorContext(InErrorContext)
		{}

		Error(int32 InErrorCode, FString InErrorInfo)
			: ErrorCode(InErrorCode)
			, ErrorInfo(MakeShareable(new FString(InErrorInfo)))
		{}

		Error(int32 InErrorCode, int32 InErrorContext, FString InErrorInfo)
			: ErrorCode(InErrorCode)
			, ErrorContext(InErrorContext)
			, ErrorInfo(MakeShareable(new FString(InErrorInfo)))
		{}

		int32 GetErrorCode() const
		{
			return ErrorCode;
		}

		int32 GetErrorContext() const
		{
			return ErrorContext;
		}

		const ErrorStringPtr GetErrorInfo() const
		{
			return ErrorInfo;
		}

		ErrorStringPtr GetErrorInfo()
		{
			return ErrorInfo;
		}

	private:
		int32 ErrorCode = 0;
		int32 ErrorContext = 0;
		ErrorStringPtr ErrorInfo = nullptr;
	};
}