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

    
}
RBusIpcService::RBusIpcService(BusType busType, 
                            const std::string& serviceName,
                            int defaultTimeoutMs)
    :mDefaultTimeoutUsecs(25*1000*1000)
    , mRBus(nullptr)
    , mStarted(false)
    , mHandlerTag(1)
    , mExecCounter(1)
    , mLastExecTag(0)
    , mExecEventFd(-1)
{
    rbusHandle_t bus = nullptr;
    rbusError_t rc = (rbusError_t)1;
    switch (busType){
        case SystemBus:
            rc = rbus_open(&bus,"SYSTEM");
            break;
        case SessionBus:
            rc = rbus_open(&bus,"SESSION");
            break;
    }
    if((rc>(rbusError_t)0)||!bus){
        AI_LOG_SYS_FATAL(-rc, "Failed to open connection to rbus")
        return;
    }

    mRBus=&bus;
    //do you need to delete?
}
RBusIpcService::~RBusIpcService()
{
    if(mRBus){
        rbus_close(*mRBus);
    }
    if ((mExecEventFd >= 0) && (close(mExecEventFd) != 0))
    {
        AI_LOG_SYS_ERROR(errno, "failed to close eventfd");
    }
}
    
bool RBusIpcService::start()
{  
}

bool RBusIpcService::init(const std::string &serviceName, int defaultTimeoutMs)
{    
    if (defaultTimeoutMs <= 0)
        mDefaultTimeoutUsecs = (25 * 1000 * 1000);
    else
        mDefaultTimeoutUsecs = (defaultTimeoutMs * 1000);
    
    mExecEventFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (mExecEventFd < 0)
    {
        AI_LOG_SYS_ERROR(errno, "failed to created eventfd");
        return false;
    }
}

bool RBusIpcService::invokeMethod(const Method &method,
                                  const VariantList &args,
                                  VariantList &replyArgs,
                                  int timeoutMs)
{
    uint64_t timeoutUsecs;
    if(timeoutMs<0)
        timeoutUsecs=mDefaultTimeoutUsecs;
    else 
        timeoutUsecs = (timeoutMs*1000);
    
    replyArgs.clear();

    bool success = false;
    std::function<void()> methodCallLambda = [&]()
    {
        rtMessage *msg;
        rtMessage_Create(msg);
        rtMessage_AddString(*msg,"service",method.service.c_str());
        rtMessage_AddString(*msg,"object",method.object.c_str());
        rtMessage_AddString(*msg,"interface",method.interface.c_str());
        rtMessage_AddString(*msg,"name",method.name.c_str());
        
        VariantList temp_args = args;
        rbusObject_t obj_for_method_in;
        rbusObject_Init(&obj_for_method_in,"MethodObjectIn");
        rbusObject_t obj_for_method_out;
        rbusObject_Init(&obj_for_method_in,"MethodObjectOut");
        int inc = 0;
        for(Variant arg:args){
            rbusProperty_t curr_prop;
            rbusValue_t curr_val;
            rbusValue_Init(&curr_val);
            if(arg.type()==typeid(bool)){
                bool numcp = boost::get<bool>(arg);
                rbusValue_SetBoolean(curr_val,numcp);
            } else if(arg.type()==typeid(int16_t)){
                int16_t numcp = boost::get<int16_t>(arg);
                rbusValue_SetInt16(curr_val,numcp);
            } else if(arg.type()==typeid(int32_t)){
                int32_t numcp = boost::get<int32_t>(arg);
                rbusValue_SetInt32(curr_val,numcp);
            } else if(arg.type()==typeid(int64_t)){
                int64_t numcp = boost::get<int64_t>(arg);
                rbusValue_SetInt64(curr_val,numcp);
            } if(arg.type()==typeid(uint16_t)){
                uint16_t numcp = boost::get<uint16_t>(arg);
                rbusValue_SetUInt16(curr_val,numcp);
            } else if(arg.type()==typeid(uint32_t)){
                uint32_t numcp = boost::get<uint32_t>(arg);
                rbusValue_SetUInt32(curr_val,numcp);
            } else if(arg.type()==typeid(uint64_t)){
                uint64_t numcp = boost::get<uint64_t>(arg);
                rbusValue_SetUInt64(curr_val,numcp);
            } else if(arg.type()==typeid(std::string)){
                const char *numcp = boost::get<char *>(arg);
                rbusValue_SetString(curr_val,numcp);
            }
            const char *prmstring = ("param" + std::to_string(inc)).c_str();
            inc++;
            rbusProperty_Init(&curr_prop,prmstring,curr_val);
            rbusObject_SetProperty(obj_for_method_in,curr_prop);
        }

        for(Variant arg:replyArgs){
            rbusProperty_t curr_prop;
            rbusValue_t curr_val;
            rbusValue_Init(&curr_val);
            if(arg.type()==typeid(bool)){
                bool numcp = boost::get<bool>(arg);
                rbusValue_SetBoolean(curr_val,numcp);
            } else if(arg.type()==typeid(int16_t)){
                int16_t numcp = boost::get<int16_t>(arg);
                rbusValue_SetInt16(curr_val,numcp);
            } else if(arg.type()==typeid(int32_t)){
                int32_t numcp = boost::get<int32_t>(arg);
                rbusValue_SetInt32(curr_val,numcp);
            } else if(arg.type()==typeid(int64_t)){
                int64_t numcp = boost::get<int64_t>(arg);
                rbusValue_SetInt64(curr_val,numcp);
            } if(arg.type()==typeid(uint16_t)){
                uint16_t numcp = boost::get<uint16_t>(arg);
                rbusValue_SetUInt16(curr_val,numcp);
            } else if(arg.type()==typeid(uint32_t)){
                uint32_t numcp = boost::get<uint32_t>(arg);
                rbusValue_SetUInt32(curr_val,numcp);
            } else if(arg.type()==typeid(uint64_t)){
                uint64_t numcp = boost::get<uint64_t>(arg);
                rbusValue_SetUInt64(curr_val,numcp);
            } else if(arg.type()==typeid(std::string)){
                const char *numcp = boost::get<char *>(arg);
                rbusValue_SetString(curr_val,numcp);
            }
            const char *prmstring = ("param" + std::to_string(inc)).c_str();
            inc++;
            rbusProperty_Init(&curr_prop,prmstring,curr_val);
            rbusObject_SetProperty(obj_for_method_out,curr_prop);
        }
        rbusError_t rc = rbusMethod_Invoke(*mRBus,method.name.c_str(),obj_for_method_in,&obj_for_method_out);
        if(rc>=1){
            AI_LOG_SYS_FATAL("Failed to invoke")
            return false;
        }
        return true;
    };
}                                  

//probably not needed?
void RBusIpcService::eventLoopThread()
{
    AI_LOG_WARN("EVENT LOOP ATTEMPTED RUN")
}

/* create a new bus, set it's address then open it
    //rbusHandle_t *bus = nullptr;
    //char *temp;
    strcpy(temp,serviceName.c_str());
    int rc =  rbus_open(bus,temp);
    if ((rc < 0) || !bus)
    {
        AI_LOG_SYS_FATAL(-rc, "failed to create sd-bus object");
        return;
    };
    

    // start the bus
    //this starts the software process on the bus. Need to start bus
    /*(probably don't need to start the bus)rc = start(); 
    if (rc < 0)
    {
        AI_LOG_SYS_FATAL(-rc, "failed to start the bus");
        return;
    }

    // store the open connection
    mRBus = bus;

    // initialise the reset and register ourselves on the bus
    if (!init(serviceName, defaultTimeoutMs))
    {
        AI_LOG_FATAL("failed to init object");
        return;
    }*/

    /*AI_LOG_INFO("Start rbus event loop thread")
    //probably don't need to set name of event loop thread
    pthread_setname_np(pthread_self(),"AI_IPC_RBUS");
    rbusObject_t *loop = nullptr;
    int rc = rbusEventHandler_t*/

    /*
    if (defaultTimeoutMs<=0)
        mDefaultTimeoutUsecs = (25*1000*1000);
    else
        mDefaultTimeoutUsecs = (defaultTimeoutMs * 1000);
    
    mExecEventFd = eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(mExecEventFd < 0){
        AI_LOG_SYS_ERROR(errno,"Failed to create EventFD")
    }
    if(!serviceName.empty())
    {
        //might need to initialize event service?
        //an option
        //rbus_publishEvent
        //The below says that it replaced register event for handling, but
        //When used, these disable the rbuscore built-in server-side event subscription handling.
        //rbus_registerSubscribeHandler
        int rc = rbus_registerEvent(serviceName,);
        if (rc < 0)
        {
            AI_LOG_SYS_ERROR(-rc, "failed to register service name '%s' on bus",
                             serviceName.c_str());
        }
    }
    mThread = std::thread(&RBusIpcService::eventLoopThread,this);
    return mThread.joinable();*/

    /*if (!mRBus)
    {
        AI_LOG_ERROR("No valid Rbus object");
        return false;
    }*/