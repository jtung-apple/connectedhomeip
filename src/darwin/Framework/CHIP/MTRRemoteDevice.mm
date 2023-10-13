//
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

#import <os/lock.h>

#import "MTRDeviceController.h"
#import "MTRDeviceController_Internal.h"
#import "MTRDevice_Internal.h"
#import "MTRLogging_Internal.h"
#import "MTRRemoteDeviceController_Internal.h"
#import "MTRRemoteDeviceXPCUtils.h"
#import "MTRRemoteDevice_Internal.h"

#pragma - XPC

void __XPC_MTRRemoteDevice_registerDelegate(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSNumber * nodeID)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDevice_registerDelegate);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (nodeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "nodeID", nodeID, NULL);
        }

        if (handler != NULL) {
            xpc_dictionary_set_int64(__message, "__xpc_wants_reply__", 1);
            if (replyq == NULL)
                replyq = dispatch_get_main_queue();
            xpc_connection_send_message_with_reply(connection, __message, replyq, handler);
        } else {
            xpc_connection_send_message(connection, __message);
        }
    }
}

void __XPC_MTRRemoteDevice_unregisterDelegate(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSNumber * nodeID)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDevice_unregisterDelegate);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (nodeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "nodeID", nodeID, NULL);
        }

        if (handler != NULL) {
            xpc_dictionary_set_int64(__message, "__xpc_wants_reply__", 1);
            if (replyq == NULL)
                replyq = dispatch_get_main_queue();
            xpc_connection_send_message_with_reply(connection, __message, replyq, handler);
        } else {
            xpc_connection_send_message(connection, __message);
        }
    }
}

void __XPC_MTRRemoteDevice_estimatedStartTime(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSString * responseIdentifierString, NSNumber * nodeID)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDevice_estimatedStartTime);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (responseIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "responseIdentifierString", responseIdentifierString, NULL);
        }
        if (nodeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "nodeID", nodeID, NULL);
        }

        if (handler != NULL) {
            xpc_dictionary_set_int64(__message, "__xpc_wants_reply__", 1);
            if (replyq == NULL)
                replyq = dispatch_get_main_queue();
            xpc_connection_send_message_with_reply(connection, __message, replyq, handler);
        } else {
            xpc_connection_send_message(connection, __message);
        }
    }
}

void __XPC_MTRRemoteDevice_readAttribute(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSString * responseIdentifierString, NSNumber * nodeID, NSNumber * endpointID, NSNumber * clusterID, NSNumber * attributeID, BOOL filterByFabric)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDevice_readAttribute);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (responseIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "responseIdentifierString", responseIdentifierString, NULL);
        }
        if (nodeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "nodeID", nodeID, NULL);
        }
        if (endpointID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "endpointID", endpointID, NULL);
        }
        if (clusterID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "clusterID", clusterID, NULL);
        }
        if (attributeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "attributeID", attributeID, NULL);
        }
        if (filterByFabric != 0) {
            xpc_dictionary_set_bool(__message, "filterByFabric", (BOOL) filterByFabric);
        }

        if (handler != NULL) {
            xpc_dictionary_set_int64(__message, "__xpc_wants_reply__", 1);
            if (replyq == NULL)
                replyq = dispatch_get_main_queue();
            xpc_connection_send_message_with_reply(connection, __message, replyq, handler);
        } else {
            xpc_connection_send_message(connection, __message);
        }
    }
}

void __XPC_MTRRemoteDevice_writeAttribute(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSNumber * nodeID, NSNumber * endpointID, NSNumber * clusterID, NSNumber * attributeID, NSDictionary * value, NSNumber * expectedValueInterval, NSNumber * timedWriteTimeout)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDevice_writeAttribute);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (nodeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "nodeID", nodeID, NULL);
        }
        if (endpointID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "endpointID", endpointID, NULL);
        }
        if (clusterID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "clusterID", clusterID, NULL);
        }
        if (attributeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "attributeID", attributeID, NULL);
        }
        if (value) {
            MTRXPCInsertDictionariesToXPCDictionary(__message, "value", value, NULL);
        }
        if (expectedValueInterval) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "expectedValueInterval", expectedValueInterval, NULL);
        }
        if (timedWriteTimeout) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "timedWriteTimeout", timedWriteTimeout, NULL);
        }

        if (handler != NULL) {
            xpc_dictionary_set_int64(__message, "__xpc_wants_reply__", 1);
            if (replyq == NULL)
                replyq = dispatch_get_main_queue();
            xpc_connection_send_message_with_reply(connection, __message, replyq, handler);
        } else {
            xpc_connection_send_message(connection, __message);
        }
    }
}

void __XPC_MTRRemoteDevice_invokeCommand(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSString * responseIdentifierString, NSNumber * nodeID, NSNumber * endpointID, NSNumber * clusterID, NSNumber * commandID, NSDictionary * commandFields, NSArray * expectedValues, NSNumber * expectedValueInterval, NSNumber * timedInvokeTimeout, NSNumber * serverSideProcessingTimeout)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDevice_invokeCommand);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (responseIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "responseIdentifierString", responseIdentifierString, NULL);
        }
        if (nodeID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "nodeID", nodeID, NULL);
        }
        if (endpointID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "endpointID", endpointID, NULL);
        }
        if (clusterID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "clusterID", clusterID, NULL);
        }
        if (commandID) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "commandID", commandID, NULL);
        }
        if (commandFields) {
            MTRXPCInsertDictionariesToXPCDictionary(__message, "commandFields", commandFields, NULL);
        }
        if (expectedValues) {
            MTRXPCInsertArraysToXPCDictionary(__message, "expectedValues", expectedValues, NULL);
        }
        if (expectedValueInterval) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "expectedValueInterval", expectedValueInterval, NULL);
        }
        if (timedInvokeTimeout) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "timedInvokeTimeout", timedInvokeTimeout, NULL);
        }
        if (serverSideProcessingTimeout) {
            MTRXPCInsertCodableObjectsToXPCDictionary(__message, "serverSideProcessingTimeout", serverSideProcessingTimeout, NULL);
        }

        if (handler != NULL) {
            xpc_dictionary_set_int64(__message, "__xpc_wants_reply__", 1);
            if (replyq == NULL)
                replyq = dispatch_get_main_queue();
            xpc_connection_send_message_with_reply(connection, __message, replyq, handler);
        } else {
            xpc_connection_send_message(connection, __message);
        }
    }
}

#pragma - MTRRemoteDevice and methods

@implementation MTRRemoteDevice

- (NSString *)description
{
    return [NSString
        stringWithFormat:@"<MTRRemoteDevice: %p>[controllerID: %@ nodeID: 0x%016llX]", self, self.deviceController.uniqueIdentifier.UUIDString, self.nodeID.unsignedLongLongValue];
}

- (void)dealloc
{
    if (![self.deviceController isKindOfClass:[MTRRemoteDeviceController class]]) {
        return;
    }
    MTRRemoteDeviceController * remoteController = (MTRRemoteDeviceController *) self.deviceController;

    __XPC_MTRRemoteDevice_registerDelegate(remoteController.xpcConnection, NULL, NULL, self.deviceController.uniqueIdentifier.UUIDString, self.nodeID);
}

- (NSDictionary<NSString *, id> * _Nullable)readAttributeWithEndpointID:(NSNumber *)endpointID
                                                              clusterID:(NSNumber *)clusterID
                                                            attributeID:(NSNumber *)attributeID
                                                                 params:(MTRReadParams * _Nullable)params
{
    if (![self.deviceController isKindOfClass:[MTRRemoteDeviceController class]]) {
        return nil;
    }
    MTRRemoteDeviceController * remoteController = (MTRRemoteDeviceController *) self.deviceController;

    // Create response block identifier and register with controller first
    __block NSDictionary * dataValueDictionaryToReturn = nil;
    NSString * responseIdentifierString = [NSUUID UUID].UUIDString;

    // Use block for single event waiting to avoid semaphore
    dispatch_block_t waitBlock = dispatch_block_create(DISPATCH_BLOCK_INHERIT_QOS_CLASS, ^{ ; });
    [remoteController registerDeviceReadResponseHandler:^(NSDictionary * _Nonnull dataValueDictionary) {
        dataValueDictionaryToReturn = dataValueDictionary;
        waitBlock();
    } identifier:responseIdentifierString];

    // Issue read to remote
    __XPC_MTRRemoteDevice_readAttribute(remoteController.xpcConnection, nil, nil, self.deviceController.uniqueIdentifier.UUIDString, responseIdentifierString, self.nodeID, endpointID, clusterID, attributeID, params.filterByFabric);

    // Wait for response
    // TODO: change from DISPATCH_TIME_FOREVER to something sensible and have make mechanism
    dispatch_block_wait(waitBlock, DISPATCH_TIME_FOREVER);

    return dataValueDictionaryToReturn;
}

- (void)writeAttributeWithEndpointID:(NSNumber *)endpointID
                           clusterID:(NSNumber *)clusterID
                         attributeID:(NSNumber *)attributeID
                               value:(id)value
               expectedValueInterval:(NSNumber *)expectedValueInterval
                   timedWriteTimeout:(NSNumber * _Nullable)timedWriteTimeout
{
    if (![self.deviceController isKindOfClass:[MTRRemoteDeviceController class]]) {
        return;
    }
    MTRRemoteDeviceController * remoteController = (MTRRemoteDeviceController *) self.deviceController;

    __XPC_MTRRemoteDevice_writeAttribute(remoteController.xpcConnection, NULL, NULL, self.deviceController.uniqueIdentifier.UUIDString, self.nodeID, endpointID, clusterID, attributeID, value, expectedValueInterval, timedWriteTimeout);
}

// MTRDevice.mm implementation forwards other invoke methods to the below call properly
- (void)_invokeCommandWithEndpointID:(NSNumber *)endpointID
                           clusterID:(NSNumber *)clusterID
                           commandID:(NSNumber *)commandID
                       commandFields:(id)commandFields
                      expectedValues:(NSArray<NSDictionary<NSString *, id> *> * _Nullable)expectedValues
               expectedValueInterval:(NSNumber * _Nullable)expectedValueInterval
                  timedInvokeTimeout:(NSNumber * _Nullable)timeout
         serverSideProcessingTimeout:(NSNumber * _Nullable)serverSideProcessingTimeout
                               queue:(dispatch_queue_t)queue
                          completion:(MTRDeviceResponseHandler)completion
{
    if (![self.deviceController isKindOfClass:[MTRRemoteDeviceController class]]) {
        return;
    }
    MTRRemoteDeviceController * remoteController = (MTRRemoteDeviceController *) self.deviceController;

    NSString * responseIdentifierString = [NSUUID UUID].UUIDString;
    __XPC_MTRRemoteDevice_invokeCommand(remoteController.xpcConnection, NULL, NULL, self.deviceController.uniqueIdentifier.UUIDString, responseIdentifierString, self.nodeID, endpointID, clusterID, commandID, commandFields, expectedValues, expectedValueInterval, timeout, serverSideProcessingTimeout);
}

// No need to override this because base MTRDevice version does the right thing and calls the above
//- (void)_invokeKnownCommandWithEndpointID:(NSNumber *)endpointID
//                                clusterID:(NSNumber *)clusterID
//                                commandID:(NSNumber *)commandID
//                           commandPayload:(id)commandPayload
//                           expectedValues:(NSArray<NSDictionary<NSString *, id> *> * _Nullable)expectedValues
//                    expectedValueInterval:(NSNumber * _Nullable)expectedValueInterval
//                       timedInvokeTimeout:(NSNumber * _Nullable)timeout
//              serverSideProcessingTimeout:(NSNumber * _Nullable)serverSideProcessingTimeout
//                            responseClass:(Class _Nullable)responseClass
//                                    queue:(dispatch_queue_t)queue
//                               completion:(void (^)(id _Nullable response, NSError * _Nullable error))completion {
//}

- (void)setDelegate:(id<MTRDeviceDelegate>)delegate queue:(dispatch_queue_t)queue
{
    if (![self.deviceController isKindOfClass:[MTRRemoteDeviceController class]]) {
        return;
    }
    MTRRemoteDeviceController * remoteController = (MTRRemoteDeviceController *) self.deviceController;

    [self setDelegate:delegate queue:queue setUpSubscription:NO];

    __XPC_MTRRemoteDevice_registerDelegate(remoteController.xpcConnection, NULL, NULL, self.deviceController.uniqueIdentifier.UUIDString, self.nodeID);
}

- (void)stateChanged:(MTRDeviceState)state
{
    self.state = state;
    id<MTRDeviceDelegate> delegate = self.weakDelegate.strongObject;
    if (delegate) {
        dispatch_async(self.delegateQueue, ^{
            [delegate device:self stateChanged:state];
        });
    }
}

- (void)receivedAttributeReport:(NSArray<NSDictionary<NSString *, id> *> *)attributeReport
{
    id<MTRDeviceDelegate> delegate = self.weakDelegate.strongObject;
    if (delegate) {
        dispatch_async(self.delegateQueue, ^{
            [delegate device:self receivedAttributeReport:attributeReport];
        });
    }
}

- (void)receivedEventReport:(NSArray<NSDictionary<NSString *, id> *> *)eventReport
{
    id<MTRDeviceDelegate> delegate = self.weakDelegate.strongObject;
    if (delegate) {
        dispatch_async(self.delegateQueue, ^{
            [delegate device:self receivedEventReport:eventReport];
        });
    }
}

- (void)deviceBecameActive
{
    id<MTRDeviceDelegate> delegate = self.weakDelegate.strongObject;
    if (delegate) {
        dispatch_async(self.delegateQueue, ^{
            if ([delegate respondsToSelector:@selector(deviceBecameActive:)]) {
                [delegate deviceBecameActive:self];
            }
        });
    }
}

#pragma Unsupported MTRDevice methods

- (void)openCommissioningWindowWithDiscriminator:(NSNumber *)discriminator duration:(NSNumber *)duration queue:(dispatch_queue_t)queue completion:(MTRDeviceOpenCommissioningWindowHandler)completion
{
    MTR_LOG_ERROR("Method %s not supported for remote device case", __func__);
}

- (void)openCommissioningWindowWithSetupPasscode:(NSNumber *)setupPasscode discriminator:(NSNumber *)discriminator duration:(NSNumber *)duration queue:(dispatch_queue_t)queue completion:(MTRDeviceOpenCommissioningWindowHandler)completion
{
    MTR_LOG_ERROR("Method %s not supported for remote device case", __func__);
}

@end
