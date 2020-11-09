/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2019 Sky UK
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
//
//  SDBusIpcService.h
//  IpcService
//
//

#ifndef RBUSIPCSERVICE_H
#define RBUSIPCSERVICE_H

#include "IpcCommon.h"
#include "IIpcService.h"

#include <set>
#include <atomic>
#include <string>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>

typedef struct r_bus r_bus;
typedef struct r_bus_slot r_bus_slot;
typedef struct r_bus_message r_bus_message;
typedef struct r_event_source r_event_source;

class RBusAsyncReplyGetter;

class RBusIpcService : public AI_IPC::IIpcService
                      , public std::enable_shared_from_this<RBusIpcService>
{
    public:
        enum BusType
        {
            SessionBus,
            SystemBus
        };

    public:

        RBusIpcService(BusType busType, const std::string& serviceName,int defaultTimeoutMs = -1);
        RBusIpcService(const std::string &busAddress, const std::string& serviceName, int defaultTimeoutMs = -1);
        ~RBusIpcService() final;
        bool start() override;
        bool stop() override;

    private:
    uint64_t mDefaultTimeoutUsecs;

    std::thread mThread;
    r_bus *mRBus;

    std::atomic<bool> mStarted;

    struct RegisteredMethod
    {
        r_bus_slot *objectSlot;
        std::string path;
        std::string interface;
        std::string name;
        AI_IPC::MethodHandler callback;

        RegisteredMethod(r_bus_slot *slot,
                         const AI_IPC::Method &method,
                         const AI_IPC::MethodHandler &handler)
            : objectSlot(slot)
            , path(method.object), interface(method.interface), name(method.name)
            , callback(handler)
        { }
    };

    struct RegisteredSignal
    {
        r_bus_slot *matchSlot;
        AI_IPC::SignalHandler callback;

        RegisteredSignal(r_bus_slot *slot,
                         const AI_IPC::SignalHandler &handler)
            : matchSlot(slot)
            , callback(handler)
        { }
    };

    uint64_t mHandlerTag;
    std::map<std::string, RegisteredMethod> mMethodHandlers;
    std::map<std::string, RegisteredSignal> mSignalHandlers;

    struct Executor
    {
        uint64_t tag;
        std::function<void()> func;

        Executor(uint64_t t, std::function<void()> &&f)
            : tag(t), func(std::move(f))
        { }
    };

    mutable uint64_t mExecCounter;
    uint64_t mLastExecTag;
    int mExecEventFd;
    mutable std::mutex mExecLock;
    mutable std::condition_variable mExecCond;
    mutable std::deque< Executor > mExecQueue;

    std::map< uint64_t, std::shared_ptr<RBusAsyncReplyGetter> > mCalls;
    std::map< uint32_t, r_bus_message* > mCallReplies;
    std::queue<uint32_t> mReplyIdentifiers;
};
#endif

