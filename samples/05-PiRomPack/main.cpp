//
// main.cpp
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include <circle/startup.h>
#include <circle/gpiopin.h>
#include <circle/gpiopinfiq.h>
#include <circle/usb/usbkeyboard.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#define MREQ_PIN 26
#define IORQ_PIN 19
#define CLK_PIN

unsigned char rom[4 * 1024 * 1024];
extern "C" {
	void start_l1cache();
}
extern void MSXHandler(void *);
extern void RPMV_Init(unsigned char *, int);
extern void RPMV_Wait(int);
int main (void)
{
	// cannot return here because some destructors used in CKernel are not implemented
	int size = 0;
	start_l1cache();
	CKernel Kernel;
	if (!Kernel.Initialize ())
	{
		halt ();
		return EXIT_HALT;
	}
//	Kernel.Run (); 
	Kernel.Cleanup();
	CInterruptSystem	mInterrupt; 
	mInterrupt.Initialize();
#if 0	
	CGPIOPin AudioLeft (18, GPIOModeAlternateFunction5);	
	CDeviceNameService	mDeviceNameService;
	CTimer				mTimer(&mInterrupt);
	CEMMCDevice			mEMMC (&mInterrupt, &mTimer);
	CFATFileSystem  	mFileSystem;	
	EnableIRQs();
	mEMMC.Initialize();
	mFileSystem.Mount(mDeviceNameService.GetDevice (DEFAULT_PARTITION, true));
	CGlueStdioInit (mFileSystem);
	char const stdio_filename[] = "stdio2.txt";
	FILE *fp = fopen(stdio_filename, "w");
	fprintf(fp, "Hello!\n");
	fclose(fp);
	mFileSystem.UnMount ();
#endif
	RPMV_Init(rom, size);
	CGPIOPinFIQ		mInputPin(IORQ_PIN, GPIOModeInput, &mInterrupt);
	mInputPin.ConnectInterrupt (ARM_FIQ_GPIO0, MSXHandler, 0);	
//	mInputPin.EnableInterrupt (GPIOInterruptOnFallingEdge);
	mInputPin.EnableInterrupt (MREQ_PIN, GPIOInterruptOnFallingEdge);
	mInterrupt.DisableIRQ (ARM_IRQ_TIMER3);
//	mInterrupt.DisableIRQ (ARM_IRQ_TIMER3);
	while(true);
}
