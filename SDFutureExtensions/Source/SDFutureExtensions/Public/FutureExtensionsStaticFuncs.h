// Copyright(c) Splash Damage. All rights reserved.
#pragma once

namespace SD
{
	template<typename F>
	auto Async(F&& Function, const SD::FExpectedFutureOptions& FutureOptions = SD::FExpectedFutureOptions())
	{
		return FutureInitialisationDetails::CreateExpectedFuture(Forward<F>(Function), FutureOptions);
	}
}