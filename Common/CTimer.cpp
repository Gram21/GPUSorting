/******************************************************************************
GPU Computing / GPGPU Praktikum source code.

******************************************************************************/

#include "CTimer.h"

///////////////////////////////////////////////////////////////////////////////
// CTimer

void CTimer::Start()
{
#ifdef _WIN32
	QueryPerformanceCounter(&m_StartTime);
#else
	gettimeofday(&m_StartTime, NULL);
#endif
}

void CTimer::Stop()
{
#ifdef _WIN32
	QueryPerformanceCounter(&m_EndTime);
#else
	gettimeofday(&m_EndTime, NULL);
#endif
}

double CTimer::GetElapsedMilliseconds()
{
#ifdef _WIN32
	LARGE_INTEGER freq;
	if(QueryPerformanceFrequency(&freq))
	{
		return 1000.0 * double(m_EndTime.QuadPart - m_StartTime.QuadPart) / double(freq.QuadPart);
	}
	else
	{
		return -1;
	}
#else
	double delta = ((double)m_EndTime.tv_sec + 1.0e-6 * (double)m_EndTime.tv_usec) - 
		((double)m_StartTime.tv_sec + 1.0e-6 * (double)m_StartTime.tv_usec);
	return 1000.0 * delta;
#endif
}

///////////////////////////////////////////////////////////////////////////////
