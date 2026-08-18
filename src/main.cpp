#include <wups.h>
#include <coreinit/title.h>
#include <coreinit/thread.h>
#include <notifications/notifications.h>

#include <thread>

#include "addresses.h"
#include "utils/logger.h"
#include "server.h"

#define RANDO_TID 0x5000010143599

static std::jthread* tcpServerThread;


ON_APPLICATION_START() {
    initLogging();
    NotificationModule_InitLibrary();
    if(OSGetTitleID() != RANDO_TID) {
        DEBUG_FUNCTION_LINE("Launched title is NOT the rando, got instead %lld", OSGetTitleID());
        return;
    }
    
    tcpServerThread = new std::jthread(serverScope);
    OSSetThreadAffinity((OSThread*)tcpServerThread->native_handle(), OS_THREAD_ATTRIB_AFFINITY_CPU2);
}

ON_APPLICATION_REQUESTS_EXIT() {
    if(tcpServerThread){
        tcpServerThread->request_stop();
        tcpServerThread->join();
        delete tcpServerThread;
        tcpServerThread = NULL;
    }

    deinitLogging();
    NotificationModule_DeInitLibrary();

}