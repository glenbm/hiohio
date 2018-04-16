#include "stdafx.h"

#include "CaptureData.h"
#include "DesktopCapture.h"
#include "GdiplusInit.h"

void CaptureData::Initialize(HDC screen, BITMAPINFO *bmpInfo, int targetWidth, int targetHeight) {
	_memDC = CreateCompatibleDC(screen);
	_bits = NULL;
	_hBmp = CreateDIBSection(
		screen,
		bmpInfo,
		DIB_RGB_COLORS,
		(void**)&_bits,
		NULL,
		0
	);

	if (!SelectObject(_memDC, _hBmp))
	{

	}

	if (!PatBlt(
		_memDC,
		0,
		0,
		targetWidth,
		targetHeight,
		BLACKNESS
	))
	{

	}
}

void CaptureData::GrabDesktop(HDC screen, int targetWidth, int targetHeight, int width, int height, int srcPosx, int srcPosY) {
	SetStretchBltMode(_memDC, HALFTONE);
	StretchBlt(_memDC,
		0,
		0,
		targetWidth,
		targetHeight,
		screen,
		srcPosx,
		srcPosY,
		width,
		height,
		SRCCOPY
	);

}

void CaptureData::EncodeJpeg(ULONG quality) {
	GdiplusInit gdiplusInit;

	CLSID encoderClsid;
	int result = GetEncoderClsid(ImageType::JPG, &encoderClsid);

	IStream *stream = NULL;
	CreateStreamOnHGlobal(NULL, FALSE, (LPSTREAM*)&stream);

	Gdiplus::EncoderParameters encoderParams;
	encoderParams.Count = 1;
	encoderParams.Parameter[0].NumberOfValues = 1;
	encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
	encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
	encoderParams.Parameter[0].Value = &quality;

	Gdiplus::Bitmap *bmp = new  Gdiplus::Bitmap(_hBmp, (HPALETTE)NULL);

	bmp->Save(stream, &encoderClsid, &encoderParams);
	bmp->Save(L"C:\\Users\\Glen\\Desktop\\CUR.jpg", &encoderClsid, &encoderParams);

	const LARGE_INTEGER largeIntegerBeginning = { 0 };
	ULARGE_INTEGER largeIntEnd = { 0 };
	stream->Seek(largeIntegerBeginning, STREAM_SEEK_CUR, &largeIntEnd);
	_jpegCaptureSize = (ULONG)largeIntEnd.QuadPart;

	HGLOBAL pngInMemory;
	const HRESULT hResult = GetHGlobalFromStream(stream, &pngInMemory);

	LPVOID lpPngStreamBytes = GlobalLock(pngInMemory);

	char *charBuf = (char*)lpPngStreamBytes;
	_buffer = std::vector<char>(charBuf, charBuf + _jpegCaptureSize);
}

std::vector<char> CaptureData::GetScreenBuffer(int captureSize) {
	std::vector<char> buffer(_bits, _bits + captureSize);
	return buffer;
}

bool CaptureData::CompareBuffer(CaptureData *compareCapture, int captureSize) {
	return GetScreenBuffer(captureSize) == compareCapture->GetScreenBuffer(captureSize);
}

void CaptureData::GetDelta(CaptureData targetCapture, HDC screen, int targetWidth, int targetHeight) {
	BitBlt(
		_memDC,
		0,
		0,
		targetWidth,
		targetHeight,
		targetCapture._memDC,
		0,
		0,
		SRCINVERT
	);
}
