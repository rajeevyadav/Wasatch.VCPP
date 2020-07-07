#pragma once

#ifdef WASATCHVCPPLIB_EXPORTS
#define WASATCHVCPPLIB_API __declspec(dllexport)
#else
#define WASATCHVCPPLIB_API __declspec(dllimport)
#endif

#include "libusb.h"

namespace WasatchVCPP
{
    class Driver
    {
        public:
            WASATCHVCPPLIB_API static Driver* getInstance();

            WASATCHVCPPLIB_API bool connect();

            WASATCHVCPPLIB_API usb_dev_handle* openDevice(int vid, int pid);

            WASATCHVCPPLIB_API void log(const char* fmt, ...);

        private:
            static Driver* instance;
            Driver(); 
    };

    WASATCHVCPPLIB_API bool connect();
    WASATCHVCPPLIB_API usb_dev_handle* openDevice(int vid, int pid);
}
