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

#include "phidget.h"
#include "gpp.h"
#include "usb.h"
#include "manager.h"
#include "util/utils.h"
#include "util/phidgetlog.h"

#ifdef _FREEBSD
#include <libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
#include <sys/stat.h>
#include <sys/ioctl.h>

static libusb_context *libusbContext;

#ifdef NDEBUG
#define usblogerr(...) PhidgetLog_loge(NULL, 0, NULL, "phidget22usb", PHIDGET_LOG_ERROR, __VA_ARGS__)
#define usbloginfo(...) PhidgetLog_loge(NULL, 0, NULL, "phidget22usb", PHIDGET_LOG_INFO, __VA_ARGS__)
#define usblogwarn(...) PhidgetLog_loge(NULL, 0, NULL, "phidget22usb", PHIDGET_LOG_WARNING, __VA_ARGS__)
#define usblogdebug(...)
#define usblogverbose(...)
#else
#define usblogerr(...) PhidgetLog_loge(__FILE__, __LINE__, __func__, "phidget22usb", PHIDGET_LOG_ERROR, __VA_ARGS__)
#define usbloginfo(...) PhidgetLog_loge(__FILE__, __LINE__, __func__, "phidget22usb", PHIDGET_LOG_INFO, __VA_ARGS__)
#define usblogwarn(...) PhidgetLog_loge(__FILE__, __LINE__, __func__, "phidget22usb", PHIDGET_LOG_WARNING, __VA_ARGS__)
#define usblogdebug(...) PhidgetLog_loge(__FILE__, __LINE__, __func__, "phidget22usb", PHIDGET_LOG_DEBUG, __VA_ARGS__)
#define usblogverbose(...) PhidgetLog_loge(__FILE__, __LINE__, __func__, "phidget22usb", PHIDGET_LOG_VERBOSE, __VA_ARGS__)
#endif

static void
logBuffer(unsigned char *data, int dataLen, const char *message) {
	Phidget_LogLevel ll;
	char str[2000];
	int i, j;

	PhidgetLog_getSourceLevel("phidget22usb", &ll);
	if (ll != PHIDGET_LOG_VERBOSE)
		return;

	str[0]='\0';
	if (dataLen > 0) {
		for (i = 0, j = 0; i < dataLen; i++, j += 6) {
			if (!(i % 8)) {
				str[j] = '\n';
				str[j + 1] = '\t';
				j += 2;
			}
			mos_snprintf(str + j, sizeof (str) - j, "0x%02x, ", data[i]);
		}
		str[j - 2] = '\0'; //delete last ','
	}

	usblogdebug("%s%s", message, str);
}

PhidgetReturnCode
PhidgetUSBCloseHandle(PhidgetUSBConnectionHandle conn) {
	int ret;

	assert(conn);

	usbloginfo("");

	if (conn->deviceHandle == NULL)
		return (EPHIDGET_NOTATTACHED);

	stopUSBReadThread(conn);

	/* Lock so we do not close the handle while the read thread is using it */
	PhidgetRunLock(conn);
	ret = libusb_release_interface((libusb_device_handle *)conn->deviceHandle, conn->interfaceNum);
	if (ret != 0) {
		switch (ret) {
		case LIBUSB_ERROR_NO_DEVICE:
			//usb_release_interface called after the device was unplugged
			usblogwarn("libusb_release_interface() called on unplugged device.");
			break;
		default:
			usblogerr("libusb_release_interface() failed with error code: %d", ret);
		}
	}

#if 0
	TODO: reimplement this elsewere

	//if we notice that PHIDGET_USB_ERROR_FLAG is set, then reset this device before closing
	//this gives us a better chance of getting it back if something has gone wrong.
	if (CPhidget_statusFlagIsSet(phid->status, PHIDGET_USB_ERROR_FLAG)) {
		usblogwarn("PHIDGET_USB_ERROR_FLAG is set - resetting device.");
		if ((ret = libusb_reset_device((libusb_device_handle *)(*conn)->deviceHandle)) != 0) {
			usblogerr("libusb_reset_device failed with error code: %d", ret);
			result = EPHIDGET_UNEXPECTED;
		}
	}
#endif

	libusb_close((libusb_device_handle *)conn->deviceHandle);
	conn->deviceHandle = NULL;
	PhidgetRunUnlock(conn);

	return (EPHIDGET_OK);
}

PhidgetReturnCode
PhidgetUSBSendPacket(mosiop_t iop, PhidgetUSBConnectionHandle conn, const unsigned char *buffer, size_t bufferLen) {
	uint8_t buf[MAX_OUT_PACKET_SIZE];
	int BytesWritten = 0, ret;

	assert(conn);
	assert(buffer);
	assert(bufferLen <= conn->outputReportByteLength);
	assert(bufferLen < sizeof(buf));

	if (conn->deviceHandle == NULL) {
		usblogwarn("Handle for writing is not valid");
		return (MOS_ERROR(iop, EPHIDGET_UNEXPECTED, "Invalid USB Handle."));
	}

	memcpy(buf, buffer, bufferLen);
	memset(buf + bufferLen, 0, (sizeof(buf)) - bufferLen);

	logBuffer(buf, conn->outputReportByteLength, "Sending USB Packet: ");

	if (conn->interruptOutEndpoint) {
		ret = libusb_interrupt_transfer((libusb_device_handle *)conn->deviceHandle,
		  LIBUSB_ENDPOINT_OUT | (conn->interfaceNum + 1),
		  buf,
		  conn->outputReportByteLength, /* size */
		  &BytesWritten,
		  500); /* FIXME? timeout */
	} else {
		BytesWritten = libusb_control_transfer((libusb_device_handle *)conn->deviceHandle,
		  LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
		  LIBUSB_REQUEST_SET_CONFIGURATION,
		  0x0200, /* value */
		  conn->interfaceNum, /* index*/
		  buf,
		  conn->outputReportByteLength, /* size */
		  500); /* FIXME? timeout */
		ret = BytesWritten;
	}

	if (ret < 0) {
		switch (ret) {
		case LIBUSB_ERROR_TIMEOUT: //important case?
			if (conn->interruptOutEndpoint && BytesWritten != 0)
				goto sentdata;
			return (EPHIDGET_TIMEOUT);
		case LIBUSB_ERROR_NO_DEVICE:
			//device is gone - unplugged.
			usbloginfo("Device was unplugged - detach.");
			return (MOS_ERROR(iop, EPHIDGET_NOTATTACHED, "USB Device is not attached."));
		default:
			if (conn->interruptOutEndpoint)
				usblogerr("libusb_interrupt_transfer() failed: %d", ret);
			else
				usblogerr("libusb_control_msg() failed:: %d", ret);
			return (MOS_ERROR(iop, EPHIDGET_UNEXPECTED, "USB Send failed with error: %d", ret));
		}
	}

sentdata:
	if (BytesWritten != conn->outputReportByteLength) {
		usblogwarn("Report Length: %d, bytes written: %d",
			(int)conn->outputReportByteLength, (int)BytesWritten);
		return (MOS_ERROR(iop, EPHIDGET_UNEXPECTED, "USB Send wrote wrong number of bytes."));
	}

	return (EPHIDGET_OK);
}

PhidgetReturnCode
PhidgetUSBSetLabel(PhidgetDeviceHandle device, char *buffer) {
	PhidgetUSBConnectionHandle conn;
	int BytesWritten;
	int size;

	assert(device != NULL);

	if (deviceSupportsGeneralPacketProtocol(device))
		return (GPP_setLabel(NULL, device, buffer));

	conn = PhidgetUSBConnectionCast(device->conn);
	assert(conn);

	size = buffer[0];
	if (size > 22)
		return (EPHIDGET_INVALID);

	if (conn->deviceHandle == NULL) {
		usblogwarn("USB Connection device handle is NULL");
		return (EPHIDGET_UNEXPECTED);
	}

	BytesWritten = libusb_control_transfer((libusb_device_handle *)conn->deviceHandle,
	  LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
	  LIBUSB_REQUEST_SET_DESCRIPTOR,
	  0x0304, /* value */
	  0x0409, /* index*/
	  (unsigned char *)buffer,
	  size, /* size */
	  500); /* timeout */

	if (BytesWritten < 0) {
		switch (BytesWritten) {
		case LIBUSB_ERROR_TIMEOUT: //important case?
			usbloginfo("libusb_control_transfer() timeout (500ms)");
			return (EPHIDGET_UNSUPPORTED);
		default:
			usbloginfo("libusb_control_transfer() failed with error code: %d", BytesWritten);
			return (EPHIDGET_UNSUPPORTED);
		}
	}

	if (BytesWritten != size) {
		usblogwarn("Report Length: %d, bytes written: %d", size, (int)BytesWritten);
		return (EPHIDGET_UNEXPECTED);
	}

	return (EPHIDGET_OK);
}

PhidgetReturnCode
PhidgetUSBGetString(PhidgetUSBConnectionHandle conn, int index, char *str) {
	libusb_device_handle *handle;
	int ret;

	handle = conn->deviceHandle;
	if (handle == NULL) {
		ret = libusb_open(conn->dev, &handle);
		if (ret != 0) {
			usblogerr("libusb_open() failed with error code: %d", ret);
			return (EPHIDGET_UNEXPECTED);
		}
	}

	ret = libusb_get_string_descriptor_ascii(handle, index, (uint8_t *)str, 256);
	if (handle != conn->deviceHandle)
		libusb_close(handle);

	if (ret < 0) {
		usblogerr("libusb_get_string_descriptor_ascii() failed with error code: %d", ret);
		return (EPHIDGET_UNEXPECTED);
	}
	return EPHIDGET_OK;
}

/* Buffer should be at least 8 bytes long */
PhidgetReturnCode
PhidgetUSBReadPacket(PhidgetUSBConnectionHandle conn, unsigned char *buffer) {
	int BytesRead = 0, ret;

	assert(conn);

	PhidgetRunLock(conn);

	if (conn->deviceHandle == NULL) {
		PhidgetRunUnlock(conn);
		usblogerr("device handle is NULL");
		return (EPHIDGET_UNEXPECTED);
	}

	ret = libusb_interrupt_transfer((libusb_device_handle *)conn->deviceHandle,
	  LIBUSB_ENDPOINT_IN | (conn->interfaceNum + 1),
	  buffer,
	  conn->inputReportByteLength,
	  &BytesRead,
	  500);

	PhidgetRunUnlock(conn);

	if (ret != 0) {
		switch (ret) {
			// A timeout occured, but we'll just try again
		case LIBUSB_ERROR_TIMEOUT:
			if (BytesRead != 0)
				goto gotdata;
			return (EPHIDGET_TIMEOUT);
		case LIBUSB_ERROR_BUSY:
			/*
			 * This happens when someone else calls claim_interface on this interface
			 * (a manager for ex.) - basically just wait until they release it.
			 *
			 * This will happen if an open occurs in another app which (for some reason)
			 * can steal the interface from this one.
			 */
			usbloginfo("Device is busy on Read - try again.");
			return (EPHIDGET_AGAIN);
		case LIBUSB_ERROR_NO_DEVICE:
			//device is gone - unplugged.
			usbloginfo("Device was unplugged - detach.");
			return (EPHIDGET_NOTATTACHED);
		case LIBUSB_ERROR_PIPE:
		case LIBUSB_ERROR_OVERFLOW:
		case LIBUSB_ERROR_IO:
		default:
			usblogerr("libusb_interrupt_transfer() returned: %d", ret);
			goto tryagain;
		}
	}

gotdata:
	if (BytesRead != conn->inputReportByteLength) {
		/*
		 * Generally means the device was unplugged, but can mean that there is not enough
		 * Interrupt bandwidth. We keep trying and we'll get data, just not all data.
		 */
		usblogwarn("Report Length: %d, bytes read: %d. "
		  "Probably trying to use too many Phidgets at once, and some data is being lost.",
		  (int)conn->inputReportByteLength, (int)BytesRead);
		goto tryagain;
	}

	logBuffer(buffer, conn->inputReportByteLength, "Received USB Packet: ");

	conn->tryAgainCounter = 0;
	return (EPHIDGET_OK);

	/*
	 * If we see too many tryagains in a row, then we assume something has actually gone wrong
	 * and reset the device
	 */
tryagain:

	conn->tryAgainCounter++;
	if (conn->tryAgainCounter > 30) { //this will be hit in < 1 second for all devices
		usblogerr("EPHIDGET_AGAIN returned too many times in a row - reset device.");
		conn->tryAgainCounter = 0;
		return (EPHIDGET_UNEXPECTED);
	}
	return (EPHIDGET_AGAIN);
}

static int
getLabel(HANDLE handle, char *label, int serialNumber) {
	struct libusb_device_descriptor	desc;
	libusb_device *device;
	uint8_t labelBuf[22];
	int len;
	int ret;

	memset(labelBuf, 0, sizeof(labelBuf));
	device = libusb_get_device(handle);

	if ((ret = libusb_get_device_descriptor(device, &desc)) != 0) {
		usblogerr("libusb_get_device_descriptor() failed: %d", ret);
		return (EPHIDGET_UNEXPECTED);
	}

	if (desc.iSerialNumber == 3) {
		//Note that this returns the whole descriptor, including the length and type bytes
		len = libusb_get_string_descriptor(handle, 4, 0, labelBuf, sizeof(labelBuf));
		if (len < 0) {
			switch (len) {
			case LIBUSB_ERROR_TIMEOUT: //important case?
			default:
				usbloginfo("libusb_get_string_descriptor() failed reading label: %d", len);
				usbloginfo("device may not support labels");
			}
		} else {
			return (decodeLabelString((char *)labelBuf, label, serialNumber));
		}
	}

	memset(label, 0, MAX_LABEL_STORAGE);
	return (EPHIDGET_OK);
}

PhidgetReturnCode
PhidgetUSBRefreshLabelString(PhidgetDeviceHandle device) {
	PhidgetUSBConnectionHandle conn;
	assert(device);
	conn = PhidgetUSBConnectionCast(device->conn);
	assert(conn);
	return getLabel(conn->deviceHandle, device->deviceInfo.label, device->deviceInfo.serialNumber);
}

void
PhidgetUSBUninit() {

	if (libusbContext) {
		usbloginfo("Deinitializing libusb");
		libusb_exit(libusbContext);
		libusbContext = NULL;
	}
}

PhidgetReturnCode
PhidgetUSBScanDevices(void) {
	struct libusb_device_descriptor	desc;
	const PhidgetUniqueDeviceDef *pdd;
	libusb_device_handle *handle;
	PhidgetUSBConnectionHandle conn;
	PhidgetDeviceHandle phid;
	libusb_device *device;
	libusb_device **list;
	char unique_name[20];
	int serialNumber;
	ssize_t cnt;
	int version;
	int found;
	int ret;
	int j;

	char label[MAX_LABEL_STORAGE];
	uint8_t productString[64];
	uint8_t string[256];

	static int initFailures;

	if (initFailures > 10)
		return (EPHIDGET_UNEXPECTED);

	list = NULL;

	if (!libusbContext) {
		usbloginfo("Initializing libusb");
		if ((ret = libusb_init(&libusbContext)) != 0) {
			usblogerr("libusb_init failed with error code: %d", ret);
			libusbContext = NULL;
			initFailures++;
			return (EPHIDGET_UNEXPECTED);
		}
		initFailures = 0;
	}


	cnt = libusb_get_device_list(libusbContext, &list);
	if (cnt < 0) {
		usblogerr("libusb_get_device_list failed with error code: %zd", cnt);
		goto done;
	}

	//search through all USB devices
	for (j = 0; j < cnt; j++) {
		device = list[j];
		found = 0;

		snprintf(unique_name, 20, "%d/%d", libusb_get_bus_number(device), libusb_get_device_address(device));

		/*
		 * If the device is already in the attached list, flag it and move onto the next.
		 */
		PhidgetReadLockDevices();
		FOREACH_DEVICE(phid) {
			if (phid->connType != PHIDCONN_USB)
				continue;

			conn = PhidgetUSBConnectionCast(phid->conn);
			assert(conn);

			if (!strcmp(conn->uniqueName, unique_name)) {
				PhidgetSetFlags(phid, PHIDGET_SCANNED_FLAG);
				found = 1;
			}
		}
		PhidgetUnlockDevices();

		if (found)
			continue;

		if ((ret = libusb_get_device_descriptor(device, &desc)) != 0) {
			usblogerr("libusb_get_device_descriptor() failed with error code: %d", ret);
			continue;
		}

		//logdebug("Device %d: %04x %04x", j, desc.idVendor, desc.idProduct);

		if (desc.bcdDevice < 0x100)
			version = desc.bcdDevice * 100;
		else
			version = ((desc.bcdDevice >> 8) * 100) + ((desc.bcdDevice & 0xff));

		/*
		 * NOTE: we don't stop when we find a device, because there can be multiple matches
		 * for a device with multiple interfaces.
		 */
		for (pdd = Phidget_Unique_Device_Def; ((int)pdd->type) != END_OF_LIST; pdd++) {
			if (!(pdd->type == PHIDTYPE_USB
				&& desc.idVendor == pdd->vendorID && desc.idProduct == pdd->productID
				&& version >= pdd->versionLow && version < pdd->versionHigh))
				continue;

			usbloginfo("New Phidget found in PhidgetUSBBuildList: %s", unique_name);

			found = 1;

			ret = libusb_open(device, &handle);
			if (ret != 0) {
				usblogwarn("libusb_open() failed with error code: %d", ret);
				usbloginfo("This usually means you need to run as root, or install the udev rules.");
				continue;
			}

			ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, string,
			  sizeof(string));
			if (ret < 0) {
				usblogerr("libusb_get_string_descriptor_ascii() failed with error code: %d", ret);
				libusb_close(handle);
				continue;
			} else {
				serialNumber = atol((const char *)string);
			}

			ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, productString,
			  sizeof(productString));
			if (ret < 0) {
				usblogerr("libusb_get_string_descriptor_ascii() failed with error code: %d", ret);
				libusb_close(handle);
				continue;
			}

			getLabel(handle, label, serialNumber);
			libusb_close(handle);

			ret = createPhidgetUSBDevice(pdd, version, label, serialNumber, unique_name,
			  (char *)productString, &phid);
			if (ret != EPHIDGET_OK) {
				usblogerr("failed to create phidget device handle:0x%x", ret);
				continue;
			}

			PhidgetSetFlags(phid, PHIDGET_SCANNED_FLAG);
			ret = deviceAttach(phid, 1);

			libusb_ref_device(device); // we increase the reference count so the device isn't freed

			conn = PhidgetUSBConnectionCast(phid->conn);
			conn->dev = device;

			PhidgetRelease((void **)&phid); /* release our reference */
		}

		if (!found) {
			/*
			 * Might be a Phidget, but not one known by this version of the library: log if that is the case.
			 */
			if (desc.idVendor == USBVID_PHIDGETS && desc.idProduct >= USBPID_PHIDGETS_MIN &&
			  desc.idProduct <= USBPID_PHIDGETS_MAX) {
				usblogwarn("A USB Phidget (PID: 0x%04x Version: %d) was found that is not supported by "
				  "the library. A library upgrade is probably required to work with this Phidget",
				  desc.idProduct, version);
			}
		}
	} /* iterate over USB devices */

done:
	if (list)
		libusb_free_device_list(list, 1);
	return (ret);
}

/*
 * Got this from libusb-0.1 because 1.0 doesn't expose driver name!
 */
#define USB_MAXDRIVERNAME 255

struct usb_getdriver {
	unsigned int interface;
	char driver[USB_MAXDRIVERNAME + 1];
};

struct linux_device_handle_priv {
	int fd;
};

struct list_head {
	struct list_head *prev, *next;
};

struct libusb_device_handle_internal {
	pthread_mutex_t lock;
	unsigned long claimed_interfaces;
	struct list_head list;
	void *dev;
	unsigned char os_priv[];
};

static PhidgetReturnCode
getReportLengths(PhidgetUSBConnectionHandle phid) {
	const struct libusb_interface_descriptor *interfaceDesc;
	struct libusb_config_descriptor *configDesc;
	unsigned char buf[255];
	int i, j;
	int len;
	int res;

	memset(buf, 0, sizeof(buf));

	res = libusb_get_active_config_descriptor(libusb_get_device(phid->deviceHandle), &configDesc);
	if (res != 0) {
		usblogerr("libusb_get_active_config_descriptor() failed: %d", res);
		return (EPHIDGET_UNEXPECTED);
	}

	interfaceDesc = NULL;
	for (i = 0; i < configDesc->bNumInterfaces; i++) {
		for (j = 0; j < configDesc->interface[i].num_altsetting; j++) {
			if (configDesc->interface[i].altsetting[j].bInterfaceNumber == phid->interfaceNum) {
				interfaceDesc = &configDesc->interface[i].altsetting[j];
				break;
			}
		}
	}

	if (interfaceDesc == NULL) {
		usblogerr("Couldn't find interface descriptor!");
		return (EPHIDGET_UNEXPECTED);
	}

	if (interfaceDesc->bNumEndpoints == 2) {
		usbloginfo("Using Interrupt OUT Endpoint for Host->Device communication.");
		phid->interruptOutEndpoint = PTRUE;
	} else {
		usbloginfo("Using Control Endpoint for Host->Device communication.");
		phid->interruptOutEndpoint = PFALSE;
	}

	libusb_free_config_descriptor(configDesc);

	len = libusb_control_transfer(phid->deviceHandle, LIBUSB_ENDPOINT_IN + 1,
	  LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_REPORT << 8) + 0, phid->interfaceNum, buf,
	  sizeof(buf), 500 /* ms timeout */);

	if (len < 0) {
		usblogerr("libusb_control_transfer() failed: %d", len);
		return (EPHIDGET_UNEXPECTED);
	}

	if (len < 10) {
		usblogerr("failed to get report lengths");
		return (EPHIDGET_UNEXPECTED);
	}

	for (i = 10; i < len; i++) {
		if (buf[i] == 0x81 && buf[i - 2] == 0x95)
			phid->inputReportByteLength = buf[i - 1];
		else if (buf[i] == 0x81 && buf[i - 4] == 0x95)
			phid->inputReportByteLength=buf[i - 3];

		if (buf[i] == 0x91 && buf[i - 2] == 0x95)
			phid->outputReportByteLength = buf[i - 1];
		else if (buf[i] == 0x91 && buf[i - 4] == 0x95)
			phid->outputReportByteLength = buf[i - 3];
	}

	return (EPHIDGET_OK);
}

static int
detachDriver(PhidgetDeviceHandle device, struct libusb_device_handle *handle) {
	int res;

	res = libusb_detach_kernel_driver(handle, device->deviceInfo.UDD->interfaceNum);
	if (res != 0) {
		usblogwarn("libusb_detach_kernel_driver() failed: %d", res);
		return (EPHIDGET_UNEXPECTED);
	}

	return (EPHIDGET_OK);
}

PhidgetReturnCode
PhidgetUSBOpenHandle(PhidgetDeviceHandle device) {
	struct libusb_device_handle *handle;
	PhidgetUSBConnectionHandle conn;
	int res;

	assert(device);
	conn = PhidgetUSBConnectionCast(device->conn);
	assert(conn);

	if (conn->deviceHandle != NULL) {
		usblogerr("device handle is already open");
		return (EPHIDGET_BUSY);
	}

	res = libusb_open(conn->dev, &handle);
	if (res != 0) {
		usblogwarn("libusb_open() failed: %d", res);
		if (res == LIBUSB_ERROR_ACCESS) {
			usbloginfo("You may need to run as root, or install the udev rules.");
			return (EPHIDGET_ACCESS);
		}
		return (EPHIDGET_UNEXPECTED);
	}

	res = libusb_kernel_driver_active(handle, device->deviceInfo.UDD->interfaceNum);
	if (res < 0)
		usblogwarn("libusb_kernel_driver_active() failed: %d", res);
	else if (res == 1)
		detachDriver(device, handle);

	res = libusb_claim_interface(handle, device->deviceInfo.UDD->interfaceNum);
	if (res != 0) {
		libusb_close(handle);
		if (res == LIBUSB_ERROR_BUSY) {
			usblogwarn("libusb_claim_interface() failed with BUSY - the device may already be open");
			return (EPHIDGET_BUSY);
		} else {
			usblogwarn("libusb_claim_interface() failed: %d", res);
			return (EPHIDGET_UNEXPECTED);
		}
	}

	conn->deviceHandle = (HANDLE)handle;
	conn->interfaceNum = device->deviceInfo.UDD->interfaceNum;

	res = getReportLengths(conn);
	if (res != 0) {
		libusb_release_interface(handle, device->deviceInfo.UDD->interfaceNum);
		return (EPHIDGET_UNEXPECTED);
	}

	return (EPHIDGET_OK);
}
