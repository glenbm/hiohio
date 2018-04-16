// Screen_Capture_App.cpp : Defines the entry point for the console application.
//

#include <algorithm>
#include "stdafx.h"
#include "ScreenStreamer.h"


int main()
{
	ScreenStreamer streamer(1920, 1080, 70, true, true, {0,0,1920,1080});

	streamer.Run();

	return 0;
}

