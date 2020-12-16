#
# Find libRbus
#
# RBUS_FOUND          - true if rbus was found
# RBUS_INCLUDE_DIR    - where to find the rbus headers
# RBUS_LIBRARY        - where to find the library librbus
# RBUS_CORE_FOUND
# RBUS_CORE_INCLUDE_DIR
# RBUS_CORE_LIBRARY

include(FindPackageHandleStandardArgs)

# Check for the header files

find_path(RBUS_INCLUDE_DIR NAMES rbus/rbus.h)
find_library( RBUS_LIBRARY NAMES librbus.so rbus)


message( "RBUS_INCLUDE_DIR include dir = ${RBUS_INCLUDE_DIR}" )
message( "RBUS_LIBRARY lib = ${RBUS_LIBRARY}" )


find_package_handle_standard_args(RBUS DEFAULT_MSG
    RBUS_LIBRARY RBUS_INCLUDE_DIR
)
mark_as_advanced( RBUS_INCLUDE_DIR RBUS_LIBRARY )


if(RBUS_FOUND)
    set(RBUS_INCLUDE_DIRS ${RBUS_INCLUDE_DIR})
    set(RBUS_LIBRARIES ${RBUS_LIBRARY})

    add_library(Rbus SHARED IMPORTED)
    set_target_properties(Rbus PROPERTIES
        IMPORTED_LOCATION "${RBUS_LIBRARY}"
        IMPORTED_INCLUDE_DIRECTORIES "${RBUS_INCLUDE_DIR}"
    )
endif()