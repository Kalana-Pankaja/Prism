# Find the NDI SDK (https://ndi.video)
#
# Set NDI_ROOT to the SDK install prefix if headers/libs are not on the default path.
#
# Defines:
#   NDI_FOUND
#   NDI_INCLUDE_DIRS
#   NDI_LIBRARIES

find_path(NDI_INCLUDE_DIR
    NAMES Processing.NDI.Lib.h
    HINTS
        ${NDI_ROOT}/include
        ${NDI_ROOT}/Include
        "$ENV{ProgramFiles}/NDI/NDI 6 SDK/Include"
        "$ENV{ProgramFiles}/NDI/NDI 5 SDK/Include"
        /usr/include
        /usr/local/include
        "$ENV{HOME}/NDI SDK for Linux/include"
)

find_library(NDI_LIBRARY
    NAMES ndi Processing.NDI.Lib.x64 Processing.NDI.Lib.x86
    HINTS
        ${NDI_ROOT}/lib
        ${NDI_ROOT}/Lib/x64
        ${NDI_ROOT}/lib/x86_64-linux-gnu
        "$ENV{ProgramFiles}/NDI/NDI 6 SDK/Lib/x64"
        "$ENV{ProgramFiles}/NDI/NDI 5 SDK/Lib/x64"
        /usr/lib
        /usr/local/lib
        "$ENV{HOME}/NDI SDK for Linux/lib/x86_64-linux-gnu"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NDI DEFAULT_MSG NDI_INCLUDE_DIR NDI_LIBRARY)

if(NDI_FOUND)
    set(NDI_INCLUDE_DIRS ${NDI_INCLUDE_DIR})
    set(NDI_LIBRARIES ${NDI_LIBRARY})
endif()

mark_as_advanced(NDI_INCLUDE_DIR NDI_LIBRARY)
