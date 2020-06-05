// Copyright 2019 Splash Damage, Ltd. - All Rights Reserved.

#include "SDTestHarnessModule.h"
#include "SDAutomationTestMessagingInterface.h"

IMPLEMENT_MODULE(FSDTestHarness, SDTestHarness )

void FSDTestHarness::ShutdownModule()
{
	MessagingInterface.Reset();
}

void FSDTestHarness::RegisterMessagingInterface(TSharedPtr<SD::AutomationTestMessaging::ITestMessagingInterface, 
															ESPMode::ThreadSafe> InMessagingInterface)
{
	MessagingInterface = InMessagingInterface;
}

TWeakPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> FSDTestHarness::GetMessagingInterface() const
{
	return MessagingInterface;
}
