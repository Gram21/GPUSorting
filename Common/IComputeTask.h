/******************************************************************************
                         .88888.   888888ba  dP     dP 
                        d8'   `88  88    `8b 88     88 
                        88        a88aaaa8P' 88     88 
                        88   YP88  88        88     88 
                        Y8.   .88  88        Y8.   .8P 
                         `88888'   dP        `Y88888P' 
                                                       
                                                       
   a88888b.                                         dP   oo                   
  d8'   `88                                         88                        
  88        .d8888b. 88d8b.d8b. 88d888b. dP    dP d8888P dP 88d888b. .d8888b. 
  88        88'  `88 88'`88'`88 88'  `88 88    88   88   88 88'  `88 88'  `88 
  Y8.   .88 88.  .88 88  88  88 88.  .88 88.  .88   88   88 88    88 88.  .88 
   Y88888P' `88888P' dP  dP  dP 88Y888P' `88888P'   dP   dP dP    dP `8888P88 
                                88                                        .88 
                                dP                                    d8888P  
******************************************************************************/

#ifndef _ICOMPUTE_TASK_H
#define _ICOMPUTE_TASK_H

// All OpenCL headers
#if defined(WIN32)
    #include <CL/opencl.h>
#elif defined (__APPLE__) || defined(MACOSX)
    #include <OpenCL/opencl.h>
#else
    #include <CL/cl.h>
#endif 

#include "CommonDefs.h"

//! Common interface for the tasks within the assignment.
/*!
	Inherit a new class for each computing task.
	(There are usually more tasks in each assignment).
*/
class IComputeTask
{
public:

	virtual ~IComputeTask() {};
	
	//! Init any resources specific to the current task
	virtual bool InitResources(cl_device_id Device, cl_context Context) = 0;

	//! Release everything allocated in InitResources()
	virtual void ReleaseResources() = 0;

	//! Perform calculations on the GPU
	virtual void ComputeGPU(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3]) = 0;

	//! Compute the "golden" solution on the CPU. The GPU results must be equal to this reference
	virtual void ComputeCPU() = 0;

	//! Compare the GPU solution to the "golden" solution
	virtual bool ValidateResults() = 0;
};

#endif // _ICOMPUTE_TASK_H
