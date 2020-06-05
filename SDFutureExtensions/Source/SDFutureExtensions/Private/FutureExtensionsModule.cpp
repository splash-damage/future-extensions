// Copyright(c) Splash Damage. All rights reserved.
#include "FutureExtensionsModule.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFutureExtensions, Log, All);

class FFutureExtensions : public IFutureExtensions
{

};

IMPLEMENT_MODULE(FFutureExtensions, SDFutureExtensions)