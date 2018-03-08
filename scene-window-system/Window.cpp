#include "Window.h"
#include <stdexcept>

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (MessageBox(0, "Are you a quitter?", "QUEST: A true quitter", MB_YESNO | MB_ICONQUESTION) == IDYES)
				DestroyWindow(hwnd);
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

Window::Window(HINSTANCE hInstance, LPCTSTR windowName, LPCTSTR windowTitle, int width, int height)
	: m_Width(width), m_Height(height)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 2);
	wc.lpszClassName = windowName;
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		throw std::runtime_error("Could not register window class");
	}

	hwnd = CreateWindowEx(0,
		windowName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		nullptr, nullptr,
		hInstance, nullptr);
	if (!hwnd)
	{
		throw std::runtime_error("Could not create window");
	}

	//if everything went well, show the window.
	ShowWindow(hwnd, true);
	UpdateWindow(hwnd);
}

float Window::aspectRatio() const
{
	return static_cast<float>(m_Width) / static_cast<float>(m_Height);
}

void Window::SetTitle(const char* title)
{
	SetWindowText(hwnd, title);
}

HWND Window::GetHandle() const
{
	return hwnd;
}
