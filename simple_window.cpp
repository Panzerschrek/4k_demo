#include <windows.h>


static LRESULT CALLBACK WindowProc(const HWND hwnd, const UINT msg, const WPARAM w_param, const LPARAM l_param)
{
	switch(msg)
	{
	// Do not bothre quiting via "main" function - just exit process.
	// TODO - find more compact way to quit application.
	case WM_CLOSE:
	case WM_QUIT:
		ExitProcess(0);
		break;
	}

	return DefWindowProcA(hwnd, msg, w_param, l_param);
}

static const char* const g_window_class = "4k_demo";
static const char* const g_window_name = "4k_demo";

int main()
{
	// Create window class.

	WNDCLASSEXA window_class{};
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = CS_OWNDC;
	window_class.lpfnWndProc = WindowProc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = 0;
	window_class.hIcon = nullptr; // Use default icon.
	window_class.hCursor = LoadCursorA(nullptr, IDC_ARROW);
	window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	window_class.lpszMenuName = nullptr;
	window_class.lpszClassName = g_window_class;
	window_class.hIconSm = nullptr; // Use default icon.

	RegisterClassExA(&window_class);

	// Create window itself.

	const int window_width = 640;
	const int window_height = 480;

	const HWND hwnd =
		CreateWindowExA(
			0,
			g_window_class,
			g_window_name,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER,
			0, 0,
			window_width, window_height,
			nullptr,
			nullptr,
			0,
			nullptr);

	ShowWindow(hwnd, SW_SHOWNORMAL);

	// Main loop.

	while(true)
	{
		MSG message;
		while(PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}

		Sleep(16);
	}

	// Save some space - do not bother destroying window/unregister window class.
}
