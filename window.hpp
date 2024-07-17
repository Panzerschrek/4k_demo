#pragma once
#include <cstdint>
#include <Windows.h>

inline void ZeroMemoryInline(void* const mem, const size_t size)
{
	// Use this function, because functions like "ZeroMemory" are expanded into "memset", which isn't available without standard library.
	RtlSecureZeroMemory(mem, size);
}

class DrawableWindow
{
public:
	using PixelType = uint32_t;

public:
	// All methods are inline - for further inlining and avoiding of unnecessary calls.

	DrawableWindow(const char* const title, const uint32_t width, const uint32_t height, const WNDPROC wnd_proc = WindowProc)
		: width_(width), height_(height)
	{
		const char* const window_class_name = "4k_demo";

		// Create window class.

		WNDCLASSEXA window_class;
		ZeroMemoryInline(&window_class, sizeof(window_class));
		window_class.cbSize = sizeof(WNDCLASSEX);
		window_class.style = CS_OWNDC;
		window_class.lpfnWndProc = wnd_proc;
		window_class.lpszClassName = window_class_name;

		RegisterClassExA(&window_class);

		// Create window.

		const DWORD window_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_BORDER;

		RECT rect{ 0, 0, LONG(width), LONG(height) };
		AdjustWindowRect(&rect, window_style, false);

		hwnd_ =
			CreateWindowExA(
				0,
				window_class_name,
				title,
				window_style,
				0, 0,
				rect.right - rect.left, rect.bottom - rect.top,
				nullptr,
				nullptr,
				0,
				nullptr);

		// Create stuff for drawing.

		hdc_ = GetDC(hwnd_);

		// Create bitmap for the whole screen size with 32-bit color.
		BITMAPINFO bitmap_info{};
		bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info.bmiHeader.biWidth = int(width_);
		bitmap_info.bmiHeader.biHeight = -int(height_);
		bitmap_info.bmiHeader.biSizeImage = -int(height_ * width_);
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;

		compatible_dc_ = CreateCompatibleDC(hdc_);

		const HBITMAP bitmap_handle =
			CreateDIBSection(hdc_, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels_), nullptr, 0);

		SelectObject(compatible_dc_, bitmap_handle);

		// Show the window.

		ShowWindow(hwnd_, SW_SHOWNORMAL);
	}

	// Save some executable space - do not bother with deinitialization/clearing.
	~DrawableWindow() = default;

	PixelType* GetPixels() const
	{
		return pixels_;
	}

	uint32_t GetWidth() const
	{
		return width_;
	}

	uint32_t GetHeight() const
	{
		return height_;
	}

	HDC GetBitmapDC() const
	{
		return compatible_dc_;
	}

	void ProcessMessages()
	{
		MSG message;
		while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) != 0)
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
	}

	void Blit()
	{
		BitBlt(hdc_, 0, 0, int(width_), int(height_), compatible_dc_, 0, 0, SRCCOPY);
	}

	static LRESULT CALLBACK WindowProc(const HWND hwnd, const UINT msg, const WPARAM w_param, const LPARAM l_param)
	{
		switch (msg)
		{
			// Do not bother quiting via "main" function - just exit process.
			// TODO - find more compact way to quit application.
		case WM_CLOSE:
		case WM_QUIT:
			ExitProcess(0);
			break;
		}

		return DefWindowProcA(hwnd, msg, w_param, l_param);
	}

private:
	const uint32_t width_;
	const uint32_t height_;
	HWND hwnd_;
	HDC hdc_;
	HDC compatible_dc_;
	PixelType* pixels_;
};
