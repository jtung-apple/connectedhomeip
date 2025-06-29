/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include "DeviceAttestationSe05xCredsExample.h"

#include <CHIPCryptoPAL_se05x.h>
#include <credentials/examples/ExampleDACs.h>
#include <credentials/examples/ExamplePAI.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/core/CHIPError.h>
#include <lib/core/TLV.h>
#include <lib/core/TLVTags.h>
#include <lib/core/TLVTypes.h>
#include <lib/core/TLVUtilities.h>
#include <lib/support/Span.h>

/* Device attestation key ids */
#define DEV_ATTESTATION_KEY_SE05X_ID 0x7D300000
#define DEV_ATTESTATION_CERT_SE05X_ID 0x7D300001
#define CERT_DECLARATION_DATA_SE05X_ID 0x7D300002

/* Device attestation key ids (Used with internal sign) */
#define DEV_ATTESTATION_KEY_SE05X_ID_IS 0x7D300003
#define DEV_ATTESTATION_KEY_SE05X_ID_IS_TBS 0x7D300004

#define TAG1_ID 0x7D300005
#define TAG1_LEN_ID 0x7D300006
#define TAG1_VALUE_ID 0x7D300007
#define TAG2_ID 0x7D300008
#define TAG2_LEN_ID 0x7D300009
#define TAG2_VALUE_ID 0x7D30000A
#define TAG3_ID 0x7D30000B
#define TAG3_LEN_ID 0x7D30000C
#define TAG3_VALUE_ID 0x7D30000D
#define ATTEST_CHALLENGE_ID 0x7D30000E

#define START_CONTAINER_SE05X_ID 0x7D30000F
#define END_CONTAINER_SE05X_ID 0x7D300010

#define NO_OF_DEV_ATTEST_MSG_TAGS_TO_PARSE 3 /* Starting with TAG1 */

extern CHIP_ERROR se05x_get_certificate(uint32_t keyId, uint8_t * buf, size_t * buflen);
extern CHIP_ERROR se05x_set_certificate(uint32_t keyId, const uint8_t * buf, size_t buflen);
extern CHIP_ERROR se05x_perform_internal_sign(uint32_t keyId, uint8_t * sigBuf, size_t * sigBufLen);
extern void se05x_delete_key(uint32_t keyid);

namespace chip {
namespace Credentials {
namespace Examples {

namespace {

// Note: The example DAC provider class is only for demonstration purpose.
// Modification to SignWithDeviceAttestationKey member will be required in case there is a change in device attestation sign
// message (other TLVs are added). In the current implementation of SignWithDeviceAttestationKey only TLV1, TLV2 and TLV3 are
// expected in the sign message.

class ExampleSe05xDACProviderv2 : public DeviceAttestationCredentialsProvider
{
public:
    CHIP_ERROR GetCertificationDeclaration(MutableByteSpan & out_cd_buffer) override;
    CHIP_ERROR GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer) override;
    CHIP_ERROR GetDeviceAttestationCert(MutableByteSpan & out_dac_buffer) override;
    CHIP_ERROR GetProductAttestationIntermediateCert(MutableByteSpan & out_pai_buffer) override;
    CHIP_ERROR SignWithDeviceAttestationKey(const ByteSpan & message_to_sign, MutableByteSpan & out_signature_buffer) override;
};

CHIP_ERROR ExampleSe05xDACProviderv2::GetDeviceAttestationCert(MutableByteSpan & out_dac_buffer)
{
#if 0
    return CopySpanToMutableSpan(DevelopmentCerts::kDacCert, out_dac_buffer);
#else
    size_t buflen = out_dac_buffer.size();
    ChipLogDetail(Crypto, "Get DA certificate from se05x");
    ReturnErrorOnFailure(se05x_get_certificate(DEV_ATTESTATION_CERT_SE05X_ID, out_dac_buffer.data(), &buflen));
    out_dac_buffer.reduce_size(buflen);
    return CHIP_NO_ERROR;
#endif
}

CHIP_ERROR ExampleSe05xDACProviderv2::GetProductAttestationIntermediateCert(MutableByteSpan & out_pai_buffer)
{
    return CopySpanToMutableSpan(ByteSpan(DevelopmentCerts::kPaiCert), out_pai_buffer);
}

CHIP_ERROR ExampleSe05xDACProviderv2::GetCertificationDeclaration(MutableByteSpan & out_cd_buffer)
{
    //-> format_version = 1
    //-> vendor_id = 0xFFF1
    //-> product_id_array = [ 0x8000, 0x8001, 0x8002, 0x8003, 0x8004, 0x8005, 0x8006, 0x8007, 0x8008, 0x8009, 0x800A, 0x800B,
    // 0x800C, 0x800D, 0x800E, 0x800F, 0x8010, 0x8011, 0x8012, 0x8013, 0x8014, 0x8015, 0x8016, 0x8017, 0x8018, 0x8019, 0x801A,
    // 0x801B, 0x801C, 0x801D, 0x801E, 0x801F, 0x8020, 0x8021, 0x8022, 0x8023, 0x8024, 0x8025, 0x8026, 0x8027, 0x8028, 0x8029,
    // 0x802A, 0x802B, 0x802C, 0x802D, 0x802E, 0x802F, 0x8030, 0x8031, 0x8032, 0x8033, 0x8034, 0x8035, 0x8036, 0x8037, 0x8038,
    // 0x8039, 0x803A, 0x803B, 0x803C, 0x803D, 0x803E, 0x803F, 0x8040, 0x8041, 0x8042, 0x8043, 0x8044, 0x8045, 0x8046, 0x8047,
    // 0x8048, 0x8049, 0x804A, 0x804B, 0x804C, 0x804D, 0x804E, 0x804F, 0x8050, 0x8051, 0x8052, 0x8053, 0x8054, 0x8055, 0x8056,
    // 0x8057, 0x8058, 0x8059, 0x805A, 0x805B, 0x805C, 0x805D, 0x805E, 0x805F, 0x8060, 0x8061, 0x8062, 0x8063 ]
    //-> device_type_id = 0x0016
    //-> certificate_id = "ZIG20142ZB330003-24"
    //-> security_level = 0
    //-> security_information = 0
    //-> version_number = 0x2694
    //-> certification_type = 0
    //-> dac_origin_vendor_id is not present
    //-> dac_origin_product_id is not present
#if 0
    const uint8_t kCdForAllExamples[] = {
        0x30, 0x82, 0x02, 0x19, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x02, 0xa0, 0x82, 0x02, 0x0a, 0x30,
        0x82, 0x02, 0x06, 0x02, 0x01, 0x03, 0x31, 0x0d, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02,
        0x01, 0x30, 0x82, 0x01, 0x71, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x07, 0x01, 0xa0, 0x82, 0x01, 0x62,
        0x04, 0x82, 0x01, 0x5e, 0x15, 0x24, 0x00, 0x01, 0x25, 0x01, 0xf1, 0xff, 0x36, 0x02, 0x05, 0x00, 0x80, 0x05, 0x01, 0x80,
        0x05, 0x02, 0x80, 0x05, 0x03, 0x80, 0x05, 0x04, 0x80, 0x05, 0x05, 0x80, 0x05, 0x06, 0x80, 0x05, 0x07, 0x80, 0x05, 0x08,
        0x80, 0x05, 0x09, 0x80, 0x05, 0x0a, 0x80, 0x05, 0x0b, 0x80, 0x05, 0x0c, 0x80, 0x05, 0x0d, 0x80, 0x05, 0x0e, 0x80, 0x05,
        0x0f, 0x80, 0x05, 0x10, 0x80, 0x05, 0x11, 0x80, 0x05, 0x12, 0x80, 0x05, 0x13, 0x80, 0x05, 0x14, 0x80, 0x05, 0x15, 0x80,
        0x05, 0x16, 0x80, 0x05, 0x17, 0x80, 0x05, 0x18, 0x80, 0x05, 0x19, 0x80, 0x05, 0x1a, 0x80, 0x05, 0x1b, 0x80, 0x05, 0x1c,
        0x80, 0x05, 0x1d, 0x80, 0x05, 0x1e, 0x80, 0x05, 0x1f, 0x80, 0x05, 0x20, 0x80, 0x05, 0x21, 0x80, 0x05, 0x22, 0x80, 0x05,
        0x23, 0x80, 0x05, 0x24, 0x80, 0x05, 0x25, 0x80, 0x05, 0x26, 0x80, 0x05, 0x27, 0x80, 0x05, 0x28, 0x80, 0x05, 0x29, 0x80,
        0x05, 0x2a, 0x80, 0x05, 0x2b, 0x80, 0x05, 0x2c, 0x80, 0x05, 0x2d, 0x80, 0x05, 0x2e, 0x80, 0x05, 0x2f, 0x80, 0x05, 0x30,
        0x80, 0x05, 0x31, 0x80, 0x05, 0x32, 0x80, 0x05, 0x33, 0x80, 0x05, 0x34, 0x80, 0x05, 0x35, 0x80, 0x05, 0x36, 0x80, 0x05,
        0x37, 0x80, 0x05, 0x38, 0x80, 0x05, 0x39, 0x80, 0x05, 0x3a, 0x80, 0x05, 0x3b, 0x80, 0x05, 0x3c, 0x80, 0x05, 0x3d, 0x80,
        0x05, 0x3e, 0x80, 0x05, 0x3f, 0x80, 0x05, 0x40, 0x80, 0x05, 0x41, 0x80, 0x05, 0x42, 0x80, 0x05, 0x43, 0x80, 0x05, 0x44,
        0x80, 0x05, 0x45, 0x80, 0x05, 0x46, 0x80, 0x05, 0x47, 0x80, 0x05, 0x48, 0x80, 0x05, 0x49, 0x80, 0x05, 0x4a, 0x80, 0x05,
        0x4b, 0x80, 0x05, 0x4c, 0x80, 0x05, 0x4d, 0x80, 0x05, 0x4e, 0x80, 0x05, 0x4f, 0x80, 0x05, 0x50, 0x80, 0x05, 0x51, 0x80,
        0x05, 0x52, 0x80, 0x05, 0x53, 0x80, 0x05, 0x54, 0x80, 0x05, 0x55, 0x80, 0x05, 0x56, 0x80, 0x05, 0x57, 0x80, 0x05, 0x58,
        0x80, 0x05, 0x59, 0x80, 0x05, 0x5a, 0x80, 0x05, 0x5b, 0x80, 0x05, 0x5c, 0x80, 0x05, 0x5d, 0x80, 0x05, 0x5e, 0x80, 0x05,
        0x5f, 0x80, 0x05, 0x60, 0x80, 0x05, 0x61, 0x80, 0x05, 0x62, 0x80, 0x05, 0x63, 0x80, 0x18, 0x24, 0x03, 0x16, 0x2c, 0x04,
        0x13, 0x5a, 0x49, 0x47, 0x32, 0x30, 0x31, 0x34, 0x32, 0x5a, 0x42, 0x33, 0x33, 0x30, 0x30, 0x30, 0x33, 0x2d, 0x32, 0x34,
        0x24, 0x05, 0x00, 0x24, 0x06, 0x00, 0x25, 0x07, 0x94, 0x26, 0x24, 0x08, 0x00, 0x18, 0x31, 0x7d, 0x30, 0x7b, 0x02, 0x01,
        0x03, 0x80, 0x14, 0x62, 0xfa, 0x82, 0x33, 0x59, 0xac, 0xfa, 0xa9, 0x96, 0x3e, 0x1c, 0xfa, 0x14, 0x0a, 0xdd, 0xf5, 0x04,
        0xf3, 0x71, 0x60, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x30, 0x0a, 0x06, 0x08,
        0x2a, 0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x04, 0x47, 0x30, 0x45, 0x02, 0x20, 0x24, 0xe5, 0xd1, 0xf4, 0x7a, 0x7d,
        0x7b, 0x0d, 0x20, 0x6a, 0x26, 0xef, 0x69, 0x9b, 0x7c, 0x97, 0x57, 0xb7, 0x2d, 0x46, 0x90, 0x89, 0xde, 0x31, 0x92, 0xe6,
        0x78, 0xc7, 0x45, 0xe7, 0xf6, 0x0c, 0x02, 0x21, 0x00, 0xf8, 0xaa, 0x2f, 0xa7, 0x11, 0xfc, 0xb7, 0x9b, 0x97, 0xe3, 0x97,
        0xce, 0xda, 0x66, 0x7b, 0xae, 0x46, 0x4e, 0x2b, 0xd3, 0xff, 0xdf, 0xc3, 0xcc, 0xed, 0x7a, 0xa8, 0xca, 0x5f, 0x4c, 0x1a,
        0x7c,
    };

    return CopySpanToMutableSpan(ByteSpan{ kCdForAllExamples }, out_cd_buffer);

#else
    size_t buflen = out_cd_buffer.size();
    ChipLogDetail(Crypto, "Get certificate declaration from se05x");
    ReturnErrorOnFailure(se05x_get_certificate(CERT_DECLARATION_DATA_SE05X_ID, out_cd_buffer.data(), &buflen));
    out_cd_buffer.reduce_size(buflen);
    return CHIP_NO_ERROR;
#endif
}

CHIP_ERROR ExampleSe05xDACProviderv2::GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer)
{
    // TODO: We need a real example FirmwareInformation to be populated.
    out_firmware_info_buffer.reduce_size(0);

    return CHIP_NO_ERROR;
}

CHIP_ERROR ExampleSe05xDACProviderv2::SignWithDeviceAttestationKey(const ByteSpan & message_to_sign,
                                                                   MutableByteSpan & out_signature_buffer)
{
    CHIP_ERROR err                                                   = CHIP_NO_ERROR;
    uint8_t signature_se05x[Crypto::kMax_ECDSA_Signature_Length_Der] = { 0 };
    size_t signature_se05x_len                                       = sizeof(signature_se05x);
    VerifyOrReturnError(!out_signature_buffer.empty(), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(!message_to_sign.empty(), CHIP_ERROR_INVALID_ARGUMENT);

    ChipLogDetail(Crypto, "Sign using DA key from se05x (Using internal sign)");

    TLV::TLVReader msg_reader;
    TLV::TLVReader tagReader;

    msg_reader.Init(message_to_sign);

    uint16_t taglen = 0;
    ByteSpan tagvalue;
    uint8_t tempBuf[2] = {
        0,
    };

    tempBuf[0] = (uint8_t) TLV::TLVElementType::Structure;
    SuccessOrExit(err = se05x_set_certificate(START_CONTAINER_SE05X_ID, tempBuf, 1));

    for (int i = 1; i <= NO_OF_DEV_ATTEST_MSG_TAGS_TO_PARSE; i++)
    {
        CHIP_ERROR tlverr = CHIP_NO_ERROR;
        tlverr            = TLV::Utilities::Find(msg_reader, TLV::ContextTag(i), tagReader);
        if ((i == 3) && (tlverr == CHIP_ERROR_TLV_TAG_NOT_FOUND))
        {
            continue;
        }
        // TODO: Should this be setting err = tlverr?
        SuccessOrExit(tlverr);

        // Transient binary object ids starting from location 0x7D300005 (TAG1_ID) to 0x7D30000D (TAG3_VALUE_ID)
        // are used to store the TLV contents.
        // Binary object id are calculated using the loop iterator in the below code.

        taglen     = tagReader.GetLength();
        tempBuf[0] = tagReader.GetControlByte();
        tempBuf[1] = i;
        SuccessOrExit(err = se05x_set_certificate(TAG1_ID + (3 /* tag + length + value ids */ * (i - 1)), tempBuf, 2));
        if (taglen > 256)
        {
            tempBuf[0] = taglen & 0xFF;
            tempBuf[1] = (taglen >> 8) & 0xFF;
            SuccessOrExit(err = se05x_set_certificate(TAG1_LEN_ID + (3 * (i - 1)), tempBuf, 2));
        }
        else
        {
            tempBuf[0] = taglen;
            SuccessOrExit(err = se05x_set_certificate(TAG1_LEN_ID + (3 * (i - 1)), tempBuf, 1));
        }
        if (taglen > 0)
        {
            SuccessOrExit(err = tagReader.Get(tagvalue));
            SuccessOrExit(err = se05x_set_certificate(TAG1_VALUE_ID + (3 * (i - 1)), tagvalue.data(), taglen));
        }
    }

    tempBuf[0] = (uint8_t) TLV::TLVElementType::EndOfContainer;
    SuccessOrExit(err = se05x_set_certificate(END_CONTAINER_SE05X_ID, tempBuf, 1));

    if ((tagReader.GetRemainingLength() + 1 /* End container */) >= 16)
    {
        /* Set attestation challenge */
        SuccessOrExit(err = se05x_set_certificate(ATTEST_CHALLENGE_ID, (message_to_sign.end() - 16), 16));
    }

    SuccessOrExit(err = se05x_perform_internal_sign(DEV_ATTESTATION_KEY_SE05X_ID_IS, signature_se05x, &signature_se05x_len));

    err = chip::Crypto::EcdsaAsn1SignatureToRaw(chip::Crypto::kP256_FE_Length, ByteSpan{ signature_se05x, signature_se05x_len },
                                                out_signature_buffer);

exit:
    // Delete existing objects if any
    se05x_delete_key(START_CONTAINER_SE05X_ID);
    se05x_delete_key(TAG1_ID);
    se05x_delete_key(TAG1_LEN_ID);
    se05x_delete_key(TAG1_VALUE_ID);
    se05x_delete_key(TAG2_ID);
    se05x_delete_key(TAG2_LEN_ID);
    se05x_delete_key(TAG2_VALUE_ID);
    se05x_delete_key(TAG3_ID);
    se05x_delete_key(TAG3_LEN_ID);
    se05x_delete_key(TAG3_VALUE_ID);
    se05x_delete_key(END_CONTAINER_SE05X_ID);
    se05x_delete_key(ATTEST_CHALLENGE_ID);

    return err;
}

} // namespace

DeviceAttestationCredentialsProvider * GetExampleSe05xDACProviderv2()
{
    static ExampleSe05xDACProviderv2 example_dac_provider;

    return &example_dac_provider;
}

} // namespace Examples
} // namespace Credentials
} // namespace chip
