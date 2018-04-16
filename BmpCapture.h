#pragma once

//
#include "windows.h"
#include <iostream>
#include <assert.h>
//

#include "GdiplusInit.h"
#include "DesktopScreenData.h"
#include "vector"
#include "GdiPlusInit.h"

#include <cstdlib>

#include <chrono>

class BmpCapture {
public:

	bool compressed = true;

	std::vector<char> GetBuffer()
	{
		return buffer_;
	}
	
	~BmpCapture()
	{
		ReleaseHandles();
	}

	BmpCapture()
	{

	}

	BmpCapture(DesktopScreenData *dekstopInfo, int targetWidth, int targetHeight, ULONG quality, RECT captureRect)
	{
		Init(dekstopInfo, targetWidth, targetHeight, quality, captureRect);
	}

	void Init(DesktopScreenData *dekstopInfo, int targetWidth, int targetHeight, ULONG quality,  RECT captureRect)
	{
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		desktopInfo_ = dekstopInfo;


		// assign handles for capturing
		HWND hWndDesktop = GetDesktopWindow();
		hSource_ = GetDC(hWndDesktop);
		hDest_ = CreateCompatibleDC(hSource_);


		// assign target dimensions
		width_ = targetWidth;
		height_ = targetHeight;

		if (width_ <= 0 || height_ <= 0) {
			std::cout << "exception: BmpCapture.BmpCapture(...):" << " width_ or height_ should not be 0 or less" << std::endl;
			return;
		}

		// Assign capture dimensions
		capDimensions_.width = std::abs(captureRect.right - captureRect.left);
		capDimensions_.height = std::abs(captureRect.bottom - captureRect.top);
		capDimensions_.x = captureRect.left;
		capDimensions_.y = captureRect.top;

		if (capDimensions_.x < 0)
		{
			capDimensions_.x = 0;
		}

		if (capDimensions_.y < 0)
		{
			capDimensions_.y = 0;
		}

		if (capDimensions_.width <= 0 || capDimensions_.height <= 0)
		{
			std::cout << "exception: BmpCapture.BmpCapture(...):" << " width or height of capDimensions_ should not be 0 or less" << std::endl;
			return;
		}


		// assign bitmapinfo header
		bitmapInfo_ = { 0 };
		bitmapInfo_.bmiHeader.biSize = sizeof(bitmapInfo_.bmiHeader);
		bitmapInfo_.bmiHeader.biWidth = width_;
		bitmapInfo_.bmiHeader.biHeight = -height_;
		bitmapInfo_.bmiHeader.biPlanes = 1;
		bitmapInfo_.bmiHeader.biCompression = BI_RGB;
		bitmapInfo_.bmiHeader.biBitCount = dekstopInfo->bpp;


		// create the device independant bitmap
		hBitmap_ = CreateDIBSection(hDest_, &bitmapInfo_, DIB_RGB_COLORS, (void**)&pBits_, NULL, 0);
		if (!hBitmap_)
		{
			std::cout << "exception: BmpCapture.BmpCapture(...):" << " failed to create a DIB section" << std::endl;
			return;
		}

		if(pBits_ == NULL)
		{
			std::cout << "exception: BmpCapture.BmpCapture(...):" << " pBits_ should not be NULL" << std::endl;
			return;
		}
		
		if (!SelectObject(hDest_, hBitmap_))
		{
			std::cout << "exception: BmpCapture.BmpCapture(...):" << " SelectObject() failed" << std::endl;
			return;
		}

		buffer_.resize( width_ * height_ * (desktopInfo_->bpp/8) );

		InitializeJpegEncoder(quality);

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Initializing a bmpCapture, time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
	}

	void InitializeJpegEncoder(ULONG quality)
	{
		encoderParams_.Count = 1;
		encoderParams_.Parameter[0].NumberOfValues = 1;
		encoderParams_.Parameter[0].Guid = Gdiplus::EncoderQuality;
		encoderParams_.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
		encoderParams_.Parameter[0].Value = &quality;
	}

	void CaptureScreen()
	{
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		BitBltOrStretchBlt();

		int bufferSize = width_ * height_ * 4;

		buffer_ = std::vector<char>(pBits_, pBits_ + bufferSize);

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << std::endl << "Capturing the desktop(" << width_ << "/" << height_ <<  "), time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
	}

	void CaptureDiff(BmpCapture *cap, int x, int y)
	{
		if (!BitBlt(hDest_, 0, 0, width_, height_, cap->hDest_, x, y, SRCCOPY))
		{
			std::cout << "exception: BmpCapture.CaptureDiff(...):" << " failed to grab the desktop via BitBlt" << std::endl;
			return;
		}

		if (pBits_ == NULL)
		{
			std::cout << "exception: BmpCapture.BmpCapture(...):" << " pBits_ should not be NULL" << std::endl;
			return;
		}

		int bufferSize = (width_ * height_ * 4);
		buffer_ = std::vector<char>(pBits_, pBits_ + bufferSize);
	}

	void ReleaseHandles()
	{
		DeleteDC(hDest_);
		DeleteObject(hBitmap_);
	}

	static int Difference(RECT *capRect, BmpCapture *imgOne, BmpCapture *imgTwo)
	{
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		int top = -1;
		int bottom = -1;
		int left = -1;
		int right = -1;

		auto pixelBufferOne = (int*)imgOne->buffer_.data();
		auto pixelBufferTwo = (int*)imgTwo->buffer_.data();

		if (imgOne->buffer_ == imgTwo->buffer_)
		{
			//buffers are the exact same, so no differences to find
			capRect = new RECT{0, 0, imgOne->width_, imgOne->height_};
			return 2;
		}

		if (imgOne->buffer_.size() != imgTwo->buffer_.size())
		{
			std::cout << "exception: BmpCapture.Difference(...):" << " the two buffers are not the same size" << std::endl;
			capRect = new RECT{ 0, 0, imgOne->width_, imgOne->height_ };
			return 0;
		}

		// for every row
		for (int row = 0; row < imgOne->height_; row++)
		{
			//std::cout << "asessing row:" << row << "/" << imgOne.height_ << std::endl;

			// for every pixel
			for (int pixel = 0; pixel < imgOne->width_; pixel += 4)
			{

				//get current index of where the pixel data starts
				auto offsetCount = pixel + (row * imgOne->width_);

				// get value of pixel by getting the sum of all its data
				int pixelNumberOne = pixelBufferOne[offsetCount] +
					pixelBufferOne[offsetCount + 1] + 
					pixelBufferOne[offsetCount + 2] + 
					pixelBufferOne[offsetCount + 3];

				int pixelNumberTwo = pixelBufferTwo[offsetCount] +
					pixelBufferTwo[offsetCount + 1] +
					pixelBufferTwo[offsetCount + 2] +
					pixelBufferTwo[offsetCount + 3];
				
				if (pixelNumberOne != pixelNumberTwo)
				{
					auto temp = pixel;

					if (top == -1)
					{
						top = row;
						if (bottom == -1)
						{
							bottom = row + 1;
						}
					}
					else
					{
						bottom = row + 1;
					}

					if (left == -1)
					{
						left = pixel;
						if (right == -1)
						{
							right = pixel + 1;
						}
					}
					else if (pixel < left)
					{
						left = pixel;
					}
					else if (right < pixel)
					{
						right = pixel + 1;
					}
				}
			}
		}

		if ((right - left <= 0) || (bottom - top <= 0))
			return 0;

		if (left > 0)
			left -= 1;
		if (top > 0)
			top -= 1;
		if (top > 0)
			top -= 1;
		if (top > 0)
			top -= 1;
		if (right < imgTwo->width_ - 1)
			right += 1;
		if (right < imgTwo->width_ - 1)
			right += 1;
		if (right < imgTwo->width_ - 1)
			right += 1;
		if (bottom < imgTwo->height_ - 1)
			bottom += 1;

		capRect->top = top;
		capRect->left = left;
		capRect->right = right;
		capRect->bottom = bottom;

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Getting the difference RECT, time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
	}

	int GetJpegInMemory(LONG quality, std::vector<char> *buff)
	{
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		if (quality < 0)
		{
			quality = 0;
		}

		if (quality > 100)
		{
			quality = 100;
		}

		GdiplusInit initGDI;
		CLSID clisd;

		IStream *stream = NULL;
		CreateStreamOnHGlobal(NULL, FALSE, (LPSTREAM*)&stream);

		Gdiplus::Bitmap *bmp = new  Gdiplus::Bitmap(hBitmap_, (HPALETTE)NULL);

		if (!GetEncoderClsid(ImageType::JPG, &clisd))
		{
			std::cout << "exception: BmpCapture.GetJpegInMemory(...):"  << " failed to get encoder" << std::endl;
			return 0;
		}

		try {
			bmp->Save(stream, &clisd, &encoderParams_);
		}
		catch (std::string e) {
			std::cout << "exception: BmpCapture.GetJpegInMemory(...):"  << " failed to encode the bitmap [bmp->save(...)]" << std::endl;
			return 0;
		}

		delete bmp;

		const LARGE_INTEGER largeIntegerBeginning = { 0 };
		ULARGE_INTEGER largeIntEnd = { 0 };
		stream->Seek(largeIntegerBeginning, STREAM_SEEK_CUR, &largeIntEnd);
		int jpegBufferSize = (ULONG)largeIntEnd.QuadPart;

		HGLOBAL pngInMemory;
		const HRESULT hResult = GetHGlobalFromStream(stream, &pngInMemory);

		LPVOID lpPngStreamBytes = GlobalLock(pngInMemory);

		char *charBuf = (char*)lpPngStreamBytes;

		if (jpegBufferSize <= 0)
		{
			std::cout << "exception: BmpCapture.GetJpegInMemory(...):" << "  jpegBufferSize should not be 0 or less" << std::endl;
			return 0;
		}

		buff = &std::vector<char>(charBuf, 2 + charBuf + jpegBufferSize);

		//CloseHandle(pngInMemory);

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::cout << "Converting to JPEG in memory, time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
		
		compressed = true;
		return 1;
	}

	void SaveToDisk(bool compressed)
	{
		GdiplusInit initGDI;
		CLSID clisd;

		Gdiplus::Bitmap bmp(hBitmap_, NULL);

		if (!compressed)
		{
			GetEncoderClsid(ImageType::BMP, &clisd);
			bmp.Save(L"C://Users//Glen//Desktop//TWO.bmp", &clisd, &encoderParams_);
		}
		else
		{
			GetEncoderClsid(ImageType::JPG, &clisd);
			bmp.Save(L"C://Users//Glen//Desktop//TWO.jpg", &clisd, NULL);
		}
	}

private:

	struct CaptureDimensions {
		int width;
		int height;
		int x;
		int y;
	};

	HDC hDest_ = NULL;
	HDC hSource_ = NULL;
	HBITMAP hBitmap_ = NULL;
	HGDIOBJ hOldBitmap_ = NULL;

	BITMAPINFO bitmapInfo_{};

	DesktopScreenData *desktopInfo_ = NULL;
	CaptureDimensions capDimensions_;

	Gdiplus::EncoderParameters encoderParams_;

	char* pBits_ = NULL;

	int width_ = 0, height_ = 0;

	std::vector<char> buffer_{};

	void BitBltOrStretchBlt()
	{
		// Bitblt is faster, so avoid using stretchblt if no scaling is needed
		if (width_ == capDimensions_.width && height_ == capDimensions_.height) {
			if (!BitBlt(
				hDest_,
				0,
				0,
				capDimensions_.width,
				capDimensions_.height,
				hSource_,
				capDimensions_.x,
				capDimensions_.y,
				SRCCOPY))
			{
				std::cout << "exception: BmpCapture.BitBltOrStretchBlt(...):" << " failed to grab the desktop via BitBlt" << std::endl;
				return;
			}
			return;
		}

		SetStretchBltMode(hSource_, HALFTONE);
		if (!StretchBlt(
			hDest_,
			0,
			0,
			width_,
			height_,
			hSource_,
			capDimensions_.x,
			capDimensions_.y,
			capDimensions_.width,
			capDimensions_.height,
			SRCCOPY
		))
		{
			std::cout << "exception: BmpCapture.BitBltOrStretchBlt(...):" << " failed to grab the desktop via StretchBlt" << std::endl;
			return;
		}
	}

};