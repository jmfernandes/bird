/*
 * This file is part of libphidget22
 *
 * Copyright 2015 Phidgets Inc <patrick@phidgets.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>
 */

#ifndef __PhidgetUSB
#define __PhidgetUSB

#if defined(_MACOSX) && !defined(_IPHONE)
#include <IOKit/usb/IOUSBLib.h>
#endif

#define MAX_USB_IN_PACKET_SIZE				64
#define MAX_USB_OUT_PACKET_SIZE				64

typedef struct {
	PHIDGET_STRUCT_START

#ifdef _WINDOWS
	WCHAR  DevicePath[128];
	WCHAR  DevicePDOName[128];
	OVERLAPPED asyncRead;
	BOOL readPending;
	HANDLE closeReadEvent;
	OVERLAPPED asyncWrite;
	unsigned char inbuf[MAX_USB_IN_PACKET_SIZE + 1];
#elif defined(_MACOSX) && !defined(_IPHONE)
	io_object_t usbDevice;
	IOUSBInterfaceInterface **intf;
	int interfaceNum;
#elif defined(_LINUX) || defined (_FREEBSD)
	void *dev;
	char uniqueName[20];
	int tryAgainCounter;
	int interfaceNum;
#endif
#if defined(_ANDROID)
	char dev[256];
#endif

	unsigned short outputReportByteLength;
	unsigned short inputReportByteLength;

	unsigned char lastReadPacket[MAX_USB_IN_PACKET_SIZE];

	unsigned char interruptOutEndpoint;

	mos_mutex_t usbwritelock; /* protects write - exclusive */

	mos_mutex_t readLock;
	mos_task_t readThread;
	mos_cond_t readCond;
	int readRun;

	HANDLE deviceHandle;

} PhidgetUSBConnection, *PhidgetUSBConnectionHandle;

PhidgetReturnCode PhidgetUSBConnectionCreate(PhidgetUSBConnectionHandle *conn);
PhidgetUSBConnectionHandle PhidgetUSBConnectionCast(void *);

PhidgetReturnCode openAttachedUSBDevice(PhidgetDeviceHandle);
void joinUSBReadThread(PhidgetUSBConnectionHandle);
void stopUSBReadThread(PhidgetUSBConnectionHandle);

void PhidgetUSBError(PhidgetDeviceHandle device);

PhidgetReturnCode PhidgetUSBScanDevices(void);
PhidgetReturnCode PhidgetUSBOpenHandle(PhidgetDeviceHandle device);
PhidgetReturnCode PhidgetUSBCloseHandle(PhidgetUSBConnectionHandle conn);
PhidgetReturnCode PhidgetUSBSetLabel(PhidgetDeviceHandle device, char *buffer);
void PhidgetUSBCleanup(void);
PhidgetReturnCode PhidgetUSBRefreshLabelString(PhidgetDeviceHandle device);
//NOTE: str must have a length of 256 bytes
PhidgetReturnCode PhidgetUSBGetString(PhidgetUSBConnectionHandle conn, int index, char *str);
PhidgetReturnCode PhidgetUSBReadPacket(PhidgetUSBConnectionHandle conn, unsigned char *buffer);
PhidgetReturnCode PhidgetUSBSendPacket(mosiop_t iop, PhidgetUSBConnectionHandle conn, const unsigned char *buffer, size_t bufferLen);

#if defined(_IPHONE) || defined(_MACOSX)
PhidgetReturnCode PhidgetUSBResetDevice(PhidgetDeviceHandle device);
PhidgetReturnCode PhidgetUSBSetupNotifications(CFRunLoopRef runloop);
PhidgetReturnCode PhidgetUSBTeardownNotifications(void);
#endif

#if defined(_LINUX) || defined(_FREEBSD) && !defined(_ANDROID)
void PhidgetUSBUninit(void);
#endif

PhidgetReturnCode encodeLabelString(char *buffer, char *out, size_t *outLen);
PhidgetReturnCode decodeLabelString(char *labelBuf, char *out, int serialNumber);
BOOL labelHasWrapError(int serialNumber, char *labelBuf);

PhidgetReturnCode CCONV UTF16toUTF8(char *in, int inBytes, char *out);

MOS_TASK_RESULT PhidgetUSBReadThreadFunction(void *arg);

#endif
