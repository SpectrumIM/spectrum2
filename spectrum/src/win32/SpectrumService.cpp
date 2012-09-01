#include "SpectrumService.h"

SpectrumService::SpectrumService(void) {
	serviceName = "Spectrum2";
	displayName = "Spectrum2 XMPP Transport";
	username = NULL;
	password = NULL;
}

SpectrumService::~SpectrumService(void) {}

void SpectrumService::Stop() {
	ReportStatus((DWORD)SERVICE_STOP_PENDING);
}

void SpectrumService::Run(DWORD argc, LPTSTR *argv) {
	ReportStatus((DWORD)SERVICE_RUNNING);
	main(argc, argv);
}