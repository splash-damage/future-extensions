// Copyright(c) Splash Damage. All rights reserved.
#pragma once

#include "Templates/IntegralConstant.h"
#include "Runtime/Launch/Resources/Version.h"

namespace SD
{
	template <class ResultType>
	class TExpected;

	template <class ResultType>
	class TExpectedFuture;

	template <class ResultType>
	class TExpectedPromise;

	namespace FutureExtensionTypeTraits
	{
		/*
		*	Helper type traits for determining if types are either Expected or ExpectedFuture.
		*	Required as certain functionality (such as implicit unwrapping of futures/types) requires
		*	specialization when dealing with these types.
		*/
		template <class T>
		struct TIsExpectedFutureImpl : std::false_type
		{};

		template <class R>
		struct TIsExpectedFutureImpl<TExpectedFuture<R>> : std::true_type
		{};

		template <class T>
		using TIsExpectedFuture = TIsExpectedFutureImpl<std::decay_t<T>>;

		template <class T>
		struct TIsExpectedImpl : std::false_type
		{};

		template <class R>
		struct TIsExpectedImpl<TExpected<R>> : std::true_type
		{};

		template <class T>
		using TIsExpected = TIsExpectedImpl<std::decay_t<T>>;

		/*
		*	These template types allow us to pass in a type and 'unwrap' it down to it's underlying 'raw' type.
		*	Important for working with 'wrapper' types such as TExpected and TExpectedFuture.
		*	Sort of hacky as it relies on TExpected and TExpectedFuture defining a type called "ResultType".
		*/
		template<class T, class Enable = void>
		class TUnwrap {};

		template<class T>
		class TUnwrap<T, typename TEnableIf<!TIsExpectedFuture<T>::value && !TIsExpected<T>::value>::Type>
		{
		public:
			using Type = T;
		};

		template<class T>
		class TUnwrap<T, typename TEnableIf<TIsExpectedFuture<T>::value || TIsExpected<T>::value>::Type>
		{
		public:
			using Type = typename TUnwrap<typename T::ResultType>::Type;
		};

		/*
		*	These functions allow us to deduce what types are used for 'initialization' functors, i.e.
		*	the first functor in a potential chain of futures (as opposed to a 'continuation' functor below).
		*	See below for a more in-depth explanation of how this template wizardry works.
		*/
		struct BadInitFunctorSignature {};

		template <typename F>
		auto InitFunctorReturnTypeHelper(F Func, int, ...) -> decltype(Func());

		template <typename F>
		auto InitFunctorReturnTypeHelper(F Func, ...)->BadInitFunctorSignature;

		//Aliases for supported specializations for functor return values (both initialization and continuation)
		template<typename ReturnType>
		struct TReturnTypeSpecializations
		{
			using IsExpectedFuture = TIsExpectedFuture<ReturnType>;

			using IsPlainValueOrExpected = TIntegralConstant<bool,	!IsExpectedFuture::value &&
																	(TIsExpected<ReturnType>::value ||
																	!TIsVoidType<ReturnType>::Value)>;

			using IsNonExpectedAndVoid = TIntegralConstant<bool, !TIsExpected<ReturnType>::value &&
																TIsVoidType<ReturnType>::Value>;
		};

		template<typename ParamType>
		struct TParamTypeSpecializations
		{
			using IsVoidValueBased = TIsVoidType<ParamType>;
			using IsNonVoidValueBased = TIntegralConstant<bool, !TIsVoidType<ParamType>::Value && 
																!TIsExpected<ParamType>::value>;
			using IsExpectedBased = TIsExpected<ParamType>;
		};

		//Aliases for supported specializations of initialization functors
		template<typename R>
		class TInitializationFunctorSpecializations
		{
		public:
			using ReturnSpec = TReturnTypeSpecializations<R>;

			using ReturnsPlainValueOrExpected = TEnableIf<ReturnSpec::IsPlainValueOrExpected::Value>;
			using ReturnsVoid = TEnableIf<ReturnSpec::IsNonExpectedAndVoid::Value>;
			using ReturnsExpectedFuture = TEnableIf<ReturnSpec::IsExpectedFuture::value>;
		};

		template <typename F>
		struct TInitialFunctorTypes
		{
			using ReturnType = decltype(InitFunctorReturnTypeHelper(DeclVal<F>(), 0));
			static_assert(!std::is_same<ReturnType, BadInitFunctorSignature>::value, "Initial function cannot accept parameters.");
		
			using Specialization = TInitializationFunctorSpecializations<ReturnType>;
		};

		struct BadContinuationFunctorSignature {};

		template<typename P>
		TExpected<P> ToExpected(P p);

		TExpected<void> ToExpected();

		void ToVoid();

		/*
		*	These functions allow us to deduce what types are used for 'continuation' functors.
		*	By defining function overloads that use ... these functions all match the call used below with (F, P, 0, 0)
		*	but only one function template will actually be instantiated (using SFINAE) based on whether the
		*	decltype evaluation succeeds or not.
		*
		*	The fallthrough case (as above) returns an identity type that allows us to statically assert on incorrect usage.
		*
		*	The decltype uses the built-in comma operator to evaluate the passed in function, discard the result, and then use the
		*	actual type of the second parameter. See: https://en.cppreference.com/w/cpp/language/operator_other
		*/

		//Continuations can either take a parameter of TExpected<P>
		template <typename F, typename P>
		auto ContinuationFunctorParamTypeHelper(F Func, P PrevType, int, int, ...) -> decltype(Func(ToExpected(PrevType)), ToExpected(PrevType));

		//Or a parameter type of P
		template <typename F, typename P>
		auto ContinuationFunctorParamTypeHelper(F Func, P PrevType, int, ...) -> decltype(Func(PrevType), PrevType);

		//And no other type of parameter
		template <typename F, typename P>
		auto ContinuationFunctorParamTypeHelper(F Func, P PrevType, ...)->BadContinuationFunctorSignature;

		//Continuations where the previous type was void can either take a parameter of TExpected<void>
		template <typename F>
		auto ContinuationFunctorVoidParamTypeHelper(F Func, int, int, ...) -> decltype(Func(ToExpected()), ToExpected());

		//Or a specifically void parameter type
		template <typename F>
		auto ContinuationFunctorVoidParamTypeHelper(F Func, int, ...) -> decltype(Func(), ToVoid());

		//And no other type of parameter
		template <typename F>
		auto ContinuationFunctorVoidParamTypeHelper(F Func, ...)->BadContinuationFunctorSignature;

		template<typename F, typename P, typename Enable = void>
		struct TContinuationFunctorReturnType {};

		template<typename F, typename P>
		struct TContinuationFunctorReturnType<F, P, typename TEnableIf<!TIsVoidType<P>::Value>::Type>
		{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 26 && ENGINE_PATCH_VERSION >= 1
			using ReturnType = typename TInvokeResult<typename std::decay_t<F>, P>::Type;
#elif /* C++17 */ __cplusplus >= 201703L || /* MSVC */ (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
			using ReturnType = std::invoke_result_t<typename std::decay_t<F>, P>;
#else
			using ReturnType = std::result_of_t<typename std::decay_t<F>(P)>;
#endif
		};

		template<typename F, typename P>
		struct TContinuationFunctorReturnType<F, P, typename TEnableIf<TIsVoidType<P>::Value>::Type>
		{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 26 && ENGINE_PATCH_VERSION >= 1
			using ReturnType = typename TInvokeResult<typename std::decay_t<F>>::Type;
#elif /* C++17 */ __cplusplus >= 201703L || /* MSVC */ (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
			using ReturnType = std::invoke_result_t<typename std::decay_t<F>>;
#else
			using ReturnType = std::result_of_t<typename std::decay_t<F>()>;
#endif
		};

		//Aliases for supported specializations of continuation functors
		template<typename R, typename P>
		class TContinuationFunctorSpecializations
		{
		private:
			using ReturnSpec = TReturnTypeSpecializations<R>;
			using ParamSpec = TParamTypeSpecializations<P>;

			using IsReturnsVoidAndTakesVoid =
				TIntegralConstant<bool, ReturnSpec::IsNonExpectedAndVoid::Value &&
										ParamSpec::IsVoidValueBased::Value>;

			using IsReturnsVoidAndTakesValue =
				TIntegralConstant<bool, ReturnSpec::IsNonExpectedAndVoid::Value &&
										ParamSpec::IsNonVoidValueBased::Value>;

			using IsReturnsVoidAndTakesExpected =
				TIntegralConstant<bool, ReturnSpec::IsNonExpectedAndVoid::Value &&
										ParamSpec::IsExpectedBased::value>;

			using IsReturnsExpectedFutureAndTakesVoid =
				TIntegralConstant<bool, ReturnSpec::IsExpectedFuture::value &&
										ParamSpec::IsVoidValueBased::Value>;

			using IsReturnsExpectedFutureAndTakesValue =
				TIntegralConstant<bool, ReturnSpec::IsExpectedFuture::value &&
										ParamSpec::IsNonVoidValueBased::Value>;

			using IsReturnsExpectedFutureAndTakesExpected =
				TIntegralConstant<bool, ReturnSpec::IsExpectedFuture::value &&
										ParamSpec::IsExpectedBased::value>;

			using IsReturnsPlainValueOrExpectedAndTakesVoid =
				TIntegralConstant<bool, ReturnSpec::IsPlainValueOrExpected::Value &&
										ParamSpec::IsVoidValueBased::Value>;

			using IsReturnsPlainValueOrExpectedAndTakesValue =
				TIntegralConstant<bool, ReturnSpec::IsPlainValueOrExpected::Value &&
										ParamSpec::IsNonVoidValueBased::Value>;

			using IsReturnsPlainValueOrExpectedAndTakesExpected =
				TIntegralConstant<bool, ReturnSpec::IsPlainValueOrExpected::Value &&
										ParamSpec::IsExpectedBased::value>;
		
		public:
			using ReturnsVoidAndTakesVoid = TEnableIf<IsReturnsVoidAndTakesVoid::Value>;
			using ReturnsVoidAndTakesValue = TEnableIf<IsReturnsVoidAndTakesValue::Value>;
			using ReturnsVoidAndTakesExpected = TEnableIf<IsReturnsVoidAndTakesExpected::Value>;

			using ReturnsExpectedFutureAndTakesVoid = TEnableIf<IsReturnsExpectedFutureAndTakesVoid::Value>;
			using ReturnsExpectedFutureAndTakesValue = TEnableIf<IsReturnsExpectedFutureAndTakesValue::Value>;
			using ReturnsExpectedFutureAndTakesExpected = TEnableIf<IsReturnsExpectedFutureAndTakesExpected::Value>;

			using ReturnsPlainValueOrExpectedAndTakesVoid = TEnableIf<IsReturnsPlainValueOrExpectedAndTakesVoid::Value>;
			using ReturnsPlainValueOrExpectedAndTakesValue = TEnableIf<IsReturnsPlainValueOrExpectedAndTakesValue::Value>;
			using ReturnsPlainValueOrExpectedAndTakesExpected = TEnableIf<IsReturnsPlainValueOrExpectedAndTakesExpected::Value>;
		};

		/*
		*	These structs essentially 'cache' a set of useful types that are used frequently throughout the FutureExtensions module.
		*/
		template <typename F, typename P>
		struct TContinuationFunctorTypes
		{
			using ParamType = decltype(ContinuationFunctorParamTypeHelper(DeclVal<F>(), DeclVal<P>(), 0, 0));
			static_assert(!std::is_same<ParamType, BadContinuationFunctorSignature>::value, "Continuation function parameter can either be TExpected<P> or P");

			using ReturnType = typename TContinuationFunctorReturnType<F, ParamType>::ReturnType;
			using Specialization = TContinuationFunctorSpecializations<ReturnType, ParamType>;
		};

		template <typename F>
		struct TContinuationFunctorTypes<F, void>
		{
			using ParamType = decltype(ContinuationFunctorVoidParamTypeHelper(DeclVal<F>(), 0, 0));
			static_assert(!std::is_same<ParamType, BadContinuationFunctorSignature>::value, "Continuation function parameter can either be TExpected<void> or void");

			using ReturnType = typename TContinuationFunctorReturnType<F, ParamType>::ReturnType;
			using Specialization = TContinuationFunctorSpecializations<ReturnType, ParamType>;
		};
	}
}
