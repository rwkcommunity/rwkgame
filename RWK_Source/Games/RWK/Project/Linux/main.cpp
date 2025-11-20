#include "MyApp.h"
#include "os_core.h"

MyApp gApp;

void AppThread(void *theArg)
{
	gApp.Go();
}

//
// Windows entry point...
//
int main(const int argc, char **argv)
{
	String aCMD;
	for (int aCount=0;aCount<argc;aCount++)
	{
		if (aCount>0) aCMD+=" ";
		String aFix=argv[aCount];
		aFix.Replace(" ","{::SPACE::}");
		aCMD+=aFix;
	}

#ifdef AppRunsInThread

	//
	// Note: This main tries to emulate the behavior of Cocoa and other visual systems, where
	// the startup and shutdown will be handled by an overarching class.  On those systems, AppThread will
	// usually be invoked in a function within that overarching class.
	//

	OS_Core::Startup();
	OS_Core::SetCommandLine(aCMD);
    OS_Core::Thread(AppThread,NULL);
    OS_Core::SetThreadPriority(-.9f);

	while (!gApp.mInitComplete) OS_Core::Sleep(1);
	while (!gApp.IsShutdown()) {OS_Core::Pump();OS_Core::Sleep(1);}
	OS_Core::Shutdown();
#else

	//
	// Note: This Winmain runs threadless. Use it on systems when you can use something like PeekMessage
	// to push message processing.
	//

	OS_Core::Startup();
	OS_Core::SetCommandLine(aCMD);
	AppThread(NULL);
	OS_Core::Shutdown();

#endif
}

