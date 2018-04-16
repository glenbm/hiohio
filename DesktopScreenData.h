#pragma once

#include <windows.h>

class DesktopScreenData {

public:
	HDC handle;

	int width;
	int height;
	int bpp;

	DesktopScreenData()
	{
		GetDesktopData();
	}

	void GetDesktopData()
	{
		handle = CreateDCW(L"DISPLAY", L"\\\\.\\DISPLAY1", NULL, NULL);
		handle = GetDC(0);
		width = GetDeviceCaps(handle, HORZRES);
		height = GetDeviceCaps(handle, VERTRES);
		bpp = 32;
	}

	int GetDesktopBmpSize() {
		return (bpp / 8) * width * height;
	}

	int GetStride() {
		int hello = width * (bpp/8);
		int blue = ((((width * bpp) + 31)&~31) >> 3);;
		return((((width * bpp) + 31)&~31) >> 3);
	}

	RECT getScreenRECT()
	{
		return {0, 0, width, height};
	}

};