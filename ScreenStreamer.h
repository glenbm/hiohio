#pragma once

#include <iostream>

#include "DesktopScreenData.h"
#include "BmpCapture.h"


class ScreenStreamer {
public:

	DesktopScreenData screenData_;
	BmpCapture capOne_;
	BmpCapture capTwo_;

	BmpCapture *curCap_;
	BmpCapture *prevCap_;

	ScreenStreamer(int width, int height, int quality = 70, bool captureDeltas = true, bool compressCaptures = true, RECT captureRect = { 0,0,0,0 })
	{
		screenData_.GetDesktopData();

		screenData_.height = 1920;
		screenData_.width = 1080;

		if (quality > 100)	quality = 100;
		if (quality < 0)	quality = 0;

		compressionQuality_ = (ULONG)quality;

		if (captureRect.right + captureRect.left + captureRect.top + captureRect.bottom == 0)
		{
			captureRect = screenData_.getScreenRECT();
		}

		// Init two captures to capture the full desktop screen
		capOne_.Init(&screenData_, width, height, compressionQuality_, captureRect);
		capTwo_.Init(&screenData_, width, height, compressionQuality_, captureRect);

		captureDeltas_ = captureDeltas;
		compressCaptures_ = compressCaptures;

		curCap_ = &capOne_;
		prevCap_ = &capTwo_;
	}

	void Run()
	{
		std::vector<char> buffer;
		if (CaptureScreen(&buffer))
		{
			// websocket send (buffer)
		}
		Sleep(1000);
	}

	void DeledageSendData(std::vector<char>)
	{

	}

	int GetCaptureCount()
	{
		return captureCount_;
	}

	bool CaptureScreen(std::vector<char> *buffer)
	{
		curCap_->CaptureScreen();

		// get if we can get a delta yet
		if (captureCount_ > 0 && captureDeltas_)
		{
			RECT capRect;
			
			int status = BmpCapture::Difference(&capRect, curCap_, prevCap_);
			switch (status)
			{
			case 2:
				// screen has not changed
				return false;
				break;
			}

			int width = std::abs(capRect.right - capRect.left);
			int height = std::abs(capRect.bottom - capRect.top);

			BmpCapture diff(&screenData_, width, height, compressionQuality_, { 0, 0, width, height });
			diff.CaptureDiff(curCap_, capRect.left, capRect.top);

			diff.SaveToDisk(compressCaptures_);

			if (compressCaptures_)
			{
				if (!diff.GetJpegInMemory(compressionQuality_, buffer))
				{
					std::cout << "exception: ScreenStreamer.CaptureScreen(...):" << " failed to encode to JPG" << std::endl;
					return;
				}
			}
			else
			{
				buffer = &diff.GetBuffer();
			}
		}
		else
		{
			if (compressCaptures_)
			{
				if (!curCap_->GetJpegInMemory(100, buffer))
				{
					std::cout << "exception: ScreenStreamer.CaptureScreen(...):"  << " failed to encode to JPG" << std::endl;
					return;
				}
			}
			else
			{
				buffer = &curCap_->GetBuffer();
			}
		}

		captureCount_++;
		SwapBuffers();

		return true;
	}

private:

	int captureCount_;
	bool compressCaptures_;
	bool captureDeltas_;
	ULONG compressionQuality_;

	void SwapBuffers()
	{
		if (curCap_ == &capOne_)
		{
			curCap_ = &capTwo_;
			prevCap_ = &capOne_;
			return;
		}

		curCap_ = &capOne_;
		prevCap_ = &capTwo_;
	}

};