// Copyright(c) Splash Damage. All rights reserved.
#include "SDAutomationTestLatentFuture.h"

#define MODULE_TEST_FLAGS EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
#define IMPLEMENT_MODULE_TEST(TestName) IMPLEMENT_SD_SIMPLE_AUTOMATION_TEST(TestName, Core.FutureExtensions.Core, MODULE_TEST_FLAGS)

#define TEST_ERROR_CONTEXT 0xbaadf00d
#define TEST_ERROR_CODE 0xdeadbeef
#define TEST_ERROR_INFO TEXT("Bad times!")

namespace FutureExtensionsBasicTest
{
	static SD::TExpectedFuture<FString> AddTen(int Value)
	{
		return SD::Async([Value]() {
			return FString::Printf(TEXT("%i"), Value + 10);
		});
	}

	static SD::TExpectedFuture<void> DoVoidAsyncWork()
	{
		int CaptureVal = 0;
		return SD::Async([&]() {
			CaptureVal = 10;
		});
	}
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedFutureMadeReadyWithValue_WhenResolved_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> ExpectedFutureMadeReadyWithValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(13);
}

void ExpectedFutureMadeReadyWithValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 13);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(VoidExpectedFutureMadeReadyWithValue_WhenResolved_ReturnsSuccessfulExpected, void);

SD::TExpectedFuture<void> VoidExpectedFutureMadeReadyWithValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture();
}

void VoidExpectedFutureMadeReadyWithValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																									FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ValueBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> ValueBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(13).Then([](int Result) {
		return Result + 7;
	});
}

void ValueBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																									FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 20);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> ExpectedBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(13).Then([](SD::TExpected<int> Result) {
		return *Result + 7;
	});
}

void ExpectedBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 20);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(ValueBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected, void, int);

SD::TExpectedFuture<void> ValueBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(13).Then([this](int Result) {
		CaptureParam = Result + 7;
	});
}

void ValueBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																									FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("Lambda captured value", CaptureParam, 20);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(ExpectedBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected, void, int);

SD::TExpectedFuture<void> ExpectedBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(13).Then([this](SD::TExpected<int> Result) {
		CaptureParam = *Result + 7;
	});
}

void ExpectedBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("Lambda captured value", CaptureParam, 20);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(VoidBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> VoidBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture().Then([]() {
		return 20;
	});
}

void VoidBasedContinuationThatReturnsValue_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																									FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 20);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(VoidBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected, void, int);

SD::TExpectedFuture<void> VoidBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture().Then([this]() {
		CaptureParam = 20;
	});
}

void VoidBasedContinuationThatReturnsVoid_WhenResolved_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																									FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("Lambda captured value", CaptureParam, 20);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(VoidBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected, FString);

SD::TExpectedFuture<FString> VoidBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::MakeReadyFuture().Then([]() {
		return FutureExtensionsBasicTest::AddTen(10);
	});
}

void VoidBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<FString>& Expected,
																														FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEXT("20"));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ValueBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected, FString);

SD::TExpectedFuture<FString> ValueBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(10).Then([](int Result) {
		return FutureExtensionsBasicTest::AddTen(Result);
	});
}

void ValueBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<FString>& Expected,
																														FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEXT("20"));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected, FString);

SD::TExpectedFuture<FString> ExpectedBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(10).Then([](SD::TExpected<int> Result) {
		return FutureExtensionsBasicTest::AddTen(*Result);
	});
}

void ExpectedBasedContinuationThatReturnsExpectedFuture_WhenResolved_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<FString>& Expected,
																															FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEXT("20"));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsPlainValue_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> FunctionThatReturnsPlainValue_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::Async([]() {
		return 5;
	});
}

void FunctionThatReturnsPlainValue_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																											FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 5);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsVoidFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfullyUnwrappedExpected, void);

SD::TExpectedFuture<void> FunctionThatReturnsVoidFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::Async([]() {
		return FutureExtensionsBasicTest::DoVoidAsyncWork();
	});
}

void FunctionThatReturnsVoidFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																													FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsExpected_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> FunctionThatReturnsExpected_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeReadyExpected(5);
	});
}

void FunctionThatReturnsExpected_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 5);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfullyUnwrappedExpected, FString);

SD::TExpectedFuture<FString> FunctionThatReturnsFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::Async([]() {
		return FutureExtensionsBasicTest::AddTen(20);
	});
}

void FunctionThatReturnsFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<FString>& Expected,
																												FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEXT("30"));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> FunctionThatReturnsFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeReadyFuture(10);
	});
}

void FunctionThatReturnsFuture_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 10);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsVoidExpected_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected, void);

SD::TExpectedFuture<void> FunctionThatReturnsVoidExpected_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeReadyExpected();
	});
}

void FunctionThatReturnsVoidExpected_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																											FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(FunctionThatReturnsVoid_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected, void, int);

SD::TExpectedFuture<void> FunctionThatReturnsVoid_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::Async([this]() {
		CaptureParam = 5;
	});
}

void FunctionThatReturnsVoid_WhenUsedToInitialiseFuture_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																									FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestEqual("Lambda captured value", CaptureParam, 5);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(FunctionThatReturnsError_WhenUsedToInitialiseFuture_ReturnsFailedExpected, int);

SD::TExpectedFuture<int> FunctionThatReturnsError_WhenUsedToInitialiseFuture_ReturnsFailedExpected_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeErrorExpected<int>(SD::Error(TEST_ERROR_CODE, TEST_ERROR_CONTEXT, TEST_ERROR_INFO));
	});
}

void FunctionThatReturnsError_WhenUsedToInitialiseFuture_ReturnsFailedExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Result is an error", Expected.IsError());
	CurrentTest.TestEqual("Error Code", Expected.GetError()->GetErrorCode(), TEST_ERROR_CODE);
	CurrentTest.TestEqual("Error Context", Expected.GetError()->GetErrorContext(), TEST_ERROR_CONTEXT);
	CurrentTest.TestEqual("Error Message", *Expected.GetError()->GetErrorInfo(), TEST_ERROR_INFO);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedFuture_WhenExpectedBasedContinuationAdded_ReturnsChainedExpected, int);

SD::TExpectedFuture<int> ExpectedFuture_WhenExpectedBasedContinuationAdded_ReturnsChainedExpected_Latent::Setup()
{
	SD::TExpectedFuture<int> FirstFuture = SD::Async([]() {
		return 10;
	});

	return FirstFuture.Then([](SD::TExpected<int> ExpectedResult) {
		//Pass initial result straight through so we can use it for test cases
		return ExpectedResult;
	});
}

void ExpectedFuture_WhenExpectedBasedContinuationAdded_ReturnsChainedExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 10);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedFuture_WhenValueBasedContinuationAdded_ReturnsChainedExpected, int);

SD::TExpectedFuture<int> ExpectedFuture_WhenValueBasedContinuationAdded_ReturnsChainedExpected_Latent::Setup()
{
	SD::TExpectedFuture<int> FirstFuture = SD::Async([]() {
		return 10;
	});

	return FirstFuture.Then([](int ExpectedResult) {
		//Pass initial result straight through so we can use it for test cases
		return ExpectedResult;
	});
}

void ExpectedFuture_WhenValueBasedContinuationAdded_ReturnsChainedExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																							FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 10);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST_ONE_PARAM(ExpectedFutureWithError_WhenValueBasedContinuationAdded_ContinuationIsNotCalled, FString, bool);

SD::TExpectedFuture<FString> ExpectedFutureWithError_WhenValueBasedContinuationAdded_ContinuationIsNotCalled_Latent::Setup()
{
	SD::TExpectedFuture<int> FirstFuture = SD::Async([]() {
		return SD::MakeErrorExpected<int>(SD::Error(TEST_ERROR_CODE, TEST_ERROR_CONTEXT));
	});

	return FirstFuture.Then([this](int ExpectedResult) {
		CaptureParam = true;
		return FString(TEXT("Return a string to implicitly test conversion"));
	});
}

void ExpectedFutureWithError_WhenValueBasedContinuationAdded_ContinuationIsNotCalled_Latent::Validate(const SD::TExpected<FString>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Result is an error", Expected.IsError());
	CurrentTest.TestEqual("Error Code", Expected.GetError()->GetErrorCode(), TEST_ERROR_CODE);
	CurrentTest.TestEqual("Error Context", Expected.GetError()->GetErrorContext(), TEST_ERROR_CONTEXT);
	CurrentTest.TestFalse("Continuation has not been called", CaptureParam);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedFutureWithError_WhenExpectedBasedContinuationAdded_ContinuationIsCalled, FString);

SD::TExpectedFuture<FString> ExpectedFutureWithError_WhenExpectedBasedContinuationAdded_ContinuationIsCalled_Latent::Setup()
{
	SD::TExpectedFuture<int> FirstFuture = SD::Async([]() {
		return SD::MakeErrorExpected<int>(SD::Error(TEST_ERROR_CODE, TEST_ERROR_CONTEXT, TEST_ERROR_INFO));
	});


	return FirstFuture.Then([](SD::TExpected<int> ExpectedResult) {
		if (ExpectedResult.IsError())
		{
			return *ExpectedResult.GetError()->GetErrorInfo();
		}

		return FString(TEXT(""));
	});
}

void ExpectedFutureWithError_WhenExpectedBasedContinuationAdded_ContinuationIsCalled_Latent::Validate(const SD::TExpected<FString>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Result is successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEST_ERROR_INFO);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ValueBasedContinuationThatReturnsExpectedFuture_WhenEvaluated_ReturnsSuccessfullyUnwrappedExpected, FString);

SD::TExpectedFuture<FString> ValueBasedContinuationThatReturnsExpectedFuture_WhenEvaluated_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::Async([]() {
		return 10;
	}).Then([](int Value) {
		return FutureExtensionsBasicTest::AddTen(Value);
	});
}

void ValueBasedContinuationThatReturnsExpectedFuture_WhenEvaluated_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<FString>& Expected,
																															FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEXT("20"));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedBasedContinuationThatReturnsExpectedFuture_WhenEvaluated_ReturnsSuccessfullyUnwrappedExpected, FString);

SD::TExpectedFuture<FString> ExpectedBasedContinuationThatReturnsExpectedFuture_WhenEvaluated_ReturnsSuccessfullyUnwrappedExpected_Latent::Setup()
{
	return SD::Async([]() {
		return 10;
	}).Then([](SD::TExpected<int> Value) {
		if (Value.IsCompleted())
		{
			return FutureExtensionsBasicTest::AddTen(*Value);
		}

		return SD::MakeReadyFutureFromExpected<FString>(Value);
	});
}

void ExpectedBasedContinuationThatReturnsExpectedFuture_WhenEvaluated_ReturnsSuccessfullyUnwrappedExpected_Latent::Validate(const SD::TExpected<FString>& Expected,
																															FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, TEXT("20"));
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(VoidBasedContinuationThatReturnsVoidExpected_WhenEvaluated_ReturnsSuccessfulExpected, void);

SD::TExpectedFuture<void> VoidBasedContinuationThatReturnsVoidExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture().Then([]() {
		return SD::MakeReadyExpected();
	});
}

void VoidBasedContinuationThatReturnsVoidExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
	FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(VoidBasedContinuationThatReturnsValueExpected_WhenEvaluated_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> VoidBasedContinuationThatReturnsValueExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture().Then([]() {
		return SD::MakeReadyExpected(10);
	});
}

void VoidBasedContinuationThatReturnsValueExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																											FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 10);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ValueBasedContinuationThatReturnsValueExpected_WhenEvaluated_ReturnsSuccessfulExpected, int);

SD::TExpectedFuture<int> ValueBasedContinuationThatReturnsValueExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(10).Then([](int Expected) {
		return SD::MakeReadyExpected<int>(MoveTemp(Expected));
	});
}

void ValueBasedContinuationThatReturnsValueExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<int>& Expected,
																												FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected result was successful", Expected.IsCompleted());
	CurrentTest.TestTrue("Expected result TOptional is set", Expected.GetValue().IsSet());
	CurrentTest.TestEqual("ExpectedFuture value", *Expected, 10);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ExpectedBasedContinuationThatReturnsVoidExpected_WhenEvaluated_ReturnsSuccessfulExpected, void);

SD::TExpectedFuture<void> ExpectedBasedContinuationThatReturnsVoidExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Setup()
{
	return SD::MakeReadyFuture(10).Then([](SD::TExpected<int> Expected) {
		return SD::MakeReadyExpected();
	});
}

void ExpectedBasedContinuationThatReturnsVoidExpected_WhenEvaluated_ReturnsSuccessfulExpected_Latent::Validate(const SD::TExpected<void>& Expected,
																												FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Expected future expected value is successful", Expected.IsCompleted());
}

#undef IMPLEMENT_MODULE_TEST
#undef MODULE_TEST_FLAGS
#undef TEST_ERROR_CONTEXT
#undef TEST_ERROR_CODE
#undef TEST_ERROR_INFO