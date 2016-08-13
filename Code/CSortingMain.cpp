/******************************************************************************
GPU Computing / GPGPU Praktikum source code.

******************************************************************************/

#include "CSortingMain.h"

#include "CSortTask.h"

#include <iostream>

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// CAssignment2

bool CSortingMain::DoCompute()
{
	// Task 1: parallel reduction
	cout << "########################################" << endl;
	cout << "Running sort task..." << endl << endl;
	{
		// set work size and size of input array
		size_t LocalWorkSize[3] = { 256, 1, 1 };
		unsigned int arraySize = 1024 * 256;

		// info output
		cout << "Start sorting array of size " << arraySize;
		cout << " using LocalWorkSize " << LocalWorkSize[0] << endl << endl;

		// create sorting task and start it
		CSortTask sorting(arraySize, LocalWorkSize);
		RunComputeTask(sorting, LocalWorkSize);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// NOTE: With bigger arrays we need to compile for 64-Bit!
