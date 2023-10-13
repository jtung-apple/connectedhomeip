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

#import "MTRDeviceControllerStartupParams_Internal.h"
#import "MTRDeviceController_Internal.h"
#import "MTRDevice_Internal.h"
#import "MTRError_Internal.h"
#import "MTRLogging_Internal.h"
#import "MTRRemoteDeviceController_Internal.h"
#import "MTRRemoteDeviceXPCUtils.h"
#import "MTRRemoteDevice_Internal.h"

void __XPC_MTRRemoteDeviceController_initWithParameters(xpc_connection_t connection, dispatch_queue_t replyq, xpc_handler_t handler, NSString * controllerIdentifierString, NSString * responseIdentifierString)
{
    if (connection == NULL) {
        return;
    }

    xpc_object_t __message = xpc_dictionary_create(NULL, NULL, 0);

    if (__message != NULL) {
        xpc_dictionary_set_int64(__message, "__xpc__event_code__", _MTRRemoteDeviceController_initWithParameters);
        if (controllerIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "controllerIdentifierString", controllerIdentifierString, NULL);
        }
        if (responseIdentifierString) {
            MTRXPCInsertNSStringsToXPCDictionary(__message, "responseIdentifierString", responseIdentifierString, NULL);
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

@interface MTRRemoteDeviceController () {
    NSNumber * _controllerNodeID;
}
@end

@implementation MTRRemoteDeviceController

- (void)handleXPCConnection:(xpc_connection_t)connection event:(xpc_object_t)event
{
    xpc_type_t __xpctype = xpc_get_type(event);
    if (__xpctype == XPC_TYPE_ERROR) {
        if (event == XPC_ERROR_CONNECTION_INVALID) {
            // the client process on the other end of the connection has either
            // crashed or cancelled the connection
            MTR_LOG_INFO("peer(%d) received XPC_ERROR_CONNECTION_INVALID", xpc_connection_get_pid(_xpcConnection));

            // Recovery??
            xpc_connection_cancel(_xpcConnection);
            _xpcConnection = nil;
        } else if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
            // the client process on the other end of the connection has either
            // crashed or cancelled the connection
            MTR_LOG_INFO("peer(%d) received XPC_ERROR_CONNECTION_INTERRUPTED", xpc_connection_get_pid(_xpcConnection));
        } else if (event == XPC_ERROR_TERMINATION_IMMINENT) {
            // handle per-connection termination cleanup
            MTR_LOG_INFO("peer(%d) received XPC_ERROR_TERMINATION_IMMINENT", xpc_connection_get_pid(_xpcConnection));
        }
    } else if (__xpctype == XPC_TYPE_DICTIONARY) {
        xpc_object_t __message = event;
        char * messageDescription = xpc_copy_description(__message);
        MTR_LOG_INFO("received message from peer(%d): %s", xpc_connection_get_pid(_xpcConnection), messageDescription);
        free(messageDescription);

        NSInteger messageType = xpc_dictionary_get_int64(__message, "__xpc__event_code__");
        switch (messageType) {
        case _MTRRemoteDeviceController_initWithParameters_response: {
            NSString * responseIdentifierString = MTRXPCGetStringFromDictionary(__message, "responseIdentifierString");
            NSNumber * nodeID = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "nodeID");
            MTRDeviceControllerInitResponseHandler handler = _serverResponseHandlers[responseIdentifierString];
            if (handler) {
                handler(nodeID);
            }
            break;
        }
        case _MTRRemoteDevice_estimatedStartTime_response: {
            NSString * responseIdentifierString = MTRXPCGetStringFromDictionary(__message, "responseIdentifierString");
            NSDate * estimatedStartTime = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "estimatedStartTime");
            MTRDeviceEstimatedStartTimeResponseHandler handler = _serverResponseHandlers[responseIdentifierString];
            if (handler) {
                handler(estimatedStartTime);
            }
            break;
        }
        case _MTRRemoteDevice_readAttribute_response: {
            NSString * responseIdentifierString = MTRXPCGetStringFromDictionary(__message, "responseIdentifierString");
            NSDictionary * dataValueDictionary = MTRXPCGetKeyedCodableFromDictionaryWithSecureCoding(__message, "dataValueDictionary", [NSDictionary class]);
            MTRDeviceReadResponseHandler handler = _serverResponseHandlers[responseIdentifierString];
            if (handler) {
                handler(dataValueDictionary);
            }
            break;
        }
        case _MTRRemoteDevice_invokeCommand_response: {
            NSString * responseIdentifierString = MTRXPCGetStringFromDictionary(__message, "responseIdentifierString");
            NSArray * invokeResponse = MTRXPCGetKeyedCodableFromDictionaryWithSecureCoding(__message, "invokeResponse", [NSDictionary class]);
            NSError * error = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "error");
            MTRDeviceInvokeResponseHandler handler = _serverResponseHandlers[responseIdentifierString];
            if (handler) {
                handler(invokeResponse, error);
            }
            break;
        }
        case _MTRRemoteDevice_delegate_stateChanged: {
            NSNumber * nodeID = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "nodeID");
            NSNumber * state = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "state");
            MTRRemoteDevice * device = (MTRRemoteDevice *) [self deviceForNodeID:nodeID];
            [device stateChanged:(MTRDeviceState) state.unsignedIntValue];
            break;
        }
        case _MTRRemoteDevice_delegate_receivedAttributeReport: {
            NSNumber * nodeID = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "nodeID");
            NSArray * attributeReport = MTRXPCGetKeyedCodableFromDictionaryWithSecureCoding(__message, "attributeReport", [NSDictionary class]);
            MTRRemoteDevice * device = (MTRRemoteDevice *) [self deviceForNodeID:nodeID];
            [device receivedAttributeReport:attributeReport];
            break;
        }
        case _MTRRemoteDevice_delegate_receivedEventReport: {
            NSNumber * nodeID = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "nodeID");
            NSArray * eventReport = MTRXPCGetKeyedCodableFromDictionaryWithSecureCoding(__message, "eventReport", [NSDictionary class]);
            MTRRemoteDevice * device = (MTRRemoteDevice *) [self deviceForNodeID:nodeID];
            [device receivedEventReport:eventReport];
            break;
        }
        case _MTRRemoteDevice_delegate_deviceBecameActive: {
            NSNumber * nodeID = MTRXPCGetCodableFromDictionaryWithStandardAllowlist(__message, "nodeID");
            MTRRemoteDevice * device = (MTRRemoteDevice *) [self deviceForNodeID:nodeID];
            [device deviceBecameActive];
            break;
        }
        }
    }
}

- (nullable instancetype)initWithParameters:(MTRRemoteDeviceControllerParameters *)parameters error:(NSError * __autoreleasing *)error
{
    if (!parameters.uniqueIdentifier || !parameters.xpcServiceName) {
        MTR_LOG_DEFAULT("%s invalid parameters identifier %@ service name %@", __func__, parameters.uniqueIdentifier, parameters.xpcServiceName);
        *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeInvalidArgument userInfo:nil];
        return nil;
    }

    if (self = [super initWithUniqueIdentifier:parameters.uniqueIdentifier]) {
        _xpcConnectionQueue = dispatch_queue_create("org.csa-iot.matter.framework.mtrdeviceplugin.workqueue", DISPATCH_QUEUE_SERIAL_WITH_AUTORELEASE_POOL);
        _xpcConnection = xpc_connection_create_mach_service(parameters.xpcServiceName.UTF8String, _xpcConnectionQueue, 0);
        mtr_weakify(_xpcConnection);
        xpc_connection_set_event_handler(_xpcConnection, ^(xpc_object_t event) {
            mtr_strongify(_xpcConnection);
            if (!_xpcConnection) {
                return;
            }
            [self handleXPCConnection:_xpcConnection event:event];
        });

        // TODO: when xpc connection tears down, need to mark this controller defunct

        xpc_connection_activate(_xpcConnection);
        NSString * responseIdentifierString = [NSUUID UUID].UUIDString;
        __XPC_MTRRemoteDeviceController_initWithParameters(
            _xpcConnection, NULL, NULL, parameters.uniqueIdentifier.UUIDString, responseIdentifierString);

        _deviceMapLock = OS_UNFAIR_LOCK_INIT;
        _nodeIDToDeviceMap = [NSMutableDictionary dictionary];
        _serverResponseHandlers = [NSMutableDictionary dictionary];

        // Now wait for init response to return controller node ID
        dispatch_block_t waitBlock = dispatch_block_create(DISPATCH_BLOCK_INHERIT_QOS_CLASS, ^{ ; });
        [self registerDeviceControllerInitResponseHandler:^(NSNumber * _Nonnull controllerNodeID) {
            self->_controllerNodeID = controllerNodeID;
            waitBlock();
        } identifier:responseIdentifierString];
        // TODO: change from DISPATCH_TIME_FOREVER to something sensible and have make mechanism
        dispatch_block_wait(waitBlock, DISPATCH_TIME_FOREVER);

        if (!_controllerNodeID) {
            MTR_LOG_DEFAULT("%s could not retrieve controller node ID", __func__);
            *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
            return nil;
        }
    }
    return self;
}

- (NSNumber *)controllerNodeID
{
    return _controllerNodeID;
}

- (void)_registerResponseHandler:(id)handler identifier:(NSString *)identifier
{
    _serverResponseHandlers[identifier] = handler;
}

- (void)registerDeviceControllerInitResponseHandler:(MTRDeviceControllerInitResponseHandler)handler identifier:(NSString *)identifier
{
    [self _registerResponseHandler:handler identifier:identifier];
}

- (void)registerDeviceStateResponseHandler:(MTRDeviceStateResponseHandler)handler identifier:(NSString *)identifier
{
    [self _registerResponseHandler:handler identifier:identifier];
}

- (void)registerDeviceEstimatedStartTimeResponseHandler:(MTRDeviceEstimatedStartTimeResponseHandler)handler identifier:(NSString *)identifier
{
    [self _registerResponseHandler:handler identifier:identifier];
}

- (void)registerDeviceReadResponseHandler:(MTRDeviceReadResponseHandler)handler identifier:(NSString *)identifier
{
    [self _registerResponseHandler:handler identifier:identifier];
}

- (void)registerDeviceInvokeResponseHandler:(MTRDeviceInvokeResponseHandler)handler identifier:(NSString *)identifier
{
    [self _registerResponseHandler:handler identifier:identifier];
}

- (void)dealloc
{
    xpc_connection_cancel(_xpcConnection);
}

- (MTRDevice *)deviceForNodeID:(NSNumber *)nodeID
{
    os_unfair_lock_lock(&_deviceMapLock);
    MTRDevice * deviceToReturn = _nodeIDToDeviceMap[nodeID];
    if (!deviceToReturn) {
        deviceToReturn = [[MTRRemoteDevice alloc] initWithNodeID:nodeID controller:self];
        _nodeIDToDeviceMap[nodeID] = deviceToReturn;
    }
    os_unfair_lock_unlock(&_deviceMapLock);

    return deviceToReturn;
}

- (BOOL)isRunning
{
    // Assume this is only used when a controller is running
    return YES;
}

#pragma Unsupported DeviceController methods

- (BOOL)setupCommissioningSessionWithPayload:(MTRSetupPayload *)payload newNodeID:(NSNumber *)newNodeID error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)setupCommissioningSessionWithDiscoveredDevice:(MTRCommissionableBrowserResult *)discoveredDevice payload:(MTRSetupPayload *)payload newNodeID:(NSNumber *)newNodeID error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)commissionNodeWithID:(NSNumber *)nodeID commissioningParams:(MTRCommissioningParameters *)commissioningParams error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)continueCommissioningDevice:(void *)opaqueDeviceHandle ignoreAttestationFailure:(BOOL)ignoreAttestationFailure error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)cancelCommissioningForNodeID:(NSNumber *)nodeID error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (MTRBaseDevice *)deviceBeingCommissionedWithNodeID:(NSNumber *)nodeID error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return nil;
}

- (void)preWarmCommissioningSession
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
}

- (void)setDeviceControllerDelegate:(id<MTRDeviceControllerDelegate>)delegate queue:(dispatch_queue_t)queue
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
}

- (BOOL)startBrowseForCommissionables:(id<MTRCommissionableBrowserDelegate>)delegate queue:(dispatch_queue_t)queue
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    return NO;
}

- (BOOL)stopBrowseForCommissionables
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    return NO;
}

- (NSData *)attestationChallengeForDeviceID:(NSNumber *)deviceID
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    return nil;
}

+ (NSData *)computePASEVerifierForSetupPasscode:(NSNumber *)setupPasscode iterations:(NSNumber *)iterations salt:(NSData *)salt error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return nil;
}

- (void)shutdown
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
}

#pragma Unsupported and deprecated DeviceController methods

- (BOOL)getBaseDevice:(uint64_t)deviceID queue:(dispatch_queue_t)queue completionHandler:(MTRDeviceConnectionCallback)completionHandler
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    return NO;
}

- (BOOL)pairDevice:(uint64_t)deviceID discriminator:(uint16_t)discriminator setupPINCode:(uint32_t)setupPINCode error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)pairDevice:(uint64_t)deviceID address:(NSString *)address port:(uint16_t)port setupPINCode:(uint32_t)setupPINCode error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)pairDevice:(uint64_t)deviceID onboardingPayload:(NSString *)onboardingPayload error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)commissionDevice:(uint64_t)deviceId commissioningParams:(MTRCommissioningParameters *)commissioningParams error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (BOOL)stopDevicePairing:(uint64_t)deviceID error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (MTRBaseDevice *)getDeviceBeingCommissioned:(uint64_t)deviceId error:(NSError * __autoreleasing _Nullable *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return nil;
}

- (BOOL)openPairingWindow:(uint64_t)deviceID
                 duration:(NSUInteger)duration
                    error:(NSError * __autoreleasing *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return NO;
}

- (nullable NSString *)openPairingWindowWithPIN:(uint64_t)deviceID
                                       duration:(NSUInteger)duration
                                  discriminator:(NSUInteger)discriminator
                                       setupPIN:(NSUInteger)setupPIN
                                          error:(NSError * __autoreleasing *)error
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    *error = [NSError errorWithDomain:MTRErrorDomain code:MTRErrorCodeGeneralError userInfo:nil];
    return nil;
}

- (NSData *)computePaseVerifier:(uint32_t)setupPincode iterations:(uint32_t)iterations salt:(NSData *)salt
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
    return nil;
}

- (void)setPairingDelegate:(id<MTRDevicePairingDelegate>)delegate queue:(dispatch_queue_t)queue
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
}

- (void)setNocChainIssuer:(id<MTRNOCChainIssuer>)nocChainIssuer queue:(dispatch_queue_t)queue
{
    MTR_LOG_ERROR("Method %s not supported for remote controller case", __func__);
}

@end
