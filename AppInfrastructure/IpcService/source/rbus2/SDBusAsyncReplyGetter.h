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
//  SDBusAsyncReplyGetter.h
//  IpcService
//
//

#ifndef SDBUSASYNCREPLYGETTER_H
#define SDBUSASYNCREPLYGETTER_H

#include <IpcCommon.h>

#include <mutex>
#include <condition_variable>


class SDBusAsyncReplyGetter : public AI_IPC::IAsyncReplyGetter
{
public:
    SDBusAsyncReplyGetter();
    ~SDBusAsyncReplyGetter() final = default;

    bool getReply(AI_IPC::VariantList& argList) override;
    void setReply(bool succeeded, AI_IPC::VariantList &&argList);

private:
    bool mFinished;
    bool mSucceeded;
    AI_IPC::VariantList mArgs;
    std::mutex mLock;
    std::condition_variable mCond;
};


#endif // SDBUSASYNCREPLYGETTER_H
