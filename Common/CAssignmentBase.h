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

#ifndef _CASSIGNMENT_BASE_H
#define _CASSIGNMENT_BASE_H

#include "IComputeTask.h"

#include "CommonDefs.h"

//! Base class for all assignments
/*! 
	Inherit a new class for each specific assignment.
	This class is abstract.

	Usage of class: from your main CPP you typically call
	EnterMainLoop(). This returns when the assignment is finished.

	Internally the assignment class should initialize the context,
	run one or more compute tasks and then release the context.
*/
class CAssignmentBase
{
public:
	CAssignmentBase();

	virtual ~CAssignmentBase();

	//! Main loop. You only need to overload this if you do some rendering in your assignment.
	virtual bool EnterMainLoop(int argc, char** argv);

	//! You need to overload this to define a specific behavior for your assignments
	virtual bool DoCompute() = 0;

protected:	
	virtual bool InitCLContext();

	virtual void ReleaseCLContext();

	virtual bool RunComputeTask(IComputeTask& Task, size_t LocalWorkSize[3]);

	cl_platform_id		m_CLPlatform;
	cl_device_id		m_CLDevice;
	cl_context			m_CLContext;
	cl_command_queue	m_CLCommandQueue;
};

#endif // _CASSIGNMENT_BASE_H
