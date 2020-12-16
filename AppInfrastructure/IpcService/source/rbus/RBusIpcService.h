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

#include <IpcCommon.h>
#include <IIpcService.h>
#include <IpcVariantList.h>
#include <set>
#include <atomic>
#include <string>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <type_traits>

typedef struct _rbusHandle_t* rbusHandle_t;

class RBusAsyncReplyGetter;

class RBusIpcService : public AI_IPC::IIpcService, public std::enable_shared_from_this<RBusIpcService>
{
public:
    enum BusType
    {
        SessionBus,
        SystemBus
    };

public:
    RBusIpcService(BusType busType, const std::string &serviceName, int defaultTimeoutMs = -1);
    RBusIpcService(const std::string &busAddress, const std::string &serviceName, int defaultTimeoutMs = -1);
    ~RBusIpcService() final;
    bool start() override;
    bool stop() override;
    bool init(const std::string &serviceName, int defaultTimeoutMs);
    void eventLoopThread();
    bool invokeMethod(const AI_IPC::Method &method, const AI_IPC::VariantList &args, AI_IPC::VariantList &replyArgs, int timeoutMs) override;
    std::shared_ptr<AI_IPC::IAsyncReplyGetter> invokeMethod(const AI_IPC::Method &method, const AI_IPC::VariantList &args, int timeoutMs) override;
    bool emitSignal(const AI_IPC::Signal &signal, const AI_IPC::VariantList &args) override;
    std::string registerSignalHandler(const AI_IPC::Signal &signal, const AI_IPC::SignalHandler &handler) override;
    bool unregisterHandler(const std::string &regId) override;
    bool enableMonitor(const std::set<std::string> &matchRules, const AI_IPC::MonitorHandler &handler);
    bool disableMonitor() override;
    bool isServiceAvailable(const std::string &serviceName) const override;
    void flush() override;
    std::string getBusAddress() const override;
    std::string registerMethodHandler(const AI_IPC::Method &method,
                                      const AI_IPC::MethodHandler &handler) override;

private:
    uint64_t mDefaultTimeoutUsecs;

    std::thread mThread;
    rbusHandle_t *mRBus;
    int subscribed1;
    int subscribed2;
    std::atomic<bool> mStarted;

    uint64_t mHandlerTag;

    struct Executor
    {
        uint64_t tag;
        std::function<void()> func;

        Executor(uint64_t t, std::function<void()> &&f)
            : tag(t), func(std::move(f))
        {
        }
    };

    mutable uint64_t mExecCounter;
    uint64_t mLastExecTag;
    int mExecEventFd;
    mutable std::mutex mExecLock;
    mutable std::condition_variable mExecCond;
    mutable std::deque<Executor> mExecQueue;
    int runtime = 30;

    std::map<uint64_t, std::shared_ptr<RBusAsyncReplyGetter>> mCalls;
    std::queue<uint32_t> mReplyIdentifiers;
};
#endif
