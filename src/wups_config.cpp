#include <wups.h>
#include "utils/logger.h"
#include "version.h"

#include <wups/config/WUPSConfigItemIPAddress.h>
#include <wups/config/WUPSConfigItemIntegerRange.h>
#include <wups/config/WUPSConfigItemMultipleValues.h>
#include <wups/config/WUPSConfigItemStub.h>
#include <nn/ac.h>

WUPS_PLUGIN_NAME("TWWHD AP Helper");
WUPS_PLUGIN_DESCRIPTION("Helper socket to link the pc client to the wii u");
WUPS_PLUGIN_VERSION(VERSION);
WUPS_PLUGIN_AUTHOR("Teotia444");
WUPS_PLUGIN_LICENSE("idk");


WUPS_USE_WUT_DEVOPTAB();                // Use the wut devoptabs

#define IP_ADDR_ID "wwhd-ap-helper_ip_address"

int ipAddress = 0x2a2a2a2a;
void ipAddressItemChangedConfig(ConfigItemIPAddress *item, uint32_t newValue) {
    if (std::string_view(IP_ADDR_ID) == item->identifier) {
        ipAddress = newValue;
    }
}

WUPSConfigAPICallbackStatus ConfigMenuOpenedCallback(WUPSConfigCategoryHandle rootHandle) {
    WUPSConfigCategory root = WUPSConfigCategory(rootHandle);
    try {
        uint32_t hostIpAddress = 0;
        nn::ac::GetAssignedAddress(&hostIpAddress);
        char ipSettings[50];
        if (hostIpAddress != 0)
        {
            snprintf(ipSettings,
                     50,
                     "IP Address of this console: %u.%u.%u.%u",
                     (hostIpAddress >> 24) & 0xFF,
                     (hostIpAddress >> 16) & 0xFF,
                     (hostIpAddress >> 8) & 0xFF,
                     (hostIpAddress >> 0) & 0xFF);
        }
        else
        {
            snprintf(
                ipSettings, sizeof(ipSettings), "The console is not connected to a network.");
        }
        root.add(WUPSConfigItemStub::Create(ipSettings));
        std::string version_date = "Version Date: ";
        version_date += VERSION_DATE;
        root.add(WUPSConfigItemStub::Create(version_date));
        std::string version_tag = "Version Tag: ";
        version_tag += VERSION;
        root.add(WUPSConfigItemStub::Create(version_tag));

    } catch (std::exception &e) {
        return WUPSCONFIG_API_CALLBACK_RESULT_ERROR;
    }
    return WUPSCONFIG_API_CALLBACK_RESULT_SUCCESS;
}

void ConfigMenuClosedCallback() {
    WUPSStorageAPI::SaveStorage();
}

/**
    Gets called ONCE when the plugin was loaded.
**/
INITIALIZE_PLUGIN() {
    WUPSConfigAPIOptionsV1 configOptions = {.name = "TWWHD AP Helper"};
    if (WUPSConfigAPI_Init(configOptions, ConfigMenuOpenedCallback, ConfigMenuClosedCallback) != WUPSCONFIG_API_RESULT_SUCCESS) {
        DEBUG_FUNCTION_LINE_ERR("Failed to init config api");
    }
}