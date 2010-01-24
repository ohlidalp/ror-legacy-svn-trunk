/*
The zlib/libpng License

Copyright (c) 2005-2007 Phillip Castaneda (pjcast -- www.wreckedgames.com)

This software is provided 'as-is', without any express or implied warranty. In no event will
the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial 
applications, and to alter it and redistribute it freely, subject to the following
restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that 
		you wrote the original software. If you use this software in a product, 
		an acknowledgment in the product documentation would be appreciated but is 
		not required.

    2. Altered source versions must be plainly marked as such, and must not be 
		misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/
#ifndef OIS_Win32LogitechLEDs_H
#define OIS_Win32LogitechLEDs_H

#include "OISPrereqs.h"
#include "OISInterface.h"
#include "Win32/Win32Prereqs.h"

namespace OIS
{
	class Win32LogitechLEDs : public Interface
	{
	public:
		Win32LogitechLEDs(IDirectInputDevice8* pDIJoy)
		{
			mJoyStick=pDIJoy;
		}
		~Win32LogitechLEDs() {};
		bool play(float currentRPM, float rpmFirstLedTurnsOn, float rpmRedLine)
			{
				if (!mJoyStick) return false;

				WheelData wheelData_;
				ZeroMemory(&wheelData_, sizeof(wheelData_));

				wheelData_.size = sizeof(WheelData);
				wheelData_.versionNbr = LEDS_VERSION_NUMBER;
				wheelData_.rpmData.currentRPM = currentRPM;
				wheelData_.rpmData.rpmFirstLedTurnsOn = rpmFirstLedTurnsOn;
				wheelData_.rpmData.rpmRedLine = rpmRedLine;

				DIEFFESCAPE data_;
				ZeroMemory(&data_, sizeof(data_));

				data_.dwSize = sizeof(DIEFFESCAPE);
				data_.dwCommand = ESCAPE_COMMAND_LEDS;
				data_.lpvInBuffer = &wheelData_;
				data_.cbInBuffer = sizeof(wheelData_);

				return SUCCEEDED(mJoyStick->Escape(&data_));

			}

	protected:
		static CONST DWORD ESCAPE_COMMAND_LEDS = 0;

		static CONST DWORD LEDS_VERSION_NUMBER = 0x00000001;

		struct LedsRpmData
		{
			FLOAT currentRPM;
			FLOAT rpmFirstLedTurnsOn;
			FLOAT rpmRedLine;
		};

		struct WheelData
		{
			DWORD size;
			DWORD versionNbr;
			LedsRpmData rpmData;
		};

		// Joystick device descriptor.
		IDirectInputDevice8* mJoyStick;
	};
}
#endif //OIS_Win32LogitechLEDs_H
