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

#include "phidgetbase.h"
#include "device/genericdevice.h"

// === Internal Functions === //

//initAfterOpen - sets up the initial state of an object, reading in packets from the device if needed
//				  used during attach initialization - on every attach
static PhidgetReturnCode CCONV
PhidgetGenericDevice_initAfterOpen(PhidgetDeviceHandle device) {
	PhidgetGenericDeviceHandle phid = (PhidgetGenericDeviceHandle)device;
	PhidgetUSBConnectionHandle usbConn;

	assert(phid);

	phid->OUTPacketLength[0] = (uint32_t)getMaxOutPacketSize((PhidgetDeviceHandle)phid);
	if (phid->phid.deviceInfo.UDD->type == PHIDTYPE_SPI)
		phid->INPacketLength[0] = MAX_SPI_PACKET_SIZE;
	else if (phid->phid.deviceInfo.UDD->type == PHIDTYPE_USB) {
		usbConn = PhidgetUSBConnectionCast(device->conn);
		assert(usbConn);
		phid->INPacketLength[0] = usbConn->inputReportByteLength;
	}
	return (EPHIDGET_OK);
}

//dataInput - parses device packets
static PhidgetReturnCode CCONV
PhidgetGenericDevice_dataInput(PhidgetDeviceHandle device, uint8_t *buffer, size_t length) {
	PhidgetGenericDeviceHandle phid = (PhidgetGenericDeviceHandle)device;
	PhidgetChannelHandle channel;

	assert(phid);
	assert(buffer);

	if ((channel = getChannel(phid, 0)) != NULL) {
		bridgeSendToChannel(channel, BP_PACKET, "%*R", (uint32_t)length, buffer);
		PhidgetRelease(&channel);
	}

	return (EPHIDGET_OK);
}

static PhidgetReturnCode CCONV
PhidgetGenericDevice_bridgeInput(PhidgetChannelHandle ch, BridgePacket *bp) {
	PhidgetGenericDeviceHandle phid = (PhidgetGenericDeviceHandle)ch->parent;

	assert(phid->phid.deviceInfo.class == PHIDCLASS_GENERIC);
	assert(ch->class == PHIDCHCLASS_GENERIC);

	switch (bp->vpkt) {
	case BP_SENDPACKET:
		if (getBridgePacketArrayLen(bp, 0) != (int)phid->OUTPacketLength[0])
			return (EPHIDGET_INVALIDARG);
		return (PhidgetDevice_sendpacket(bp->iop, (PhidgetDeviceHandle)phid, getBridgePacketUInt8Array(bp, 0), getBridgePacketArrayLen(bp, 0)));

	case BP_OPENRESET:
	case BP_CLOSERESET:
	case BP_ENABLE:
		return (EPHIDGET_OK);
	default:
		MOS_PANIC("Unexpected packet type");
	}
}

static void CCONV
PhidgetGenericDevice_free(PhidgetDeviceHandle *phid) {

	mos_free(*phid, sizeof(struct _PhidgetGenericDevice));
	*phid = NULL;
}

PhidgetReturnCode
PhidgetGenericDevice_create(PhidgetGenericDeviceHandle *phidp) {
	DEVICECREATE_BODY(GenericDevice, PHIDCLASS_GENERIC);
	return (EPHIDGET_OK);
}
