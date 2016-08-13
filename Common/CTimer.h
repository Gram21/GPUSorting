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

#ifndef _CTIMER_H
#define _CTIMER_H

//Simple wrapper class that can be used to measure time intervals
//using the built-in precision timer of the OS

// We reverted from std::chrono, because that timer implementation seems to be very imprecise
// (at least under windows)

#ifdef _WIN32

#include <Windows.h>

#elif defined (__APPLE__) || defined(MACOSX)

#include <sys/time.h>

#else

#include <sys/time.h>
#include <time.h>

#endif

//! Simple wrapper class for the measurement of time intervals
/*!
	Use this timer to measure elapsed time on the HOST side.
	Not suitable for measuring the execution of DEVICE code
	without synchronization with the HOST.

	NOTE: This class is not thread-safe (like most other classes in these
	examples), but we are not doing CPU multithreading in the praktikum...
*/
class CTimer
{
public:

	CTimer(){};

	~CTimer(){};

	void Start();

	void Stop();

	//! Returns the elapsed time between Start() and Stop() in ms.
	double GetElapsedMilliseconds();

protected:

#ifdef WIN32
	LARGE_INTEGER		m_StartTime;
	LARGE_INTEGER		m_EndTime;
#else
	struct timeval		m_StartTime;
	struct timeval		m_EndTime;
#endif
};

#endif // _CTIMER_H
