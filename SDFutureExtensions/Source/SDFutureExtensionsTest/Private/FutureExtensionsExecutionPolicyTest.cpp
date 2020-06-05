// Copyright(c) Splash Damage. All rights reserved.
#include "SDAutomationTestLatentFuture.h"

#define MODULE_TEST_FLAGS EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter
#define IMPLEMENT_MODULE_TEST(TestName) IMPLEMENT_SD_SIMPLE_AUTOMATION_TEST(TestName, Core.FutureExtensions.ExecutionPolicy, MODULE_TEST_FLAGS)

DEFINE_EXPECTED_FUTURE_LATENT_TEST(InitialisationFunctorScheduledOnNamedThread_WhenRun_ExecutesOnCorrectNamedThread, ENamedThreads::Type);

SD::TExpectedFuture<ENamedThreads::Type> InitialisationFunctorScheduledOnNamedThread_WhenRun_ExecutesOnCorrectNamedThread_Latent::Setup()
{
	//Given an initialization function that is scheduled to run on a NamedThread (i.e. Stats)
	return SD::Async([]() {
		return SD::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
	}, SD::FExpectedFutureOptionsBuilder()
		.SetDesiredExecutionThread(ENamedThreads::ActualRenderingThread)
		.Build());
}

void InitialisationFunctorScheduledOnNamedThread_WhenRun_ExecutesOnCorrectNamedThread_Latent::Validate(const SD::TExpected<ENamedThreads::Type>& Expected,
																										FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Async function ran successfully", Expected.IsCompleted());
	CurrentTest.TestEqual("Execution thread", (*Expected), ENamedThreads::ActualRenderingThread);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(InitialisationFunctorScheduledOnGameThread_WhenRun_ExecutesOnGameThread, ENamedThreads::Type);

SD::TExpectedFuture<ENamedThreads::Type> InitialisationFunctorScheduledOnGameThread_WhenRun_ExecutesOnGameThread_Latent::Setup()
{
	check(FTaskGraphInterface::Get().GetCurrentThreadIfKnown() == ENamedThreads::GameThread);

	return SD::Async([]() {
		return SD::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
	}, SD::FExpectedFutureOptionsBuilder()
		.SetExecutionPolicy(SD::EExpectedFutureExecutionPolicy::Current)
		.Build());
}

void InitialisationFunctorScheduledOnGameThread_WhenRun_ExecutesOnGameThread_Latent::Validate(const SD::TExpected<ENamedThreads::Type>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Async function ran successfully", Expected.IsCompleted());
	CurrentTest.TestEqual("Execution thread", (*Expected), ENamedThreads::GameThread);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ContinuationFunctorScheduledOnNamedThread_WhenRun_ExecutesOnNamedThread, ENamedThreads::Type);

SD::TExpectedFuture<ENamedThreads::Type> ContinuationFunctorScheduledOnNamedThread_WhenRun_ExecutesOnNamedThread_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeReadyExpected();
	}).Then([](SD::TExpected<void> Expected) {
		return SD::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
	}, SD::FExpectedFutureOptionsBuilder()
		.SetExecutionPolicy(SD::EExpectedFutureExecutionPolicy::NamedThread)
		.SetDesiredExecutionThread(ENamedThreads::GameThread)
		.Build());
}

void ContinuationFunctorScheduledOnNamedThread_WhenRun_ExecutesOnNamedThread_Latent::Validate(const SD::TExpected<ENamedThreads::Type>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Async function ran successfully", Expected.IsCompleted());
	CurrentTest.TestEqual("Execution thread", (*Expected), ENamedThreads::GameThread);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ContinuationFunctorScheduledOnInlineThread_WhenRun_ExecutesOnInlineThread, ENamedThreads::Type);

SD::TExpectedFuture<ENamedThreads::Type> ContinuationFunctorScheduledOnInlineThread_WhenRun_ExecutesOnInlineThread_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeReadyExpected();
	}, SD::FExpectedFutureOptionsBuilder()
		.SetExecutionPolicy(SD::EExpectedFutureExecutionPolicy::NamedThread)
		.SetDesiredExecutionThread(ENamedThreads::GameThread)
		.Build())
	.Then([](SD::TExpected<void> Expected) {
		return SD::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
	}, SD::FExpectedFutureOptionsBuilder()
		.SetExecutionPolicy(SD::EExpectedFutureExecutionPolicy::Inline)
		.Build());
}

void ContinuationFunctorScheduledOnInlineThread_WhenRun_ExecutesOnInlineThread_Latent::Validate(const SD::TExpected<ENamedThreads::Type>& Expected,
																								FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Async function ran successfully", Expected.IsCompleted());
	CurrentTest.TestEqual("Execution thread", (*Expected), ENamedThreads::GameThread);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(InitialisationFunctorScheduledThreadPool_WhenRun_ExecutesOnThreadPool, ENamedThreads::Type);

SD::TExpectedFuture<ENamedThreads::Type> InitialisationFunctorScheduledThreadPool_WhenRun_ExecutesOnThreadPool_Latent::Setup()
{
	//Given an initialization function that is scheduled to run on a NamedThread (i.e. Stats)
	return SD::Async([]() {
		return SD::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
	}, SD::FExpectedFutureOptionsBuilder()
		.SetExecutionPolicy(SD::EExpectedFutureExecutionPolicy::ThreadPool)
		.Build());
}

void InitialisationFunctorScheduledThreadPool_WhenRun_ExecutesOnThreadPool_Latent::Validate(const SD::TExpected<ENamedThreads::Type>& Expected,
																							FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Async function ran successfully", Expected.IsCompleted());
	CurrentTest.TestEqual("Execution thread", (*Expected), ENamedThreads::AnyThread);
}

DEFINE_EXPECTED_FUTURE_LATENT_TEST(ContinuationFunctorScheduledOnThreadPool_WhenRun_ExecutesOnThreadPool, ENamedThreads::Type);

SD::TExpectedFuture<ENamedThreads::Type> ContinuationFunctorScheduledOnThreadPool_WhenRun_ExecutesOnThreadPool_Latent::Setup()
{
	return SD::Async([]() {
		return SD::MakeReadyExpected();
	}).Then([](SD::TExpected<void> Expected) {
		return SD::MakeReadyExpected(FTaskGraphInterface::Get().GetCurrentThreadIfKnown());
	}, SD::FExpectedFutureOptionsBuilder()
		.SetExecutionPolicy(SD::EExpectedFutureExecutionPolicy::ThreadPool)
		.Build());
}

void ContinuationFunctorScheduledOnThreadPool_WhenRun_ExecutesOnThreadPool_Latent::Validate(const SD::TExpected<ENamedThreads::Type>& Expected,
																							FSDAutomationTest& CurrentTest)
{
	CurrentTest.TestTrue("Async function ran successfully", Expected.IsCompleted());
	CurrentTest.TestEqual("Execution thread", (*Expected), ENamedThreads::AnyThread);
}

#undef IMPLEMENT_MODULE_TEST
#undef MODULE_TEST_FLAGS