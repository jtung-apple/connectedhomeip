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

#include <lib/support/CodeUtils.h>
#include <lib/support/SafeInt.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/silabs/NetworkCommissioningWiFiDriver.h>
#include <platform/silabs/SilabsConfig.h>

#include <limits>

using namespace ::chip;
using namespace ::chip::DeviceLayer::Internal;
using namespace ::chip::DeviceLayer::Silabs;

namespace chip {
namespace DeviceLayer {
namespace NetworkCommissioning {

namespace {
NetworkCommissioning::WiFiScanResponse * sScanResult;
SlScanResponseIterator<NetworkCommissioning::WiFiScanResponse> mScanResponseIter(sScanResult);
} // namespace

SlWiFiDriver * SlWiFiDriver::mDriver = nullptr;

CHIP_ERROR SlWiFiDriver::Init(NetworkStatusChangeCallback * networkStatusChangeCallback)
{
    CHIP_ERROR err;
    size_t ssidLen         = 0;
    size_t credentialsLen  = 0;
    mpScanCallback         = nullptr;
    mpConnectCallback      = nullptr;
    mpStatusChangeCallback = networkStatusChangeCallback;
    mDriver                = this;

#ifdef SL_ONNETWORK_PAIRING
    memcpy(&mSavedNetwork.ssid[0], SL_WIFI_SSID, sizeof(SL_WIFI_SSID));
    memcpy(&mSavedNetwork.credentials[0], SL_WIFI_PSK, sizeof(SL_WIFI_PSK));
    credentialsLen               = sizeof(SL_WIFI_PSK);
    ssidLen                      = sizeof(SL_WIFI_SSID);
    mSavedNetwork.credentialsLen = credentialsLen;
    mSavedNetwork.ssidLen        = ssidLen;
    mStagingNetwork              = mSavedNetwork;
    err                          = CHIP_NO_ERROR;
#else
    // If reading fails, wifi is not provisioned, no need to go further.
    err = SilabsConfig::ReadConfigValueStr(SilabsConfig::kConfigKey_WiFiSSID, mSavedNetwork.ssid, sizeof(mSavedNetwork.ssid),
                                           ssidLen);
    VerifyOrReturnError(err == CHIP_NO_ERROR, CHIP_NO_ERROR);

    err = SilabsConfig::ReadConfigValueStr(SilabsConfig::kConfigKey_WiFiPSK, mSavedNetwork.credentials,
                                           sizeof(mSavedNetwork.credentials), credentialsLen);
    VerifyOrReturnError(err == CHIP_NO_ERROR, CHIP_NO_ERROR);

    mSavedNetwork.credentialsLen = credentialsLen;
    mSavedNetwork.ssidLen        = ssidLen;
    mStagingNetwork              = mSavedNetwork;
#endif
    ConnectWiFiNetwork(mSavedNetwork.ssid, ssidLen, mSavedNetwork.credentials, credentialsLen);
    return err;
}

CHIP_ERROR SlWiFiDriver::CommitConfiguration()
{
    uint8_t securityType = WFX_SEC_WPA2;

    ReturnErrorOnFailure(
        SilabsConfig::WriteConfigValueStr(SilabsConfig::kConfigKey_WiFiSSID, mStagingNetwork.ssid, mStagingNetwork.ssidLen));
    ReturnErrorOnFailure(SilabsConfig::WriteConfigValueStr(SilabsConfig::kConfigKey_WiFiPSK, mStagingNetwork.credentials,
                                                           mStagingNetwork.credentialsLen));
    ReturnErrorOnFailure(SilabsConfig::WriteConfigValueBin(SilabsConfig::kConfigKey_WiFiSEC, &securityType, sizeof(securityType)));

    mSavedNetwork = mStagingNetwork;
    return CHIP_NO_ERROR;
}

CHIP_ERROR SlWiFiDriver::RevertConfiguration()
{
    mStagingNetwork = mSavedNetwork;
    return CHIP_NO_ERROR;
}

bool SlWiFiDriver::NetworkMatch(const WiFiNetwork & network, ByteSpan networkId)
{
    return networkId.size() == network.ssidLen && memcmp(networkId.data(), network.ssid, network.ssidLen) == 0;
}

Status SlWiFiDriver::AddOrUpdateNetwork(ByteSpan ssid, ByteSpan credentials, MutableCharSpan & outDebugText,
                                        uint8_t & outNetworkIndex)
{
    outDebugText.reduce_size(0);
    outNetworkIndex = 0;
    VerifyOrReturnError(mStagingNetwork.ssidLen == 0 || NetworkMatch(mStagingNetwork, ssid), Status::kBoundsExceeded);
    VerifyOrReturnError(credentials.size() <= sizeof(mStagingNetwork.credentials), Status::kOutOfRange);
    VerifyOrReturnError(ssid.size() <= sizeof(mStagingNetwork.ssid), Status::kOutOfRange);

    memset(mStagingNetwork.credentials, 0, sizeof(mStagingNetwork.credentials));
    memcpy(mStagingNetwork.credentials, credentials.data(), credentials.size());
    mStagingNetwork.credentialsLen = static_cast<decltype(mStagingNetwork.credentialsLen)>(credentials.size());

    memset(mStagingNetwork.ssid, 0, sizeof(mStagingNetwork.ssid));
    memcpy(mStagingNetwork.ssid, ssid.data(), ssid.size());
    mStagingNetwork.ssidLen = static_cast<decltype(mStagingNetwork.ssidLen)>(ssid.size());

    return Status::kSuccess;
}

Status SlWiFiDriver::RemoveNetwork(ByteSpan networkId, MutableCharSpan & outDebugText, uint8_t & outNetworkIndex)
{
    outDebugText.reduce_size(0);
    outNetworkIndex = 0;
    VerifyOrReturnError(NetworkMatch(mStagingNetwork, networkId), Status::kNetworkIDNotFound);

    // Use empty ssid for representing invalid network
    mStagingNetwork.ssidLen = 0;
    return Status::kSuccess;
}

Status SlWiFiDriver::ReorderNetwork(ByteSpan networkId, uint8_t index, MutableCharSpan & outDebugText)
{
    outDebugText.reduce_size(0);
    // Only one network is supported for now
    VerifyOrReturnError(index == 0, Status::kOutOfRange);
    VerifyOrReturnError(NetworkMatch(mStagingNetwork, networkId), Status::kNetworkIDNotFound);
    return Status::kSuccess;
}

CHIP_ERROR SlWiFiDriver::ConnectWiFiNetwork(const char * ssid, uint8_t ssidLen, const char * key, uint8_t keyLen)
{
    if (ConnectivityMgr().IsWiFiStationProvisioned())
    {
        ChipLogProgress(DeviceLayer, "Disconnecting for current wifi");
        ReturnErrorOnFailure(WifiInterface::GetInstance().TriggerDisconnection());
    }
    ReturnErrorOnFailure(ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled));

    // Set the wifi configuration
    WifiInterface::WifiCredentials wifiConfig;

    VerifyOrReturnError(ssidLen <= WFX_MAX_SSID_LENGTH, CHIP_ERROR_BUFFER_TOO_SMALL);
    memcpy(wifiConfig.ssid, ssid, ssidLen);
    wifiConfig.ssidLength = ssidLen;

    VerifyOrReturnError(keyLen < WFX_MAX_PASSKEY_LENGTH, CHIP_ERROR_BUFFER_TOO_SMALL);
    memcpy(wifiConfig.passkey, key, keyLen);
    wifiConfig.passkeyLength = keyLen;

    wifiConfig.security = WFX_SEC_WPA2;

    ChipLogProgress(NetworkProvisioning, "Setting up connection for WiFi SSID: %s", NullTerminated(ssid, ssidLen).c_str());
    // Configure the WFX WiFi interface.
    WifiInterface::GetInstance().SetWifiCredentials(wifiConfig);
    ReturnErrorOnFailure(ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled));
    ReturnErrorOnFailure(ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled));
    return CHIP_NO_ERROR;
}

// TODO: Re-write implementation with proper driver based callback
void SlWiFiDriver::UpdateNetworkingStatus()
{
    if (mpStatusChangeCallback == nullptr)
    {
        ChipLogError(NetworkProvisioning, "networkStatusChangeCallback is nil");
        return;
    }

    if (mStagingNetwork.ssidLen == 0)
    {
        ChipLogError(NetworkProvisioning, "ssidLen is 0");
        return;
    }

    ByteSpan networkId = ByteSpan((const unsigned char *) mStagingNetwork.ssid, mStagingNetwork.ssidLen);
    if (!WifiInterface::GetInstance().IsStationConnected())
    {
        // TODO: https://github.com/project-chip/connectedhomeip/issues/26861
        mpStatusChangeCallback->OnNetworkingStatusChange(Status::kUnknownError, MakeOptional(networkId),
                                                         MakeOptional(static_cast<int32_t>(SL_STATUS_FAIL)));
        return;
    }
    mpStatusChangeCallback->OnNetworkingStatusChange(Status::kSuccess, MakeOptional(networkId), NullOptional);
}

void SlWiFiDriver::OnConnectWiFiNetwork()
{
    if (mpConnectCallback)
    {
        CommitConfiguration();
        mpConnectCallback->OnResult(Status::kSuccess, CharSpan(), 0);
        mpConnectCallback = nullptr;
    }
}

void SlWiFiDriver::ConnectNetwork(ByteSpan networkId, ConnectCallback * callback)
{
    CHIP_ERROR err          = CHIP_NO_ERROR;
    Status networkingStatus = Status::kUnknownError;

    VerifyOrExit(NetworkMatch(mStagingNetwork, networkId), networkingStatus = Status::kNetworkIDNotFound);
    VerifyOrExit(mpConnectCallback == nullptr, networkingStatus = Status::kUnknownError);

    err = ConnectWiFiNetwork(mStagingNetwork.ssid, mStagingNetwork.ssidLen, mStagingNetwork.credentials,
                             mStagingNetwork.credentialsLen);
    if (err == CHIP_NO_ERROR)
    {
        mpConnectCallback = callback;
        networkingStatus  = Status::kSuccess;
    }

exit:
    if (networkingStatus != Status::kSuccess)
    {
        ChipLogError(NetworkProvisioning, "Failed to connect to WiFi network: %" CHIP_ERROR_FORMAT, err.Format());
        mpConnectCallback = nullptr;
        callback->OnResult(networkingStatus, CharSpan(), 0);
    }
}

chip::BitFlags<WiFiSecurity> SlWiFiDriver::ConvertSecuritytype(wfx_sec_t security)
{
    chip::BitFlags<WiFiSecurity> securityType;
    if (security == WFX_SEC_NONE)
    {
        securityType = WiFiSecurity::kUnencrypted;
    }
    else if (security == WFX_SEC_WEP)
    {
        securityType = WiFiSecurity::kWep;
    }
    else if (security == WFX_SEC_WPA)
    {
        securityType = WiFiSecurity::kWpaPersonal;
    }
    else if (security == WFX_SEC_WPA2)
    {
        securityType = WiFiSecurity::kWpa2Personal;
    }
    else if (security == WFX_SEC_WPA3)
    {
        securityType = WiFiSecurity::kWpa3Personal;
    }
    else
    {
        // wfx_sec_t support more type
        securityType = WiFiSecurity::kUnencrypted;
    }

    return securityType;
}

uint32_t SlWiFiDriver::GetSupportedWiFiBandsMask() const
{
    return WifiInterface::GetInstance().GetSupportedWiFiBandsMask();
}

bool SlWiFiDriver::StartScanWiFiNetworks(ByteSpan ssid)
{
    ChipLogProgress(DeviceLayer, "Start Scan WiFi Networks");
    CHIP_ERROR err = WifiInterface::GetInstance().StartNetworkScan(ssid, OnScanWiFiNetworkDone);

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "StartNetworkScan failed: %" CHIP_ERROR_FORMAT, err.Format());
        return false;
    }

    return true;
}

void SlWiFiDriver::OnScanWiFiNetworkDone(wfx_wifi_scan_result_t * aScanResult)
{
    SlWiFiDriver * nwDriver = NetworkCommissioning::SlWiFiDriver::GetInstance();
    // Cannot use the driver if the instance is not initialized.
    VerifyOrDie(nwDriver != nullptr); // should never be null

    if (!aScanResult)
    {
        ChipLogProgress(DeviceLayer, "OnScanWiFiNetworkDone: Receive all scanned networks information.");

        if (nwDriver->mpScanCallback != nullptr)
        {
            if (mScanResponseIter.Count() == 0)
            {
                // if there is no network found, return kNetworkNotFound
                DeviceLayer::SystemLayer().ScheduleLambda([nwDriver]() {
                    nwDriver->mpScanCallback->OnFinished(NetworkCommissioning::Status::kNetworkNotFound, CharSpan(), nullptr);
                    nwDriver->mpScanCallback = nullptr;
                });
                return;
            }
            DeviceLayer::SystemLayer().ScheduleLambda([nwDriver]() {
                nwDriver->mpScanCallback->OnFinished(NetworkCommissioning::Status::kSuccess, CharSpan(), &mScanResponseIter);
                nwDriver->mpScanCallback = nullptr;
            });
        }
    }
    else
    {
        NetworkCommissioning::WiFiScanResponse scanResponse = {};

        scanResponse.security.Set(nwDriver->ConvertSecuritytype(aScanResult->security));
        scanResponse.channel = aScanResult->chan;
        scanResponse.rssi    = aScanResult->rssi;
        scanResponse.ssidLen = aScanResult->ssid_length;
        memcpy(scanResponse.ssid, aScanResult->ssid, scanResponse.ssidLen);
        memcpy(scanResponse.bssid, aScanResult->bssid, sizeof(scanResponse.bssid));
        scanResponse.wiFiBand = aScanResult->wiFiBand;

        mScanResponseIter.Add(&scanResponse);
    }
}

void SlWiFiDriver::ScanNetworks(ByteSpan ssid, WiFiDriver::ScanCallback * callback)
{
    if (callback != nullptr)
    {
        mpScanCallback = callback;
        if (!StartScanWiFiNetworks(ssid))
        {
            ChipLogError(DeviceLayer, "ScanWiFiNetworks failed to start");
            mpScanCallback = nullptr;
            callback->OnFinished(Status::kUnknownError, CharSpan(), nullptr);
        }
    }
}

CHIP_ERROR GetConnectedNetwork(Network & network)
{
    WifiInterface::WifiCredentials wifiConfig;
    network.networkIDLen = 0;
    network.connected    = false;

    // we are able to fetch the wifi provision data and STA should be connected
    VerifyOrReturnError(WifiInterface::GetInstance().IsStationConnected(), CHIP_ERROR_NOT_CONNECTED);
    ReturnErrorOnFailure(WifiInterface::GetInstance().GetWifiCredentials(wifiConfig));
    VerifyOrReturnError(wifiConfig.ssidLength <= NetworkCommissioning::kMaxNetworkIDLen, CHIP_ERROR_BUFFER_TOO_SMALL);

    network.connected = true;

    ByteSpan ssidSpan(wifiConfig.ssid, wifiConfig.ssidLength);
    MutableByteSpan networkIdSpan(network.networkID, NetworkCommissioning::kMaxNetworkIDLen);

    ReturnErrorOnFailure(CopySpanToMutableSpan(ssidSpan, networkIdSpan));
    network.networkIDLen = networkIdSpan.size();

    return CHIP_NO_ERROR;
}

size_t SlWiFiDriver::WiFiNetworkIterator::Count()
{
    return mDriver->mStagingNetwork.ssidLen == 0 ? 0 : 1;
}

bool SlWiFiDriver::WiFiNetworkIterator::Next(Network & item)
{
    if (mExhausted || mDriver->mStagingNetwork.ssidLen == 0)
    {
        return false;
    }
    memcpy(item.networkID, mDriver->mStagingNetwork.ssid, mDriver->mStagingNetwork.ssidLen);
    item.networkIDLen = mDriver->mStagingNetwork.ssidLen;
    item.connected    = false;
    mExhausted        = true;

    Network connectedNetwork;
    CHIP_ERROR err = GetConnectedNetwork(connectedNetwork);
    if (err == CHIP_NO_ERROR)
    {
        if (connectedNetwork.networkIDLen == item.networkIDLen &&
            memcmp(connectedNetwork.networkID, item.networkID, item.networkIDLen) == 0)
        {
            item.connected = true;
        }
    }
    return true;
}

} // namespace NetworkCommissioning
} // namespace DeviceLayer
} // namespace chip
