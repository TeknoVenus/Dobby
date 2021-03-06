# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# Thread is just the generic lib link for the pthread libraries (on platforms
# that don't have a separate pthread library this is a NOP)
find_package( Threads REQUIRED )



add_executable( # Sets the name of the exec.
        DobbyInit

        # The entry point
        source/InitMain.cpp

        )


# Add support for logging via the ethanlog API
# (FIXME this is a bit hacky - pulling the source from the one of the plugins
#  directories, but it works for now)
target_compile_definitions(
        DobbyInit
        PRIVATE
        -DUSE_ETHANLOG=1
        )

target_sources(
        DobbyInit
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../plugins/EthanLog/client/ethanlog.c
        )

target_include_directories(
        DobbyInit
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../../plugins/EthanLog/client
        )



# Specifies libraries CMake should link to your target binary. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target exec
        DobbyInit

        # Adds pthread support (if it's a separate library on target)
        Threads::Threads

        )

install( # Specifies the target to install
        TARGETS DobbyInit

        RUNTIME DESTINATION libexec/

        )
