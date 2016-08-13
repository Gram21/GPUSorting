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

#ifndef _IGUI_ENABLED_COMPUTE_TASK_H
#define _IGUI_ENABLED_COMPUTE_TASK_H

#include "IComputeTask.h"

//! Common interface for task that have and OpenGL UI
/*!
	Currently we only use this interface in Assignment4
	to perform GL rendering and respond to user input with keyboard and mouse.
*/
class IGUIEnabledComputeTask : public IComputeTask
{
public:
	virtual ~IGUIEnabledComputeTask() {};

	// OpenGL render callback
	virtual void Render() = 0;

	virtual void OnKeyboard(int Key, int Action) = 0;

	virtual void OnMouse(int Button, int Action) = 0;
	virtual void OnMouseMove(int X, int Y) = 0;

	virtual void OnIdle(double Time, float ElapsedTime) = 0;

	virtual void OnWindowResized(int Width, int Height) = 0;
};


#endif // _IGUI_ENABLED_COMPUTE_TASK_H
