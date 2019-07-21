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
#include "manager.h"
#include "device/hubdevice.h"
#include "device/vintdevice.h"
#include "vintpackets.h"
#include "network/network.h"

static PhidgetReturnCode
resetVINTPortModeIfNeeded(PhidgetHubDeviceHandle hub, int port, PhidgetHub_PortMode mode) {
	PhidgetChannelHandle channel;
	PhidgetReturnCode res;

	if (mode == PORT_MODE_VINT_PORT)
		return (EPHIDGET_OK);

	/*
	 * Check if we have opened a non-hub-port device and the hub port is in port mode..
	 * in this case switch the port mode. This only applied if we have opened by serial number.
	 */

	res = EPHIDGET_OK;

	PhidgetReadLockChannels();
	FOREACH_CHANNEL(channel) {
		if (ISATTACHED(channel))
			continue;

		if (channel->openInfo->hubPort != port)
			continue;

		if (channel->openInfo->hubPortMode != PORT_MODE_VINT_PORT)
			continue;

		if (!(channel->openInfo->openFlags & (PHIDGETOPEN_SERIAL | PHIDGETOPEN_LABEL)))
			continue;

		if (channel->openInfo->openFlags & PHIDGETOPEN_SERIAL)
			if (channel->openInfo->serialNumber != hub->phid.deviceInfo.serialNumber)
				continue;

		if (channel->openInfo->openFlags & PHIDGETOPEN_LABEL)
			if (strcmp(channel->openInfo->label, hub->phid.deviceInfo.label) != 0)
				continue;

		// Open the hub
		res = openDevice((PhidgetDeviceHandle)hub);
		if (res != EPHIDGET_OK) {
			logwarn("Unable to open Hub: "PRC_FMT, PRC_ARGS(res));
			break;
		}

		// Send out a port mode change message
		res = PhidgetHubDevice_setPortMode(NULL, hub, port, PORT_MODE_VINT_PORT);
		if (res != EPHIDGET_OK) {
			logerr("Setting Hub Port mode failed: "PRC_FMT, PRC_ARGS(res));
			break;
		}

		NotifyCentralThread();
	}

	PhidgetUnlockChannels();

	return (res);
}

/*
 * Called with device list locked.
 */
PhidgetReturnCode
scanVintDevice(PhidgetDeviceHandle device, int childIndex, int id, int version, int port) {
	const PhidgetUniqueDeviceDef *pdd;
	PhidgetDeviceHandle vint;
	PhidgetReturnCode res;

again:
	for (pdd = Phidget_Unique_Device_Def; (int)(pdd->type) != END_OF_LIST; pdd++) {
		if (pdd->type != PHIDTYPE_VINT)
			continue;
		if (pdd->vintID != id)
			continue;
		if (version >= pdd->versionHigh || version < pdd->versionLow)
			continue;

		/*
		 * If the device is already here, just mark as scanned and move on.
		 * If an old device is here, remove it.
		 */
		vint = getChild(device, childIndex);
		if (vint != NULL) {
			if (vint->deviceInfo.UDD == pdd && vint->deviceInfo.version == version) {
				PhidgetSetFlags(vint, PHIDGET_SCANNED_FLAG);
				PhidgetRelease(&vint);
				return (EPHIDGET_OK);
			}
			deviceDetach(vint);
			setChild(device, childIndex, NULL);
			PhidgetRelease(&vint);
		}

		res = createPhidgetVINTDevice(pdd, version, device->deviceInfo.label,
		  device->deviceInfo.serialNumber, &vint);
		if (res != EPHIDGET_OK)
			return (res);

		PhidgetSetFlags(vint, PHIDGET_SCANNED_FLAG);
		vint->deviceInfo.uniqueIndex = childIndex;
		vint->deviceInfo.hubPort = port;
		vint->deviceInfo.isHubPort = id <= HUB_PORT_ID_MAX ? PTRUE : PFALSE;

		setParent(vint, device);
		setChild(device, childIndex, vint);

		deviceAttach(vint, 0);
		PhidgetRelease(&vint);	/* release our reference */
		return (EPHIDGET_OK);
	}

	logwarn("A VINT Phidget (ID: 0x%03x Version: %d Hub Port: %d) was found which is "
	  "not supported by the library. A library upgrade is required to work with this Phidget",
	  id, version, port);

	// Attach device as the special unknown device
	if (id != VINTID_0xff0) {
		id = VINTID_0xff0;
		goto again;
	}

	return (EPHIDGET_NOENT);
}

/*
 * Expects devices lock to be held.
 */
PhidgetReturnCode
scanVintDevices(PhidgetDeviceHandle device) {
	char vintPortString[256];
	PhidgetDeviceHandle child;
	PhidgetUSBConnectionHandle usbConn;
	char *vintPortStringPtr;
	int i, j, childIndex;
	int deviceID;
	int version;
	int len;

	if (isNetworkPhidget(device))
		return (EPHIDGET_OK);

	vintPortStringPtr = vintPortString;

	switch (device->deviceInfo.UDD->id) {
	case PHIDID_HUB0000:
	case PHIDID_HUB5000:
		usbConn = PhidgetUSBConnectionCast(device->conn);
		assert(usbConn);
		memset(vintPortString, 0, VINTHUB_MAXPORTS * 12 + 1);
		if (PhidgetUSBGetString(usbConn, 5, vintPortString) != EPHIDGET_OK) {
			logwarn("Couldn't get VINT string from a Hub - maybe detaching.");
			return (EPHIDGET_OK);
		}
		len = (int)strlen(vintPortString);
		break;
	case PHIDID_HUB0004:
		memset(vintPortString, 0, VINTHUB_MAXPORTS * 12 + 1);
		if (PhidgetSPIGetVINTDevicesString(vintPortString, sizeof(vintPortString)) != EPHIDGET_OK) {
			logerr("Failed to get port string from SPI attached hub");
			return (EPHIDGET_UNEXPECTED);
		}
		len = (int)strlen(vintPortString);
		break;
	case PHIDID_HUB0001:
	case PHIDID_HUB0005:
		vintPortStringPtr = ((PhidgetHubDeviceHandle)device)->portDescString;
		len = (int)strlen(vintPortStringPtr);
		break;
	default:
		MOS_PANIC("Unexpected device.");
	}

	if (len != device->dev_hub.numVintPorts * 6 + device->dev_hub.numVintPortModes * 6) {
		logerr("Wrong string length of VINT string from Hub: %d/%d (%s)", len,
		  device->dev_hub.numVintPorts * 6 + device->dev_hub.numVintPortModes * 6, vintPortStringPtr);
		return (EPHIDGET_OK);
	}

	// Add/Detach Vint devices
	for (childIndex = 0, i = 0; i < device->dev_hub.numVintPorts; i++) {
		deviceID = ((unsigned int)(vintPortStringPtr[0] - '0') << 8)
			+ ((unsigned int)(vintPortStringPtr[1] - '0') << 4)
			+ (vintPortStringPtr[2] - '0');

		version = ((unsigned int)(vintPortStringPtr[3] - '0') * 100)
			+ ((unsigned int)(vintPortStringPtr[4] - '0') << 4)
			+ (vintPortStringPtr[5] - '0');

		// deviceID is going to either be 0x000 for nothing attached, or > HUB_PORT_ID_MAX for a VINT device plugged in
		if (deviceID > HUB_PORT_ID_MAX) {
			scanVintDevice(device, childIndex, deviceID, version, i);
		} else {
			// Nothing plugged in - detach previous VINT device if any
			child = getChild(device, childIndex);
			if (child) {
				chlog("vint replace %"PRIphid"", child);
				deviceDetach(child);
				setChild(device, childIndex, NULL);
				PhidgetRelease(&child);
			}
		}

		childIndex++;
		vintPortStringPtr += 6;

		if (deviceID > PORT_MODE_VINT_PORT && deviceID <= HUB_PORT_ID_MAX)
			resetVINTPortModeIfNeeded((PhidgetHubDeviceHandle)device, i, (PhidgetHub_PortMode)deviceID);
	}

	// Add port devices
	for (j = 0; j < device->dev_hub.numVintPortModes; j++) {
		deviceID = ((unsigned int)(vintPortStringPtr[0] - '0') << 8)
			+ ((unsigned int)(vintPortStringPtr[1] - '0') << 4)
			+ (vintPortStringPtr[2] - '0');

		version = ((unsigned int)(vintPortStringPtr[3] - '0') * 100)
			+ ((unsigned int)(vintPortStringPtr[4] - '0') << 4)
			+ (vintPortStringPtr[5] - '0');

		for (i = 0; i < device->dev_hub.numVintPorts; i++)
			scanVintDevice(device, childIndex++, deviceID, version, i);

		vintPortStringPtr += 6;
	}

	return (EPHIDGET_OK);
}

static PhidgetReturnCode
sendVINTPacketTransaction(mosiop_t iop, PhidgetChannelHandle channel, VINTDeviceCommand command,
  VINTPacketType devicePacketType, const uint8_t *buffer, size_t bufferLen, PhidgetTransactionHandle trans,
  int *sent) {
	unsigned char bufferOut[MAX_OUT_PACKET_SIZE] = { 0 };
	PhidgetVINTDeviceHandle vintDevice;
	PhidgetReturnCode ret;
	size_t bufferOutLen;

	bufferOutLen = sizeof(bufferOut);

	vintDevice = (PhidgetVINTDeviceHandle)channel->parent;
	assert(vintDevice);

	ret = PhidgetVINTDevice_makePacket(vintDevice, channel, command, devicePacketType, buffer,
	  bufferLen, bufferOut, &bufferOutLen);
	if (ret != EPHIDGET_OK)
		return (ret);

	return (PhidgetDevice_sendpacketTransaction(iop, (PhidgetDeviceHandle)vintDevice, bufferOut, bufferOutLen,
		trans, sent));
}

PhidgetReturnCode
sendVINTPacket(mosiop_t iop, PhidgetChannelHandle channel, VINTDeviceCommand command,
  VINTPacketType devicePacketType, const uint8_t *buffer, size_t bufferLen, int *sent) {

	return (sendVINTPacketTransaction(iop, channel, command, devicePacketType, buffer, bufferLen, NULL, sent));
}

PhidgetReturnCode
sendVINTDataPacket(mosiop_t iop, PhidgetChannelHandle channel,
  VINTPacketType devicePacketType, const uint8_t *buffer, size_t bufferLen, int *sent) {

	return (sendVINTPacketTransaction(iop, channel,
		(VINTDeviceCommand)VINT_DATA, devicePacketType, buffer, bufferLen, NULL, sent));
}

PhidgetReturnCode
sendVINTDataPacketTransaction(mosiop_t iop, PhidgetChannelHandle channel,
  VINTPacketType devicePacketType, const uint8_t *buffer, size_t bufferLen, PhidgetTransactionHandle trans, int *sent) {

	return (sendVINTPacketTransaction(iop, channel,
		(VINTDeviceCommand)VINT_DATA, devicePacketType, buffer, bufferLen, trans, sent));
}

PhidgetReturnCode
VINTPacketStatusCode_to_PhidgetReturnCode(VINTPacketStatusCode code) {
	switch (code) {
	case VINTPacketStatusCode_ACK:
		return (EPHIDGET_OK);
	case VINTPacketStatusCode_NOSPACE:
		return (EPHIDGET_NOSPC);
	case VINTPacketStatusCode_NAK:
		return (EPHIDGET_BUSY);
	case VINTPacketStatusCode_TOOBIG:
		return (EPHIDGET_FBIG);
	case VINTPacketStatusCode_NOTATTACHED:
		return (EPHIDGET_NOTATTACHED);
	case VINTPacketStatusCode_INVALIDSEQUENCE:
		return (EPHIDGET_NOTCONFIGURED);
	case VINTPacketStatusCode_INVALIDARG:
		return (EPHIDGET_INVALIDARG);
	case VINTPacketStatusCode_INVALIDCOMMAND:
	case VINTPacketStatusCode_MALFORMED:
		return (EPHIDGET_INVALID);
	case VINTPacketStatusCode_INVALIDPACKETTYPE:
		return (EPHIDGET_INVALIDPACKET);
	case VINTPacketStatusCode_FAILSAFE:
		return (EPHIDGET_FAILSAFE);
	case VINTPacketStatusCode_UNEXPECTED:
	default:
		return (EPHIDGET_UNEXPECTED);
	}
}

const char *
Phidget_strVINTPacketStatusCode(VINTPacketStatusCode code) {

	switch (code) {
	case VINTPacketStatusCode_ACK:
		return ("Success");
	case VINTPacketStatusCode_NAK:
		return ("Not ready");
	case VINTPacketStatusCode_INVALIDARG:
		return ("Invalid Argument");
	case VINTPacketStatusCode_INVALIDPACKETTYPE:
		return ("Invalid Packet Type");
	case VINTPacketStatusCode_INVALIDSEQUENCE:
		return ("Invalid Sequence");
	case VINTPacketStatusCode_INVALIDCOMMAND:
		return ("Invalid Command");
	case VINTPacketStatusCode_MALFORMED:
		return ("Malformed");
	case VINTPacketStatusCode_NOSPACE:
		return ("No Space");
	case VINTPacketStatusCode_UNEXPECTED:
		return ("Unexpected");
	case VINTPacketStatusCode_NOTATTACHED:
		return ("Not Attached");
	case VINTPacketStatusCode_TOOBIG:
		return ("Packet Too Big");
	case VINTPacketStatusCode_FAILSAFE:
		return ("Failsafe Tripped");
	default:
		return ("<Unknown Code>");
	}
}

static void
PhidgetVINTConnectionDelete(PhidgetVINTConnectionHandle *phid) {

	mos_free(*phid, sizeof(PhidgetVINTConnection));
}

PhidgetReturnCode
PhidgetVINTConnectionCreate(PhidgetVINTConnectionHandle *phid) {

	assert(phid);

	*phid = mos_zalloc(sizeof(PhidgetVINTConnection));
	phidget_init((PhidgetHandle)*phid, PHIDGET_VINT_CONNECTION, (PhidgetDelete_t)PhidgetVINTConnectionDelete);

	return (EPHIDGET_OK);
}

/**
 * Exported API Below
 */



API_PRETURN
Phidget_getDeviceVINTID(PhidgetHandle deviceOrChannel, uint32_t *VINTID) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(VINTID);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*VINTID = device->deviceInfo.UDD->vintID;

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getHubPort(PhidgetHandle _phid, int *port) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;
	PhidgetHandle phid;

	phid = PhidgetCast(_phid);
	TESTPTR_PR(phid);

	channel = PhidgetChannelCast(_phid);
	GETDEVICE(device, phid);

	if (ISATTACHEDORDETACHING(phid)) {
		*port = device->deviceInfo.hubPort;
	} else if (channel) {
		*port = channel->openInfo->hubPort;
	} else {
		PhidgetRelease(&device);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setHubPort(PhidgetHandle phid, int newVal) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);
	TESTRANGE_PR(newVal, "%d", PHIDGET_HUBPORT_ANY, VINTHUB_MAXPORTS);

	channel->openInfo->hubPort = newVal;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getHubPortCount(PhidgetHandle _phid, int *portCount) {
	PhidgetDeviceHandle pdevice;
	PhidgetDeviceHandle device;
	PhidgetHandle phid;

	TESTPTR_PR(portCount);

	phid = PhidgetCast(_phid);
	TESTPTR_PR(phid);
	TESTATTACHEDORDETACHING_PR(phid);

	GETDEVICE(device, phid);

	while (device != NULL) {
		if (device->deviceInfo.class == PHIDCLASS_HUB)
			break;

		pdevice = getParent(device);
		PhidgetRelease(&device);
		device = pdevice;
	}

	if (device && device->deviceInfo.class == PHIDCLASS_HUB) {
		*portCount = device->deviceInfo.UDD->channelCnts.hub.numVintPorts;
		PhidgetRelease(&device);
		return (EPHIDGET_OK);
	}

	if (device)
		PhidgetRelease(&device);

	return (PHID_RETURN(EPHIDGET_WRONGDEVICE));
}

/*
 * Caller must release the device.
 */
API_PRETURN
Phidget_getHub(PhidgetHandle _phid, PhidgetHandle *hub) {
	PhidgetDeviceHandle pdevice;
	PhidgetDeviceHandle device;
	PhidgetHandle phid;

	phid = PhidgetCast(_phid);
	TESTPTR_PR(phid);

	device = getParent(phid);
	while (device != NULL) {
		if (device->deviceInfo.class == PHIDCLASS_HUB)
			break;

		pdevice = getParent(device);
		PhidgetRelease(&device);
		device = pdevice;
	}

	if (device && device->deviceInfo.class == PHIDCLASS_HUB) {
		*hub = (PhidgetHandle)device;
		return (EPHIDGET_OK);
	}

	if (device)
		PhidgetRelease(&device);

	return (PHID_RETURN(EPHIDGET_WRONGDEVICE));
}

API_PRETURN
Phidget_getIsHubPortDevice(PhidgetHandle _phid, int *isHubPortDevice) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;
	PhidgetHandle phid;

	TESTPTR_PR(isHubPortDevice);
	phid = PhidgetCast(_phid);
	TESTPTR_PR(phid);

	GETDEVICE(device, phid);
	channel = PhidgetChannelCast(phid);

	if (ISATTACHEDORDETACHING(phid)) {
		*isHubPortDevice = device->deviceInfo.isHubPort;
	} else if (channel && channel->openInfo) {
		*isHubPortDevice = channel->openInfo->isHubPort;
	} else {
		PhidgetRelease(&device);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setIsHubPortDevice(PhidgetHandle phid, int newVal) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);

	if (newVal) {
		switch (channel->class) {
		case PHIDCHCLASS_DIGITALINPUT:
		case PHIDCHCLASS_DIGITALOUTPUT:
		case PHIDCHCLASS_VOLTAGEINPUT:
		case PHIDCHCLASS_VOLTAGERATIOINPUT:
			break;
		default:
			return (PHID_RETURN_ERRSTR(EPHIDGET_INVALIDARG, "IsHubPortDevice cannot be set to true for this channel class."));
		}
	}

	channel->openInfo->isHubPort = newVal;
	return (EPHIDGET_OK);
}