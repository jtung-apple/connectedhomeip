/*
 *
 *    Copyright (c) 2020-2022 Project CHIP Authors
 *    Copyright (c) 2020 Nest Labs, Inc.
 *    Copyright 2023-2024 NXP
 *    All rights reserved.
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
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/ConnectivityManager.h>
#include <platform/DiagnosticDataProvider.h>

#include <platform/internal/GenericConnectivityManagerImpl_UDP.ipp>

#if INET_CONFIG_ENABLE_TCP_ENDPOINT
#include <platform/internal/GenericConnectivityManagerImpl_TCP.ipp>
#endif

#if CHIP_SYSTEM_CONFIG_USE_LWIP
#include <lwip/dns.h>
#include <lwip/ip_addr.h>
#include <lwip/nd6.h>
#include <lwip/netif.h>
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#include <platform/internal/BLEManager.h>
#include <platform/internal/GenericConnectivityManagerImpl_BLE.ipp>
#endif

#if CONFIG_CHIP_ETHERNET
#include "NxpEthDriver.h"
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
#include "NetworkCommissioningDriver.h"
extern "C" {
#include "wlan.h"
#include "wm_net.h"
}

#include <platform/internal/GenericConnectivityManagerImpl_WiFi.ipp>

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD

#include <openthread/mdns.h>

#include "border_agent.h"
#include "br_rtos_manager.h"
#include "infra_if.h"
#endif /* CHIP_DEVICE_CONFIG_ENABLE_THREAD */

#endif /* CHIP_DEVICE_CONFIG_ENABLE_WPA */

#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
#include "ConnectivityManagerImpl.h"
#include <platform/internal/GenericConnectivityManagerImpl_Thread.ipp>
#endif /* CHIP_DEVICE_CONFIG_ENABLE_THREAD */

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::DeviceLayer::Internal;
using namespace ::chip::DeviceLayer::DeviceEventType;

namespace chip {
namespace DeviceLayer {

ConnectivityManagerImpl ConnectivityManagerImpl::sInstance;
#if CHIP_DEVICE_CONFIG_ENABLE_WPA
netif_ext_callback_t ConnectivityManagerImpl::sNetifCallback;
#endif /* CHIP_DEVICE_CONFIG_ENABLE_WPA */

CHIP_ERROR ConnectivityManagerImpl::_Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    mWiFiStationMode                = kWiFiStationMode_Disabled;
    mWiFiStationState               = kWiFiStationState_NotConnected;
    mWiFiStationReconnectIntervalMS = CHIP_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL;
#endif

    // Initialize the generic base classes that require it.
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>::_Init();
#endif

    SuccessOrExit(err);

exit:
    return err;
}

void ConnectivityManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    // Forward the event to the generic base classes as needed.
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    GenericConnectivityManagerImpl_Thread<ConnectivityManagerImpl>::_OnPlatformEvent(event);
#endif
#if CHIP_DEVICE_CONFIG_ENABLE_WPA || CONFIG_CHIP_ETHERNET
    if (event->Type == kPlatformNxpIpChangeEvent)
    {
        UpdateInternetConnectivityState();
    }
#endif
#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    else if (event->Type == kPlatformNxpWlanEvent)
    {
        ProcessWlanEvent(event->Platform.WlanEventReason);
    }
    else if (event->Type == kPlatformNxpStartWlanConnectEvent)
    {
        bool is_wlan_added                  = false;
        struct wlan_network searchedNetwork = { 0 };

        /* If network was added before on a previous connection call or other API, do not add it again */
        if (wlan_get_network_byname(event->Platform.pNetworkDataEvent->name, &searchedNetwork) != WM_SUCCESS)
        {
            if (wlan_add_network(event->Platform.pNetworkDataEvent) == WM_SUCCESS)
            {
                ChipLogProgress(DeviceLayer, "Added WLAN \"%s\"", event->Platform.pNetworkDataEvent->name);
                is_wlan_added = true;
            }
        }
        else
        {
            /* In case network was added before, signal that it is added and that connection can start */
            is_wlan_added = true;
        }

        /* At this point, the network details should be registered in the wlan driver */
        if (is_wlan_added == true)
        {
            _SetWiFiStationState(kWiFiStationState_Connecting);
            ChipLogProgress(DeviceLayer, "WLAN connecting to network.name = \"%s\"", event->Platform.pNetworkDataEvent->name);
#if WIFI_DFS_OPTIMIZATION
            /* Skip DFS (Dynamic Frequency Selection) channels during scan, DFS is used to avoid interferences */
            wlan_connect_opt(event->Platform.pNetworkDataEvent->name, true);
#else
            wlan_connect(event->Platform.pNetworkDataEvent->name);
#endif
        }
        if (event->Platform.pNetworkDataEvent != NULL)
        {
            free(event->Platform.pNetworkDataEvent);
        }
    }
    else if (event->Type == kPlatformNxpScanWiFiNetworkDoneEvent)
    {
        NetworkCommissioning::NXPWiFiDriver::GetInstance().ScanWiFINetworkDoneFromMatterTaskContext(
            event->Platform.ScanWiFiNetworkCount);
    }
    else if (event->Type == kPlatformNxpStartWlanInitWaitTimerEvent)
    {
        DeviceLayer::SystemLayer().StartTimer(System::Clock::Milliseconds32(kWlanInitWaitMs), ConnectNetworkTimerHandler,
                                              (void *) event->Platform.pNetworkDataEvent);
    }
    else if (event->Type == DeviceLayer::DeviceEventType::kFailSafeTimerExpired)
    {
        /*
         * This special case must be handled to address Wi-Fi connection failures during Matter commissioning.
         * For instance, if an incorrect SSID or password is provided, we need a mechanism to stop the Wi-Fi driver from attempting
         * to connect, allowing a new connection attempt later. If the failSafeTimer expires while the system is still in the
         * process of connecting, a disconnect event should be scheduled. This ensures that, once disconnected, the
         * wlan_remove_network function is called as part of the RevertConfiguration process.
         */
        if (mWiFiStationState == kWiFiStationState_Connecting)
        {
            mWiFiStationState = kWiFiStationState_Connecting_Failed;
            int ret           = wlan_disconnect();
            if (ret != WM_SUCCESS)
            {
                ChipLogError(NetworkProvisioning, "Failed to disconnect from network with error: %u", (uint8_t) ret);
            }
        }
    }
#endif
}

#if CHIP_DEVICE_CONFIG_ENABLE_WPA || CONFIG_CHIP_ETHERNET
void ConnectivityManagerImpl::UpdateInternetConnectivityState()
{
    bool haveIPv4Conn      = false;
    bool haveIPv6Conn      = false;
    const bool hadIPv4Conn = mFlags.Has(ConnectivityFlags::kHaveIPv4InternetConnectivity);
    const bool hadIPv6Conn = mFlags.Has(ConnectivityFlags::kHaveIPv6InternetConnectivity);
    const ip_addr_t * addr4;
    const ip6_addr_t * addr6;
    CHIP_ERROR err;
    ChipDeviceEvent event;

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    // If the WiFi station is currently in the connected state...
    if (_IsWiFiStationConnected())
    {
        // Get the LwIP netif for the WiFi station interface.
        struct netif * netif = static_cast<struct netif *>(net_get_mlan_handle());
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

#if CONFIG_CHIP_ETHERNET
        // Get the LwIP netif for the Ethernet station interface.
        struct netif * netif = DeviceLayer::NetworkCommissioning::NxpEthDriver::Instance().GetEthInetIf();
#endif

        // If interface is up...
        if ((netif != nullptr) && netif_is_up(netif) && netif_is_link_up(netif))
        {
#if INET_CONFIG_ENABLE_IPV4
            // Check if a DNS server is currently configured.  If so...
            ip_addr_t dnsServerAddr = *dns_getserver(0);
            if (!ip_addr_isany_val(dnsServerAddr))
            {
                // If the station interface has been assigned an IPv4 address, and has
                // an IPv4 gateway, then presume that the device has IPv4 Internet
                // connectivity.
                if (!ip4_addr_isany_val(*netif_ip4_addr(netif)) && !ip4_addr_isany_val(*netif_ip4_gw(netif)))
                {
                    haveIPv4Conn = true;
                    addr4        = &netif->ip_addr;
                }
            }
#endif

            // Search among the IPv6 addresses assigned to the interface for an
            // address that is in the valid state. Search goes backwards because
            // the link-local address is in the first slot and we prefer to report
            // other than the link-local address value if there are multiple addresses.
            for (int i = (LWIP_IPV6_NUM_ADDRESSES - 1); i >= 0; i--)
            {
                if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i)))
                {
                    haveIPv6Conn = true;
                    addr6        = netif_ip6_addr(netif, i);
                    break;
                }
            }
        }
#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    }
#endif

    // Update the current state.
    mFlags.Set(ConnectivityFlags::kHaveIPv4InternetConnectivity, haveIPv4Conn)
        .Set(ConnectivityFlags::kHaveIPv6InternetConnectivity, haveIPv6Conn);

    if (haveIPv4Conn != hadIPv4Conn)
    {
        /* Check if the */
        event.Type                            = DeviceEventType::kInternetConnectivityChange;
        event.InternetConnectivityChange.IPv4 = GetConnectivityChange(hadIPv4Conn, haveIPv4Conn);
        event.InternetConnectivityChange.IPv6 = kConnectivity_NoChange;
        if (haveIPv4Conn)
        {
            event.InternetConnectivityChange.ipAddress = IPAddress(*addr4);
        }
        err = PlatformMgr().PostEvent(&event);
        VerifyOrDie(err == CHIP_NO_ERROR);

        ChipLogProgress(DeviceLayer, "%s Internet connectivity %s", "IPv4", (haveIPv4Conn) ? "ESTABLISHED" : "LOST");
    }

    if (haveIPv6Conn != hadIPv6Conn)
    {
        event.Type                            = DeviceEventType::kInternetConnectivityChange;
        event.InternetConnectivityChange.IPv4 = kConnectivity_NoChange;
        event.InternetConnectivityChange.IPv6 = GetConnectivityChange(hadIPv6Conn, haveIPv6Conn);

        if (haveIPv6Conn)
        {
            event.InternetConnectivityChange.ipAddress = IPAddress(*addr6);
#if CHIP_ENABLE_OPENTHREAD
            // In case of boot, start the Border Router services including MDNS Server
            // The posted event will signal the application to restart the Matter mDNS server instance
            BrHandleStateChange();
#endif
        }
        err = PlatformMgr().PostEvent(&event);
        VerifyOrDie(err == CHIP_NO_ERROR);

        ChipLogProgress(DeviceLayer, "%s Internet connectivity %s", "IPv6", (haveIPv6Conn) ? "ESTABLISHED" : "LOST");
    }
}
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA || CONFIG_CHIP_ETHERNET

#if CHIP_DEVICE_CONFIG_ENABLE_WPA

ConnectivityManager::WiFiStationMode ConnectivityManagerImpl::_GetWiFiStationMode()
{
    return mWiFiStationMode;
}

CHIP_ERROR ConnectivityManagerImpl::_SetWiFiStationMode(ConnectivityManager::WiFiStationMode val)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrExit(val != ConnectivityManager::kWiFiStationMode_NotSupported, err = CHIP_ERROR_INVALID_ARGUMENT);

    if (mWiFiStationMode != val)
    {
        ChipLogProgress(DeviceLayer, "WiFi station mode change: %s -> %s", WiFiStationModeToStr(mWiFiStationMode),
                        WiFiStationModeToStr(val));
    }

    mWiFiStationMode = val;
exit:
    return err;
}

void ConnectivityManagerImpl::_SetWiFiStationState(ConnectivityManager::WiFiStationState val)
{
    if (mWiFiStationState != val)
    {
        ChipLogProgress(DeviceLayer, "WiFi station state change: %s -> %s", WiFiStationStateToStr(mWiFiStationState),
                        WiFiStationStateToStr(val));
    }

    mWiFiStationState = val;
}

CHIP_ERROR ConnectivityManagerImpl::_SetWiFiAPMode(WiFiAPMode val)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrExit(val != kWiFiAPMode_NotSupported, err = CHIP_ERROR_INVALID_ARGUMENT);

    if (mWiFiAPMode != val)
    {
        ChipLogProgress(DeviceLayer, "WiFi AP mode change: %s -> %s", WiFiAPModeToStr(mWiFiAPMode), WiFiAPModeToStr(val));
    }

    mWiFiAPMode = val;
exit:
    return err;
}

bool ConnectivityManagerImpl::_IsWiFiStationEnabled()
{
    return GetWiFiStationMode() == kWiFiStationMode_Enabled;
}

bool ConnectivityManagerImpl::_IsWiFiStationConnected()
{
    return (mWiFiStationState == kWiFiStationState_Connected);
}

bool ConnectivityManagerImpl::_IsWiFiStationProvisioned()
{
    return mWifiIsProvisioned;
}

bool ConnectivityManagerImpl::_IsWiFiStationApplicationControlled()
{
    return mWiFiStationMode == ConnectivityManager::kWiFiStationMode_ApplicationControlled;
}

void ConnectivityManagerImpl::ProcessWlanEvent(enum wlan_event_reason wlanEvent)
{
    WiFiDiagnosticsDelegate * delegate = GetDiagnosticDataProvider().GetWiFiDiagnosticsDelegate();
    uint8_t associationFailureCause =
        chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCauseEnum::kUnknown);
    uint16_t wlan_status_code = wlan_get_status_code(wlanEvent);

#if CHIP_DETAIL_LOGGING
    enum wlan_connection_state state;
    int result;

    result = wlan_get_connection_state(&state);
    if (result == WM_SUCCESS)
    {
        ChipLogDetail(DeviceLayer, "WLAN event: %d, WLAN connection state: %d", wlanEvent, state);
    }
    else
    {
        ChipLogDetail(DeviceLayer, "WLAN event: %d, WLAN connection state: unknown", wlanEvent);
    }
#endif /* CHIP_DETAIL_LOGGING */

    switch (wlanEvent)
    {
    case WLAN_REASON_SUCCESS:
        ChipLogProgress(DeviceLayer, "Connected to WLAN network = %d", is_sta_ipv6_connected());
        if (sInstance._GetWiFiStationState() != kWiFiStationState_Connected)
        {
            sInstance._SetWiFiStationState(kWiFiStationState_Connecting_Succeeded);
            sInstance._SetWiFiStationState(kWiFiStationState_Connected);
            NetworkCommissioning::NXPWiFiDriver::GetInstance().OnConnectWiFiNetwork(NetworkCommissioning::Status::kSuccess,
                                                                                    CharSpan(), wlanEvent);
            sInstance.OnStationConnected();
        }
        break;

    case WLAN_REASON_AUTH_SUCCESS:
        ChipLogProgress(DeviceLayer, "Associated to WLAN network");
        break;

    case WLAN_REASON_CONNECT_FAILED:
        ChipLogError(DeviceLayer, "WLAN (re)connect failed");
        sInstance._SetWiFiStationState(kWiFiStationState_NotConnected);
        associationFailureCause =
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCauseEnum::kAssociationFailed);
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, wlan_status_code);
        }
        UpdateInternetConnectivityState();
        break;

    case WLAN_REASON_NETWORK_NOT_FOUND:
        ChipLogError(DeviceLayer, "WLAN network not found");
        NetworkCommissioning::NXPWiFiDriver::GetInstance().OnConnectWiFiNetwork(NetworkCommissioning::Status::kNetworkNotFound,
                                                                                CharSpan(), wlanEvent);
        associationFailureCause =
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCauseEnum::kSsidNotFound);
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, wlan_status_code);
        }
        break;

    case WLAN_REASON_NETWORK_AUTH_FAILED:
        ChipLogError(DeviceLayer, "Authentication to WLAN network failed");
        NetworkCommissioning::NXPWiFiDriver::GetInstance().OnConnectWiFiNetwork(NetworkCommissioning::Status::kAuthFailure,
                                                                                CharSpan(), wlanEvent);
        ChipLogError(DeviceLayer, "Authentication to WLAN network failed end");
        associationFailureCause =
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::AssociationFailureCauseEnum::kAuthenticationFailed);
        if (delegate)
        {
            delegate->OnAssociationFailureDetected(associationFailureCause, wlan_status_code);
        }
        break;

    case WLAN_REASON_LINK_LOST:
        ChipLogError(DeviceLayer, "WLAN link lost");
        if (sInstance._GetWiFiStationState() == kWiFiStationState_Connected)
        {
            sInstance._SetWiFiStationState(kWiFiStationState_NotConnected);
            sInstance.OnStationDisconnected();
            if (delegate)
            {
                delegate->OnDisconnectionDetected(wlan_status_code);
            }
        }
        break;

    case WLAN_REASON_USER_DISCONNECT:
        ChipLogProgress(DeviceLayer, "Disconnected from WLAN network");
        /*
         * If a disconnect was scheduled by the fail-safe timer while Wi-Fi was still in the connecting state,
         * a new call to RevertConfiguration must be made to remove the Wi-Fi credentials from the driver.
         * This step is necessary because the Wi-Fi driver's state machine requires a complete disconnection before
         * wlan_remove_network can be invoked.
         * Additionally mWifiIsProvisioned needs to be re-initialized to false
         *
         */
        if (mWiFiStationState == kWiFiStationState_Connecting_Failed)
        {
            NetworkCommissioning::NXPWiFiDriver::GetInstance().RevertConfiguration();
            mWifiIsProvisioned = false;
        }
        sInstance._SetWiFiStationState(kWiFiStationState_NotConnected);
        sInstance.OnStationDisconnected();
        if (delegate)
        {
            delegate->OnDisconnectionDetected(wlan_status_code);
        }
        break;

    case WLAN_REASON_INITIALIZED:
        sInstance._SetWiFiStationState(kWiFiStationState_NotConnected);
        sInstance._SetWiFiStationMode(kWiFiStationMode_Enabled);
        break;

    default:
        break;
    }
}

int ConnectivityManagerImpl::_WlanEventCallback(enum wlan_event_reason wlanEvent, void * data)
{
    ChipDeviceEvent event;
    event.Type                     = DeviceEventType::kPlatformNxpWlanEvent;
    event.Platform.WlanEventReason = wlanEvent;
    (void) PlatformMgr().PostEvent(&event);
    return 0;
}

void ConnectivityManagerImpl::OnStationConnected()
{
    ChipDeviceEvent event;

    event.Type                          = DeviceEventType::kWiFiConnectivityChange;
    event.WiFiConnectivityChange.Result = kConnectivity_Established;
    (void) PlatformMgr().PostEvent(&event);

    /* Update the connectivity state in case the connected event has been received after getting an IP addr */
    UpdateInternetConnectivityState();
    WiFiDiagnosticsDelegate * delegate = GetDiagnosticDataProvider().GetWiFiDiagnosticsDelegate();

    if (delegate)
    {
        delegate->OnConnectionStatusChanged(
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::ConnectionStatusEnum::kConnected));
    }
}

void ConnectivityManagerImpl::OnStationDisconnected()
{
    ChipDeviceEvent event;

    event.Type                          = DeviceEventType::kWiFiConnectivityChange;
    event.WiFiConnectivityChange.Result = kConnectivity_Lost;
    (void) PlatformMgr().PostEvent(&event);

    WiFiDiagnosticsDelegate * delegate = GetDiagnosticDataProvider().GetWiFiDiagnosticsDelegate();

    if (delegate)
    {
        delegate->OnConnectionStatusChanged(
            chip::to_underlying(chip::app::Clusters::WiFiNetworkDiagnostics::ConnectionStatusEnum::kNotConnected));
    }

    /* Update the connectivity state in case the connected event has been received after getting an IP addr */
    UpdateInternetConnectivityState();
}

void ConnectivityManagerImpl::_NetifExtCallback(struct netif * netif, netif_nsc_reason_t reason,
                                                const netif_ext_callback_args_t * args)
{
    struct netif * station_netif;
    ChipDeviceEvent event;

    ChipLogDetail(DeviceLayer, "_NetifExtCallback: netif=%p, reason=0x%04x", netif, reason);

    station_netif = static_cast<struct netif *>(net_get_mlan_handle());
    if (netif == station_netif)
    {
        event.Type = DeviceEventType::kPlatformNxpIpChangeEvent;
        (void) PlatformMgr().PostEvent(&event);
    }
}

void ConnectivityManagerImpl::StartWiFiManagement()
{
    struct netif * netif = nullptr;
    int32_t result;

    LOCK_TCPIP_CORE();
    netif = static_cast<struct netif *>(net_get_mlan_handle());
    if (netif != nullptr)
    {
        memset(&ConnectivityManagerImpl::sNetifCallback, 0, sizeof(ConnectivityManagerImpl::sNetifCallback));
        netif_add_ext_callback(&ConnectivityManagerImpl::sNetifCallback, &_NetifExtCallback);
    }
    UNLOCK_TCPIP_CORE();

    result = wlan_start(_WlanEventCallback);

    if (result != WM_SUCCESS)
    {
        ChipLogError(DeviceLayer, "Failed to start WLAN Connection Manager");
        chipDie();
    }
}
#if CHIP_ENABLE_OPENTHREAD
void ConnectivityManagerImpl::BrHandleStateChange()
{
    if (mBorderRouterInit == false)
    {
        struct netif * extNetIfPtr = static_cast<struct netif *>(net_get_mlan_handle());
        struct netif * thrNetIfPtr = ThreadStackMgrImpl().ThreadNetIf();

        otMdnsHost mdnsHost;
        uint8_t macBuffer[ConfigurationManager::kPrimaryMACAddressLength];
        MutableByteSpan mac(macBuffer);

        // Need to wait for the wifi to be connected because the mlan netif can be !=null but not initialized
        // properly. If the thread netif is !=null it means that it was fully initialized

        // Lock OT task ?
        if ((thrNetIfPtr) && (mWiFiStationState == kWiFiStationState_Connected))
        {
            // Initalize internal interface variables, these can be used by other modules like the DNSSD Impl to
            // get the underlying IP interface
            Inet::InterfaceId tmpExtIf(extNetIfPtr);
            Inet::InterfaceId tmpThrIf(thrNetIfPtr);
            mExternalNetIf    = tmpExtIf;
            mThreadNetIf      = tmpThrIf;
            mBorderRouterInit = true;

            DeviceLayer::ConfigurationMgr().GetPrimaryMACAddress(mac);
            chip::Dnssd::MakeHostName(mHostname, sizeof(mHostname), mac);

            BrInitAppLock(LockThreadStack, UnlockThreadStack);
            BrInitPlatform(ThreadStackMgrImpl().OTInstance(), extNetIfPtr, thrNetIfPtr);
            BrUpdateLwipThrIf();
            BrInitMdnsHost(mHostname);
        }
    }
}

void ConnectivityManagerImpl::LockThreadStack()
{
    ThreadStackMgrImpl().LockThreadStack();
}

void ConnectivityManagerImpl::UnlockThreadStack()
{
    ThreadStackMgrImpl().UnlockThreadStack();
}

Inet::InterfaceId ConnectivityManagerImpl::GetThreadInterface()
{
    return sInstance.mThreadNetIf;
}

Inet::InterfaceId ConnectivityManagerImpl::GetExternalInterface()
{
    return sInstance.mExternalNetIf;
}
#endif // CHIP_ENABLE_OPENTHREAD
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

CHIP_ERROR ConnectivityManagerImpl::ProvisionWiFiNetwork(const char * ssid, uint8_t ssidLen, const char * key, uint8_t keyLen)
{
#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    CHIP_ERROR ret                     = CHIP_NO_ERROR;
    struct wlan_network * pNetworkData = (struct wlan_network *) malloc(sizeof(struct wlan_network));

    VerifyOrExit(pNetworkData != NULL, ret = CHIP_ERROR_NO_MEMORY);
    VerifyOrExit(ssidLen <= IEEEtypes_SSID_SIZE, ret = CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(mWiFiStationState != kWiFiStationState_Connecting, ret = CHIP_ERROR_BUSY);

    // Need to enable the WIFI interface here when Thread is enabled as a secondary network interface. We don't want to enable
    // WIFI from the init phase anymore and we will only do it in case the commissioner is provisioning the device with
    // the WIFI credentials.
    if (mWifiManagerInit == false)
    {
        StartWiFiManagement();
        mWifiManagerInit = true;
    }

    memset(pNetworkData, 0, sizeof(struct wlan_network));

    if (ssidLen < WLAN_NETWORK_NAME_MAX_LENGTH)
    {
        memcpy(pNetworkData->name, ssid, ssidLen);
        pNetworkData->name[ssidLen] = '\0';
    }
    else
    {
        memcpy(pNetworkData->name, ssid, WLAN_NETWORK_NAME_MAX_LENGTH);
        pNetworkData->name[WLAN_NETWORK_NAME_MAX_LENGTH] = '\0';
    }

    memcpy(pNetworkData->ssid, ssid, ssidLen);
    pNetworkData->ip.ipv4.addr_type = ADDR_TYPE_DHCP;
    pNetworkData->ssid_specific     = 1;

    pNetworkData->security.type = WLAN_SECURITY_NONE;

    if (keyLen > 0)
    {
        pNetworkData->security.type = WLAN_SECURITY_WILDCARD;
        if (keyLen <= sizeof(pNetworkData->security.psk))
        {
            /* Needed for WEP, WPA and WPA2 support */
            memcpy(pNetworkData->security.psk, key, keyLen);
            pNetworkData->security.psk_len = keyLen;
        }
        else
        {
            /* set psk len to 0 to avoid connection issues if the key is too long */
            pNetworkData->security.psk_len = 0;
        }
        /* Needed for WPA3 SAE support as the max length of SAE password is larger than max length of WPA-PSK */
        size_t password_actual_copy_len = std::min(static_cast<size_t>(keyLen), sizeof(pNetworkData->security.password));
        memcpy(pNetworkData->security.password, key, password_actual_copy_len);
        pNetworkData->security.password_len = static_cast<uint8_t>(password_actual_copy_len);
    }

    mWifiIsProvisioned = true;
    ConnectNetworkTimerHandler(NULL, (void *) pNetworkData);

exit:
    return ret;
#else
    return CHIP_ERROR_NOT_IMPLEMENTED;
#endif
}

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
void ConnectivityManagerImpl::ConnectNetworkTimerHandler(::chip::System::Layer * aLayer, void * context)
{
    ChipDeviceEvent event;

    /*
     * Make sure to have the Wi-Fi station enabled before scheduling a connect event .
     * Otherwise start a new timer to check again the status later.
     */
    if (ConnectivityMgr().IsWiFiStationEnabled())
    {
        /* Post an event to start the connection asynchronously in the Matter task context */
        event.Type                       = DeviceEventType::kPlatformNxpStartWlanConnectEvent;
        event.Platform.pNetworkDataEvent = (struct wlan_network *) context;
        (void) PlatformMgr().PostEvent(&event);
    }
    else
    {
        /* Post an event to start a delay timer asynchronously in the Matter task context */
        event.Type                       = DeviceEventType::kPlatformNxpStartWlanInitWaitTimerEvent;
        event.Platform.pNetworkDataEvent = (struct wlan_network *) context;
        (void) PlatformMgr().PostEvent(&event);
    }
}

/* Can be used to disconnect from WiFi network.
 */
CHIP_ERROR ConnectivityManagerImpl::_DisconnectNetwork(void)
{
    int ret        = 0;
    CHIP_ERROR err = CHIP_NO_ERROR;

    if (ConnectivityMgrImpl().IsWiFiStationConnected() || (mWiFiStationState == kWiFiStationState_Connecting))
    {
        ChipLogProgress(NetworkProvisioning, "Disconnecting from WiFi network.");

        ret = wlan_disconnect();

        if (ret != WM_SUCCESS)
        {
            ChipLogError(NetworkProvisioning, "Failed to disconnect from network with error: %u", (uint8_t) ret);
            err = CHIP_ERROR_UNEXPECTED_EVENT;
        }
    }
    else
    {
        ChipLogError(NetworkProvisioning, "Error: WiFi not connected!");
        err = CHIP_ERROR_INCORRECT_STATE;
    }

    return err;
}

#if CHIP_CONFIG_ENABLE_ICD_SERVER
CHIP_ERROR ConnectivityManagerImpl::_SetPollingInterval(System::Clock::Milliseconds32 pollingInterval)
{
    /*
     * ToDo: Call API to put device into sleep
     */
    return CHIP_NO_ERROR;
}
#endif // CHIP_CONFIG_ENABLE_ICD_SERVER
#endif

} // namespace DeviceLayer
} // namespace chip
