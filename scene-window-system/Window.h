#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>


class Window
{
public:
	Window(HINSTANCE hInstance, LPCTSTR windowName, LPCTSTR windowTitle, int width, int height);
	HWND GetHandle() const;
	UINT width() const { return m_Width; }
	UINT height() const { return m_Height; }
	float aspectRatio() const;
	void SetTitle(const char* title);
private:
	UINT m_Width;
	UINT m_Height;
	HWND hwnd;
};