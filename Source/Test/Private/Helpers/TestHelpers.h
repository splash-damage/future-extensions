// Copyright 2020 Splash Damage, Ltd. - All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Automatron.h>


class FFutureTestSpec : public FTestSpec
{
public:
	FFutureTestSpec() : FTestSpec()
	{
		bUseWorld = false;
	}
};
