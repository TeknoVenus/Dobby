#
# Find libRbus
#
# RBUS_FOUND          - true if rbus was found
# RBUS_INCLUDE_DIR    - where to find the rbus headers
# RBUS_LIBRARY        - where to find the library librbus
#

include(FindPackageHandleStandardArgs)

# Check for the header files

find_path(RBUS_INCLUDE_DIR NAMES rbus/rbus.h)

find_package_handle_standard_args(RBUS DEFAULT_MSG
    RBUS_LIBRARY RBUS_INCLUDE_DIR
)
find_library( RBUS_LIBRARY NAMES librbus.so librbus)
mark_as_advanced( RBUS_INCLUDE_DIR RBUS_LIBRARY )

if(RBUS_FOUND)
    set(RBUS_INCLUDE_DIRS ${RBUS_INCLUDE_DIR})
    set(RBUS_LIBRARIES ${RBUS_LIBRARY})

    add_library(RBUS UNKNOWN IMPORTED)
    set_target_properties(RBUS PROPERTIES
        IMPORTED_LOCATION "${RBUS_LIBRARY}"
        IMPORTED_INCLUDE_DIRECTORIES "${RBUS_INCLUDE_DIR}"
    )
endif()