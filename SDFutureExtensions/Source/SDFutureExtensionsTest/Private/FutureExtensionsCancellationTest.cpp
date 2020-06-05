// Copyright(c) Splash Damage. All rights reserved.
#include "SDAutomationTestLatentFuture.h"

#define MODULE_TEST_FLAGS EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
#define IMPLEMENT_MODULE_TEST(TestName) IMPLEMENT_SD_SIMPLE_AUTOMATION_TEST(TestName, Core.FutureExtensions.Cancellation, MODULE_TEST_FLAGS)

DEFINE_EXPECTED_FUTURE_LATENT_TEST(CancellableInitialisationFunction_WhenSetOrCancelled_HandlesPotentialRaceCondition, int);

SD::TExpectedFuture<int> CancellableInitialisationFunction_WhenSetOrCancelled_HandlesPotentialRaceCondition_Latent::Setup()
{
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	SD::TExpectedFuture<int> CancellableFuture = SD::Async([]() {
		return SD::MakeReadyExpected(5);
	}, SD::FExpectedFutureOptions(CancellationHandle));

	CancellationHandle->Cancel();

	return CancellableFuture;
}

void CancellableInitialisationFunction_WhenSetOrCancelled_HandlesPotentialRaceCondition_Latent::Validate(const SD::TExpected<int>& Expected,
																											FSDAutomationTest& CurrentTest)
{
	//This is weird, but we *expect* cancellation to be a race condition. Cancellation is a best-effort process.
	//It may, or may not, cancel before the value is set. This test is here to make sure that we handle the race condition.
	CurrentTest.TestTrue("Expected result was cancelled or set", Expected.IsCancelled() || Expected.IsCompleted());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(CancellableInitialisationFunction_WhenCancelled_ReturnsCancelledExpected, int);

SD::TExpectedFuture<int> CancellableInitialisationFunction_WhenCancelled_ReturnsCancelledExpected_Latent::Setup()
{
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	SD::TExpectedFuture<int> CancellableFuture = SD::Async([]() {
		//Force a sleep here to avoid the race condition described above - we want to ensure a cancel.
		FPlatformProcess::Sleep(10.0f);
		return SD::MakeReadyExpected(5);
	}, SD::FExpectedFutureOptions(CancellationHandle));

	CancellationHandle->Cancel();

	return CancellableFuture;
}

void CancellableInitialisationFunction_WhenCancelled_ReturnsCancelledExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was cancelled", Expected.IsCancelled());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(CancellableInitialisationFunction_WhenCancelledBeforePromiseSet_ReturnsCancelledExpected, int);

SD::TExpectedFuture<int> CancellableInitialisationFunction_WhenCancelledBeforePromiseSet_ReturnsCancelledExpected_Latent::Setup()
{
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();
	CancellationHandle->Cancel();

	return SD::Async([]() {
		return SD::MakeReadyExpected(5);
	}, SD::FExpectedFutureOptions(CancellationHandle));
}

void CancellableInitialisationFunction_WhenCancelledBeforePromiseSet_ReturnsCancelledExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																												FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was cancelled", Expected.IsCancelled());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(CancellableInitialisationFunctionWithExpectedBasedContinuation_WhenCancelled_CallsContinuationWithCancelledExpected, void, bool);

SD::TExpectedFuture<void> CancellableInitialisationFunctionWithExpectedBasedContinuation_WhenCancelled_CallsContinuationWithCancelledExpected_Latent::Setup()
{
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	SD::TExpectedFuture<void> CancellableFuture = SD::Async([]() {
		//Force a sleep here to avoid the race condition described above - we want to ensure a cancel.
		FPlatformProcess::Sleep(10.0f);
		return SD::MakeReadyExpected(5);
	}, SD::FExpectedFutureOptions(CancellationHandle))
	.Then([this](SD::TExpected<int> Expected) {
		CaptureParam = true;
		return SD::Convert(Expected);
	});

	CancellationHandle->Cancel();

	return CancellableFuture;
}

void CancellableInitialisationFunctionWithExpectedBasedContinuation_WhenCancelled_CallsContinuationWithCancelledExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																																			FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was cancelled", Expected.IsCancelled());
	CurrentTest.TestTrue("Expected-based continuation was called", CaptureParam);
}

struct FSCancellationValidationParamHolder
{
	bool bInitFunctionCalled = false;
	bool bContinuationCalled = false;
};

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(CancellableContinuationFunction_WhenCancelled_ContinuationIsNotCalled, void, FSCancellationValidationParamHolder);

SD::TExpectedFuture<void> CancellableContinuationFunction_WhenCancelled_ContinuationIsNotCalled_Latent::Setup()
{
	//Given a cancellation handle passed to a continuation
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	//When Cancelled
	CancellationHandle->Cancel();

	return SD::Async([this]() {
		CaptureParam.bInitFunctionCalled = true;
		return SD::MakeReadyExpected(5);
	})
	.Then([this](SD::TExpected<int> Expected) {
		//Then continuation with cancellation handle not called
		CaptureParam.bContinuationCalled = true;
		return SD::Convert(Expected);
	}, SD::FExpectedFutureOptions(CancellationHandle));
}

void CancellableContinuationFunction_WhenCancelled_ContinuationIsNotCalled_Latent::Validate(const SD::TExpected<void>& Expected,
																							FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was cancelled", Expected.IsCancelled());
	CurrentTest.TestTrue("Initial function was called", CaptureParam.bInitFunctionCalled);
	CurrentTest.TestFalse("Cancelled continuation was not called", CaptureParam.bContinuationCalled);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(CancellableContinuationFunctionWithExpectedBasedContinuation_WhenCancelled_SecondContinuationIsCalled, bool);

SD::TExpectedFuture<bool> CancellableContinuationFunctionWithExpectedBasedContinuation_WhenCancelled_SecondContinuationIsCalled_Latent::Setup()
{
	//Given a cancellation handle passed to a continuation
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	//When Cancelled
	CancellationHandle->Cancel();

	return SD::Async([]() {
		return SD::MakeReadyExpected(5);
	})
	.Then([](SD::TExpected<int> Expected) {
		return Expected;
	}, SD::FExpectedFutureOptions(CancellationHandle))
	.Then([](SD::TExpected<int> Expected) {
		//Then expected-based continuation is called with "Cancelled" status
		return SD::MakeReadyExpected<bool>(Expected.IsCancelled());
	});
}

void CancellableContinuationFunctionWithExpectedBasedContinuation_WhenCancelled_SecondContinuationIsCalled_Latent::Validate(const SD::TExpected<bool>& Expected,
																															FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("First continuation was cancelled", Expected.IsCompleted() && (*Expected));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(CancellableContinuationFunctionWithValueBasedContinuation_WhenCancelled_SecondContinuationIsNotCalled, bool);

SD::TExpectedFuture<bool> CancellableContinuationFunctionWithValueBasedContinuation_WhenCancelled_SecondContinuationIsNotCalled_Latent::Setup()
{
	//Given a cancellation handle passed to a continuation
	SD::SharedCancellationHandleRef CancellationHandle = SD::CreateCancellationHandle();

	//When Cancelled
	CancellationHandle->Cancel();

	return SD::Async([]() {
		return SD::MakeReadyExpected(5);
	})
	.Then([](SD::TExpected<int> Expected) {
		return Expected;
	}, SD::FExpectedFutureOptions(CancellationHandle))
	.Then([](int Expected) {
		//Then value-based continuation is not called
		return true;
	});
}

void CancellableContinuationFunctionWithValueBasedContinuation_WhenCancelled_SecondContinuationIsNotCalled_Latent::Validate(const SD::TExpected<bool>& Expected,
																																FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Second continuation was cancelled", Expected.IsCancelled());
}

#undef IMPLEMENT_MODULE_TEST
#undef MODULE_TEST_FLAGS