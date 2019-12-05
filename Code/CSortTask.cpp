/******************************************************************************
GPU Computing / GPGPU Praktikum source code.

******************************************************************************/

#include "CSortTask.h"

#include "../Common/CLUtil.h"
#include "../Common/CTimer.h"
#include <time.h>
#include <math.h>
#include <sstream>
#include <cstring>
#include <climits>

using namespace std;



#define MERGESORT_SMALL_STRIDE 1024 * 64
#define SSN_LIMIT 1024 * 512
#define MERGE_LIMIT 1024 * 1024 * 2

///////////////////////////////////////////////////////////////////////////////
// CSortTask

string g_kernelNames[4] = {
	"Mergesort",
	"SimpleSortingNetwork",
	"BitonicMergesort",
};

// std::min<unsigned long>(ulongarg, uintarg);

CSortTask::CSortTask(size_t ArraySize, size_t LocWorkSize[3])
	: m_N(ArraySize), LocalWorkSize(),
	m_hInput(NULL),
	m_dPingArray(NULL),
	m_dPongArray(NULL),
	m_Program(NULL),
	m_MergesortStartKernel(NULL), m_MergesortGlobalSmallKernel(NULL), m_MergesortGlobalBigKernel(NULL),
	m_SimpleSortingNetworkKernel(NULL), m_SimpleSortingNetworkLocalKernel(NULL),
	m_BitonicGlobalKernel(NULL), m_BitonicLocalKernel(NULL), m_BitonicStartKernel(NULL)
{
	m_N_padded = getPaddedSize(m_N);
	LocalWorkSize[0] = LocWorkSize[0];
	LocalWorkSize[1] = LocWorkSize[1];
	LocalWorkSize[2] = LocWorkSize[2];
}

CSortTask::~CSortTask()
{
	ReleaseResources();
}

bool CSortTask::InitResources(cl_device_id Device, cl_context Context)
{
	//CPU resources
	m_hInput = new unsigned int[m_N_padded];
	m_resultCPU = new unsigned int[m_N_padded];

	srand((unsigned int)time(NULL)); // To get each "time" another seed for rand()
	//fill the array with some values
	for (unsigned int i = 0; i < m_N; i++)
		//m_hInput[i] = m_N - i;			// Use this for debugging. Use 1 or i or similar
		m_hInput[i] = rand();

	//pad the array with max value so we can sort arbitrarily long arrays, not only power of 2
	for (size_t i = m_N; i < m_N_padded; i++)
		m_hInput[i] = UINT_MAX;

	//device resources
	cl_int clError, clError2;
	m_dPingArray = clCreateBuffer(Context, CL_MEM_READ_WRITE, sizeof(cl_uint) * m_N_padded, NULL, &clError2);
	clError = clError2;
	m_dPongArray = clCreateBuffer(Context, CL_MEM_READ_WRITE, sizeof(cl_uint) * m_N_padded, NULL, &clError2);
	clError |= clError2;
	V_RETURN_FALSE_CL(clError, "Error allocating device arrays");

	//load and compile kernels with compileoptions
	string programCode;

	stringstream compileOptions;
	compileOptions << "-cl-fast-relaxed-math" << " -D MAX_LOCAL_SIZE=" << LocalWorkSize[0];
	CLUtil::LoadProgramSourceToMemory("Sort.cl", programCode);
	m_Program = CLUtil::BuildCLProgramFromMemory(Device, Context, programCode, compileOptions.str());
	if (m_Program == nullptr) return false;

	//create kernels for mergesort
	m_MergesortGlobalSmallKernel = clCreateKernel(m_Program, "Sort_MergesortGlobalSmall", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_MergesortGlobalSmall.");
	m_MergesortGlobalBigKernel = clCreateKernel(m_Program, "Sort_MergesortGlobalBig", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_MergesortGlobalBig.");
	m_MergesortStartKernel = clCreateKernel(m_Program, "Sort_MergesortStart", &clError); //local variant to start with
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_MergesortStart.");

	//create kernels for simple sorting network
	m_SimpleSortingNetworkKernel = clCreateKernel(m_Program, "Sort_SimpleSortingNetwork", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_SimpleSortingNetwork.");
	m_SimpleSortingNetworkLocalKernel = clCreateKernel(m_Program, "Sort_SimpleSortingNetworkLocal", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_SimpleSortingNetworkLocal.");

	//create kernels for bitonic sort
	m_BitonicStartKernel = clCreateKernel(m_Program, "Sort_BitonicMergesortStart", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_BitonicMergesortStart.");
	m_BitonicGlobalKernel = clCreateKernel(m_Program, "Sort_BitonicMergesortGlobal", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_BitonicMergesortGlobal.");
	m_BitonicLocalKernel = clCreateKernel(m_Program, "Sort_BitonicMergesortLocal", &clError);
	V_RETURN_FALSE_CL(clError, "Failed to create kernel: Sort_BitonicMergesortLocal.");

	return true;
}

void CSortTask::ReleaseResources()
{
	// host resources
	SAFE_DELETE_ARRAY(m_hInput);
	SAFE_DELETE_ARRAY(m_resultCPU);
	for (int i = 0; i < 3; i++)
		SAFE_DELETE_ARRAY(m_resultGPU[i]);

	// device resources
	SAFE_RELEASE_MEMOBJECT(m_dPingArray);
	SAFE_RELEASE_MEMOBJECT(m_dPongArray);

	SAFE_RELEASE_KERNEL(m_MergesortGlobalBigKernel);
	SAFE_RELEASE_KERNEL(m_MergesortGlobalSmallKernel);
	SAFE_RELEASE_KERNEL(m_MergesortStartKernel);
	SAFE_RELEASE_KERNEL(m_SimpleSortingNetworkKernel);
	SAFE_RELEASE_KERNEL(m_SimpleSortingNetworkLocalKernel);
	SAFE_RELEASE_KERNEL(m_BitonicStartKernel);
	SAFE_RELEASE_KERNEL(m_BitonicGlobalKernel);
	SAFE_RELEASE_KERNEL(m_BitonicLocalKernel);

	SAFE_RELEASE_PROGRAM(m_Program);
}

size_t CSortTask::getPaddedSize(size_t n)
{
	unsigned int log2val = (unsigned int)ceil(log((float)n) / log(2.f));
	return (size_t)pow(2, log2val);
}

void CSortTask::ComputeGPU(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3])
{
	// Execute Tasks
	ExecuteTask(Context, CommandQueue, LocalWorkSize, 0);
	ExecuteTask(Context, CommandQueue, LocalWorkSize, 1);
	ExecuteTask(Context, CommandQueue, LocalWorkSize, 2);

	// Test Performance
	TestPerformance(Context, CommandQueue, LocalWorkSize, 0);
	TestPerformance(Context, CommandQueue, LocalWorkSize, 1);
	TestPerformance(Context, CommandQueue, LocalWorkSize, 2);
}

void CSortTask::ComputeCPU()
{
	unsigned int nIterations = 1;
	double ms;

	//CTimer timer;
	//copy(m_hInput, m_hInput + m_N_padded, m_resultCPU); // if we want to compare to a std lib sorting implementation
	//cout << endl << " std:sort " << endl;
	//timer.Start();
	//for (unsigned int j = 0; j < nIterations; j++) {
	//	sort(m_resultCPU, m_resultCPU + m_N_padded);
	//}

	//timer.Stop();

	//ms = timer.GetElapsedMilliseconds() / double(nIterations);
	//cout << "  average time: " << ms << " ms, throughput: " << 1.0e-3 * (double)m_N / ms << " Melem/s" << endl;

	CTimer timer2;
	cout << " own mergesort " << endl;
	timer2.Start();
	for (unsigned int j = 0; j < nIterations; j++) {
		Mergesort();
	}

	timer2.Stop();

	ms = timer2.GetElapsedMilliseconds() / double(nIterations);
	cout << "  average time: " << ms << " ms, throughput: " << 1.0e-3 * (double)m_N / ms << " Melem/s" << endl;

	// Check CPU implementation
	// ValidateCPU();
}

void CSortTask::Mergesort()
{
	//temporary buffer as an helper array
	unsigned int* tmpBuffer = new unsigned int[m_N_padded];
	memcpy(tmpBuffer, m_hInput, m_N_padded * sizeof(unsigned int));
	for (unsigned int stride = 2; stride <= m_N_padded; stride *= 2) {
		for (unsigned int i = 0; i < m_N_padded; i += stride) {
			unsigned int middle = i + (stride / 2);
			unsigned int left = i, right = middle;
			unsigned int rightBoundary = min(i + stride, (unsigned int)m_N_padded);
			for (unsigned int j = i; j < rightBoundary; j++) {
				if (left < middle &&
					(right == rightBoundary || tmpBuffer[left] <= tmpBuffer[right])) {
					m_resultCPU[j] = tmpBuffer[left];
					left++;
				}
				else {
					m_resultCPU[j] = tmpBuffer[right];
					right++;
				}
			}
		}
		swap(m_resultCPU, tmpBuffer);
	}
	// final swap to have the result in the correct array
	swap(m_resultCPU, tmpBuffer);

	// delete helper array
	SAFE_DELETE_ARRAY(tmpBuffer);
}

void CSortTask::ValidateCPU()
{
	bool sorted = true;
	for (int i = 1; i < m_N; i++) {
		if (m_resultCPU[i - 1] > m_resultCPU[i]) {
			sorted = false;
			break;
		}
	}
	if (!sorted) cout << "CPU was not sorted correctly! INVALID ORDER!" << endl;
}

bool CSortTask::ValidateResults()
{
	bool success = true;

	for (int i = 0; i < 3; i++)
		if (memcmp(m_resultGPU[i], m_resultCPU, m_N) != 0)
		{
			cout << "Validation of sorting kernel " << g_kernelNames[i] << " failed." << endl;
			success = false;
		}

	return success;
}

void CSortTask::Sort_Mergesort(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3])
{
	//TODO fix memory problem when many elements. -> CL_OUT_OF_RESOURCES
	cl_int clError;
	size_t globalWorkSize[1];
	size_t localWorkSize[1];

	localWorkSize[0] = LocalWorkSize[0];
	globalWorkSize[0] = CLUtil::GetGlobalWorkSize(m_N_padded / 2, localWorkSize[0]);
	unsigned int locLimit = 1;

	if (m_N_padded >= LocalWorkSize[0] * 2) {
		locLimit = 2 * LocalWorkSize[0];

		// start with a local variant first, ASSUMING we have more than localWorkSize[0] * 2 elements
		clError = clSetKernelArg(m_MergesortStartKernel, 0, sizeof(cl_mem), (void*)&m_dPingArray);
		clError |= clSetKernelArg(m_MergesortStartKernel, 1, sizeof(cl_mem), (void*)&m_dPongArray);
		V_RETURN_CL(clError, "Failed to set kernel args: MergeSortStart");

		clError = clEnqueueNDRangeKernel(CommandQueue, m_MergesortStartKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
		V_RETURN_CL(clError, "Error executing MergeSortStart kernel!");

		swap(m_dPingArray, m_dPongArray);
	}

	// proceed with the global variant
	unsigned int stride = 2 * locLimit;

	localWorkSize[0] = LocalWorkSize[0];
	globalWorkSize[0] = CLUtil::GetGlobalWorkSize(m_N_padded / 2, localWorkSize[0]);

	if (m_N_padded <= MERGESORT_SMALL_STRIDE) {
		// set not changing arguments
		clError = clSetKernelArg(m_MergesortGlobalSmallKernel, 3, sizeof(cl_uint), (void*)&m_N_padded);
		V_RETURN_CL(clError, "Failed to set kernel args: MergeSortGlobal");

		for (; stride <= m_N_padded; stride <<= 1) {
			//calculate work sizes
			size_t neededWorkers = m_N_padded / stride;

			localWorkSize[0] = min(LocalWorkSize[0], neededWorkers);
			globalWorkSize[0] = CLUtil::GetGlobalWorkSize(neededWorkers, localWorkSize[0]);

			clError = clSetKernelArg(m_MergesortGlobalSmallKernel, 0, sizeof(cl_mem), (void*)&m_dPingArray);
			clError |= clSetKernelArg(m_MergesortGlobalSmallKernel, 1, sizeof(cl_mem), (void*)&m_dPongArray);
			clError |= clSetKernelArg(m_MergesortGlobalSmallKernel, 2, sizeof(cl_uint), (void*)&stride);
			V_RETURN_CL(clError, "Failed to set kernel args: MergeSortGlobal");

			clError = clEnqueueNDRangeKernel(CommandQueue, m_MergesortGlobalSmallKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
			V_RETURN_CL(clError, "Error executing kernel!");

			swap(m_dPingArray, m_dPongArray);
		}
	}
	else {
		// set not changing arguments
		clError = clSetKernelArg(m_MergesortGlobalBigKernel, 3, sizeof(cl_uint), (void*)&m_N_padded);
		V_RETURN_CL(clError, "Failed to set kernel args: MergeSortGlobal");

		for (; stride <= m_N_padded; stride <<= 1) {
			//calculate work sizes
			size_t neededWorkers = m_N_padded / stride;

			localWorkSize[0] = min(LocalWorkSize[0], neededWorkers);
			globalWorkSize[0] = CLUtil::GetGlobalWorkSize(neededWorkers, localWorkSize[0]);

			clError = clSetKernelArg(m_MergesortGlobalBigKernel, 0, sizeof(cl_mem), (void*)&m_dPingArray);
			clError |= clSetKernelArg(m_MergesortGlobalBigKernel, 1, sizeof(cl_mem), (void*)&m_dPongArray);
			clError |= clSetKernelArg(m_MergesortGlobalBigKernel, 2, sizeof(cl_uint), (void*)&stride);
			V_RETURN_CL(clError, "Failed to set kernel args: MergeSortGlobal");

			clError = clEnqueueNDRangeKernel(CommandQueue, m_MergesortGlobalBigKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
			V_RETURN_CL(clError, "Error executing kernel!");

			if (stride >= 1024 * 1024) V_RETURN_CL(clFinish(CommandQueue), "Failed finish CommandQueue at mergesort for bigger strides.");
			swap(m_dPingArray, m_dPongArray);
		}
	}
}

void CSortTask::Sort_SimpleSortingNetwork(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3])
{
	cl_int clError;
	size_t globalWorkSize[1];
	size_t localWorkSize[1];

	// "padded" n, so we get an even amount of values
	unsigned int n = (m_N & 1) ? (m_N + 1) : (m_N);

	localWorkSize[0] = min<unsigned int>(LocalWorkSize[0], n);
	globalWorkSize[0] = CLUtil::GetGlobalWorkSize(n >> 1, localWorkSize[0]);

	// set general arguments
	clError = clSetKernelArg(m_SimpleSortingNetworkKernel, 0, sizeof(cl_mem), (void*)&m_dPingArray);
	clError |= clSetKernelArg(m_SimpleSortingNetworkKernel, 1, sizeof(cl_mem), (void*)&m_dPingArray);
	clError |= clSetKernelArg(m_SimpleSortingNetworkKernel, 3, sizeof(cl_uint), (void*)&n);
	V_RETURN_CL(clError, "Failed to set kernel args: SimpleSortingNetwork");

	for (unsigned int i = 0; i < n; i++) {
		unsigned int offset = i & 1;

		// set arguments
		clError |= clSetKernelArg(m_SimpleSortingNetworkKernel, 2, sizeof(cl_uint), (void*)&offset);
		V_RETURN_CL(clError, "Failed to set kernel args: SimpleSortingNetwork");

		// start kernel
		clError = clEnqueueNDRangeKernel(CommandQueue, m_SimpleSortingNetworkKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
		V_RETURN_CL(clError, "Error executing kernel!");
	}
}

void CSortTask::Sort_SimpleSortingNetworkLocal(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3])
{
	cl_int clError;
	size_t globalWorkSize[1];
	size_t localWorkSize[1];
	// "padded" n, so we get an even amount of values
	unsigned int n = (m_N & 1) ? (m_N + 1) : (m_N);

	localWorkSize[0] = min<unsigned int>(LocalWorkSize[0], n);
	globalWorkSize[0] = CLUtil::GetGlobalWorkSize(n >> 1, localWorkSize[0]);
	unsigned int loop_lim = (n / localWorkSize[0]);
	unsigned int offset = 0;

	for (unsigned int i = 0; i < loop_lim; i++) {
		offset = (i & 1);
		clError = clSetKernelArg(m_SimpleSortingNetworkLocalKernel, 0, sizeof(cl_mem), (void*)&m_dPingArray);
		clError |= clSetKernelArg(m_SimpleSortingNetworkLocalKernel, 1, sizeof(cl_uint), (void*)&n);
		clError |= clSetKernelArg(m_SimpleSortingNetworkLocalKernel, 2, sizeof(cl_uint), (void*)&offset);
		V_RETURN_CL(clError, "Failed to set kernel args: SimpleSortingNetworkLocal");

		// start kernel
		clError = clEnqueueNDRangeKernel(CommandQueue, m_SimpleSortingNetworkLocalKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
		V_RETURN_CL(clError, "Error executing SimpleSortingNetworkLocal kernel!");
	}
}

void CSortTask::Sort_BitonicMergesort(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3])
{
	cl_int clError;
	size_t globalWorkSize[1];
	size_t localWorkSize[1];

	localWorkSize[0] = LocalWorkSize[0];
	globalWorkSize[0] = CLUtil::GetGlobalWorkSize(m_N_padded / 2, localWorkSize[0]);
	unsigned int limit = (unsigned int)2 * LocalWorkSize[0]; //limit is double the localWorkSize

	// start with Sort_BitonicMergesortLocalBegin to sort local until we reach the limit
	clError = clSetKernelArg(m_BitonicStartKernel, 0, sizeof(cl_mem), (void *)&m_dPingArray);
	clError |= clSetKernelArg(m_BitonicStartKernel, 1, sizeof(cl_mem), (void *)&m_dPongArray);
	V_RETURN_CL(clError, "Failed to set kernel args: BitonicStartKernel");

	clError = clEnqueueNDRangeKernel(CommandQueue, m_BitonicStartKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
	V_RETURN_CL(clError, "Error executing BitonicStartKernel!");

	// proceed with global and local kernels
	for (unsigned int blocksize = limit; blocksize <= m_N_padded; blocksize <<= 1) {
		for (unsigned int stride = blocksize / 2; stride > 0; stride >>= 1) {
			if (stride >= limit) {
				//Sort_BitonicMergesortGlobal
				clError = clSetKernelArg(m_BitonicGlobalKernel, 0, sizeof(cl_mem), (void *)&m_dPongArray);
				clError |= clSetKernelArg(m_BitonicGlobalKernel, 1, sizeof(cl_uint), (void *)&m_N_padded);
				clError |= clSetKernelArg(m_BitonicGlobalKernel, 2, sizeof(cl_uint), (void *)&blocksize);
				clError |= clSetKernelArg(m_BitonicGlobalKernel, 3, sizeof(cl_uint), (void *)&stride);
				V_RETURN_CL(clError, "Failed to set kernel args: BitonicGlobalKernel");

				clError = clEnqueueNDRangeKernel(CommandQueue, m_BitonicGlobalKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
				V_RETURN_CL(clError, "Error executing BitonicGlobalKernel!");
			}
			else {
				//Sort_BitonicMergesortLocal
				clError = clSetKernelArg(m_BitonicLocalKernel, 0, sizeof(cl_mem), (void *)&m_dPongArray);
				clError |= clSetKernelArg(m_BitonicLocalKernel, 1, sizeof(cl_uint), (void *)&m_N_padded);
				clError |= clSetKernelArg(m_BitonicLocalKernel, 2, sizeof(cl_uint), (void *)&blocksize);
				clError |= clSetKernelArg(m_BitonicLocalKernel, 3, sizeof(cl_uint), (void *)&stride);
				V_RETURN_CL(clError, "Failed to set kernel args: BitonicLocalKernel");

				clError = clEnqueueNDRangeKernel(CommandQueue, m_BitonicLocalKernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
				V_RETURN_CL(clError, "Error executing BitonicLocalKernel!");
			}
		}
	}
	swap(m_dPingArray, m_dPongArray);
}

void CSortTask::ExecuteTask(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3], unsigned int Task)
{
	//write input data to the GPU
	V_RETURN_CL(clEnqueueWriteBuffer(CommandQueue, m_dPingArray, CL_FALSE, 0, m_N_padded * sizeof(cl_uint), m_hInput, 0, NULL, NULL), "Error copying data from host to device!");

	bool skipped = false;
	//run selected task
	switch (Task){
	case 0:
		if (m_N_padded <= MERGE_LIMIT)
			Sort_Mergesort(Context, CommandQueue, LocalWorkSize);
		else {
			cout << endl << "Skipping Mergesort on GPU!" << endl;
			skipped = true;
		}
		break;
	case 1:
		if (m_N_padded <= SSN_LIMIT)
			//Sort_SimpleSortingNetwork(Context, CommandQueue, LocalWorkSize); //uncomment etc if you want to run slower global variant. then also below
			Sort_SimpleSortingNetworkLocal(Context, CommandQueue, LocalWorkSize);
		else {
			cout << endl << "Skipping SimpleSortingNetwork!" << endl;
			skipped = true;
		}
		break;
	case 2:
		Sort_BitonicMergesort(Context, CommandQueue, LocalWorkSize);
		break;
	}

	//read back the results synchronously.
	m_resultGPU[Task] = new unsigned int[m_N];
	if (skipped) memcpy(m_resultGPU[Task], m_resultCPU, m_N);
	else V_RETURN_CL(clEnqueueReadBuffer(CommandQueue, m_dPingArray, CL_TRUE, 0, m_N * sizeof(cl_uint), m_resultGPU[Task], 0, NULL, NULL), "Error reading data from device!");

	//DEBUG TODO: change Task number or delete
	if (Task == 012) {
		cout << endl;
		cout << "GPU\tCPU\tInput" << endl;
		for (unsigned int i = 0; i < 50; i++) {
			cout << m_resultGPU[Task][i] << "\t" << m_resultCPU[i] << "\t" << m_hInput[i] << endl;
			//cout << m_resultGPU[Task][i] << ",\t";
		}
		cout << endl;
		for (unsigned int i = 0; i < m_N; i++) {
			cout << m_resultGPU[Task][i] << "-" << m_resultCPU[i] << ",\t";
		}
		cout << endl;
	}
}

void CSortTask::TestPerformance(cl_context Context, cl_command_queue CommandQueue, size_t LocalWorkSize[3], unsigned int Task)
{
	cout << "Testing performance of task " << g_kernelNames[Task] << endl;

	//write input data to the GPU
	V_RETURN_CL(clEnqueueWriteBuffer(CommandQueue, m_dPingArray, CL_FALSE, 0, m_N_padded * sizeof(cl_uint), m_hInput, 0, NULL, NULL), "Error copying data from host to device!");
	//finish all before we start meassuring the time
	V_RETURN_CL(clFinish(CommandQueue), "Error finishing the queue!");

	bool skipped = false;
	CTimer timer;
	timer.Start();

	//run the kernel N times TODO vary this if necessary!
	unsigned int nIterations = 10;
	for (unsigned int i = 0; i < nIterations; i++) {
		//run selected task
		switch (Task){
		case 0:
			if (m_N_padded <= MERGE_LIMIT)
				Sort_Mergesort(Context, CommandQueue, LocalWorkSize);
			else skipped = true;
			break;
		case 1:
			if (m_N_padded <= SSN_LIMIT)
				//Sort_SimpleSortingNetwork(Context, CommandQueue, LocalWorkSize);
				Sort_SimpleSortingNetworkLocal(Context, CommandQueue, LocalWorkSize);
			else skipped = true;
			break;
		case 2:
			Sort_BitonicMergesort(Context, CommandQueue, LocalWorkSize);
			break;
		}
	}

	//wait until the command queue is empty again
	V_RETURN_CL(clFinish(CommandQueue), "Error finishing the queue!");

	timer.Stop();

	if (!skipped) {
		double ms = timer.GetElapsedMilliseconds() / double(nIterations);
		cout << "  average time: " << ms << " ms, throughput: " << 1.0e-3 * (double)m_N / ms << " Melem/s" << endl;
	}
	else {
		cout << "  skipped" << endl;
	}
}

///////////////////////////////////////////////////////////////////////////////
