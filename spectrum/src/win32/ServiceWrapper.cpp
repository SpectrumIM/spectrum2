#include "ServiceWrapper.h"


LPSTR ServiceName;
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE ServiceStatusHandle;	

bool doAction (int action, int desiredAccess, LPSTR path = NULL);

enum actions {
	DO_INSTALL, DO_DELETE, DO_CHECK 
};


ServiceWrapper::ServiceWrapper(LPSTR serviceName)
{
	ServiceName = serviceName;
	ServiceStatusHandle = 0;	
}

ServiceWrapper::~ServiceWrapper(void)
{
}

bool ServiceWrapper::Install(LPSTR commandLine) {
	return doAction(DO_INSTALL, SC_MANAGER_ALL_ACCESS, commandLine);		
}

bool ServiceWrapper::UnInstall() {
	return doAction(DO_DELETE, SC_MANAGER_ALL_ACCESS);
}

bool ServiceWrapper::IsInstalled() {
	return doAction(DO_CHECK, SC_MANAGER_CONNECT);	
}

bool doAction(int action, int desiredAccess, LPSTR path) {
	SC_HANDLE scm = OpenSCManager(NULL, NULL, desiredAccess);
	SC_HANDLE service = NULL;
	if (!scm) return FALSE;

	switch(action) {
	case DO_INSTALL:
		service = CreateServiceA(
			scm, 
			ServiceName, 
			ServiceName, 
			SERVICE_ALL_ACCESS, 
			SERVICE_WIN32_OWN_PROCESS, 
			SERVICE_DEMAND_START, 
			SERVICE_ERROR_NORMAL, 
			path,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL
			);
		return (service != NULL);
		break;
	case DO_DELETE:
		service = OpenServiceA(scm, ServiceName, DELETE);
		if (service == NULL)
			return FALSE;
		if (DeleteService(service))
			return TRUE;
		break;
	case DO_CHECK:
		service = OpenServiceA(scm, ServiceName, SERVICE_QUERY_STATUS);
		return (service != NULL);
	default:
		return FALSE;
	}
	CloseServiceHandle(service);		
	CloseServiceHandle(scm);
	return FALSE;
}

void WINAPI ServiceControlHandler(DWORD controlCode) {
	switch (controlCode) {
	case SERVICE_CONTROL_INTERROGATE:
		break;
	case SERVICE_CONTROL_STOP:
		ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;		
		break;
	default:
		break;
	}
	SetServiceStatus(ServiceStatusHandle, &ServiceStatus);	
	stop();
}

void WINAPI ServiceMain(DWORD argc, LPSTR *argv) {
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;

	ServiceStatusHandle = RegisterServiceCtrlHandlerA(ServiceName, ServiceControlHandler);
	if (ServiceStatusHandle) {
		ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
		SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
		ServiceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		ServiceStatus.dwCurrentState = SERVICE_RUNNING;
		SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
		mainloop();
		ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
		ServiceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(ServiceStatusHandle, &ServiceStatus);
	}	
}

void ServiceWrapper::RunService() {
	SERVICE_TABLE_ENTRYA serviceTable[] = {
		{ ServiceName, ServiceMain },
		{ NULL, NULL}
	};

	StartServiceCtrlDispatcherA(serviceTable);
}