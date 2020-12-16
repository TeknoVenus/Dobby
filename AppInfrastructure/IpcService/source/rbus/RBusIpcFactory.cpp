#include "IpcCommon.h"
#include "IpcFactory.h"
#include "RBusIpcService.h"

#include <Logging.h>

#include <set>
#include <string>

std::shared_ptr<AI_IPC::IIpcService> AI_IPC::createIpcService(const std::string& address,
                                                              const std::string& serviceName,
                                                              int defaultTimeoutMs)
{
    return std::make_shared<RBusIpcService>(address,serviceName,defaultTimeoutMs);
}
std::shared_ptr<AI_IPC::IIpcService> AI_IPC::createSystemBusIpcService(const std::string& serviceName,
                                                                       int defaultTimeoutMs)
{
    return std::make_shared<RBusIpcService>(RBusIpcService::SystemBus, serviceName, defaultTimeoutMs);
}