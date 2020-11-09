#include "RBusIpcService.h"
#include <Logging.h>
#include <rbus/rbus.h>
#include <rbus-core/rbus_core.h>
#include <systemd/sd-event.h>

#include <sstream>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/eventfd.h>
using namespace AI_IPC;

RBusIpcService::RBusIpcService(const std::string &busAddress,
                                 const std::string& serviceName,
                                 int defaultTimeoutMs)
    : mDefaultTimeoutUsecs(25 * 1000 * 1000)
    , mRBus(nullptr)
    , mStarted(false)
    , mHandlerTag(1)
    , mExecCounter(1)
    , mLastExecTag(0)
    , mExecEventFd(-1)
{

    // create a new bus, set it's address then open it
    r_bus *bus = nullptr;
    int rc = r_bus_new(&bus);
    if ((rc < 0) || !bus)
    {
        AI_LOG_SYS_FATAL(-rc, "failed to create sd-bus object");
        return;
    }

    rc = sd_bus_set_address(bus, busAddress.c_str());
    if (rc < 0)
    {
        AI_LOG_SYS_FATAL(-rc, "failed to create sd-bus object");
        return;
    }

    // set some boilerplate stuff for the connection
    sd_bus_set_bus_client(bus, true);
    sd_bus_set_trusted(bus, false);
    sd_bus_negotiate_creds(bus, true, (SD_BUS_CREDS_UID | SD_BUS_CREDS_EUID |
                                       SD_BUS_CREDS_EFFECTIVE_CAPS));

    // start the bus
    rc = sd_bus_start(bus);
    if (rc < 0)
    {
        AI_LOG_SYS_FATAL(-rc, "failed to start the bus");
        return;
    }

    // store the open connection
    mSDBus = bus;

    // initialise the reset and register ourselves on the bus
    if (!init(serviceName, defaultTimeoutMs))
    {
        AI_LOG_FATAL("failed to init object");
        return;
    }
}

    
bool RBusIpcService::start()
{
    if (!mRBus)
    {
        AI_LOG_ERROR("No valid Rbus object");
        return false;
    }
}