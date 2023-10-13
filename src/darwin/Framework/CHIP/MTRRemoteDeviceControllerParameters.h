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

#import <Matter/MTRDefines.h>
#import <Matter/MTRDeviceControllerParameters.h>

#if defined(MTR_INTERNAL_INCLUDE) && defined(MTR_INCLUDED_FROM_UMBRELLA_HEADER)
#error Internal includes should not happen from the umbrella header
#endif

@interface MTRRemoteDeviceControllerParameters : MTRDeviceControllerAbstractParameters
- (instancetype)initWithXPCServiceName:(NSString *)xpcServiceName uniqueIdentifier:(NSUUID *)uniqueIdentifier;
@property (nonatomic, strong, readonly) NSUUID * uniqueIdentifier;
@end
