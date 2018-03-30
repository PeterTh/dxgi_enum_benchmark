#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>

#include <Windows.h>

#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

#include <PhysicalMonitorEnumerationAPI.h>
#pragma comment(lib, "dxva2.lib")

constexpr int NUM_IT = 500;
constexpr int MAX_MODES = 10000;

// Yes, this is really bad style
// No, it really doesn't matter in this context
// Just please don't copy/paste this into a real program
DXGI_MODE_DESC modes[MAX_MODES];

int main(int argc, char** argv) {

	IDXGIFactory *pFactory = nullptr;
	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory));
	if(!SUCCEEDED(hr) || !pFactory) exit(-1);

	IDXGIAdapter *pAdapter = nullptr;
	hr = pFactory->EnumAdapters(0, &pAdapter);
	if(!SUCCEEDED(hr) || !pAdapter) exit(-1);

	std::vector<IDXGIOutput*> vOutputs;
	{
		UINT i = 0;
		IDXGIOutput *pOutput;
		while(pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND) {
			vOutputs.push_back(pOutput);
			++i;
		}
	}


	for(auto output : vOutputs) {

		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);
		std::cout << "\nBenchmarking display mode queries on output ";
		std::wcout << desc.DeviceName << "\n";

		DWORD numMonitors;
		if(!GetNumberOfPhysicalMonitorsFromHMONITOR(desc.Monitor, &numMonitors)) {
			std::cerr << "Error: could not get number of physical monitors\n";
			exit(-1);
		}
		LPPHYSICAL_MONITOR monitorinfos = (LPPHYSICAL_MONITOR)alloca(sizeof(PHYSICAL_MONITOR) * numMonitors);
		if(!GetPhysicalMonitorsFromHMONITOR(desc.Monitor, numMonitors, monitorinfos)) {
			std::cerr << "Error: could not get physical monitor description\n";
			exit(-1);
		}
		for(DWORD i = 0; i < numMonitors; ++i) {
			std::cout << "    associated with monitor ";
			std::wcout << monitorinfos->szPhysicalMonitorDescription << "\n";
		}

		UINT numModes;
		// We perform 2 calls per iteration, since that's the only way to really do a correct complete query
		// given the way the API is set up
		auto startTime = std::chrono::high_resolution_clock::now();
		for(int i = 0; i < NUM_IT; ++i) {
			output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING, &numModes, NULL);
			if(numModes > MAX_MODES) {
				std::cerr << "Error: too many modes\n";
				exit(-1);
			}
			output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING, &numModes, modes);
		}
		auto endTime = std::chrono::high_resolution_clock::now();

		std::cout << "Number of modes: " << std::setw(5) << numModes << "\n";
		std::cout << std::setw(12) << std::fixed << std::setprecision(2)
			<< (NUM_IT * 1000000.0) / std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count()
			<< " Full Display Mode Queries / second\n";
	}
}
