cmake_minimum_required(VERSION 2.6)

project(libusb)

find_library(COREFOUNDATION_LIBRARY CoreFoundation)
find_library(         IOKIT_LIBRARY IOKit)

set(usb_SOURCES 
    libusb/libusbi.h
    libusb/config.h
    libusb/core.c
    libusb/descriptor.c
    libusb/io.c
    libusb/sync.c
    libusb/os/darwin_usb.c
    libusb/os/darwin_usb.h
    libusb/os/threads_posix.h
    libusb/os/poll_posix.h
)

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "NDEBUG")

include_directories(libusb)

add_library(usb STATIC ${usb_SOURCES})

target_link_libraries(usb ${IOKIT_LIBRARY} ${COREFOUNDATION_LIBRARY})

install(TARGETS usb EXPORT usb-exports DESTINATION lib)

install(FILES libusb/libusb.h DESTINATION include/libusb-1.0)

install(FILES usb-config.cmake DESTINATION share/usb)

install(EXPORT usb-exports DESTINATION share/usb)
