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

#import "MTRBaseDevice.h"
#import "MTRLogging_Internal.h"
#import <Foundation/Foundation.h>

#define MTRXPCWarn(x, ...) MTR_LOG_INFO(x, ##__VA_ARGS__)

#define USE_SHARED_MEMORY 0

inline NSData * MTRXPCEncodeDictionary(NSDictionary * dictionary)
{
    if (!dictionary || [dictionary count] == 0)
        return nil;

#if !(TARGET_OS_IPHONE)
    if (![NSPropertyListSerialization propertyList:dictionary
                                  isValidForFormat:NSPropertyListBinaryFormat_v1_0])
        return nil;
#endif

    NSError * error = nil;
    NSData * result = [NSPropertyListSerialization dataWithPropertyList:dictionary
                                                                 format:NSPropertyListBinaryFormat_v1_0
                                                                options:0
                                                                  error:&error];
    if (!result && error) {
        MTRXPCWarn("Error while encoding dictionary %@", error);
        return nil;
    }

    return result;
}

inline char * _MTRXPCStringToCharPtr(CFStringRef S, BOOL * needsFree)
{ // UTF8String
    *needsFree = NO;
    if (!S)
        return NULL;

    CFStringEncoding encoding = kCFStringEncodingUTF8;
    char * buffer = (char *) CFStringGetCStringPtr(S, encoding);
    if (buffer) {
        return buffer;
    }

    CFIndex size = 0;
    CFStringGetBytes((S), CFRangeMake(0, CFStringGetLength(S)), encoding, 0, false, NULL, 0, &size);
    buffer = (char *) malloc((size_t) size * sizeof(char) + 1);
    CFStringGetBytes((S), CFRangeMake(0, CFStringGetLength(S)), encoding, 0, false, (UInt8 *) buffer, size, NULL);
    buffer[size] = 0;

    *needsFree = YES;
    return buffer;
}

inline NSData * _MTRXPCEncodeKeyedCodableObject(id object)
{
    if (object) {
        NSData * data = nil;
        NSKeyedArchiver * archiver = [[NSKeyedArchiver alloc] initRequiringSecureCoding:NO];
        [archiver setRequiresSecureCoding:YES];
        [archiver encodeObject:object forKey:NSKeyedArchiveRootObjectKey];
        data = [archiver encodedData];
        return data;
    }

    return nil;
}

NSData * MTRXPCEncodeKeyedCodableObject(id object)
{
    return _MTRXPCEncodeKeyedCodableObject(object);
}

inline NSData * _MTRXPCEncodeArray(NSArray * array)
{
    if (!array || [array count] == 0)
        return nil;

#if !(TARGET_OS_IPHONE)
    if (![NSPropertyListSerialization propertyList:array
                                  isValidForFormat:NSPropertyListBinaryFormat_v1_0])
        return nil;
#endif

    NSError * error = nil;
    NSData * result = [NSPropertyListSerialization dataWithPropertyList:array
                                                                 format:NSPropertyListBinaryFormat_v1_0
                                                                options:0
                                                                  error:&error];
    if (!result && error) {
        MTRXPCWarn("Error while encoding array %@", error);
        return nil;
    }

    return result;
}

NSData * MTRXPCEncodeArray(NSArray * array)
{
    return _MTRXPCEncodeArray(array);
}

#pragma - MTRXPC stuff

void _MTRXPCInsertDataAsSharedMemoryIntoObject(const void * bytes, size_t length, const char * key, xpc_object_t xpc_dictionary, BOOL isString)
{
    if (!key || length == 0 || !xpc_dictionary || bytes == NULL)
        return;

#if !USE_SHARED_MEMORY
    xpc_dictionary_set_data(xpc_dictionary, key, bytes, length);
#else
    if (length <= kMinSizeToUseSharedMemory) {
        xpc_dictionary_set_data(xpc_dictionary, key, bytes, length);
        return;
    }

    const char * shmemSuffix = ".shmem.length";
    char shmemKey[MAX_SHMEMKEY_SIZE + 1];

    if (shmemKey != NULL) {
        strlcpy(shmemKey, key, sizeof(shmemKey));
        strlcat(shmemKey, shmemSuffix, sizeof(shmemKey));

        void * mem = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
        if (mem) {
            memcpy(mem, bytes, length);
            int mprotectOpts = PROT_READ;
            mprotect(mem, length, mprotectOpts);
            xpc_object_t shmem = xpc_shmem_create(mem, length);
            xpc_dictionary_set_value(xpc_dictionary, key, shmem);
            xpc_dictionary_set_int64(xpc_dictionary, shmemKey, length);
            xpc_release(shmem);
            if (mem) {
                munmap(mem, length);
            }
        } else {
            xpc_dictionary_set_data(xpc_dictionary, key, bytes, length);
        }
    }
#endif
}

void _MTRXPCInsertNSDataAsSharedMemoryIntoObject(NSData * data, const char * key, xpc_object_t xpc_dictionary)
{
    return _MTRXPCInsertDataAsSharedMemoryIntoObject([data bytes], [data length], key, xpc_dictionary, NO);
}

BOOL MTRXPCInsertNSStringsToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            NSString * string = va_arg(args, NSString *);
            if ([string length] > 0) {
                BOOL needsFree = NO;
                char * char_ptr = _MTRXPCStringToCharPtr((__bridge CFStringRef) string, &needsFree);
                if (char_ptr) {
                    xpc_dictionary_set_string(dictionary, key, char_ptr);
                }

                if (char_ptr && needsFree) {
                    free(char_ptr);
                }
            }
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertStringsToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            const char * string = va_arg(args, const char *);
            if (string)
                xpc_dictionary_set_string(dictionary, key, string);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertDictionariesToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            NSDictionary * result = va_arg(args, NSDictionary *);
            NSData * dataValue = MTRXPCEncodeDictionary(result);
            if (!dataValue) {
                if ([result count] > 0)
                    MTRXPCWarn("Failed to encode dictionary: %@", result);
                dataValue = [NSData data];
            }

            _MTRXPCInsertNSDataAsSharedMemoryIntoObject(dataValue, key, dictionary);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertOptionallyCodableDictionariesToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    BOOL success = YES;
    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            NSDictionary * result = va_arg(args, NSDictionary *);
            NSData * dataValue = MTRXPCEncodeDictionary(result);
            if ([dataValue length] == 0)
                dataValue = MTRXPCEncodeKeyedCodableObject(result);
            if ([dataValue length] == 0) {
                MTRXPCWarn("Failed to encode codable: %@", result);
                dataValue = [NSData data];
                success = NO;
            }

            _MTRXPCInsertNSDataAsSharedMemoryIntoObject(dataValue, key, dictionary);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return success;
}

BOOL MTRXPCInsertArraysToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            NSArray * array = va_arg(args, NSArray *);
            NSData * dataValue = MTRXPCEncodeArray(array);
            if (!dataValue) {
                MTRXPCWarn("Failed to encode array: %@", array);
                dataValue = [NSData data];
            }
            _MTRXPCInsertNSDataAsSharedMemoryIntoObject(dataValue, key, dictionary);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertCodableObjectsToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            id object = va_arg(args, id);
            NSData * dataValue;

            dataValue = MTRXPCEncodeKeyedCodableObject(object);
            if (!dataValue)
                dataValue = [NSData data];

            _MTRXPCInsertNSDataAsSharedMemoryIntoObject(dataValue, key, dictionary);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertKeyedCodableObjectsToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            id object = va_arg(args, id);
            NSData * dataValue;

            dataValue = MTRXPCEncodeKeyedCodableObject(object);
            if (!dataValue)
                dataValue = [NSData data];

            _MTRXPCInsertNSDataAsSharedMemoryIntoObject(dataValue, key, dictionary);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertDatasToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            NSData * dataValue = va_arg(args, NSData *);
            if (!dataValue)
                dataValue = [NSData data];

            _MTRXPCInsertNSDataAsSharedMemoryIntoObject(dataValue, key, dictionary);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertIntsToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            int integer = va_arg(args, int);
            xpc_dictionary_set_int64(dictionary, key, integer);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

BOOL MTRXPCInsertBoolsToXPCDictionary(xpc_object_t dictionary, const char * key, ...)
{
    if (!dictionary || !key)
        return NO;

    va_list args;
    va_start(args, key);
    if (key != NULL) {
        do {
            BOOL booleanValue = va_arg(args, int) ? YES : NO;
            xpc_dictionary_set_bool(dictionary, key, booleanValue);
        } while ((key = va_arg(args, const char *)) != NULL);
    }
    va_end(args);

    return YES;
}

static NSData * _MTRCopySharedMemoryDataFromDictionary(const char * key, xpc_object_t xpc_dictionary, BOOL copy)
{
    if (!key || !xpc_dictionary)
        return nil;

    if (xpc_get_type(xpc_dictionary) != XPC_TYPE_DICTIONARY)
        return nil;

#if !USE_SHARED_MEMORY
    size_t length = 0;
    const void * dataValue = xpc_dictionary_get_data(xpc_dictionary, key, &length);

    if (dataValue) {
        if (copy)
            return [[NSData alloc] initWithBytes:(void *) dataValue length:length];
        else
            return [[NSData alloc] initWithBytesNoCopy:(void *) dataValue length:length freeWhenDone:NO];
    }

    return nil;
#else
    const char * shmemSuffix = ".shmem.length";
    char shmemKey[MAX_SHMEMKEY_SIZE + 1];

    if (shmemKey != NULL) {
        strlcpy(shmemKey, key, sizeof(shmemKey));
        strlcat(shmemKey, shmemSuffix, sizeof(shmemKey));
    }

    int64_t dataLength = shmemKey ? xpc_dictionary_get_int64(xpc_dictionary, shmemKey) : 0;

    if (dataLength == 0) {
        size_t length = 0;
        const void * dataValue = xpc_dictionary_get_data(xpc_dictionary, key, &length);

        if (dataValue && length > 0) {
            if (copy)
                return [[NSData alloc] initWithBytes:(void *) dataValue length:length];
            else
                return [[NSData alloc] initWithBytesNoCopy:(void *) dataValue length:length freeWhenDone:NO];
        }

        return nil;
    }

    const void * mappedRegion = NULL;
    xpc_object_t sharedMemoryObject = xpc_dictionary_get_value(xpc_dictionary, key);
    size_t mappedLength = 0;

    if (!sharedMemoryObject || xpc_get_type(sharedMemoryObject) != XPC_TYPE_SHMEM) {
        IDSWarn("Cannot get shared data buffer.");
        return nil;
    }

    mappedLength = xpc_shmem_map(sharedMemoryObject, (void **) &mappedRegion);
    if (mappedLength == 0) {
        IDSWarn("Cannot map shared data buffer in.");
        return nil;
    }

    NSData * data = nil;
    if (dataLength <= mappedLength) {
        if (copy)
            data = [[NSData alloc] initWithBytes:(void *) mappedRegion length:dataLength];
        else
            data = [[NSData alloc] initWithBytesNoCopy:(void *) mappedRegion length:dataLength freeWhenDone:NO];
    }

    if (mappedRegion)
        munmap((void *) mappedRegion, mappedLength);

    return data;
#endif
}

inline id _MTRDecodeKeyedCodableObjectWithSecureCoding(NSData * data, Class requestedClass, NSArray<Class> * allowlistedClasses)
{
    if (!data || [data length] == 0)
        return nil;

    id object = nil;
    NSKeyedUnarchiver * unarchiver = nil;

    static dispatch_once_t onceToken;
    static NSSet * sPlistClasses;
    static NSSet * sCollectionClasses;
    static NSSet * sMatterClasses;
    dispatch_once(&onceToken, ^{
        sPlistClasses = [NSSet setWithObjects:[NSDictionary class], [NSArray class], [NSString class], [NSAttributedString class], [NSNumber class], [NSData class], [NSDate class], [NSCalendar class], [NSNull class], [NSValue class], [NSError class], [NSURL class], [NSURLRequest class], [NSURLResponse class], nil];
        sCollectionClasses = [NSSet setWithObjects:[NSDictionary class], [NSSet class], [NSArray class], [NSOrderedSet class], nil];
        sMatterClasses = [NSSet setWithObjects:[MTRAttributePath class], [MTREventPath class], [MTRCommandPath class], nil];
    });

    @try {
        unarchiver = [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:nil];
        [unarchiver setDecodingFailurePolicy:NSDecodingFailurePolicyRaiseException];
        NSMutableSet * allowedClasses = [[NSMutableSet alloc] init];
        if (allowlistedClasses) {
            [allowedClasses addObjectsFromArray:allowlistedClasses];
        }
        if (requestedClass) {
            // Collections can include objects of the types held in collectionClasses. We need to allowlist these classes when we're deserializing a collection.
            if ([sCollectionClasses containsObject:requestedClass]) {
                // Allow dictionaries to contain other collections
                if ([requestedClass isEqual:[NSDictionary class]]) {
                    [allowedClasses unionSet:sCollectionClasses];
                }
                [allowedClasses unionSet:sPlistClasses];
                [allowedClasses unionSet:sMatterClasses];
            }
            [allowedClasses addObject:requestedClass];
        } else {
            [allowedClasses unionSet:sPlistClasses];
        }

        object = [unarchiver decodeObjectOfClasses:allowedClasses forKey:NSKeyedArchiveRootObjectKey];
    }
    @catch (NSException * exception) {
        MTRXPCWarn("IMRemoteObjectXPC Error - Exception caught unarchiving data: %@ exception %@", data, exception);
    }
    return object;
}

id MTRDecodeKeyedCodableObjectWithSecureCoding(NSData * data, Class requestedClass)
{
    return _MTRDecodeKeyedCodableObjectWithSecureCoding(data, requestedClass, nil);
}

inline NSDictionary * _MTRDecodeDictionary(NSData * data)
{
    if (!data || [data length] == 0)
        return nil;

    NSPropertyListFormat propertyListFormat;
    NSError * error = nil;
    NSDictionary * dictionary = [NSPropertyListSerialization propertyListWithData:data
                                                                          options:NSPropertyListImmutable
                                                                           format:&propertyListFormat
                                                                            error:&error];
    if (error)
        return nil;

    if (![dictionary isKindOfClass:[NSDictionary class]]) {
        return nil;
    }

    return dictionary;
}

NSDictionary * MTRDecodeDictionary(NSData * data)
{
    return _MTRDecodeDictionary(data);
}

inline NSArray * _MTRDecodeArray(NSData * data)
{
    if (!data || [data length] == 0)
        return nil;

    NSPropertyListFormat propertyListFormat;
    NSError * error = nil;
    NSArray * array = [NSPropertyListSerialization propertyListWithData:data
                                                                options:NSPropertyListImmutable
                                                                 format:&propertyListFormat
                                                                  error:&error];
    if (error)
        return nil;

    if (![array isKindOfClass:[NSArray class]]) {
        return nil;
    }

    return array;
}

NSArray * MTRDecodeArray(NSData * data)
{
    return _MTRDecodeArray(data);
}

id MTRXPCGetKeyedCodableFromDictionaryWithSecureCoding(xpc_object_t dictionary, const char * keyname, Class requestedClass)
{
    if (!keyname || !dictionary)
        return nil;

    NSData * data = _MTRCopySharedMemoryDataFromDictionary(keyname, dictionary, NO);
    id object = MTRDecodeKeyedCodableObjectWithSecureCoding(data, requestedClass);

    return object;
}

id MTRXPCGetCodableFromDictionaryWithStandardAllowlist(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return nil;

    NSData * data = _MTRCopySharedMemoryDataFromDictionary(keyname, dictionary, NO);
    id object = MTRDecodeKeyedCodableObjectWithSecureCoding(data, nil);

    return object;
}

NSString * MTRXPCGetStringFromDictionary(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return nil;

    xpc_object_t object = xpc_dictionary_get_value(dictionary, keyname);
    if (object && xpc_get_type(object) == XPC_TYPE_STRING) {
        const char * ptr = xpc_string_get_string_ptr(object);

        if (ptr) {
            return [NSString stringWithUTF8String:ptr];
        }
    }

    return nil;
}

NSDictionary * MTRXPCGetDictionaryFromDictionary(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return nil;

    NSData * data = _MTRCopySharedMemoryDataFromDictionary(keyname, dictionary, NO);
    NSDictionary * result = MTRDecodeDictionary(data);

    return result;
}

NSArray * MTRXPCGetArrayFromDictionary(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return nil;

    NSData * data = _MTRCopySharedMemoryDataFromDictionary(keyname, dictionary, NO);
    NSArray * array = MTRDecodeArray(data);

    return array;
}
NSData * MTRXPCGetDataFromDictionary(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return nil;

    return _MTRCopySharedMemoryDataFromDictionary(keyname, dictionary, YES);
}

int64_t MTRXPCGetIntFromDictionary(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return 0;

    return xpc_dictionary_get_int64(dictionary, keyname);
}

BOOL MTRXPCGetBoolFromDictionary(xpc_object_t dictionary, const char * keyname)
{
    if (!keyname || !dictionary)
        return NO;

    return xpc_dictionary_get_bool(dictionary, keyname);
}
