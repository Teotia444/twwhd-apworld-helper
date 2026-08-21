#include <coreinit/thread.h>
#include <thread>
#include <notifications/notifications.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>
#include <nn/ac.h>

#include "server.h"
#include "utils/logger.h"
#include "addresses.h"
#include "commands.h"

int client_fd, server_fd;
struct sockaddr_in server_addr;

int server_setup() {
    uint32_t ip;
    ACGetAssignedAddress(&ip);
	if (ip == 0) {
        DEBUG_FUNCTION_LINE_ERR("The console is not connected to a network");
		return 1;
	}
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(3599);
	server_addr.sin_addr.s_addr = INADDR_ANY;
	while (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		OSSleepTicks(OSMillisecondsToTicks(100));
	}
	listen(server_fd, 1);
	return 0;
}

int accept_conn() {
	return (client_fd = accept(server_fd, NULL, NULL));
}

int read_msg(char *buf, int size) {
	memset(buf, 0, size);
	return recv(client_fd, buf, size, 0);
}

bool send_all_msg(const void* data, int size){
    const char* buf = static_cast<const char*>(data);
    while(size>0){
        int sent = send(client_fd, (char*)buf, size, 0);
        if(sent <= 0) return false;
        buf += sent;
        size -= sent;
    }
    return true;
}

int send_msg(char *buf, int size) {
    if(size < 0) return false;
    int len = htonl(size);
    // start with the 4 byte header
    if(!send_all_msg(&len, sizeof(len))) return -1;
    if(!send_all_msg(buf, size)) return -2;

	return 0;
}

void serverTakedown() {
	if (client_fd > 0) {
		close(client_fd);
	}
	close(server_fd);
}

int build_payload(char *buf){
    return snprintf(buf, 256, "hello this is a dumb payload");
}

void serverScope(std::stop_token stop){
    char payload[4096] = {'{', 0};
    char client_msg[256];
    int client_len;
	int payload_len = 1;
    struct pollfd pfd;
    if(server_setup() != 0) return;

    while (!stop.stop_requested()){
        uint32_t hostIpAddress = 0;
        nn::ac::GetAssignedAddress(&hostIpAddress);
        if(hostIpAddress == 0) break;

        pfd = {
            .fd = server_fd,
            .events = POLLIN
        };
        if(poll(&pfd, 1, 100) <= 0) continue;

        if (accept_conn() < 0) {
			serverTakedown();
			server_setup();
			continue;
		}
        pfd = {
            .fd = client_fd,
            .events = POLLIN
        };
        while (!stop.stop_requested()) {
            hostIpAddress = 0;
            nn::ac::GetAssignedAddress(&hostIpAddress);
            if(hostIpAddress == 0) break;

            if(poll(&pfd, 1, 100) <= 0) continue;

            if ((client_len = read_msg(client_msg, 256)) <= 0) {
				break;
			}
            
            switch (client_msg[0]) {
            case 'g':
                DEBUG_FUNCTION_LINE("Give command");
                payload_len += giveItem(payload, payload_len, client_msg);
                break;
            case 'u':
                DEBUG_FUNCTION_LINE("Update command");
                payload_len += updateStatus(payload, payload_len, client_msg);
                break;
            case 'h':
                DEBUG_FUNCTION_LINE("Health set command");
                payload_len += setHealth(payload, payload_len, client_msg);
                break;
            case 'i':
                DEBUG_FUNCTION_LINE("Increment index addr command");
                payload_len += incrIdx(payload, payload_len, client_msg);
                break;
            case 'n':
                DEBUG_FUNCTION_LINE("Notification command");
                payload_len += addNotification(payload, payload_len, client_msg);
                break;
            default:
                DEBUG_FUNCTION_LINE("Unknown command : %c", client_msg[0]);
                payload_len += snprintf(&payload[payload_len], 256, "\"command\": \"unknown\", \"result\": 0");
                break;
            }

            if (payload_len > 1) {
				payload[payload_len] = '}';
				int ret = send_msg(payload, payload_len+1);
				memset(&payload[1], 0, sizeof(payload) - 1);
				payload_len = 1;
				if (ret < 0) {
					break;
				}
			}
        }
    }
    serverTakedown();
    if(!stop.stop_requested()) {
        NotificationModule_AddInfoNotification("Warning: lost connection to your wifi network. Restart your Wii U!");
    }
}
