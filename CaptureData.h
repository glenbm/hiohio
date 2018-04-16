#pragma once

#include <windows.h>
#include <vector>

class CaptureData {
public:

	HDC					_memDC;
	char*				_bits;
	HBITMAP				_hBmp;

	std::vector<char>	_buffer;

	int _jpegCaptureSize;

	void Initialize(HDC screen, BITMAPINFO *bmpInfo, int targetWidth, int targetHeight);

	void GrabDesktop(HDC screen, int targetWidth, int targetHeight, int width, int height, int srcPosx, int srcPosY);

	void EncodeJpeg(ULONG quality);

	std::vector<char> GetScreenBuffer(int captureSize);

	bool CompareBuffer(CaptureData *compareCapture, int captureSize);

	void GetDelta(CaptureData targetCapture, HDC screen, int targetWidth, int targetHeight);

};

