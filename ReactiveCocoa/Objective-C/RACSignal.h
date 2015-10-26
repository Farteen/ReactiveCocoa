//
//  RACSignal.h
//  ReactiveCocoa
//
//  Created by Josh Abernathy on 3/1/12.
//  Copyright (c) 2012 GitHub, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RACStream.h"

@class RACDisposable;
@class RACScheduler;
@class RACSubject;
@protocol RACSubscriber;

@interface RACSignal : RACStream

///创建一个新的信号.最好使用这个方法来创建一个信号
///事件可以直接发送到新的订阅者在didSubscribe这个block中,但是订阅者无法在RACDisposable从didSubscribe返回前销毁信号
///在无限多信号的情况下,如果时间被直接发送,信号的销毁将不会发生.
///为了保证消息是可以销毁的,事件可以被安排在当前的scheduler

///didSubscribe - 当信号被订阅时会被调用.新的订阅者传入.之后你可以手动控制Subscriber,通过发送sendNext:,sendError:,
///   sendError:,和sendComplete,正如你实现的操作的定义.这个block回返回一个用来取消任何被
///   订阅产生的正在进行的任务,并且回清理任何相关资源,或者销毁创建他的一部分的RACDisposable对象.
///   当disposable被销毁了,这个信号必须不能发送任何事件到订阅者们.如果没有需要清除的可以返回nil

///**Note:** 注意didSubscribe的block当每次有一个subscriber订阅的时候都会调用.任何副作用在这个block种将会因此对每次订阅都会执行,并且不是必须在一个线程,有可能是同步的
/// Creates a new signal. This is the preferred way to create a new signal
/// operation or behavior.
///
/// Events can be sent to new subscribers immediately in the `didSubscribe`
/// block, but the subscriber will not be able to dispose of the signal until
/// a RACDisposable is returned from `didSubscribe`. In the case of infinite
/// signals, this won't _ever_ happen if events are sent immediately.
///
/// To ensure that the signal is disposable, events can be scheduled on the
/// +[RACScheduler currentScheduler] (so that they're deferred, not sent
/// immediately), or they can be sent in the background. The RACDisposable
/// returned by the `didSubscribe` block should cancel any such scheduling or
/// asynchronous work.
///
/// didSubscribe - Called when the signal is subscribed to. The new subscriber is
///                passed in. You can then manually control the <RACSubscriber> by
///                sending it -sendNext:, -sendError:, and -sendCompleted,
///                as defined by the operation you're implementing. This block
///                should return a RACDisposable which cancels any ongoing work
///                triggered by the subscription, and cleans up any resources or
///                disposables created as part of it. When the disposable is
///                disposed of, the signal must not send any more events to the
///                `subscriber`. If no cleanup is necessary, return nil.
///
/// **Note:** The `didSubscribe` block is called every time a new subscriber
/// subscribes. Any side effects within the block will thus execute once for each
/// subscription, not necessarily on one thread, and possibly even
/// simultaneously!
+ (RACSignal *)createSignal:(RACDisposable * (^)(id<RACSubscriber> subscriber))didSubscribe;

///返回一个信号,立即发送一个给定的error
/// Returns a signal that immediately sends the given error.
+ (RACSignal *)error:(NSError *)error;

///返回一个不会完成的信号
/// Returns a signal that never completes.
+ (RACSignal *)never;

///立即计划一个给定的block在给定的scheduler种.这个block被指定一个可以发送事件的订阅者.
///返回一个singal将会发送所有的事件到这个订阅者.所有时间将会在scheduler中发送,
///并且他将会replay任何错过的事件到新的订阅者
/// Immediately schedules the given block on the given scheduler. The block is
/// given a subscriber to which it can send events.
///
/// scheduler - The scheduler on which `block` will be scheduled and results
///             delivered. Cannot be nil.
/// block     - The block to invoke. Cannot be NULL.
///
/// Returns a signal which will send all events sent on the subscriber given to
/// `block`. All events will be sent on `scheduler` and it will replay any missed
/// events to new subscribers.
+ (RACSignal *)startEagerlyWithScheduler:(RACScheduler *)scheduler block:(void (^)(id<RACSubscriber> subscriber))block;

///调用给定的block在首次订阅.这个block是可以发送时间的到给定的订阅者
///注意
/// Invokes the given block only on the first subscription. The block is given a
/// subscriber to which it can send events.
///
/// Note that disposing of the subscription to the returned signal will *not*
/// dispose of the underlying subscription. If you need that behavior, see
/// -[RACMulticastConnection autoconnect]. The underlying subscription will never
/// be disposed of. Because of this, `block` should never return an infinite
/// signal since there would be no way of ending it.
///
/// scheduler - The scheduler on which the block should be scheduled. Note that 
///             if given +[RACScheduler immediateScheduler], the block will be
///             invoked synchronously on the first subscription. Cannot be nil.
/// block     - The block to invoke on the first subscription. Cannot be NULL.
///
/// Returns a signal which will pass through the events sent to the subscriber
/// given to `block` and replay any missed events to new subscribers.
+ (RACSignal *)startLazilyWithScheduler:(RACScheduler *)scheduler block:(void (^)(id<RACSubscriber> subscriber))block;

@end

@interface RACSignal (RACStream)
///返回一个给定值的signal
/// Returns a signal that immediately sends the given value and then completes.
+ (RACSignal *)return:(id)value;
///返回一个empty的signal
/// Returns a signal that immediately completes.
+ (RACSignal *)empty;
///拼接一个signal,当这个signal完成
/// Subscribes to `signal` when the source signal completes.
- (RACSignal *)concat:(RACSignal *)signal;
///zip会拼装一个signal成为一个RACTuple的信号
/// Zips the values in the receiver with those of the given signal to create
/// RACTuples.
///
/// The first `next` of each stream will be combined, then the second `next`, and
/// so forth, until either signal completes or errors.
///
/// signal - The signal to zip with. This must not be `nil`.
///
/// Returns a new signal of RACTuples, representing the combined values of the
/// two signals. Any error from one of the original signals will be forwarded on
/// the returned signal.
- (RACSignal *)zipWith:(RACSignal *)signal;

@end

@interface RACSignal (Subscription)

///消息接收者跟随被订阅者的变化而变化
///订阅将会一直在一个有效的scheduler上发生.如果当前的
/// Subscribes `subscriber` to changes on the receiver. The receiver defines which
/// events it actually sends and in what situations the events are sent.
///
/// Subscription will always happen on a valid RACScheduler. If the
/// +[RACScheduler currentScheduler] cannot be determined at the time of
/// subscription (e.g., because the calling code is running on a GCD queue or
/// NSOperationQueue), subscription will occur on a private background scheduler.
/// On the main thread, subscriptions will always occur immediately, with a
/// +[RACScheduler currentScheduler] of +[RACScheduler mainThreadScheduler].
///
/// This method must be overridden by any subclasses.
///
/// Returns nil or a disposable. You can call -[RACDisposable dispose] if you
/// need to end your subscription before it would "naturally" end, either by
/// completing or erroring. Once the disposable has been disposed, the subscriber
/// won't receive any more events from the subscription.
- (RACDisposable *)subscribe:(id<RACSubscriber>)subscriber;

/// Convenience method to subscribe to the `next` event.
///
/// This corresponds to `IObserver<T>.OnNext` in Rx.
- (RACDisposable *)subscribeNext:(void (^)(id x))nextBlock;

/// Convenience method to subscribe to the `next` and `completed` events.
- (RACDisposable *)subscribeNext:(void (^)(id x))nextBlock completed:(void (^)(void))completedBlock;

/// Convenience method to subscribe to the `next`, `completed`, and `error` events.
- (RACDisposable *)subscribeNext:(void (^)(id x))nextBlock error:(void (^)(NSError *error))errorBlock completed:(void (^)(void))completedBlock;

/// Convenience method to subscribe to `error` events.
///
/// This corresponds to the `IObserver<T>.OnError` in Rx.
- (RACDisposable *)subscribeError:(void (^)(NSError *error))errorBlock;

/// Convenience method to subscribe to `completed` events.
///
/// This corresponds to the `IObserver<T>.OnCompleted` in Rx.
- (RACDisposable *)subscribeCompleted:(void (^)(void))completedBlock;

/// Convenience method to subscribe to `next` and `error` events.
- (RACDisposable *)subscribeNext:(void (^)(id x))nextBlock error:(void (^)(NSError *error))errorBlock;

/// Convenience method to subscribe to `error` and `completed` events.
- (RACDisposable *)subscribeError:(void (^)(NSError *error))errorBlock completed:(void (^)(void))completedBlock;

@end

/// Additional methods to assist with debugging.
@interface RACSignal (Debugging)

/// Logs all events that the receiver sends.
- (RACSignal *)logAll;

/// Logs each `next` that the receiver sends.
- (RACSignal *)logNext;

/// Logs any error that the receiver sends.
- (RACSignal *)logError;

/// Logs any `completed` event that the receiver sends.
- (RACSignal *)logCompleted;

@end

/// Additional methods to assist with unit testing.
///
/// **These methods should never ship in production code.**
@interface RACSignal (Testing)

/// Spins the main run loop for a short while, waiting for the receiver to send a `next`.
///
/// **Because this method executes the run loop recursively, it should only be used
/// on the main thread, and only from a unit test.**
///
/// defaultValue - Returned if the receiver completes or errors before sending
///                a `next`, or if the method times out. This argument may be
///                nil.
/// success      - If not NULL, set to whether the receiver completed
///                successfully.
/// error        - If not NULL, set to any error that occurred.
///
/// Returns the first value received, or `defaultValue` if no value is received
/// before the signal finishes or the method times out.
- (id)asynchronousFirstOrDefault:(id)defaultValue success:(BOOL *)success error:(NSError **)error;

/// Spins the main run loop for a short while, waiting for the receiver to complete.
///
/// **Because this method executes the run loop recursively, it should only be used
/// on the main thread, and only from a unit test.**
///
/// error - If not NULL, set to any error that occurs.
///
/// Returns whether the signal completed successfully before timing out. If NO,
/// `error` will be set to any error that occurred.
- (BOOL)asynchronouslyWaitUntilCompleted:(NSError **)error;

@end

@interface RACSignal (Unavailable)

+ (RACSignal *)start:(id (^)(BOOL *success, NSError **error))block __attribute__((unavailable("Use +startEagerlyWithScheduler:block: instead")));
+ (RACSignal *)startWithScheduler:(RACScheduler *)scheduler subjectBlock:(void (^)(RACSubject *subject))block __attribute__((unavailable("Use +startEagerlyWithScheduler:block: instead")));
+ (RACSignal *)startWithScheduler:(RACScheduler *)scheduler block:(id (^)(BOOL *success, NSError **error))block __attribute__((unavailable("Use +startEagerlyWithScheduler:block: instead")));

@end
