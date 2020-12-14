#include "RBusIpcService.h"
#include <Logging.h>
#include <rbus/rbus.h>
#include <rbus-core/rbus_core.h>
#include <systemd/sd-event.h>
#include <IpcCommon.h>
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
    rbusHandle_t bus = nullptr;
    char *tmp;
    tmp = strdup(serviceName.c_str());
    rbusError_t rc = rbus_open(&bus,tmp);
    mRBus = &bus;
    
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
    char *tmp;
    switch (busType){
        case SystemBus:
            tmp = strdup("SYSTEM");
            rc = rbus_open(&bus,tmp);
            break;
        case SessionBus:
            tmp = strdup("SESSION");
            rc = rbus_open(&bus,tmp);
            break;
    }
    if((rc>(rbusError_t)0)||!bus){
        AI_LOG_SYS_FATAL(-rc, "Failed to open connection to rbus");
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
    return true; 
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
    return true;
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
        /*
        rtMessage *msg;
        rtMessage_Create(msg);
        rtMessage_AddString(*msg,"service",method.service.c_str());
        rtMessage_AddString(*msg,"object",method.object.c_str());
        rtMessage_AddString(*msg,"interface",method.interface.c_str());
        rtMessage_AddString(*msg,"name",method.name.c_str());
        */
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
                const char *tmstring = boost::get<std::string>(arg).c_str();
                char *numcp = strdup(tmstring);
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
                const char *numcp = boost::get<std::string>(arg).c_str();
                rbusValue_SetString(curr_val,numcp);
            }
            const char *prmstring = ("param" + std::to_string(inc)).c_str();
            inc++;
            rbusProperty_Init(&curr_prop,prmstring,curr_val);
            rbusObject_SetProperty(obj_for_method_out,curr_prop);
        }
        rbusError_t rc = rbusMethod_Invoke(*mRBus,method.name.c_str(),obj_for_method_in,&obj_for_method_out);
        if(rc>=1){
            AI_LOG_SYS_FATAL(rc,"Failed to invoke");
            return false;
        }
        for(int i=0;i<replyArgs.size();i++){
            std::string s = "param" + i;
            const char *stochar = s.c_str();
            Variant arg = replyArgs[i];
            rbusValue_t rbv;
            rbusValue_Init(&rbv);
            rbv = rbusProperty_GetValue(rbusObject_GetProperty(obj_for_method_out,stochar));
            rbusValue_Retain(rbv);

            if(arg.type()==typeid(bool)){
                arg = rbusValue_GetBoolean(rbv);
            } else if(arg.type()==typeid(int16_t)){
                arg = rbusValue_GetInt16(rbv);
            } else if(arg.type()==typeid(int32_t)){
                arg = rbusValue_GetInt32(rbv);
            } else if(arg.type()==typeid(int64_t)){
                arg = rbusValue_GetInt64(rbv);
            } if(arg.type()==typeid(uint16_t)){
                arg = rbusValue_GetUInt16(rbv);
            } else if(arg.type()==typeid(uint32_t)){
                arg = rbusValue_GetUInt32(rbv);
            } else if(arg.type()==typeid(uint64_t)){
                arg = rbusValue_GetUInt64(rbv);
            } else if(arg.type()==typeid(std::string)){
                arg = rbusValue_GetString(rbv,NULL);
            }
        return true;
    };
    };                               
}
rbusError_t RBusIpcService::eventSubHandler(rbusHandle_t handle, rbusEventSubAction_t action, const char* eventName, rbusFilter_t filter, int32_t interval, bool* autoPublish)
{
    (void)handle;
    (void)filter;
    (void)autoPublish;
    (void)interval;

    int subscribed1 = 0;
    int subscribed2 = 0;
    printf(
        "eventSubHandler called:\n" \
        "\taction=%s\n" \
        "\teventName=%s\n",
        action == RBUS_EVENT_ACTION_SUBSCRIBE ? "subscribe" : "unsubscribe",
        eventName);

    if(!strcmp("Device.Provider1.Event1!", eventName))
    {
        subscribed1 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else if(!strcmp("Device.Provider1.Event2!", eventName))
    {
        subscribed2 = action == RBUS_EVENT_ACTION_SUBSCRIBE ? 1 : 0;
    }
    else
    {
        printf("provider: eventSubHandler unexpected eventName %s\n", eventName);
    }

    return RBUS_ERROR_SUCCESS;
}

rbusMethodHandler_t RBusIpcService::handlerRegistration(rbusHandle_t handle,char const* methodName,rbusObject_t inParams,rbusObject_t outParams,rbusMethodAsyncHandle_t asyncHandle)
{
    if(strcmp(methodName, "DOBBY_CTRL_METHOD_START_FROM_BUNDLE") == 0)
    {

    }
    else if(strcmp(methodName, "DOBBY_CTRL_METHOD_STOP") == 0)
    {

    }
}
std::string RBusIpcService::registerMethodHandler(const Method &method,
                                                  const MethodHandler &handler)
{
    rbusObject_t outParams;
    rbusObject_Init(&outParams,"outparameters");
    rbusMethodHandler_t mhandler = handlerRegistration(*mRBus,method.name.c_str(),NULL,outParams,NULL);
    char *tmp = strdup(method.name.c_str());
    rbusDataElement_t dataElements[1] ={
        {tmp, RBUS_ELEMENT_TYPE_METHOD, {NULL, NULL, NULL, NULL, NULL, mhandler}}
    };

    //register methods like in the methodprovider???

    
    rbusError_t rc = rbus_regDataElements(*mRBus,1,dataElements);
    return "RBUS_ERROR_SUCCESS";
    //maybe this isn't even needed because of rbus structure.
    /*std::string tag;
    std::function<void()> registerLambda =
        [&]()
        {
            bool existing = false;
            r_bus_slot *objSlot = nullptr;
            for(const auto &it : mMethodHandlers)
            {
                const RegisteredMethod &regMethod = it.second;
                if(regMethod.name == method.name)
                {
                    existing=true;
                    break;
                }
            }

            if (existing){
                AI_LOG_WARN("Already have method:%s registered",method.name.c_str());
                return;
            }
        };
    */
    
}                                                  

//probably not needed?
void RBusIpcService::eventLoopThread()
{
    AI_LOG_WARN("EVENT LOOP ATTEMPTED RUN");
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