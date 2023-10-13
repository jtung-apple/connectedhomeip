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

#import <Foundation/Foundation.h>

MTR_HIDDEN
BOOL MTRXPCInsertNSStringsToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertStringsToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertDictionariesToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertOptionallyCodableDictionariesToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertArraysToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertCodableObjectsToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertKeyedCodableObjectsToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertDatasToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertIntsToXPCDictionary(xpc_object_t dictionary, const char * key, ...);
MTR_HIDDEN
BOOL MTRXPCInsertBoolsToXPCDictionary(xpc_object_t dictionary, const char * key, ...);

MTR_HIDDEN
id MTRXPCGetKeyedCodableFromDictionaryWithSecureCoding(xpc_object_t dictionary, const char * keyname, Class requestedClass);
MTR_HIDDEN
id MTRXPCGetCodableFromDictionaryWithStandardAllowlist(xpc_object_t dictionary, const char * keyname);
MTR_HIDDEN
NSString * MTRXPCGetStringFromDictionary(xpc_object_t dictionary, const char * keyname);
MTR_HIDDEN
NSDictionary * MTRXPCGetDictionaryFromDictionary(xpc_object_t dictionary, const char * keyname);
MTR_HIDDEN
NSArray * MTRXPCGetArrayFromDictionary(xpc_object_t dictionary, const char * keyname);
MTR_HIDDEN
NSData * MTRXPCGetDataFromDictionary(xpc_object_t dictionary, const char * keyname);
MTR_HIDDEN
int64_t MTRXPCGetIntFromDictionary(xpc_object_t dictionary, const char * keyname);
MTR_HIDDEN
BOOL MTRXPCGetBoolFromDictionary(xpc_object_t dictionary, const char * keyname);
