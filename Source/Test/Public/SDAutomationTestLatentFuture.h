#pragma once

#include "FutureExtensions.h"

#ifdef DEPRECATED_TESTS

// FExpectedFutureLatentTestBase provides an implementation of a latent command that
// monitors an underlying future computation.
//
// The Setup() function is used to establish the asynchronous operation, and must return a TExpectedFuture.
// When the returned future is resolved, Validate() will be called with the resultant TExpected which can be
// used to validate the Then part of a Given - When - Then test.
//
// This abstracts out to:
// 
// Given
//  - An asynchronous operation declared in Setup()
// When
//  - The asynchronous operation is resolved
// Then
//  - The resultant expected value is evaluated in Validate()
//
// See FutureExtensionsBasicTest.cpp for examples.

template<class T>
class FExpectedFutureLatentTestBase : public IAutomationLatentCommand
{
protected:
	FExpectedFutureLatentTestBase()
		: IAutomationLatentCommand()
	{}

	virtual SD::TExpectedFuture<T> Setup() = 0;
	virtual void Validate(const SD::TExpected<T>& Expected, FSDAutomationTest& CurrentTest) = 0;

private:
	enum class EState
	{
		NotStarted,
		Running,
		Complete
	};

	// Begin IAutomationLatentCommand override
	virtual bool Update() final;
	// End IAutomationLatentCommand override

	EState TestState = EState::NotStarted;
};

template<class T>
bool FExpectedFutureLatentTestBase<T>::Update()
{
	if (TestState == EState::NotStarted)
	{
		TestState = EState::Running;

		Setup().Then([this](SD::TExpected<T> Expected) {
			FSDAutomationTest* CurrentSDTest = 
				static_cast<FSDAutomationTest*>(FAutomationTestFramework::Get().GetCurrentTest());
			if (CurrentSDTest != nullptr)
			{
				this->Validate(Expected, *CurrentSDTest);
			}
			else
			{
				UE_LOG(LogCore, Fatal, 
					TEXT("ExpectedFuture latent tests must be run from a test derived from FSDAutomationTest"));
			}

			this->TestState = EState::Complete;
		});
	}

	return TestState == EState::Complete;
}

#define IMPLEMENT_EXPECTED_FUTURE_LATENT_TEST(TestName)			\
	IMPLEMENT_MODULE_TEST(TestName)								\
	{															\
		ADD_LATENT_AUTOMATION_COMMAND(TestName##_Latent());		\
		return true;											\
	}

#define DEFINE_EXPECTED_FUTURE_LATENT_TEST(TestName, Type)					\
	class TestName##_Latent : public FExpectedFutureLatentTestBase<Type>	\
	{																		\
	protected:																\
		SD::TExpectedFuture<Type> Setup() override;							\
		void Validate(const SD::TExpected<Type>& Expected,					\
						FSDAutomationTest& CurrentTest) override;			\
	};																		\
	IMPLEMENT_EXPECTED_FUTURE_LATENT_TEST(TestName)

#define DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(TestName, Type, Param)		\
	class TestName##_Latent : public FExpectedFutureLatentTestBase<Type>		\
	{																			\
	protected:																	\
		SD::TExpectedFuture<Type> Setup() override;								\
		void Validate(const SD::TExpected<Type>& Expected,						\
						FSDAutomationTest& CurrentTest) override;				\
		Param CaptureParam;														\
	};																			\
	IMPLEMENT_EXPECTED_FUTURE_LATENT_TEST(TestName)

#endif