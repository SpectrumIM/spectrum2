#include <windows.h>
#include "WindowsService.h"

class SpectrumService : public WindowsService {

public:
	SpectrumService(void);
	~SpectrumService(void);
protected:
	void Stop();
	void Run(DWORD argc, LPTSTR *argv);
};

int main(int argc, char **argv);