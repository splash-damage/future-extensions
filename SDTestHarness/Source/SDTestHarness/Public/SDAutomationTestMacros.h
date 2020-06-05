// Copyright 2019 Splash Damage, Ltd. - All Rights Reserved.

#pragma once

#include "SDAutomationTest.h"

/**
*	Extension macros to build on those found in AutomationTest.h, and allow injection of SD specific logging.
*	Also provides scaffolding to ensure all SD automation tests follow the same standards for writing and categorising tests.
*	
*	Example usage:
*
*	#define MODULE_TEST_FLAGS EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
*	#define FULL_MODULE_TEST_NAME(TestName) Fully.Qualified.Path.#TestName
*	#define IMPLEMENT_MODULE_TEST(TestName) IMPLEMENT_SD_SIMPLE_AUTOMATION_TEST(TestName, FULL_MODULE_TEST_NAME(TestName), MODULE_TEST_FLAGS)
*
*	IMPLEMENT_MODULE_TEST(FGiven_When_Then)
*	{
*		//Setup
*		const bool Expected = false;
*
*		//Run
*		const bool Actual = true;
*
*		//Validate
*		TestEqual(TEXT("Example Test"), Expected, Actual);
*
*		return true;
*	}
*/

#define IMPLEMENT_SD_AUTOMATION_TEST_PRIVATE(TClass)	\
		bool TClass::RunTest(const FString& Parameters)

#define MAKE_SD_AUTOMATION_TEST_NAMESPACE(TestName, Category) PREPROCESSOR_TO_STRING_INNER(SplashDamage.Category.TestName)

#define IMPLEMENT_SD_SIMPLE_AUTOMATION_TEST(TestName, Category, Flags) \
		IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(TestName, FSDAutomationTest, MAKE_SD_AUTOMATION_TEST_NAMESPACE(TestName, Category), Flags) \
		IMPLEMENT_SD_AUTOMATION_TEST_PRIVATE(TestName)

#define IMPLEMENT_SD_COMPLEX_AUTOMATION_TEST(TestName, Category, Flags) \
		IMPLEMENT_CUSTOM_COMPLEX_AUTOMATION_TEST(TestName, FSDAutomationTest, MAKE_SD_AUTOMATION_TEST_NAMESPACE(TestName, Category), Flags) \
		IMPLEMENT_SD_AUTOMATION_TEST_PRIVATE(TestName)

#define ABORT_TEST_IF_NULL(Pointer) if (Pointer == nullptr) \
{ \
	const FString ErrorString = FString::Printf(TEXT("%s failed: " #Pointer " was nullptr."), *TestName); \
	AddErrorS(ErrorString, __FILE__, __LINE__); \
	return false; \
}

#define ABORT_TEST_IF_INVALID(SharedPointer) if (!SharedPointer.IsValid()) \
{ \
	const FString ErrorString = FString::Printf(TEXT("%s failed: " #SharedPointer " was not valid."), *TestName); \
	AddErrorS(ErrorString, __FILE__, __LINE__); \
	return false; \
}