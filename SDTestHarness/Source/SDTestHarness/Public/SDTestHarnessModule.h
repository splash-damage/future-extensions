// Copyright 2019 Splash Damage, Ltd. - All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

namespace SD { namespace AutomationTestMessaging { class ITestMessagingInterface; } }

/**
 * The public interface to this module
 */
class ISDTestHarness : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ISDTestHarness& Get()
	{
		return FModuleManager::LoadModuleChecked< ISDTestHarness >( "SDTestHarness" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SDTestHarness" );
	}
};

class SDTESTHARNESS_API FSDTestHarness : public ISDTestHarness
{
public:
	
	// Begin IModuleInterface override
	virtual void ShutdownModule() override;
	// End IModuleInterface override

	void RegisterMessagingInterface(TSharedPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> InMessagingInterface);
	TWeakPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> GetMessagingInterface() const;

private:
	TSharedPtr<SD::AutomationTestMessaging::ITestMessagingInterface, ESPMode::ThreadSafe> MessagingInterface;
};