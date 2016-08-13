/******************************************************************************
GPU Computing / GPGPU Praktikum source code.

******************************************************************************/

#include "CAssignmentBase.h"

#include "CLUtil.h"
#include "CTimer.h"

#include <vector>

using namespace std;

#if defined (__APPLE__) || defined(MACOSX)
   #define GL_SHARING_EXTENSION "cl_APPLE_gl_sharing"
#else
   #define GL_SHARING_EXTENSION "cl_khr_gl_sharing"
#endif

// required for OpenGL interop
#ifdef _WIN32
    #include <windows.h>
#endif

#ifdef linux
    #if defined (__APPLE__) || defined(MACOSX)
        #include <OpenGL/OpenGL.h>
    #else
        #include <GL/glx.h>
    #endif
#endif

///////////////////////////////////////////////////////////////////////////////
// CAssignmentBase

CAssignmentBase::CAssignmentBase()
	: m_CLPlatform(nullptr), m_CLDevice(nullptr), m_CLContext(nullptr), m_CLCommandQueue(nullptr)
{
}

CAssignmentBase::~CAssignmentBase()
{
	ReleaseCLContext();
}

bool CAssignmentBase::EnterMainLoop(int, char**)
{
	if(!InitCLContext())
		return false;

	bool success = DoCompute();

	ReleaseCLContext();

	return success;
}

#define PRINT_INFO(title, buffer, bufferSize, maxBufferSize, expr) { expr; buffer[bufferSize] = '\0'; std::cout << title << ": " << buffer << std::endl; }

bool CAssignmentBase::InitCLContext()
{
	//////////////////////////////////////////////////////
	//(Sect 4.3)

	// 1. get all platform IDs

	std::vector<cl_platform_id> platformIds;
	const cl_uint c_MaxPlatforms = 16;
	platformIds.resize(c_MaxPlatforms);
	
	cl_uint countPlatforms;
	V_RETURN_FALSE_CL(clGetPlatformIDs(c_MaxPlatforms, &platformIds[0], &countPlatforms), "Failed to get CL platform ID");
	platformIds.resize(countPlatforms);

	// 2. find all available GPU devices
	std::vector<cl_device_id> deviceIds;
	const int maxDevices = 16;
	deviceIds.resize(maxDevices);
	int countAllDevices = 0;


	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;

	for (size_t i = 0; i < platformIds.size(); i++)
	{
		// Getting the available devices.
		cl_uint countDevices;
		clGetDeviceIDs(platformIds[i], deviceType, 1, &deviceIds[countAllDevices], &countDevices);
		countAllDevices += countDevices;
	}
	deviceIds.resize(countAllDevices);

	if (countAllDevices == 0)
	{
		std::cout << "No device of the selected type with OpenCL support was found.";
		return false;
	}
	// Choosing the first available device.
	m_CLDevice = deviceIds[0];
	clGetDeviceInfo(m_CLDevice, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &m_CLPlatform, NULL);

	// Printing platform and device data.
	const int maxBufferSize = 1024;
	char buffer[maxBufferSize];
	size_t bufferSize;
	std::cout << "OpenCL platform:" << std::endl << std::endl;
	PRINT_INFO("Name", buffer, bufferSize, maxBufferSize, clGetPlatformInfo(m_CLPlatform, CL_PLATFORM_NAME, maxBufferSize, (void*)buffer, &bufferSize));
	PRINT_INFO("Vendor", buffer, bufferSize, maxBufferSize, clGetPlatformInfo(m_CLPlatform, CL_PLATFORM_VENDOR, maxBufferSize, (void*)buffer, &bufferSize));
	PRINT_INFO("Version", buffer, bufferSize, maxBufferSize, clGetPlatformInfo(m_CLPlatform, CL_PLATFORM_VERSION, maxBufferSize, (void*)buffer, &bufferSize));
	PRINT_INFO("Profile", buffer, bufferSize, maxBufferSize, clGetPlatformInfo(m_CLPlatform, CL_PLATFORM_PROFILE, maxBufferSize, (void*)buffer, &bufferSize));
	std::cout << std::endl << "Device:" << std::endl << std::endl;
	PRINT_INFO("Name", buffer, bufferSize, maxBufferSize, clGetDeviceInfo(m_CLDevice, CL_DEVICE_NAME, maxBufferSize, (void*)buffer, &bufferSize));
	PRINT_INFO("Vendor", buffer, bufferSize, maxBufferSize, clGetDeviceInfo(m_CLDevice, CL_DEVICE_VENDOR, maxBufferSize, (void*)buffer, &bufferSize));
	PRINT_INFO("Driver version", buffer, bufferSize, maxBufferSize, clGetDeviceInfo(m_CLDevice, CL_DRIVER_VERSION, maxBufferSize, (void*)buffer, &bufferSize));
	cl_ulong localMemorySize;
	clGetDeviceInfo(m_CLDevice, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &localMemorySize, &bufferSize);
	std::cout << "Local memory size: " << localMemorySize << " Byte" << std::endl;
	std::cout << std::endl << "******************************" << std::endl << std::endl;

        
	cl_int clError;

	m_CLContext = clCreateContext(NULL, 1, &m_CLDevice, NULL, NULL, &clError);
		
	V_RETURN_FALSE_CL(clError, "Failed to create OpenCL context.");

	// Finally, create a command queue. All the asynchronous commands to the device will be issued
	// from the CPU into this queue. This way the host program can continue the execution until some results
	// from that device are needed.

	m_CLCommandQueue = clCreateCommandQueue(m_CLContext, m_CLDevice, 0, &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create the command queue in the context");

	return true;
}

void CAssignmentBase::ReleaseCLContext()
{
	if (m_CLCommandQueue != nullptr)
	{
		clReleaseCommandQueue(m_CLCommandQueue);
		m_CLCommandQueue = nullptr;
	}

	if (m_CLContext != nullptr)
	{
		clReleaseContext(m_CLContext);
		m_CLContext = nullptr;
	}
}

bool CAssignmentBase::RunComputeTask(IComputeTask& Task, size_t LocalWorkSize[3])
{
	if(m_CLContext == nullptr)
	{
		std::cerr<<"Error: RunComputeTask() cannot execute because the OpenCL context has not been created first."<<endl;
	}
	
	if(!Task.InitResources(m_CLDevice, m_CLContext))
	{
		std::cerr << "Error during resource allocation. Aborting execution." <<endl;
		Task.ReleaseResources();
		return false;
	}

	// Compute the golden result.
	cout << "Computing CPU reference result...";
	Task.ComputeCPU();
	cout << "DONE" << endl;

	// Running the same task on the GPU.
	cout << "Computing GPU result...";

	// Runing the kernel N times. This make the measurement of the execution time more accurate.
	Task.ComputeGPU(m_CLContext, m_CLCommandQueue, LocalWorkSize);
	cout << "DONE" << endl;

	// Validating results.
	if (Task.ValidateResults())
	{
		cout << "GOLD TEST PASSED!" << endl;
	}
	else
	{
		cout << "INVALID RESULTS!" << endl;
	}
	
	// Cleaning up.
	Task.ReleaseResources();

	return true;
}

///////////////////////////////////////////////////////////////////////////////
