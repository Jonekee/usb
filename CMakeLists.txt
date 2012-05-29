cmake_minimum_required(VERSION 2.6)

project(libusb)

if(UNIX AND CMAKE_SYSTEM_NAME MATCHES Linux)
    set(LINUX ON)
endif()
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(M64 ON)
else()
    set(M32 ON)
endif()

set(usb_POSIX_SOURCES
    libusb/os/threads_posix.h
    libusb/os/threads_posix.c
    libusb/os/poll_posix.h
)

set(usb_DARWIN_SOURCES
    libusb/os/darwin_usb.c
    libusb/os/darwin_usb.h
)

set(usb_LINUX_SOURCES
    libusb/os/linux_usbfs.c
    libusb/os/linux_usbfs.h
)

set(usb_WINDOWS_SOURCES
    libusb/os/poll_windows.c
    libusb/os/poll_windows.h
    libusb/os/threads_windows.c
    libusb/os/threads_windows.h
    libusb/os/windows_usb.c
    libusb/os/windows_usb.h
)

set(usb_SOURCES
    libusb/libusbi.h
    libusb/core.c
    libusb/descriptor.c
    libusb/io.c
    libusb/sync.c
)

set(ENABLE_DEBUG_LOGGING OFF)
set(ENABLE_LOGGING OFF)

set(USBI_OS_HANDLES_TIMEOUT ON)
set(USBI_TIMERFD_AVAILABLE OFF)
set(INLINE __inline)

macro(require_library var lib)
    find_library(${var} ${lib})
    if(NOT ${var})
        message(SEND_ERROR "Required library ${lib} was not found.")
    endif()
endmacro()

if(UNIX)
    list(APPEND usb_SOURCES ${usb_POSIX_SOURCES})
    set(THREADS_POSIX ON)
    set(DEFAULT_VISIBILITY "__attribute__((visibility(\"default\")))")
    set(HAS_POLL_NFDS_TYPE ON)
    set(HAVE_POLL_H ON)
    set(HAVE_SYS_TIME_H ON)
    set(HAVE_STRUCT_TIMESPEC ON)
    set(HAVE_INTTYPES_H ON)
    if(LINUX)
        set(OS_LINUX ON)
        list(APPEND usb_SOURCES ${usb_LINUX_SOURCES})
        if(M64)
            list(APPEND usb_COMPILE_FLAGS "-fPIC")
        endif()
        find_package(Threads REQUIRED)
        list(APPEND usb_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} rt)
    elseif(APPLE)
        set(OS_DARWIN ON)
        list(APPEND usb_SOURCES ${usb_DARWIN_SOURCES})
        require_library(COREFOUNDATION_LIBRARY CoreFoundation)
        require_library(         IOKIT_LIBRARY IOKit)
        require_library(          OBJC_LIBRARY objc)
        list(APPEND usb_LIBRARIES ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY} ${OBJC_LIBRARY})
    endif()
elseif(WIN32)
    set(OS_WINDOWS ON)
    set(DEFAULT_VISIBILITY "")
    include_directories(msvc)
    list(APPEND usb_SOURCES ${usb_WINDOWS_SOURCES})
endif()

configure_file(cmake-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "NDEBUG")

include_directories(libusb ${CMAKE_CURRENT_BINARY_DIR})

add_library(usb STATIC ${usb_SOURCES})

if(usb_COMPILE_FLAGS)
    set_property(TARGET usb PROPERTY COMPILE_FLAGS ${usb_COMPILE_FLAGS})
endif()

if(usb_LIBRARIES)
    target_link_libraries(usb ${usb_LIBRARIES})
endif()

set_property(TARGET usb PROPERTY OUTPUT_NAME usb-1.0)

install(TARGETS usb EXPORT usb-exports DESTINATION lib)

install(FILES libusb/libusb.h DESTINATION include/libusb-1.0)

install(FILES usb-config.cmake DESTINATION share/usb)

install(EXPORT usb-exports DESTINATION share/usb)
