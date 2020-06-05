// Copyright 2019 Splash Damage, Ltd. - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace SD
{
    namespace AutomationTestMessaging
    {
        enum class ECaptureStandardOutput
        {
            No,
            Yes
        };

        enum class EStandardMessageType
        {
            Output,
            Error
        };

        class SDTESTHARNESS_API ITestMessagingInterface
        {
		public:
            virtual void WriteStartTestSuiteMessage(const FString& SuiteName) const = 0;
			virtual void WriteFinishTestSuiteMessage(const FString& SuiteName) const = 0;

			virtual void WriteStartTestMessage(const FString& TestName, const ECaptureStandardOutput CaptureStandardOutput) const = 0;
			virtual void WriteEndTestMessage(const FString& TestName) const = 0;
			virtual void WriteEndTestMessage(const FString& TestName, const FTimespan& TestDuration) const = 0;

			virtual void WriteTestOutputMessage(const FString& TestName, const FString& OutputMessage, const EStandardMessageType MessageType) const = 0;

			virtual void WriteTestFailedMessage(const FString& TestName, const FString& Message, const FString& Details) const = 0;
			virtual void WriteTestComparisonFailedMessage(const FString& TestName, const FString& Message, const FString& Details,
                                                            const FString& ExpectedValue, const FString& ActualValue) const = 0;
        };
    }
}