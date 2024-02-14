#include "D3DSample.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	int result = 0;
	{
		terrain::D3DSample sample(hInstance, 1920, 1080, L"TestApp");

		if (!sample.Init())
		{
			return 0;
		}
		result = sample.Run();
	}

	_CrtDumpMemoryLeaks();

	return result;
}