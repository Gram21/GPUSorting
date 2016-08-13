/******************************************************************************
GPU Computing / GPGPU Praktikum source code.

******************************************************************************/

#include "CSortingMain.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	CSortingMain mySortingMain;

	auto success = mySortingMain.EnterMainLoop(argc, argv);

#ifdef _MSC_VER
	cout << "Press any key..." << endl;
	cin.get();
#endif

	return success ? 0 : 1;
}
