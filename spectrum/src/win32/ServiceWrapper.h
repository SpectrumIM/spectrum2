#pragma once
#include "transport/config.h"
#include <windows.h>
#include <tchar.h>

class ServiceWrapper
{
public:
	ServiceWrapper(LPSTR serviceName);
	~ServiceWrapper(void);
	bool Install(LPSTR commandLine);
	bool UnInstall();
	bool IsInstalled();
	void RunService();
};

int mainloop();
BOOL spectrum_control_handler( DWORD fdwCtrlType );

