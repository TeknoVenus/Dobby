/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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
/*
 * File:   OpenCDMPlugin.cpp
 *
 * Copyright (C) Sky UK 2019
 */
#include "OpenCDMPlugin.h"

#include <Logging.h>

#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

#include <fstream>


// -----------------------------------------------------------------------------
/**
 *  @brief Registers the main plugin object.
 *
 *  The object is constructed at the start of the daemon and only destructed
 *  when the daemon is shutting down.
 *
 */
REGISTER_DOBBY_PLUGIN(OpenCDMPlugin);



OpenCDMPlugin::OpenCDMPlugin(const std::shared_ptr<IDobbyEnv> &env,
                             const std::shared_ptr<IDobbyUtils> &utils)
    : mName("OpenCDM")
    , mUtilities(utils)
{
    AI_LOG_FN_ENTRY();

    // Check that we can write to the temp directory
    if (access("/tmp", W_OK) != 0)
    {
        AI_LOG_SYS_ERROR(errno, "Cannot access /tmp directory");
    }

    AI_LOG_FN_EXIT();
}

OpenCDMPlugin::~OpenCDMPlugin()
{
    AI_LOG_FN_ENTRY();

    AI_LOG_FN_EXIT();
}

// -----------------------------------------------------------------------------
/**
 *  @brief Boilerplate that just returns the name of the hook
 *
 *  This string needs to match the name specified in the container spec json.
 *
 */
std::string OpenCDMPlugin::name() const
{
    return mName;
}

// -----------------------------------------------------------------------------
/**
 *  @brief Indicates which hook points we want and whether to run the
 *  asynchronously or synchronously with the other hooks
 *
 *  For Netflix, the mounts should be created preStart
 */
unsigned OpenCDMPlugin::hookHints() const
{
    return (IDobbyPlugin::PostConstructionSync);
}

// -----------------------------------------------------------------------------
/**
 * @brief Creates the required temp files for WPE browser to launch and decrypt
 * content. The files are created in the host filesystem and then mounted
 * into the container
 * 
 * For now, the files to create are hard coded, but could be passed in via
 * JSON in the future - FIXME
 * 
 * The JSON for the plugin should be formatted like so:
 * 
 *      {
 *          "name": "OpenCDM",
 *      }
 * 
 *  @param[in]  id              The id of the container.
 *  @param[in]  startupState    The mutable start-up state of the container.
 *  @param[in]  rootfsPath      The absolute path to the rootfs of the container.
 *  @param[in]  jsonData        The parsed json data from the container spec file.
 *
 */
bool OpenCDMPlugin::postConstruction(const ContainerId& id,
                                     const std::shared_ptr<IDobbyStartState>& startupState,
                                     const std::string& rootfsPath,
                                     const Json::Value& jsonData)
{
    AI_LOG_FN_ENTRY();

    const unsigned maxBufferNum = 8;
    const unsigned mountFlags = (MS_BIND | MS_NOSUID | MS_NODEV | MS_NOEXEC);

    AI_LOG_INFO("Creating OCDM buffer files");

    // create the buffer files on the host file system
    for (unsigned i = 0; i < maxBufferNum; i++)
    {
        std::string path(ocdmBufferPath(i));
        std::string adminPath(ocdmBufferAdminPath(i));

        writeFileIfNotExists(path);
        writeFileIfNotExists(adminPath);

        // bind mount in the files
        if (!startupState->addMount(path, path, "bind", mountFlags))
            AI_LOG_ERROR("failed to add bind mount for '%s'", path.c_str());
        if (!startupState->addMount(adminPath, adminPath, "bind", mountFlags))
            AI_LOG_ERROR("failed to add bind mount for '%s'", adminPath.c_str());
    }

    // adjust permissions on existing /tmp/ocdm socket
    const std::string ocdmSocketPath("/tmp/ocdm");

    if (chmod(ocdmSocketPath.c_str(), S_IRWXU | S_IRGRP | S_IWGRP) != 0)
        AI_LOG_SYS_ERROR(errno, "failed to change access on socket");
    if (chown(ocdmSocketPath.c_str(), 0, 30000) != 0)
        AI_LOG_SYS_ERROR(errno, "failed to change owner off socket");

    // mount the socket within the container
    if (!startupState->addMount(ocdmSocketPath, ocdmSocketPath, "bind", mountFlags))
        AI_LOG_ERROR("failed to add bind mount for '%s'", ocdmSocketPath.c_str());


    AI_LOG_FN_EXIT();
    return true;
}

// -----------------------------------------------------------------------------
/**
 * @brief Returns the file path of the OCDMBuffer corresponding to the specified 
 * buffer number
 * 
 * @param[in]   bufferNum       Number of buffer to create
 */
std::string OpenCDMPlugin::ocdmBufferPath(unsigned bufferNum) const
{
    return "/tmp/ocdmbuffer." + std::to_string(bufferNum);
}

// -----------------------------------------------------------------------------
/**
 * @brief Returns the file path of the OCDM admin Buffer corresponding to
 * the specified buffer number
 * 
 * @param[in]   bufferNum       Number of buffer to create
 */
std::string OpenCDMPlugin::ocdmBufferAdminPath(unsigned bufferNum) const
{
    return "/tmp/ocdmbuffer." + std::to_string(bufferNum) +  ".admin";
}

// -----------------------------------------------------------------------------
/**
 * @brief Checks if the specified file exists then creates a blank
 * file with permissions 0760 it if it doesn't exist
 * 
 * @param[in] filePath      The file to create
 * 
 */
bool OpenCDMPlugin::writeFileIfNotExists(const std::string &filePath) const
{
    AI_LOG_FN_ENTRY();

    // if the file already exists, don't bother recreating it
    int fileExists = access(filePath.c_str(), R_OK);

    // file doesn't exist, so lets create it
    if (fileExists < 0)
    {
        std::ofstream output(filePath);

        // set permissions to chmod 0660 and owner root:30000
        if (chmod(filePath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) != 0)
            AI_LOG_SYS_ERROR(errno, "failed to change access on '%s''", filePath.c_str());
        if (chown(filePath.c_str(), 0, 30000) != 0)
            AI_LOG_SYS_ERROR(errno, "failed to change owner off '%s''", filePath.c_str());

        AI_LOG_FN_EXIT();
        return true;
    }

    AI_LOG_INFO("%s already exists, skipping creation", filePath.c_str());

    AI_LOG_FN_EXIT();
    return false;
}
