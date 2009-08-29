#ifdef OPENCL

#include "opencl_beam.h"

OpenCL_Beam::OpenCL_Beam()
{}

OpenCL_Beam::~OpenCL_Beam()
{}

void OpenCL_Beam::checkForDevices()
{
/*
	// start logs
    shrSetLogFileName ("oclDeviceQuery.txt");
    shrLog(LOGBOTH, 0.0, "oclDeviceQuery.exe Starting...\n\n"); 

    char chBuffer[1024];
    cl_uint ciDeviceCount;
    cl_device_id *devices;

    // Get and log the OpenCL platform info
    shrLog(LOGBOTH, 0.0, "OpenCL SW Info:\n\n");
    clGetPlatformInfo ((cl_platform_id)CL_PLATFORM_NVIDIA, CL_PLATFORM_VERSION, sizeof(chBuffer), chBuffer, NULL);
    shrLog(LOGBOTH, 0.0, " CL_PLATFORM_VERSION: \t%s\n", chBuffer);

    // Log OpenCL SDK Version # (for convenience:  not specific to OpenCL) 
    shrLog(LOGBOTH, 0.0, " OpenCL SDK Version: \t%s\n\n\n", oclSDKVERSION);

    // Get and log the OpenCL device count 
    shrLog(LOGBOTH, 0.0, "OpenCL Device Info: \n\n");
    clGetDeviceIDs ((cl_platform_id)CL_PLATFORM_NVIDIA, CL_DEVICE_TYPE_ALL, 0, NULL, &ciDeviceCount);
    if (ciDeviceCount == 0) 
    shrLog(LOGBOTH, 0.0, " There is no device supporting OpenCL.\n\n");
    else if (ciDeviceCount == 1)
        shrLog(LOGBOTH, 0.0, " There is 1 device supporting OpenCL:\n\n");
    else
        shrLog(LOGBOTH, 0.0, " There are %d devices supporting OpenCL:\n\n", ciDeviceCount);

    // Get and log the OpenCL device ID's
    devices = (cl_device_id*) malloc(sizeof(cl_device_id) * ciDeviceCount);
    clGetDeviceIDs ((cl_platform_id)CL_PLATFORM_NVIDIA, CL_DEVICE_TYPE_ALL, ciDeviceCount, devices, &ciDeviceCount);
    for(unsigned int i = 0; i < ciDeviceCount; ++i ) 
    {  
        oclPrintDevInfo(LOGBOTH, devices[i]);
    }

    // Log system info(for convenience:  not specific to OpenCL) 
    shrLog(LOGBOTH, 0.0, "\nSystem Info: \n\n");
    #ifdef _WIN32
        SYSTEM_INFO stProcInfo;         // processor info struct
        OSVERSIONINFO stOSVerInfo;      // Win OS info struct
        SYSTEMTIME stLocalDateTime;     // local date / time struct 

        // processor
        SecureZeroMemory(&stProcInfo, sizeof(SYSTEM_INFO));
        GetSystemInfo(&stProcInfo);

        // OS
        SecureZeroMemory(&stOSVerInfo, sizeof(OSVERSIONINFO));
        stOSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&stOSVerInfo);

        // date and time
        GetLocalTime(&stLocalDateTime); 

        // write time and date to logs
        shrLog(LOGBOTH, 0.0, " Local Time/Date = %i:%i:%i, %i/%i/%i\n", 
            stLocalDateTime.wHour, stLocalDateTime.wMinute, stLocalDateTime.wSecond, 
            stLocalDateTime.wMonth, stLocalDateTime.wDay, stLocalDateTime.wYear); 

        // write proc and OS info to logs
        shrLog(LOGBOTH, 0.0, " CPU Arch: %i\n CPU Level: %i\n # of CPU processors: %u\n Windows Build: %u\n Windows Ver: %u.%u\n\n\n", 
            stProcInfo.wProcessorArchitecture, stProcInfo.wProcessorLevel, stProcInfo.dwNumberOfProcessors, 
            stOSVerInfo.dwBuildNumber, stOSVerInfo.dwMajorVersion, stOSVerInfo.dwMinorVersion);
    #endif

    // finish
    shrLog(LOGBOTH, 0.0, "TEST PASSED...\n\n"); 
    shrEXIT(argc, argv);}
*/
}

#endif