#include "commands.h"
#include "addresses.h"
#include "utils/logger.h"

#include <stdint.h>
#include <cstdio>
#include <string.h>
#include <string>
#include <cstdlib>
#include <queue>
#include <array>


#include <notifications/notifications.h>

std::queue<std::string> currentlyLoadedNotifications;
std::queue<std::string> toBeLoadedNotifications;


int setHealth(char* payload, int payload_len, char client_msg[256]){
    int res = 0;
    char stage_name[8];
    snprintf(stage_name, 8, "%s", (char*)CURR_STAGE_NAME_ADDR);
    DEBUG_FUNCTION_LINE("current stage is %s", stage_name);

    if(strcmp(stage_name, "") == 0 || strcmp(stage_name, "sea_T") == 0 || strcmp(stage_name, "Name") == 0) res = 1;
    if(res == 0){
        uint16_t value = atoi(&client_msg[1]);
        DEBUG_FUNCTION_LINE("set health to %d", value);

        *((uint16_t*)CURR_HEALTH_ADDR) = value;
    }

    return snprintf(&payload[payload_len], 256, "\"command\": \"setHealth\", \"result\": %d", res);
}

int giveItem(char* payload, int payload_len, char client_msg[256]){
    int res = 0;
    if(*((uint8_t*)(DATA_OFFSET + DCOMIFG_DATA_INIT)) != 1) res = 1;
    
    char stage_name[8];
    snprintf(stage_name, 8, "%s", (char*)CURR_STAGE_NAME_ADDR);
    DEBUG_FUNCTION_LINE("current stage is %s", stage_name);
    if(strcmp(stage_name, "") == 0 || strcmp(stage_name, "sea_T") == 0 || strcmp(stage_name, "Name") == 0) res = 2;
    if(*((uint8_t*)(DATA_OFFSET + DCOMIFG_ITEM_BYTE)) != 0xFF) res = 3;


    if(res != 1){
        uint8_t value = atoi(&client_msg[1]);
        *((uint8_t*)(DATA_OFFSET + DCOMIFG_ITEM_BYTE)) = value;
        DEBUG_FUNCTION_LINE("recieved item id 0x%02X", value);
    }

    return snprintf(&payload[payload_len], 256, "\"command\": \"giveItem\", \"result\": %d", res);
}

int updateStatus(char* payload, int payload_len, char client_msg[256]){
    /*
        update: current flags bitfield/, all stages bitfield/, chart bitfield/, story flag/, big octos stuff/, mmode/, curr stage/, health/, item get val ?/, expected idx /
        delivery bag content/, mail flags/
    */

    int res = 0;
    std::vector<std::array<uint8_t,4>> chest_bitfields;
    std::vector<std::array<uint8_t,0x10>> switches_bitfields;
    std::vector<std::array<uint8_t,4>> pickups_bitfields;

    std::array<uint8_t,4> curr_stage_chests_bitfield;
    std::array<uint8_t,0x10> curr_stage_switches_bitfield;
    std::array<uint8_t,4> curr_stage_pickups_bitfield;
    
    std::array<uint8_t,8> charts_bitfield;
    
    std::array<uint8_t,0xff> story_flags;
    std::array<uint8_t,100> octo_flags; //capture 100 bytes before the story flags as those are used by big octo flags

    uint8_t mMode;
    uint8_t curr_stage_idx;
    char stage_name[8];
    uint16_t health;
    uint8_t itemGetVal;
    uint16_t expectedIdx;
    std::array<uint8_t,8> dBag_content;
    std::array<uint8_t,8> dBag_flags;


    for (int i = 0; i < 0xE; i++) {
        auto* temp_chest_bitfield = reinterpret_cast<const std::array<uint8_t, 4>*>(BASE_CHESTS_BITFLD_ADDR + (0x24*i));
        auto* temp_switches_bitfield = reinterpret_cast<const std::array<uint8_t, 0x10>*>(BASE_SWITCHES_BITFLD_ADDR + (0x24*i));
        auto* temp_pickups_bitfield = reinterpret_cast<const std::array<uint8_t, 4>*>(BASE_PICKUPS_BITFLD_ADDR + (0x24*i));
        chest_bitfields.push_back(*temp_chest_bitfield);
        switches_bitfields.push_back(*temp_switches_bitfield);
        pickups_bitfields.push_back(*temp_pickups_bitfield);
    }

    curr_stage_chests_bitfield = *reinterpret_cast<std::array<uint8_t,4>*>(CURR_STAGE_CHESTS_BITFLD_ADDR);
    curr_stage_switches_bitfield = *reinterpret_cast<std::array<uint8_t,0x10>*>(CURR_STAGE_SWITCHES_BITFLD_ADDR);
    curr_stage_pickups_bitfield = *reinterpret_cast<std::array<uint8_t,4>*>(CURR_STAGE_PICKUPS_BITFLD_ADDR);

    charts_bitfield = *reinterpret_cast<std::array<uint8_t,8>*>(CHARTS_BITFLD_ADDR);
    story_flags = *reinterpret_cast<std::array<uint8_t,0xff>*>(STORY_FLAGS_BASE_ADDR);
    octo_flags = *reinterpret_cast<std::array<uint8_t,100>*>(STORY_FLAGS_BASE_ADDR - 100);
    
    mMode = *((uint8_t*)DCOMIFG_M_MODE);
    curr_stage_idx = *((uint8_t*)CURR_STAGE_ID_ADDR);
    snprintf(stage_name, 8, "%s", (char*)CURR_STAGE_NAME_ADDR);
    health = *((uint16_t*)CURR_HEALTH_ADDR);
    itemGetVal = *((uint8_t*)(DATA_OFFSET + DCOMIFG_ITEM_BYTE));
    expectedIdx = *((uint8_t*)(EXPECTED_INDEX_ADDR));
    dBag_content = *reinterpret_cast<std::array<uint8_t,8>*>(LETTER_BASE_ADDR);
    dBag_flags = *reinterpret_cast<std::array<uint8_t,8>*>(LETTER_OWND_ADDR);

#pragma region Json building
    std::string client_output = "\"command\": \"update\", \"data\": {";
    client_output += "\"chest_bitfields\": [";
    for(int i = 0; i<0xE; i++){
        if(i > 0) client_output += ", ";
        client_output += "[";
        for(int j = 0; j<4; j++){
            if(j>0) client_output += ", ";
            client_output += std::to_string(chest_bitfields[i][j]);
        }
        client_output += "]";
    }

    client_output += "], \"switches_bitfields\": [";
    for(int i = 0; i<0xE; i++){
        if(i > 0) client_output += ", ";
        client_output += "[";
        for(int j = 0; j<0x10; j++){
            if(j>0) client_output += ", ";
            client_output += std::to_string(switches_bitfields[i][j]);
        }
        client_output += "]";
    }

    client_output += "], \"pickups_bitfields\": [";
    for(int i = 0; i<0xE; i++){
        if(i > 0) client_output += ", ";
        client_output += "[";
        for(int j = 0; j<4; j++){
            if(j>0) client_output += ", ";
            client_output += std::to_string(pickups_bitfields[i][j]);
        }
        client_output += "]";
    }

    client_output += "], \"current_chest_bitfield\": [";
    for(int j = 0; j<4; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(curr_stage_chests_bitfield[j]);
    }
    client_output += "]";

    client_output += ", \"current_switch_bitfield\": [";
    for(int j = 0; j<0x10; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(curr_stage_switches_bitfield[j]);
    }
    client_output += "]";

    client_output += ", \"current_pickup_bitfield\": [";
    for(int j = 0; j<4; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(curr_stage_pickups_bitfield[j]);
    }
    client_output += "]";

    client_output += ", \"charts_bitfield\": [";
    for(int j = 0; j<8; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(charts_bitfield[j]);
    }
    client_output += "]";

    client_output += ", \"story_flags\": [";
    for(int j = 0; j<0xff; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(story_flags[j]);
    }
    client_output += "]";

    client_output += ", \"octo_flags\": [";
    for(int j = 0; j<100; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(octo_flags[j]);
    }
    client_output += "]";

    client_output += ", \"mMode\": " + std::to_string(mMode);
    client_output += ", \"curr_stage_idx\": " + std::to_string(curr_stage_idx);
    client_output += ", \"curr_stage_name\": \"";
    client_output += stage_name;
    client_output += "\", \"health\": " + std::to_string(health);
    client_output += ", \"itemGetVal\": " + std::to_string(itemGetVal);
    client_output += ", \"expectedIdx\": " + std::to_string(expectedIdx);
    client_output += ", \"dBag_content\": [";
    for(int j = 0; j<8; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(dBag_content[j]);
    }
    client_output += "]";
    client_output += ", \"dBag_flags\": [";
    for(int j = 0; j<8; j++){
        if(j>0) client_output += ", ";
        client_output += std::to_string(dBag_flags[j]);
    }
    client_output += "]}, \"result\": " + std::to_string(res);

#pragma endregion
    

    DEBUG_FUNCTION_LINE("payload size: %d", client_output.size());


    return snprintf(&payload[payload_len], 4096, client_output.c_str());
}

int incrIdx(char* payload, int payload_len, char client_msg[256]){
    int res = 0;
    if(*((uint8_t*)(DATA_OFFSET + DCOMIFG_DATA_INIT)) != 1) res = 1;

    if(res != 1){
        uint8_t value = atoi(&client_msg[1]);
        *((uint8_t*)(EXPECTED_INDEX_ADDR)) += value;
        DEBUG_FUNCTION_LINE("expected index was incremented by 0x%02X", value);
    }

    return snprintf(&payload[payload_len], 256, "\"command\": \"incrIdx\", \"result\": %d", res);
}

void notificationCallbackHandler(NotificationModuleHandle handle, void* args){
    DEBUG_FUNCTION_LINE("popping notification %s", currentlyLoadedNotifications.front().c_str());
    currentlyLoadedNotifications.pop();
    if(!toBeLoadedNotifications.empty()){
        std::string nextNotification = toBeLoadedNotifications.front();
        toBeLoadedNotifications.pop();
        DEBUG_FUNCTION_LINE("adding notification %s", nextNotification.c_str());
        currentlyLoadedNotifications.emplace(nextNotification);
        NotificationModule_AddInfoNotificationWithCallback(nextNotification.c_str(), notificationCallbackHandler, NULL);
    }
}

int addNotification(char* payload, int payload_len, char client_msg[256]){
    int res = 0;
    std::string message = &client_msg[1];

    if(currentlyLoadedNotifications.size() < 10) {
        currentlyLoadedNotifications.emplace(message);
        res = NotificationModule_AddInfoNotificationWithCallback(&client_msg[1], notificationCallbackHandler, NULL);
    }
    else{
        toBeLoadedNotifications.emplace(message);
    }
    
    DEBUG_FUNCTION_LINE("currently loaded notifications size: %d, to be loaded: %d, display message: %s", currentlyLoadedNotifications.size(), toBeLoadedNotifications.size(), message.c_str());
    return snprintf(&payload[payload_len], 256, "\"command\": \"addNotification\", \"result\": %d", res);
}

