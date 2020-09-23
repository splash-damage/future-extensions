# Future Extensions

`SDFutureExtensions` is a plugin that extends the existing `TFuture` and `TPromise` classes within the UE4 `Core` module to add additional features such as:

* Continuations
* Execution Policies
* Cancellation

These features are heavily influenced by those found in the [Parallel Patterns Library](https://docs.microsoft.com/en-us/cpp/parallel/concrt/parallel-patterns-library-ppl?redirectedfrom=MSDN&view=vs-2019).

## Requirements

- C++14 compatible compiler (supported by UE4)
- Unreal Engine 4.25.1 or newer
- [Automatron](https://github.com/splash-damage/Automatron) plugin for automated testing

## Usage

- Download the version of **SDFutureExtensions** that matches your engine from the [releases](https://github.com/splash-damage/future-extensions/releases) page.
  *You can opt to download any branch, understanding that this may be work in-progress.*
- Drop the plugin files inside `Plugins/SDFutureExtensions` in the desired project.

**SDFutureExtensions** is in BETA and no guarantees are given. See the [license](LICENSE).

## Motivation

Live service games invariably rely on one or more backend services. These services provide a large degree of functionality separated from the client that is therefore accessed in an asynchronous manner.

The concept of [Futures and Promises](https://en.wikipedia.org/wiki/Futures_and_promises) are well established constructs for working with these asynchronous tasks. Using these constructs instead of the more traditional UE4 approach of [Delegates](https://docs.unrealengine.com/en-US/Programming/UnrealArchitecture/Delegates/index.html) results in considerably more readable code as well as making it more straightforward to follow the flow of control when multiple asynchronous tasks happen in a chain (or in parallel).

In addition to aiding readability, this plugin also seeks to provide an additional feature set that makes it easier to create chains of asynchronous tasks in an ergonomic and composable fashion through the use of continuations.

## Roadmap

- [x] Continuations
- [x] Cancellation
- [x] Execution Policies
- [x] `When_All()`/`When_Any()` API
  * See [N3721 proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3721.pdf)

## Key classes

* `TExpected<T>`
  * A class that represents an object that either has the expected value (`<T>`), or an unexpected value providing associated error details. Conceptually a union consisting of a `TOptional<T>` and an `Error`.
* `Error`
  * A lightweight class that represents a generic error associated with a `TExpected` object.
* `TExpectedPromise<T>`
  * A wrapper around the existing `TPromise<T>` class that wraps `<T>` in a `TExpected` object.
* `TExpectedFuture<T>`
  * The `Future` associated with a `TExpectedPromise`.
* `FCancellationHandle`
  * Wrapper around a flag that can be used to cancel an in-flight `TExpectedPromise`.

Additional functionality is implemented as non-member free functions outside of the classes above.

## Key concepts

### Continuations

The most important addition to the existing `TFuture` class is the support for continuations. They are similar in nature to those described in the N3721 Proposal - [Improvements to `std::future` and Related APIs](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3721.pdf):

> In asynchronous programming, it is very common for one asynchronous operation, on completion, to invoke a second operation and pass data to it. The current C++ standard does not allow one to register a continuation to a future. With then, instead of waiting for the result, a continuation is “attached” to the asynchronous operation, which is invoked when the result is ready. Continuations registered using the then function will help to avoid blocking waits or wasting threads on polling, greatly improving the responsiveness and scalability of an application.

Key to their utility is that continuations provide a mechanism for users to use `TFuture` without having to call `WaitFor()` (and therefore block the calling thread) or `IsComplete()` (and therefore have to provide a polling loop).

`SDFutureExtensions` provides the same underlying concepts as those described in the N3721 proposal, but crucially **does not use exceptions**. Proposal N3721 uses exceptions to indicate errors that may have occured in antecendent `Future`s and propagates them descendent `Future`s through the continuation chain. Instead, we use the [expected concept that is proposed for C++](https://www.youtube.com/watch?v=JfMBLx7qE0I). Again, the `std::expected<T, E>` proposal still uses exceptions, but our version swaps that out for an `Error` object that is conceptually similar but without the overhead of requiring exceptions to be enabled.

#### Implementation details

##### Value-based vs Expected-based continuations

A **value-based continuation** is only scheduled if the antecendent `TExpectedFuture` was successful. A **value-based continuation** is defined as below, note how the continuation parameter is `int` as opposed to `TExpected<int>`:

```cpp
SD::TExpectedFuture<int> FirstFuture = SD::Async([]() {
    return SD::MakeErrorExpected<int>(SD::Error(TEST_ERROR_CODE, TEST_ERROR_CONTEXT));
});
 
bool bContinuationCalled = false;
SD::TExpectedFuture<void> SecondFuture = FirstFuture.Then([&bContinuationCalled](int ExpectedResult) {
    bContinuationCalled = true;
});
 
TestFalse("Continuation has not been called", bContinuationCalled);
```

A **expected-based continuation** is scheduled regardless of the state of the antecendent future:

```cpp
SD::TExpectedFuture<int> FirstFuture = SD::Async([]() {
    return SD::MakeErrorExpected<int>(SD::Error(TEST_ERROR_CODE, TEST_ERROR_CONTEXT, TEST_ERROR_INFO));
});
 
int InternalError = 0;
 
SD::TExpectedFuture<void> SecondFuture = FirstFuture.Then([&](SD::TExpected<int> ExpectedResult) {
    if (ExpectedResult.IsError())
    {
        InternalError = ExpectedResult.GetError()->GetErrorCode();
    }
});
 
TestEqual("Continuation has been called", InternalError, TEST_ERROR_CODE);
```

This functionality can be useful when composing multiple asynchronous calls in a chain, as you can provide a single 'catch-all' **expected-based continuation** after a chain of **value-based continuations** that only execute during normal behaviour.

### Execution policies

Execution policies indicate where in a multithreaded environment a continuation should execute and are specific to an individual continuation. continuations can also inherit the execution policy of their antecendent future.

Supported execution policies are:

* `Inline`
  * Continuations are run on the same thread as their antecendent without scheduling
* `Thread`
  * Continuations are run on a specific `ENamedThread`
* `ThreadPool`
  * Continuations are run on the Thread Pool (`FQueuedThreadPool`)

#### Implementation details

Execution policies are implemented in terms of the underlying UE4 asynchronous systems which have [different usages depending on the work being done asynchronously](https://forums.unrealengine.com/development-discussion/engine-source-github/27216-new-core-feature-async-framework-master-branch-and-4-8):

> The `TaskGraph` is shared by many other systems in the Engine and is intended for small tasks that are very short-running, never block, and must complete as soon as possible. Launching graph tasks is very cheap as compared to starting up threads, but you must ensure that your code does not block the `TaskGraph` ever. In particular, you should not set up `Async()` functions on the `TaskGraph` that in turn create other `Async<T>()` calls or may wait on some external event.This is very important, because if all worker threads are waiting then nothing else gets done in the Engine. If your code may block or create other asynchronous calls then use `Thread` or `ThreadPool` instead.

> Threads are quite expensive to create and best suited for long running tasks or tasks that may block. Operating systems generally impose limits on the number of threads that can be created, and they also slow down considerably once too many threads are alive at the same time. If you have many tasks (hundreds) or only want to maximize CPU utilization and do not care about all your tasks actually running in parallel at the same time, use `ThreadPool` instead.

> The `ThreadPool` is another set of worker threads that is independent from the `TaskGraph` system. It allows you to queue up an arbitrary number of threads, which will then be completed one after another based on the availability of worker threads. If your tasks do not fit into either `TaskGraph` or `Thread`, then execute them here.

Within `SDFutureExtensions`, the above systems are used to implement the following policies:

* `Inline` and `Thread`
  * Using the `TaskGraph` system to specify the specific thread to run on.
* `ThreadPool`
  * Using the underlying `FQueuedThreadPool` system.

`SDFutureExtensions` does not use `Threads` as specified by Epic as they have a large overhead of spinning up an entire new thread, and the same outcome can be achieved using a specific `NamedThread` with `TaskGraph`.

### Cancellation

Cancellation is an action that is taken on a `Promise` which signals that the caller no longer cares about the value that would otherwise be set on this `Promise`. Any continuations chained to the promise are still evaluated, but the `TExpected<T>` object that is passed to them is in the `Cancelled` state, and as such any **value-based continuations** will *not* be scheduled; **Expected-based continuations** will be scheduled as normal.

#### Implementation details

It is important to know that cancellation is an accepted race condition. If the function body for the asynchronous work is happening on a different thread there is every chance that it can be called before the cancellation has been propagated to the `TExpectedPromise`. Promises are only ever set once; whoever wins the race gets to set it.

This means that cancellation is best-effort cancellation and *not guaranteed*.

### Use case - Converting blocking code

``` cpp
struct FPlayerProfile
{
    FString PlayerName;
    //...
  
    FPlayerProfile ConvertFromHTTPResponse(const FString& Response);
}
 
FPlayerProfile GetPlayerProfile()
{
    //HTTP::GetPlayerProfileBlocking waits for HTTP response before returning the value
    return ConvertFromHTTPResponse(HTTPSystem::GetPlayerProfileBlocking());
}
 
//UI Scene
void UIScene_PlayerProfile::OnSceneOpen()
{
    const FPlayerProfile PlayerProfile = GetPlayerProfile();
 
    NameWidget.SetName(PlayerProfile.PlayerName);
}
```

Consider the code block above - if called from the main thread, this call would block while it waits for the `HTTP` resonse from the `GetPlayerProfileBlocking()` function. This function is called when a `UIScene` is opened, which is going to manifest in the UI  'stalling' while it waits for the data to set the appropriate widget.

Traditionally within UE4, this would be handled using a callback system (i.e. delegates) that would be triggered when the request was complete. Let's see how we can instead use `TExpectedFuture` to make this more ergonomic.

#### Performing asynchronous work

The first thing we need to do is offload the blocking call to another thread to ensure it doesn't block the main thread (ideally, you would rewrite the `HTTP` system to return a `TExpectedFuture`, but that's an exercise for the reader):

``` cpp
struct FPlayerProfile
{
    FString PlayerName;
    //...
  
    FPlayerProfile ConvertFromHTTPResponse(const FString& Response);
}
 
FPlayerProfile GetPlayerProfile()
{
    SD::TExpectedFuture<FString> ResponseFuture = SD::Async([](){
        //This call still blocks, but it now does so on a TaskGraph thread
        return HTTPSystem::GetPlayerProfileBlocking();
    });
 
    //Get() is provided here for illustrative purposes - it is not a part of the interface
    //as it is a blocking call.
    const FString Response = ResponseFuture.Get();
 
    return ConvertFromHTTPResponse(Response);
}
 
//UI Scene
void UIScene_PlayerProfile::OnSceneOpen()
{
    const FPlayerProfile PlayerProfile = GetPlayerProfile();
 
    NameWidget.SetName(PlayerProfile.PlayerName);
}
```

This is better - the blocking call to `GetPlayerProfileBlocking()` is now scheduled to run on the `TaskGraph` using `SD::Async()`, which is a good first step. However this code will still block as it calls `.Get()` - this is how `TFuture` works. `.Get()` is not exposed by the `TExpectedFuture` interface for purely that reason - our interface is non-blocking. 

Let's see how we can remove all the blocking behaviour here using continuations.

#### Using continuations

The call to `GetPlayerProfileBlocking()` returns an `TExpectedFuture`, which means we can attach a continuation to it which will be run when the call completes, and will be passed the return value:

```cpp
struct FPlayerProfile
{
    FString PlayerName;
    //...
  
    FPlayerProfile ConvertFromHTTPResponse(const FString& Response);
}
 
SD::TExpectedFuture<FPlayerProfile> GetPlayerProfileAsync()
{
    return SD::Async([](){
        //This call still blocks, but it now does so on a TaskGraph thread
        return HTTPSystem::GetPlayerProfileBlocking();
    }).Then([](FString HTTPResponse){
        return ConvertFromHTTPResponse(HTTPResponse);
    });
}
 
//UI Scene
void UIScene_PlayerProfile::OnSceneOpen()
{
    NameWidget.SetSpinner(true);
    GetPlayerProfileAsync().Then([this](FPlayerProfile PlayerProfileResult) {
        NameWidget.SetName(PlayerProfileResult.PlayerName);
    });
}
```

By moving the call to `ConvertFromHTTPResponse()` into a continuation, we're now able to avoid the blocking call to `.Get()`. However, this does mean that we've changed the function declaration to return a `TExpectedFuture` via the call to `.Then()`.

Because of this, we also change the code to set `NameWidget` to use continuations. When the continuation 'chain' from `GetPlayerProfileAsync()` resolves, it will then run the continuation with the converted `FPlayerProfile` struct and set the widget. This is an asynchronous process, so we've called a function (`SetSpinner()`) before setting up the continuation so that the user knows we're in the process of retrieving the information required to set this widget.

#### Error handling

The call to `GetPlayerProfileBlocking()` is calling an external service which could return an error - this needs to be handled to ensure we have a good player experience. This is achieved using the error-handling functionality within `TExpected`:

```cpp
struct FPlayerProfile
{
    FString PlayerName;
    //...
  
    FPlayerProfile ConvertFromHTTPResponse(const FString& Response);
}
 
SD::TExpectedFuture<FPlayerProfile> GetPlayerProfileAsync()
{
    return SD::Async([](){
        //This call still blocks, but it now does so on a TaskGraph thread
        return HTTPSystem::GetPlayerProfileBlocking();
    }).Then([](SD::TExpected<FString> HTTPResponse) {
        if(HTTPResponse.IsCompleted())
        {
            return ConvertFromHTTPResponse(*HTTPResponse);
        }
 
        return SD::Convert<FPlayerProfile>(HTTPResponse);
    });
}
 
//UI Scene
void UIScene_PlayerProfile::OnSceneOpen()
{
    NameWidget.SetSpinner(true);
    GetPlayerProfileAsync().Then([this](SD::TExpected<FPlayerProfile> PlayerProfileResult) {
        if(PlayerProfileResult.IsCompleted())
        {
            NameWidget.SetName(PlayerProfileResult.PlayerName);
        }
        else if(PlayerProfileResult.IsError())
        {
            //Assumes this function understands how to convert from a SD::Error into
            //something suitable for players to see.
            UISystem::ShowErrorDialog(PlayerProfileResult.GetError());
        }
    });
}
```

The continuation attached to `GetPlayerProfileBlocking()` has been changed to an **expected-based continuation** by changing the parameter from an `FString` to a `TExpected<FString>` - doing this means that the continuation will get called regardless of the state of the antecendent call.

Any errors that were potentially generated by the antecendent call will be propagated to this continuation. Because of this these errors need to be handled - it cannot be assumed that the result was successful. This is done by checking the state of the passed `TExpected<FString>` parameter via `IsCompleted()` - `Convert...()` is only called if `true` is returned. The `TExpected<FString>` parameter can now be dereferenced to get the contained `HTTPResponse` value which is known to exist as `IsCompleted()` returned `true`. 

However, if `IsCompleted()` returns `false`, we can't convert the response - it's up to the programmer to determine how to handle these situations on a case-by-case basis. In this example, we call `SD::Convert<...>()` to pass whatever state was contained in `HTTPResponse` back to the caller of this function, thus propagating the error downwards for the next continuation in the chain to handle (`SD::Convert` is required here as we need to convert from `SD::TExpected<FString>` to `SD::TExpected<FPlayerProfile>`).

The UI code has also been modified to use an **expected-based continuation** - in this case that the `SD::Error` object contained within the unsuccessful `SD::TExpected` parameter is retrieved and shown to the player in an error dialog.

#### Cancellation

UI scenes are a good example of when to use cancellation. Cancelling an asynchronous function (either an initial function or a continuation function) does two things:

* Sets the associated promise to the Cancelled state
* Does not run the associated function body

For instance, in the scenario above, if the UI scene is closed before the `GetPlayerProfileAsync()` resolves then unexpected behaviour may occur if the continuation is run using the now closed scene. This can be fixed by cancelling the continuation so it does not get run, regardless of the state of the previous asynchronous function.

```cpp
struct FPlayerProfile
{
    FString PlayerName;
    //...
  
    FPlayerProfile ConvertFromHTTPResponse(const FString& Response);
}
 
SD::TExpectedFuture<FPlayerProfile> GetPlayerProfileAsync()
{
    return SD::Async([](){
        //This call still blocks, but it now does so on a TaskGraph thread
        return HTTPSystem::GetPlayerProfileBlocking();
    }).Then([](SD::TExpected<FString> HTTPResponse) {
        if(HTTPResponse.IsCompleted())
        {
            return ConvertFromHTTPResponse(*HTTPResponse);
        }
 
        return SD::Convert<FPlayerProfile>(HTTPResponse);
    });
}
 
class UIScene_PlayerProfile
{
    //...
    void OnSceneOpen();
    void OnSceneClosed();
    //...
private:
    //...
    SD::SharedCancellationHandlePtr CancellationHandle;
}
 
//UI Scene
void UIScene_PlayerProfile::OnSceneOpen()
{
    CancellationHandle = SD::CreateCancellationHandle();
 
    NameWidget.SetSpinner(true);
    GetPlayerProfileAsync().Then([this](SD::TExpected<FPlayerProfile> PlayerProfileResult) {
        if(PlayerProfileResult.IsCompleted())
        {
            NameWidget.SetName(PlayerProfileResult.PlayerName);
        }
        else if(PlayerProfileResult.IsError())
        {
            //Assumes this function understands how to convert from a SD::Error into
            //something suitable for players to see.
            UISystem::ShowErrorDialog(PlayerProfileResult.GetError());
        }
    }, SD::FExpectedFutureOptions(CancellationHandle));
}
 
void UIScene_PlayerProfile::OnSceneClosed()
{
    if(CancellationHandle.IsValid())
    {
        CancellationHandle->Cancel();
        CancellationHandle.Reset();
    }
}
```

In this code snippet a `FCancellationHandle` is created using `SD::CreateCancellationHandle()` and passed to the continuation that is chained from `GetPlayerProfileAsync()`. `UIScene_PlayerProfile::OnSceneClosed()` calls `Cancel()` on the `FCancellationHandle` which will attempt to set any promises that have been associated with it to the `Cancelled` state (and therefore not call any associated function bodies).

In this case, if `Cancel()` is called before `GetPlayerProfileAsync()` has completed then the function body in the continuation will not be run.

Remember that cancellation is a best-effort race condition - there's no guarantee that the continuation function body will not start to be executed before the `TExpectedPromise` is set to the `Cancelled` state. This is why you should still ensure the lifetimes of captured variables is valid regardless of using cancellation.

### Use case - Wrapping UE4 delegates

A common pattern associated with online-related code in UE4 is to use the `Delegate` system to register for callbacks when asynchronous work has completed. Wrapping such calls with `SDFutureExtensions` functionality can create a more ergonomic and robust API.

Take for example this sample class which does a simple session search using the `IOnlineSession` API:

```cpp
//Header
UCLASS()
class UGameSessionFinder : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UGameSessionFinder();
	void FindSessions();
private:
	void OnFindSessionsComplete(bool bWasSuccessful);

	FOnFindSessionsCompleteDelegate OnFindComplete;
	FDelegateHandle OnFindCompleteHandle;

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	TArray<FOnlineSessionSearchResult> SearchResults;
};

//...
//Implementation
UGameSessionFinder::UGameSessionFinder()
{
	OnFindComplete = FOnFindSessionsCompleteDelegate::CreateUObject(
		this, &UGameSessionFinder::OnFindSessionsComplete);
}

void UGameSessionFinder::FindSessions()
{
	const auto OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const auto Sessions = OnlineSub->GetSessionInterface();
	check(Sessions.IsValid());

	const auto LocalPlayer = GEngine->GetFirstGamePlayer(GetWorld());
	check(LocalPlayer);

 	SessionSearch = MakeShareable(new FOnlineSessionSearch());

	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->PingBucketSize = 500;

	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	OnFindCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindComplete);

	Sessions->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void UGameSessionFinder::OnFindSessionsComplete(bool bWasSuccessful)
{
	const auto OnlineSub = Online::GetSubsystem(GetWorld());
	check(OnlineSub);

	const auto Sessions = OnlineSub->GetSessionInterface();
	check(Sessions.IsValid());

	Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindCompleteHandle);

	SearchResults = SessionSearch->SearchResults;
}
```

This can be converted to a single non-member free function that returns a composable `TExpectedFuture<...>` by combining the delegate call with a `TExpectedPromise<...>`, as shown below:

```cpp
SD::TExpectedFuture<TArray<FOnlineSessionSearchResult>> FindSessionsAsync(ULocalPlayer* ForPlayer, const FName SessionName, TSharedPtr<FOnlineSessionSearch> FindSessionsSettings)
{
	checkf(ForPlayer, TEXT("Invalid ULocalPlayer instance"));

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(ForPlayer->GetWorld());
	checkf(OnlineSub, TEXT("Failed to retrieve OnlineSubsystem"));

	IOnlineSessionPtr SessionPtr = OnlineSub->GetSessionInterface();
	checkf(SessionPtr, TEXT("Failed to retrieve IOnlineSession interface"));

    //Create a TExpectedPromise that wraps an array of search results, i.e. the same thing that the FindSession API delegate returns.
    //This is wrapped in a TSharedPtr as it's lifetime needs to be associated with the lambda delegate that sets it.
	TSharedPtr<SD::TExpectedPromise<TArray<FOnlineSessionSearchResult>>> Promise = MakeShared<SD::TExpectedPromise<TArray<FOnlineSessionSearchResult>>>();
	auto OnComplete = FOnFindSessionsCompleteDelegate::CreateLambda([Promise, FindSessionsSettings](bool Success) 
	{
		if (Success)
		{
			Promise->SetValue(FindSessionsSettings->SearchResults);
		}
		else
		{
			Promise->SetValue(SD::Error(-1, TEXT("Session search failed")));
		}
	});

    //Again our DelegateHandle is wrapped in a TSharedPtr as it's lifetime needs to be associated with the continuation attached to the TExpectedPromise above.
	TSharedPtr<FDelegateHandle> DelegateHandle = MakeShareable(new FDelegateHandle());
	*DelegateHandle = SessionPtr->AddOnFindSessionsCompleteDelegate_Handle(OnComplete);

	if (!SessionPtr->FindSessions(*ForPlayer->GetPreferredUniqueNetId(), FindSessionsSettings.ToSharedRef()))
	{
		Promise->SetValue(SD::Error(-1, FString::Printf(TEXT("Failed to find '%s' sessions."), *(SessionName.ToString()))));
	}

	TWeakPtr<IOnlineSession, ESPMode::ThreadSafe> SessionInterfaceWeak = SessionPtr;
	return Promise->GetFuture().Then([DelegateHandle, SessionInterfaceWeak](SD::TExpected<TArray<FOnlineSessionSearchResult>> ExpectedResults) {
		IOnlineSessionPtr SessionInterface = SessionInterfaceWeak.Pin();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(*DelegateHandle);
		}

		return ExpectedResults;
	}).Then([](TArray<FOnlineSessionSearchResult> Results) {
		return Results.FilterByPredicate([CompatibilityId, SearchType](const FOnlineSessionSearchResult& Result) {
			// Do some session filtering here based on game-specific logic.
            return true;
		});
	});
}
```

The return value of this function is now a `TExpectedFuture` that, at some point, will be fulfilled with a `TArray<FOnlineSessionSearchResult>` *or* an `Error`. This also means that code that calls this function can add their own continuations. With a fully asynchronous API for the common online session functions we can implement a simple quick-match solution using composition and error handling as such:

```cpp
//...
TWeakObjectPtr<ULocalPlayer> WeakLocalPlayer = ForPlayer;
//Try and find a session to join, or host a session for others to join
DestroySessionAsync(ForPlayer, NAME_GameSession).Then([WeakLocalPlayer](SD::TExpected<FName>) {
    if (ULocalPlayer* LP = WeakLocalPlayer.Get())
    {
        return FindSessionsAsync(LP, NAME_GameSession);
    }
    else
    {
        return SD::MakeErrorFuture<TArray<FOnlineSessionSearchResult>>(SD::Error(...));
    }
}).Then([WeakLocalPlayer](TArray<FOnlineSessionSearchResult> SessionSearchResults) {
    if (ULocalPlayer* LP = WeakLocalPlayer.Get())
    {
        const FOnlineSessionSearchResult& SessionToJoin = SessionSearchResults.Num() > 0 ? SessionSearchResults[0] : FOnlineSessionSearchResult();

        if (SessionToJoin.IsValid())
        {
            return JoinSessionAsync(LP, NAME_GameSession, SessionToJoin);
        }
        else
        {
            return HostSessionAsync(LP, NAME_GameSession);
        }
    }
    else
    {
        return SD::MakeErrorFuture<FName>(SD::Error(...));
    }
}).Then([WeakLocalPlayer](FName TravelToSessionName) {
    if (ULocalPlayer* LP = WeakLocalPlayer.Get())
    {
        IOnlineSubsystem* OnlineSub = Online::GetSubsystem(LP->GetWorld());
        checkf(OnlineSub, TEXT("Failed to retrieve OnlineSubsystem"));

        IOnlineSessionPtr SessionPtr = OnlineSub->GetSessionInterface();
        checkf(SessionPtr, TEXT("Failed to retrieve IOnlineSession interface"));

        FNamedOnlineSession* Session = SessionPtr->GetNamedSession(TravelToSessionName);
        checkf(Session, TEXT("Failed to retrieve named session"));

        FString TravelURL = TEXT("");
        if (Session->bHosting)
        {
            FString HostMapName = TEXT("");
            if (!Session->SessionSettings.Get(SETTING_MAPNAME, HostMapName))
            {
                return SD::MakeErrorFuture<void>(SD::Error(...));
            }

            TravelURL = FString::Printf(TEXT("%s?listen"), *HostMapName);
        }
        else if (!SessionPtr->GetResolvedConnectString(TravelToSessionName, TravelURL))
        {
            return SD::MakeErrorFuture<void>(SD::Error(...));
        }

        APlayerController* PC = LP->GetPlayerController(LP->GetWorld())
        if (!PC)
        {
            return SD::MakeErrorFuture<void>(SD::Error(...));
        }

        PC->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
        return SD::MakeReadyFuture();
    }
    else
    {
        return SD::MakeErrorFuture<void>(SD::Error(...));
    }
}).Then([WeakLocalPlayer](SD::TExpected<void> FinalExpected) {
    if (!FinalExpected.IsCompleted())
    {
        //Handle any errors from any of the above operations
        LogError(FinalExpected);
        if (ULocalPlayer* LP = WeakLocalPlayer.Get())
        {
            DestroySessionAsync(LP, NAME_GameSession);
        }
    }
});
//...
```
