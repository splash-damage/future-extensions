// Copyright(c) Splash Damage. All rights reserved.
#include "FutureExtensionsModule.h"
#include "FutureLogging.h"

DEFINE_LOG_CATEGORY(LogFutureExtensions);

class FFutureExtensions : public IFutureExtensions
{

};

IMPLEMENT_MODULE(FFutureExtensions, SDFutureExtensions)