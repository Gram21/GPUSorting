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

#ifndef _CSORT_TASK_H
#define _CSORT_TASK_H

#include "../Common/IComputeTask.h"

class CSortTask : public IComputeTask
{
public:
	CSortTask(size_t ArraySize, size_t LocWorkSize[3]);

	virtual ~CSortTask();

	// IComputeTask

	virtual bool InitResources(cl_device_id Device, cl_context Context);

	virtual void ReleaseResources();

	virtual void ComputeGPU(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3]);

	virtual void ComputeCPU();

	virtual bool ValidateResults();

protected:

	size_t getPaddedSize(size_t n);

	void Mergesort();
	void ValidateCPU();

	void Sort_Mergesort(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3]);
	void Sort_SimpleSortingNetwork(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3]);
	void Sort_SimpleSortingNetworkLocal(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3]);
	void Sort_BitonicMergesort(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3]);

	void ExecuteTask(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3], unsigned int task);
	void TestPerformance(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3], unsigned int task);

	//NOTE: we have two memory address spaces, so we mark pointers with a prefix
	//to avoid confusions: 'h' - host, 'd' - device

	size_t				m_N;
	size_t				m_N_padded;
	size_t				LocalWorkSize[3];

	// input data
	unsigned int		*m_hInput;
	// results
	unsigned int*		m_resultCPU;
	unsigned int*		m_resultGPU[3];

	cl_mem				m_dPingArray;
	cl_mem				m_dPongArray;

	//OpenCL program and kernels
	cl_program			m_Program;
	cl_kernel			m_MergesortStartKernel;
	cl_kernel			m_MergesortGlobalSmallKernel;
	cl_kernel			m_MergesortGlobalBigKernel;
	cl_kernel			m_SimpleSortingNetworkKernel;
	cl_kernel			m_SimpleSortingNetworkLocalKernel;
	cl_kernel			m_BitonicStartKernel;
	cl_kernel			m_BitonicGlobalKernel;
	cl_kernel			m_BitonicLocalKernel;
};

#endif // _CSORT_TASK_H
