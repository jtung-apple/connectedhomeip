/**
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#import <Foundation/Foundation.h>
#import <os/lock.h>

#import "MTRRemoteDeviceController.h"

NS_ASSUME_NONNULL_BEGIN

// Need to figure out where to put this
enum {
    _MTRRemoteDeviceController_initWithParameters,
    _MTRRemoteDevice_registerDelegate,
    _MTRRemoteDevice_unregisterDelegate,
    _MTRRemoteDevice_estimatedStartTime,
    _MTRRemoteDevice_readAttribute,
    _MTRRemoteDevice_writeAttribute,
    _MTRRemoteDevice_invokeCommand,
    _MTRRemoteDeviceController_initWithParameters_response,
    _MTRRemoteDevice_estimatedStartTime_response,
    _MTRRemoteDevice_readAttribute_response,
    _MTRRemoteDevice_invokeCommand_response,
    _MTRRemoteDevice_delegate_stateChanged,
    _MTRRemoteDevice_delegate_receivedAttributeReport,
    _MTRRemoteDevice_delegate_receivedEventReport,
    _MTRRemoteDevice_delegate_deviceBecameActive,
};

typedef void (^MTRDeviceControllerInitResponseHandler)(NSNumber * controllerNodeID);
typedef void (^MTRDeviceStateResponseHandler)(NSNumber * state);
typedef void (^MTRDeviceEstimatedStartTimeResponseHandler)(NSDate * _Nullable estimatedStartTime);
typedef void (^MTRDeviceReadResponseHandler)(NSDictionary * _Nullable dataValueDictionary);
typedef void (^MTRDeviceInvokeResponseHandler)(NSArray * _Nullable invokeResponse, NSError * error);

@interface MTRRemoteDeviceController () {
    os_unfair_lock _deviceMapLock;
    dispatch_queue_t _xpcConnectionQueue;
    NSMutableDictionary * _nodeIDToDeviceMap;
    NSMutableDictionary<NSString *, id> * _serverResponseHandlers;
}
- (nullable instancetype)initWithParameters:(MTRRemoteDeviceControllerParameters *)parameters error:(NSError * __autoreleasing *)error;
@property (readonly, nonatomic, nullable) xpc_connection_t xpcConnection;

- (void)registerDeviceControllerInitResponseHandler:(MTRDeviceControllerInitResponseHandler)handler identifier:(NSString *)identifier;
- (void)registerDeviceStateResponseHandler:(MTRDeviceStateResponseHandler)handler identifier:(NSString *)identifier;
- (void)registerDeviceEstimatedStartTimeResponseHandler:(MTRDeviceEstimatedStartTimeResponseHandler)handler identifier:(NSString *)identifier;
- (void)registerDeviceReadResponseHandler:(MTRDeviceReadResponseHandler)handler identifier:(NSString *)identifier;
- (void)registerDeviceInvokeResponseHandler:(MTRDeviceInvokeResponseHandler)handler identifier:(NSString *)identifier;
@end

NS_ASSUME_NONNULL_END
