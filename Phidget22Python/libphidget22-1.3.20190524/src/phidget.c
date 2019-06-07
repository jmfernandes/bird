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
#include "mos/mos_os.h"
#include "mos/mos_time.h"
#include "mos/mos_assert.h"
#include "mos/mos_atomic.h"
#include "mos/mos_byteorder.h"

#include "gpp.h"
#include "manager.h"
#include "device/hubdevice.h"
#include "device/meshdongledevice.h"

#ifdef DEBUG
#define matchlogerr(...) PhidgetLog_loge(NULL, 0, NULL, "_phidget22match", PHIDGET_LOG_ERROR, __VA_ARGS__)
#define matchloginfo(...) PhidgetLog_loge(NULL, 0, NULL, "_phidget22match", PHIDGET_LOG_INFO, __VA_ARGS__)
#define matchlogdebug(...) PhidgetLog_loge(NULL, 0, NULL, "_phidget22match", PHIDGET_LOG_DEBUG, __VA_ARGS__)
#else
#define matchlogerr(...)
#define matchloginfo(...)
#define matchlogdebug(...)
#endif

uint32_t matchCount = 0;

static const char *getServerName(PhidgetDeviceHandle);
/*
 * Lock order is device before channel.
 */
phidgetdevices_t phidgetDevices;
mos_tlock_t *devicesLock;
uint32_t phidgetDevicesCount;

phidgetchannels_t phidgetChannels;
mos_rwrlock_t channelsLock;
uint32_t phidgetChannelsCount;

netattachdetachentries_t netAttachDetachQueue;
mos_tlock_t *netAttachDetachQueueLock;

/*
 * Needs to be called during startup.
 *
 * Currently on Windows, in DllMain()
 */
void
PhidgetInit() {

	MTAILQ_INIT(&phidgetChannels);
	phidgetChannelsCount = 0;

	MTAILQ_INIT(&phidgetDevices);
	phidgetDevicesCount = 0;

	MTAILQ_INIT(&netAttachDetachQueue);

	mos_tlock_init(devicesLock, P22LOCK_DEVICELISTLOCK, P22LOCK_FLAGS | MOSLOCK_RWLOCK);
	mos_rwrlock_init(&channelsLock);
	mos_tlock_init(netAttachDetachQueueLock, P22LOCK_NETQUEUELISTLOCK, P22LOCK_FLAGS);
}

void
PhidgetFini() {

	mos_tlock_destroy(&devicesLock);
	mos_rwrlock_destroy(&channelsLock);
	mos_tlock_destroy(&netAttachDetachQueueLock);
}

void
PhidgetLockNetAttachDetachQueue() {

	mos_tlock_lock(netAttachDetachQueueLock);
}

void
PhidgetUnlockNetAttachDetachQueue() {

	mos_tlock_unlock(netAttachDetachQueueLock);
}

void
PhidgetWriteLockChannels() {

	mos_rwrlock_wrlock(&channelsLock);
}

void
PhidgetReadLockChannels() {

	mos_rwrlock_rdlock(&channelsLock);
}

void
PhidgetUnlockChannels() {

	mos_rwrlock_unlock(&channelsLock);
}

void
PhidgetWriteLockDevices() {

	mos_tlock_wlock(devicesLock);
}

void
PhidgetReadLockDevices() {

	mos_tlock_rlock(devicesLock);
}

void
PhidgetUnlockDevices() {

	mos_tlock_rwunlock(devicesLock);
}

static void
incReadCount(PhidgetDeviceHandle phid) {

	PLOCK(phid);
	phid->readCount++;
	mos_cond_broadcast(&phid->cond);
	PUNLOCK(phid);
}

PhidgetReturnCode
waitForReads(PhidgetDeviceHandle device, uint32_t numReads, uint32_t timeout /* ms */) {
	uint32_t readCount;
	mostime_t usec;
	mostime_t tm;

	tm = 0; // make compiler happy

	if (timeout)
		tm = mos_gettime_usec() + (timeout * 1000);

	PLOCK(device);
	readCount = device->readCount;
	/* This handles device->readCount overflowing */
	while ((uint32_t)(device->readCount - readCount) < numReads) {
		if (timeout == 0) {
			mos_fasttlock_wait(&device->cond, &device->__lock);
		} else {
			usec = tm - mos_gettime_usec();
			if (usec > 0) {
				mos_fasttlock_timedwait(&device->cond, &device->__lock, ((uint64_t)(usec * 1000)));
			} else {
				PUNLOCK(device);
				logwarn("Timed out waiting for %d reads in %d ms", numReads, timeout);
				return (EPHIDGET_TIMEOUT);
			}
		}
	}
	PUNLOCK(device);
	return (EPHIDGET_OK);
}

PhidgetDeviceHandle
getParent(void *_phid) {
	PhidgetDeviceHandle ret;
	PhidgetHandle phid;

	phid = PhidgetCast(_phid);
	if (phid == NULL)
		return (NULL);

	PhidgetRunLock(phid);
	if (phid->parent)
		PhidgetRetain(phid->parent);
	ret = phid->parent;
	PhidgetRunUnlock(phid);

	return (ret);
}

void
setParent(void *_phid, void *_device) {
	PhidgetDeviceHandle device;
	PhidgetHandle phid;

	phid = PhidgetCast(_phid);
	MOS_ASSERT(phid != NULL);

	if (_device) {
		device = PhidgetDeviceCast(_device);
		MOS_ASSERT(device != NULL);
	} else {
		device = NULL;
	}

	PhidgetRunLock(phid);
	if (phid->parent)
		PhidgetRelease(&phid->parent);
	phid->parent = device;
	if (device)
		PhidgetRetain(device);
	PhidgetRunUnlock(phid);
}

PhidgetDeviceHandle
getChild(PhidgetDeviceHandle device, int index) {
	PhidgetDeviceHandle ret;

	MOS_ASSERT(device != NULL);
	MOS_ASSERT(index >= 0 && index < PHIDGET_MAXCHILDREN);

	PhidgetDeviceMemberLock(device);
	if (device->child[index])
		PhidgetRetain(device->child[index]);
	ret = device->child[index];
	PhidgetDeviceMemberUnlock(device);

	return (ret);
}

void
setChild(PhidgetDeviceHandle device, int index, void *_child) {
	PhidgetDeviceHandle child, oldchild;

	MOS_ASSERT(device != NULL);
	MOS_ASSERT(index >= 0 && index < PHIDGET_MAXCHILDREN);

	if (_child) {
		child = PhidgetDeviceCast(_child);
		MOS_ASSERT(child != NULL);
	} else {
		child = NULL;
	}

	oldchild = NULL;

	PhidgetDeviceMemberLock(device);

	/*
	 * There is a Lock order reversal between member lock and dispatch lock
	 * so save the pointer, and release it after we unlock;
	 */
	oldchild = device->child[index];
	device->child[index] = child;
	if (child)
		PhidgetRetain(child);
	PhidgetDeviceMemberUnlock(device);

	if (oldchild)
		PhidgetRelease(&oldchild);
}

PhidgetChannelHandle
getAttachedChannel(void *device, int index) {
	PhidgetChannelHandle channel;

	channel = getChannel(device, index);
	if (channel == NULL)
		return (NULL);

	if (ISATTACHED(channel))
		return (channel);

	PhidgetRelease(&channel);
	return (NULL);
}

PhidgetChannelHandle
getChannel(void *_device, int index) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;

	device = PhidgetDeviceCast(_device);
	MOS_ASSERT(device != NULL);
	MOS_ASSERT(index >= 0 && index < PHIDGET_MAXCHANNELS);

	PhidgetDeviceMemberLock(device);
	channel = device->channel[index];
	if (channel == NULL) {
		PhidgetDeviceMemberUnlock(device);
		return (NULL);
	}

	PhidgetRetain(channel);
	PhidgetDeviceMemberUnlock(device);

	return (channel);
}

PhidgetReturnCode
setChannel(PhidgetDeviceHandle device, int index, void *_channel) {
	PhidgetChannelHandle channel;

	MOS_ASSERT(device != NULL);
	MOS_ASSERT(index >= 0 && index < PHIDGET_MAXCHANNELS);

	if (_channel) {
		channel = PhidgetChannelCast(_channel);
		MOS_ASSERT(channel);
	} else {
		channel = NULL;
	}

	PhidgetDeviceMemberLock(device);
	if (channel == NULL) {
		if (device->channel[index])
			PhidgetRelease(&device->channel[index]);
		PhidgetDeviceMemberUnlock(device);
		return (EPHIDGET_OK);
	}

	if (device->channel[index]) {
		PhidgetDeviceMemberUnlock(device);
		return (EPHIDGET_DUPLICATE);
	}
	device->channel[index] = channel;
	PhidgetRetain(channel);
	PhidgetDeviceMemberUnlock(device);
	return (EPHIDGET_OK);
}

PhidgetOpenInfoHandle
mallocPhidgetOpenInfo() {
	PhidgetOpenInfoHandle item;

	item = mos_zalloc(sizeof(PhidgetOpenInfo));

	item->channel = 0;

	item->serialNumber = PHIDGET_SERIALNUMBER_ANY;
	item->label = PHIDGET_LABEL_ANY;

	item->hubPort = PHIDGET_HUBPORT_ANY;
	item->isHubPort = PFALSE;

	/*
	 * If async is not set, we will decide at open time whether to block or
	 * not based on an attach event being registered.
	 */
	item->async = PUNK_BOOL;
	//Default timeout is guaranteed to be sufficient if the device is attached before the open call.
	item->timeout = PHIDGET_TIMEOUT_DEFAULT;

	return (item);
}

void
freePhidgetOpenInfo(PhidgetOpenInfoHandle item) {

	if (item->label)
		mos_free(item->label, mos_strlen(item->label) + 1);
	if (item->serverName)
		mos_free(item->serverName, mos_strlen(item->serverName) + 1);
	mos_free(item, sizeof(PhidgetOpenInfo));
}

PhidgetReturnCode
PhidgetDevice_read(PhidgetDeviceHandle device) {
	PhidgetReturnCode res;
	PhidgetUSBConnectionHandle usbConn;
	PhidgetSPIConnectionHandle spiConn;
	uint8_t buffer[MAX_SPI_PACKET_SIZE];
	size_t length;

	assert(device);
	TESTATTACHED(device);

	switch (device->connType) {
	case PHIDCONN_USB:
		usbConn = PhidgetUSBConnectionCast(device->conn);
		assert(usbConn);
		res = PhidgetUSBReadPacket(usbConn, usbConn->lastReadPacket);
		if (res != EPHIDGET_OK)
			return (res);

		if (usbConn->lastReadPacket[0] & PHID_GENERAL_PACKET_FLAG &&
		  deviceSupportsGeneralPacketProtocol(device)) {
			res = PhidgetGPP_dataInput(device, usbConn->lastReadPacket,
			  usbConn->inputReportByteLength);
		} else {
			res = device->dataInput(device, usbConn->lastReadPacket,
			  usbConn->inputReportByteLength);
			incReadCount(device);
		}
		return (res);

	case PHIDCONN_SPI:
		spiConn = PhidgetSPIConnectionCast(device->conn);
		assert(spiConn);
		length = MAX_SPI_PACKET_SIZE;
		res = PhidgetSPIReadPacket(spiConn, buffer, &length);
		if (res != EPHIDGET_OK)
			return (res);

		if (buffer[0] & PHID_GENERAL_PACKET_FLAG && deviceSupportsGeneralPacketProtocol(device)) {
			res = PhidgetGPP_dataInput(device, buffer, length);
		} else {
			res = device->dataInput(device, buffer, length);
			incReadCount(device);
		}
		return (res);

	default:
		return (EPHIDGET_OK);
	}
}

static PhidgetReturnCode
PhidgetSetLabel(PhidgetDeviceHandle device, char *buffer) {

	assert(device);

	switch (device->connType) {
	case PHIDCONN_USB:
		return (PhidgetUSBSetLabel(device, buffer));
	case PHIDCONN_SPI:
		return (PhidgetSPISetLabel(device, buffer));
	default:
		return (EPHIDGET_UNSUPPORTED);
	}
}

static PhidgetReturnCode
PhidgetRefreshLabelString(PhidgetDeviceHandle device) {

	assert(device);

	switch (device->connType) {
	case PHIDCONN_USB:
		return (PhidgetUSBRefreshLabelString(device));
	case PHIDCONN_SPI:
		return (PhidgetSPIRefreshLabelString(device));
	default:
		return (EPHIDGET_UNSUPPORTED);
	}
}

static PhidgetReturnCode
waitForInitialState(mosiop_t iop, PhidgetChannelHandle channel, uint32_t milliseconds) {
	mostime_t duration;
	mostime_t start;
	int matchGen;

	start = 0; // make compiler happy
	matchGen = matchCount;

	if (milliseconds)
		start = mos_gettime_usec();

	PhidgetLock(channel);
	for (;;) {
		if (_HASINITIALSTATE(channel)) {
			PhidgetUnlock(channel);
			return (EPHIDGET_OK);
		}

		if (!_ISOPEN(channel)) {
			PhidgetUnlock(channel);
			return (MOS_ERROR(iop, EPHIDGET_CLOSED, "Channel was closed while waiting for attach."));
		}

		if (milliseconds) {
			duration = (mos_gettime_usec() - start) / 1000;
			if (duration >= milliseconds) {

				PhidgetUnlock(channel);
				return (EPHIDGET_TIMEOUT);
			}
			PhidgetTimedWait(channel, milliseconds - (uint32_t)duration);
		} else {
			PhidgetWait(channel);
		}
	}
}

static PhidgetReturnCode
waitForAttach(mosiop_t iop, PhidgetChannelHandle channel, uint32_t milliseconds) {
	PhidgetReturnCode res;
	mostime_t remaining;
	mostime_t duration;
	mostime_t start;
	int matchGen;

	start = 0; // make compiler happy
	matchGen = matchCount;

	if (milliseconds)
		start = mos_gettime_usec();

	PhidgetLock(channel);
	for (;;) {
		if (_ISATTACHEDDONE(channel)) {
			PhidgetUnlock(channel);

			// We enforce the wait time range for initial state, so we're not waiting forever for something like a detached thermocouple,
			//  but also always get the initial state when we should
			remaining = 500;
			if (milliseconds)
				remaining = milliseconds - (mos_gettime_usec() - start) / 1000;
			if (remaining > 2500)
				remaining = 2500;
			if (remaining < 500)
				remaining = 500;

			res = waitForInitialState(iop, channel, (uint32_t)remaining);
			if (ISATTACHEDDONE(channel) && (res == EPHIDGET_OK || res == EPHIDGET_TIMEOUT))
				return (EPHIDGET_OK);

			return (res);
		}

		if (!_ISOPEN(channel)) {
			PhidgetUnlock(channel);
			return (MOS_ERROR(iop, EPHIDGET_CLOSED, "Channel was closed while waiting for attach."));
		}

		if (milliseconds) {
			duration = (mos_gettime_usec() - start) / 1000;
			if (duration >= milliseconds) {

				// make sure the match visitor has run a couple times
				if ((uint32_t)(matchGen - matchCount) < 2)
					MOS_ERROR(iop, EPHIDGET_TIMEOUT, "Device matching may not have completed. Try using a longer timeout.");

				// Detect whether an open was tried and failed, or we never even tried
				if (channel->openInfo->openAttempts == 0) {

					if (milliseconds > 0 && milliseconds < PHIDGET_TIMEOUT_DEFAULT)
						MOS_ERROR(iop, EPHIDGET_TIMEOUT, "Try using a longer timeout, at least %d milliseconds, to ensure reliable matching.", PHIDGET_TIMEOUT_DEFAULT);

					if (channel->openInfo->openFlags & PHIDGETOPEN_NETWORK)
						MOS_ERROR(iop, EPHIDGET_TIMEOUT, "Make sure to enable server discovery, or add the server, and to set the server password if autentication is enabled.");

					if (DEVICES_EMPTY)
						MOS_ERROR(iop, EPHIDGET_TIMEOUT, "No Phidgets were detected at all. Make sure your device is attached.");
					else
						MOS_ERROR(iop, EPHIDGET_TIMEOUT, "No matching devices were found to open. Make sure your device is attached, and that your addressing parameters are specified correctly. If your Phidget has a plug or terminal block for external power, ensure it is plugged in and powered.");
				}

				PhidgetUnlock(channel);
				return (EPHIDGET_TIMEOUT);
			}
			PhidgetTimedWait(channel, milliseconds - (uint32_t)duration);
		} else {
			PhidgetWait(channel);
		}
	}
}

PhidgetReturnCode
openDevice(PhidgetDeviceHandle device) {
	PhidgetReturnCode res;

	switch (device->connType) {
	case PHIDCONN_MESH:
	case PHIDCONN_VINT:
		res = openDevice(device->parent);
		break;
	default:
		res = EPHIDGET_OK;
		break;
	}

	/*
	 * Run lock is required to prevent open from proceeding after close has cleared the open flag but
	 * has not yet actually closed the device.
	 */
	PhidgetRunLock(device);

	// If it's already open, retain the connection and return it
	if (PhidgetCKandSetFlags(device, PHIDGET_OPEN_FLAG) != EPHIDGET_OK) {
		PhidgetRetain(device->conn);
		PhidgetRunUnlock(device);
		return (EPHIDGET_OK);
	}

	switch (device->connType) {
	case PHIDCONN_VINT:
		break;
	case PHIDCONN_SPI:
		res = openAttachedSPIDevice(device);
		break;
	case PHIDCONN_LIGHTNING:
		res = openAttachedLightningDevice(device);
		break;
	case PHIDCONN_USB:
		res = openAttachedUSBDevice(device);
		break;
	case PHIDCONN_MESH:
		res = device->initAfterOpen(device);
		if (res != EPHIDGET_OK) {
			logerr("Device Initialization functions failed: "PRC_FMT, PRC_ARGS(res));
			if (res == EPHIDGET_BADVERSION)
				logwarn("This Phidget requires a newer library - please upgrade.");
		}
		break;
	case PHIDCONN_VIRTUAL:
		res = device->initAfterOpen(device);
		break;
	case PHIDCONN_NETWORK:
	default:
		MOS_PANIC("Unexpected connection type.");
	}

	if (res != EPHIDGET_OK) {
		if (res != EPHIDGET_BUSY)
			logerr("Failed to open a device: "PRC_FMT, PRC_ARGS(res));
		PhidgetCLRFlags(device, PHIDGET_OPEN_FLAG);
	} else {
		PhidgetRetain(device->conn);
	}

	PhidgetRunUnlock(device);
	return (res);
}

void
closeDevice(PhidgetDeviceHandle device, int forceClose) {
	PhidgetUSBConnectionHandle usbConn;
	PhidgetHandle connTmp;
	PhidgetSPIConnectionHandle spiConn;

	PhidgetRunLock(device);

	//Another thread closing? wait
	while (PhidgetCKFlags(device, PHIDGET_CLOSING_FLAG) == PHIDGET_CLOSING_FLAG) {
		PhidgetRunUnlock(device);
		mos_usleep(10000);
		PhidgetRunLock(device);
	}

	// Don't try to close something not open
	if (PhidgetCKFlags(device, PHIDGET_OPEN_FLAG) != PHIDGET_OPEN_FLAG) {
		PhidgetRunUnlock(device);
		return;
	}

closeAgain:
	// Close parent. This removes our reference to the parent.
	switch (device->connType) {
	case PHIDCONN_MESH:
	case PHIDCONN_VINT:
		PhidgetSetFlags(device, PHIDGET_CLOSING_FLAG);
		PhidgetRunUnlock(device);
		/*
		 * Do not force close the parent as it may not be detached.
		 *
		 * XXX - This must be called with the run lock unlocked, is it safe?
		 */
		closeDevice(device->parent, PFALSE);
		PhidgetRunLock(device);
		PhidgetCLRFlags(device, PHIDGET_CLOSING_FLAG);
		break;
	default:
		break;
	}

	// Ref count should never reach 0 from closeDevice, because the final reference is cleared during the device destruct
	assert(PhidgetGetRefCnt(device->conn) > 1);

	// In the force case, we need to keep closing this and the parent until conn ref reaches 2
	if (forceClose && PhidgetGetRefCnt(device->conn) > 2) {
		connTmp = device->conn;
		PhidgetRelease(&connTmp);
		goto closeAgain;
	}

	// Decrement ref
	connTmp = device->conn;
	PhidgetRelease(&connTmp);

	// Still open
	if (PhidgetGetRefCnt(device->conn) > 1) {
		PhidgetRunUnlock(device);
		return;
	}

	if (PhidgetCKandCLRFlags(device, PHIDGET_OPEN_FLAG) != EPHIDGET_OK) {
		PhidgetRunUnlock(device);
		return;
	}

	if (device->_closing)
		device->_closing(device);

	switch (device->connType) {
	case PHIDCONN_USB:
		usbConn = PhidgetUSBConnectionCast(device->conn);
		assert(usbConn);
		PhidgetUSBCloseHandle(usbConn);
		break;
	case PHIDCONN_SPI:
		spiConn = PhidgetSPIConnectionCast(device->conn);
		assert(spiConn);
		PhidgetSPICloseHandle(spiConn);
		break;
	case PHIDCONN_LIGHTNING:
		break;
	case PHIDCONN_MESH:
		assert(device->parent->deviceInfo.class == PHIDCLASS_MESHDONGLE);
		setPacketsReturnCode(device->parent, device->deviceInfo.uniqueIndex, EPHIDGET_NOTATTACHED);
		break;
	case PHIDCONN_VINT:
		assert(device->parent->deviceInfo.class == PHIDCLASS_HUB);
		setPacketsReturnCode(device->parent, device->deviceInfo.hubPort, EPHIDGET_NOTATTACHED);
		break;
	case PHIDCONN_NETWORK:
		assert(1); //TODO: Implement
		break;
	}

	PhidgetRunUnlock(device);
}

/*
 * Called on the client side to open a remote channel during attach.
 */
static PhidgetReturnCode
attachNetworkChannel(PhidgetDeviceHandle device, int uniqueIndex, PhidgetChannelHandle channel) {
	PhidgetReturnCode res, lastOpenRes;
	PhidgetDeviceHandle lastOpenDevice;
	PhidgetNetworkConnectionHandle netConn;
	char *errBuf;

	TESTPTR(device);
	TESTPTR(channel);

	if (!ISOPEN(channel))
		return (EPHIDGET_INVALIDARG);

	res = getUniqueChannelDef(device->deviceInfo.UDD, channel->class, uniqueIndex, &channel->index,
	  &channel->UCD);
	if (res != EPHIDGET_OK)
		return (res);
	channel->uniqueIndex = uniqueIndex;

	MOS_ASSERT(PhidgetCKFlags(channel, PHIDGET_DETACHING_FLAG) == 0);
	startDispatch((PhidgetHandle)channel);
	PhidgetCLRFlags(channel, PHIDGET_HASINITIALSTATE_FLAG);
	PhidgetSetFlags(channel, PHIDGET_ATTACHING_FLAG);

	setParent(channel, device);
	setChannel(device, uniqueIndex, channel);
	PhidgetSetFlags(channel, PHIDGET_NETWORK_FLAG);

	lastOpenRes = channel->openInfo->lastOpenRes;
	lastOpenDevice = channel->openInfo->lastOpenDevice;
	channel->openInfo->lastOpenRes = res;
	channel->openInfo->lastOpenDevice = device;

	errBuf = NULL;
	res = openNetworkChannel(channel, device, uniqueIndex, &errBuf);
	channel->openInfo->lastOpenRes = res;
	channel->openInfo->openAttempts++;
	if (res == EPHIDGET_OK) {
		PhidgetSetFlags(channel, PHIDGET_ATTACHED_FLAG);
		wakeDispatch();
		if (errBuf != NULL)
			mos_free(errBuf, MOSM_FSTR);

		return (EPHIDGET_OK);
	}

	netConn = PhidgetNetworkConnectionCast(getPhidgetConnection(device));

#define ANC_BADVERSION_ERROR_STR "NetworkServer class version mismatch on open. Make sure client and server are running the same library version."
#define ANC_BUSY_ERROR_STR "Network device: <%"PRIphid"> on Server: <%s> open failed because device is in use. Check that the Phidget is not already open in another program, " \
						"such as the Phidget Control Panel, or another program you are developing."
#define ANC_BUSY_ERROR_ARGS device, netConn->nc->rsrvname
#define ANC_GEN_ERROR_STR "Network device: <%"PRIphid"> on Server: <%s> open failed with error: "PRC_FMT"."
#define ANC_GEN_ERROR_ARGS device, netConn->nc->rsrvname, PRC_ARGS(res)
#define ANC_SRV_ERROR_STR "Network device: <%"PRIphid"> on Server: <%s> open failed. Error details from server: %s"
#define ANC_SRV_ERROR_ARGS device, netConn->nc->rsrvname, errBuf

	PLOCK(channel);
	if (channel->iop) {

		// Only log the first error in a row (for the same device) to the iop
		if (res != lastOpenRes || device != lastOpenDevice) {
			if (errBuf != NULL && strlen(errBuf) > 0)
				MOS_ERROR(channel->iop, res, ANC_SRV_ERROR_STR, ANC_SRV_ERROR_ARGS);
			else if (res == EPHIDGET_BADVERSION || res == EPHIDGET_UNSUPPORTED)
				MOS_ERROR(channel->iop, res, ANC_BADVERSION_ERROR_STR);
			else if (res == EPHIDGET_BUSY)
				MOS_ERROR(channel->iop, res, ANC_BUSY_ERROR_STR, ANC_BUSY_ERROR_ARGS);
			else
				MOS_ERROR(channel->iop, res, ANC_GEN_ERROR_STR, ANC_GEN_ERROR_ARGS);
		}
	}
	PUNLOCK(channel);

	if (channel->Error) {
		if (errBuf != NULL && strlen(errBuf) > 0)
			FIRE_ERROR(channel, EEPHIDGET_NETWORK, ANC_SRV_ERROR_STR, ANC_SRV_ERROR_ARGS);
		else if (res == EPHIDGET_BADVERSION || res == EPHIDGET_UNSUPPORTED)
			FIRE_ERROR(channel, EEPHIDGET_BADVERSION, ANC_BADVERSION_ERROR_STR);
		else if (res == EPHIDGET_BUSY)
			FIRE_ERROR(channel, EEPHIDGET_FAILURE, ANC_BUSY_ERROR_STR, ANC_BUSY_ERROR_ARGS);
		else
			FIRE_ERROR(channel, EEPHIDGET_NETWORK, ANC_GEN_ERROR_STR, ANC_GEN_ERROR_ARGS);
	}

	if (res == EPHIDGET_BADVERSION || res == EPHIDGET_UNSUPPORTED)
		logwarn(ANC_BADVERSION_ERROR_STR);

	if (errBuf != NULL)
		mos_free(errBuf, MOSM_FSTR);

	PhidgetRelease(&netConn);

	/* Failure */
	PhidgetCLRFlags(channel, PHIDGET_NETWORK_FLAG | PHIDGET_ATTACHING_FLAG | PHIDGET_ATTACHED_FLAG);
	clearPhidgetDispatch((PhidgetHandle)channel);
	setParent(channel, NULL);
	setChannel(device, uniqueIndex, NULL);
	return (res);
}

static PhidgetReturnCode
attachLocalChannel(PhidgetDeviceHandle device, int uniqueIndex, PhidgetChannelHandle channel) {
	PhidgetReturnCode res, lastOpenRes;
	PhidgetDeviceHandle lastOpenDevice;
	PhidgetChannelHandle ch;
	int i;


	if (!ISOPEN(channel))
		return (EPHIDGET_INVALIDARG);

	res = getUniqueChannelDef(device->deviceInfo.UDD, channel->class, uniqueIndex, &channel->index,
	  &channel->UCD);
	if (res != EPHIDGET_OK)
		return (res);
	channel->uniqueIndex = uniqueIndex;

	lastOpenRes = channel->openInfo->lastOpenRes;
	lastOpenDevice = channel->openInfo->lastOpenDevice;

	// Check whether we already have another channel open which would interfere with this one because of exclusive access
	if (channel->UCD->exclusive != -1) { // -1 means not exclusive
		PhidgetDeviceMemberLock(device);
		for (i = 0; i < PHIDGET_MAXCHANNELS; i++) {
			if (i == uniqueIndex)
				continue;
			ch = device->channel[i];
			if (ch == NULL)
				continue;

			// Exclusive means same index and same exclusive integer
			if (ch->index == channel->index && ch->UCD->exclusive == channel->UCD->exclusive) {
				PhidgetDeviceMemberUnlock(device);
				PLOCK(channel);
				if (channel->iop) {

					// Only log the first error in a row (for the same device) to the iop
					if (EPHIDGET_EXIST != lastOpenRes || device != lastOpenDevice)
						MOS_ERROR(channel->iop, EPHIDGET_BUSY, "Failed to open Channel <%"PRIphid"> on local device: <%"PRIphid"> " \
							"because Channel: <%"PRIphid"> is opened.", channel, device, ch);
				}
				PUNLOCK(channel);

				if (channel->Error)
					FIRE_ERROR(channel, EEPHIDGET_BUSY, "Failed to open Channel <%"PRIphid"> on local device: <%"PRIphid"> " \
						"because Channel: <%"PRIphid"> is opened.", channel, device, ch);

				channel->openInfo->lastOpenRes = EPHIDGET_EXIST;
				channel->openInfo->lastOpenDevice = device;
				return (EPHIDGET_BUSY);
			}
		}
		PhidgetDeviceMemberUnlock(device);
	}

	// Increment 1st, because openDevice can be slow, and openWaitForAttachment could timeout while it's running.
	channel->openInfo->openAttempts++;

	res = openDevice(device);

	channel->openInfo->lastOpenRes = res;
	channel->openInfo->lastOpenDevice = device;
	if (res != EPHIDGET_OK) {

#define ALC_BUSY_ERROR_STR "Local device: <%"PRIphid"> open failed because device is in use. Check that the Phidget is not already open in another program, " \
						"such as the Phidget Control Panel, or another program you are developing."
#define ALC_BUSY_ERROR_ARGS device
#define ALC_GEN_ERROR_STR "Local device: <%"PRIphid"> open failed with error: "PRC_FMT"."
#define ALC_GEN_ERROR_ARGS device, PRC_ARGS(res)

		PLOCK(channel);
		if (channel->iop) {

			// Only log the first error in a row (for the same device) to the iop
			if (res != lastOpenRes || device != lastOpenDevice) {
				if (device->lastOpenErrstr != NULL && strlen(device->lastOpenErrstr) > 0)
					MOS_ERROR(channel->iop, res, device->lastOpenErrstr);
				else if (res == EPHIDGET_BUSY)
					MOS_ERROR(channel->iop, res, ALC_BUSY_ERROR_STR, ALC_BUSY_ERROR_ARGS);
				else
					MOS_ERROR(channel->iop, res, ALC_GEN_ERROR_STR, ALC_GEN_ERROR_ARGS);
			}
		}
		PUNLOCK(channel);

		if (channel->Error) {
			if (device->lastOpenErrstr != NULL && strlen(device->lastOpenErrstr) > 0)
				FIRE_ERROR(channel, EEPHIDGET_FAILURE, device->lastOpenErrstr);
			else if (res == EPHIDGET_BUSY)
				FIRE_ERROR(channel, EEPHIDGET_BUSY, ALC_BUSY_ERROR_STR, ALC_BUSY_ERROR_ARGS);
			else
				FIRE_ERROR(channel, EEPHIDGET_FAILURE, ALC_GEN_ERROR_STR, ALC_GEN_ERROR_ARGS);
		}

		return (res);
	}

	MOS_ASSERT(PhidgetCKFlags(channel, PHIDGET_DETACHING_FLAG) == 0);
	startDispatch((PhidgetHandle)channel);
	PhidgetCLRFlags(channel, PHIDGET_HASINITIALSTATE_FLAG);
	PhidgetSetFlags(channel, PHIDGET_ATTACHING_FLAG);

	setParent(channel, device);
	setChannel(device, uniqueIndex, channel);

	PhidgetSetFlags(channel, PHIDGET_ATTACHED_FLAG);
	wakeDispatch();

	return (EPHIDGET_OK);
}

PhidgetReturnCode
attachChannel(PhidgetDeviceHandle device, int uniqueIndex, PhidgetChannelHandle channel) {

	if (isNetworkPhidget(device))
		return (attachNetworkChannel(device, uniqueIndex, channel));
	return (attachLocalChannel(device, uniqueIndex, channel));
}

typedef enum {
	MATCHMODE_SERIALLABEL_AND_HUBPORT,
	MATCHMODE_SERIALLABEL,
	MATCHMODE_HUBPORT,
	MATCHMODE_ANY
} findMatchingAttachedChannelMode;

struct matchEntry {
	PhidgetChannelHandle channel;
	int uniqueIndex;
	MTAILQ_ENTRY(matchEntry) link;
};

MTAILQ_HEAD(match_list, matchEntry);

struct matchArgs {
	findMatchingAttachedChannelMode mode;
	struct match_list *list;
};

static void
matchAttachedChannel(PhidgetDeviceHandle device, const PhidgetUniqueChannelDef *ucd, int index,
  int uniqueIndex, findMatchingAttachedChannelMode mode, struct match_list *list) {
	PhidgetChannelHandle channel, tmpch;
	struct matchEntry *matchEntry;

	matchlogdebug("matching %"PRIphid" against open channels (mode=0x%x)", device, mode);

	if (!_ISATTACHED(device) || _ISDETACHING(device)) {
		matchloginfo("%"PRIphid" not attached, or detaching", device);
		return;
	}

	if (CHANNELS_EMPTY)
		matchloginfo("%"PRIphid": no open channels", device);

	FOREACH_CHANNEL(channel) {

		matchlogdebug("%"PRIphid" vs channel %"PRIphid"", channel, device);

		/*
		 * Fast attached check without lock
		 */
		if (_ISATTACHED(channel)) {
			matchloginfo("REJECT %"PRIphid" is attached", channel);
			continue;
		}

		/*
		 * CLASS
		 */
		if (channel->class != ucd->class) {
			matchloginfo("%"PRIphid": REJECT class does not match", channel);
			continue;
		}

		/*
		 * CHANNEL
		 */
		if (channel->openInfo->channel != PHIDGET_CHANNEL_ANY) {
			if (channel->openInfo->channel != index) {
				matchloginfo("%"PRIphid": REJECT channel index %d != %d", channel,
				  channel->openInfo->channel, index);
				continue;
			}
		}

		/*
		 * ALREADY ATTACHED
		 * Requires a lock, so check the above fast checks first.
		 */
		if (ISATTACHEDORDETACHING(channel)) {
			matchloginfo("REJECT %"PRIphid" is attached or detaching", channel);
			continue;
		}

		/*
		 * Closed, but still in the list.
		 */
		if (PhidgetCKFlags(channel, PHIDGET_OPEN_FLAG) == 0) {
			matchloginfo("REJECT %"PRIphid" is closed", channel);
			continue;
		}

		/*
		 * This attached channel has already been opened
		 */
		tmpch = getChannel(device, uniqueIndex);
		if (tmpch) {
			matchloginfo("%"PRIphid": REJECT device index in use: %d (%"PRIphid")", channel, uniqueIndex, tmpch);
			PhidgetRelease(&tmpch);
			continue;
		}

		/*
		 * Don't open device marked as local over network
		 */
		if (channel->openInfo->isLocal) {
			if (isNetworkPhidget(device)) {
				matchloginfo("%"PRIphid": REJECT device (%"PRIphid") is from network", channel, device);
				continue;
			}
		}

		/*
		 * Don't open device marked as remote locally
		 */
		if (channel->openInfo->isRemote) {
			if (!isNetworkPhidget(device)) {
				matchloginfo("%"PRIphid": REJECT device (%"PRIphid") is not from network", channel, device);
				continue;
			}
		}

		/*
		 * Network open
		 */
		if (channel->openInfo->openFlags & PHIDGETOPEN_NETWORK) {
			if (!isNetworkPhidget(device))
				continue;
			if (channel->openInfo->serverName != NULL &&
			  strcmp(channel->openInfo->serverName, getServerName(device)) != 0) {
				matchloginfo("%"PRIphid": REJECT server names do not match (%s vs %s)", channel,
				  channel->openInfo->serverName, getServerName(device));
				continue;
			}
		}

		if (device->deviceInfo.UDD->type == PHIDTYPE_VINT) {

			/*
			 * OPEN MODE
			 */
			if (channel->openInfo->openFlags & (PHIDGETOPEN_SERIAL | PHIDGETOPEN_LABEL))
				if (mode != MATCHMODE_SERIALLABEL_AND_HUBPORT && mode != MATCHMODE_SERIALLABEL)
					continue;

			if (channel->openInfo->hubPort != PHIDGET_HUBPORT_ANY)
				if (mode != MATCHMODE_SERIALLABEL_AND_HUBPORT && mode != MATCHMODE_HUBPORT)
					continue;

			/*
			 * SERIAL NUMBER
			 */
			if (channel->openInfo->openFlags & PHIDGETOPEN_SERIAL)
				if (channel->openInfo->serialNumber != device->deviceInfo.serialNumber) {
					matchloginfo("%"PRIphid": REJECT serial number does not match (%d vs %d)", channel,
					  channel->openInfo->serialNumber, device->deviceInfo.serialNumber);
					continue;
				}

			/*
			 * LABEL
			 */
			if (channel->openInfo->openFlags & PHIDGETOPEN_LABEL)
				if (strcmp(channel->openInfo->label, device->deviceInfo.label) != 0) {
					matchloginfo("%"PRIphid": REJECT label does not match (%s vs %s)", channel,
					  channel->openInfo->label, device->deviceInfo.label);
					continue;
				}

			/*
			 * HUB PORT
			 */
			if (channel->openInfo->hubPort != PHIDGET_HUBPORT_ANY)
				if (channel->openInfo->hubPort != device->deviceInfo.hubPort) {
					matchloginfo("%"PRIphid": REJECT hub port does not match (%d vs %d)", channel,
					  channel->openInfo->hubPort, device->deviceInfo.hubPort);
					continue;
				}

			/* make sure we don't attach to a hub port unless we want to */
			if (channel->openInfo->hubPortMode != PORT_MODE_VINT_PORT &&
			  device->deviceInfo.UDD->vintID > HUB_PORT_ID_MAX) {
				matchloginfo("%"PRIphid": REJECT device is hub port", channel);
				continue;
			}
			if (channel->openInfo->hubPortMode == PORT_MODE_VINT_PORT &&
			  device->deviceInfo.UDD->vintID <= HUB_PORT_ID_MAX) {
				matchloginfo("%"PRIphid": REJECT device is not hub port", channel);
				continue;
			}
		} else {
			/*
			 * OPEN MODE
			 */
			if (channel->openInfo->openFlags & (PHIDGETOPEN_SERIAL | PHIDGETOPEN_LABEL))
				if (mode != MATCHMODE_SERIALLABEL)
					continue;
			/*
			 * SERIAL NUMBER
			 */
			if (channel->openInfo->openFlags & PHIDGETOPEN_SERIAL)
				if (channel->openInfo->serialNumber != device->deviceInfo.serialNumber) {
					matchloginfo("%"PRIphid": REJECT serial number does not match (%d vs %d)", channel,
					  channel->openInfo->serialNumber, device->deviceInfo.serialNumber);
					continue;
				}

			/*
			 * LABEL
			 */
			if (channel->openInfo->openFlags & PHIDGETOPEN_LABEL)
				if (strcmp(channel->openInfo->label, device->deviceInfo.label) != 0) {
					matchloginfo("%"PRIphid": REJECT serial number does not match (%d vs %d)", channel,
					  channel->openInfo->serialNumber, device->deviceInfo.serialNumber);
					continue;
				}

			/*
			 * Do not attach to a USB phidget if hub port information has been specified.
			 */
			if (channel->openInfo->hubPort != PHIDGET_HUBPORT_ANY && channel->openInfo->hubPort != 0) {
				matchloginfo("%"PRIphid": REJECT hub port specified, but USB device", channel);
				continue;
			}

			if (channel->openInfo->isHubPort) {
				matchloginfo("%"PRIphid": REJECT hub port device specified, but USB device", channel);
				continue;
			}
		}

		matchloginfo("***** %"PRIphid" matched device %"PRIphid"", channel, device);
		matchEntry = mos_malloc(sizeof(*matchEntry));
		matchEntry->uniqueIndex = uniqueIndex;
		matchEntry->channel = channel;
		PhidgetRetain(channel);
		MTAILQ_INSERT_TAIL(list, matchEntry, link);
	}
}

static PhidgetReturnCode
matchVisitor(PhidgetDeviceHandle device, const PhidgetUniqueChannelDef *ucd, int index,
  int uniqueIndex, void *ctx) {
	struct matchArgs *ma;

	ma = ctx;
	matchAttachedChannel(device, ucd, index, uniqueIndex, ma->mode, ma->list);

	return (EPHIDGET_OK);
}

static void
_matchOpenChannels(PhidgetDeviceHandle device) {
	struct matchEntry *matchEntry, *tmp;
	struct match_list list;
	PhidgetReturnCode res;
	struct matchArgs ma;
	int matched;
	int iterations;

	iterations = 0;
again:
	// Limit so we don't end up in an infinite loop
	if (iterations++ > 32)
		return;

	MTAILQ_INIT(&list);

	ma.list = &list;

	PhidgetReadLockChannels();

	/*
	 * Find possible matches. The best matches will be first in the list
	 * This is done this way so that two open channels for the same device class will match to the
	 * most specific device channel.
	 */
	ma.mode = MATCHMODE_SERIALLABEL_AND_HUBPORT;
	walkDeviceChannels(device, matchVisitor, &ma);

	ma.mode = MATCHMODE_SERIALLABEL;
	walkDeviceChannels(device, matchVisitor, &ma);

	ma.mode = MATCHMODE_HUBPORT;
	walkDeviceChannels(device, matchVisitor, &ma);

	ma.mode = MATCHMODE_ANY;
	walkDeviceChannels(device, matchVisitor, &ma);

	PhidgetUnlockChannels();

	matched = 0;
	MTAILQ_FOREACH_SAFE(matchEntry, &list, link, tmp) {
		if (!matched) {
			res = attachChannel(device, matchEntry->uniqueIndex, matchEntry->channel);
			if (res == EPHIDGET_OK)
				matched = 1;
		}
		MTAILQ_REMOVE(&list, matchEntry, link);
		PhidgetRelease(&matchEntry->channel);
		mos_free(matchEntry, sizeof(*matchEntry));
	}
	if (matched)
		goto again;
}

void
matchOpenChannels() {
	PhidgetDeviceHandle device;

	PhidgetReadLockDevices();
	FOREACH_DEVICE(device)
		_matchOpenChannels(device);
	PhidgetUnlockDevices();

	matchCount++;
}

size_t
getMaxOutPacketSize(PhidgetDeviceHandle device) {
	PhidgetUSBConnectionHandle usbConn;

	assert(device);

	switch (device->connType) {
	case PHIDCONN_USB:
		usbConn = PhidgetUSBConnectionCast(device->conn);
		assert(usbConn);
		return (usbConn->outputReportByteLength);
	case PHIDCONN_SPI:
		return (MAX_SPI_PACKET_SIZE);
	case PHIDCONN_LIGHTNING:
		return (MAX_LIGHTNING_OUT_PACKET_SIZE);
	case PHIDCONN_VINT:
		return VINT_MAX_OUT_PACKETSIZE + 3; //max app data + 3 bytes overhead
	case PHIDCONN_MESH:
		return (getMaxOutPacketSize(device->parent) - 6); //Mesh dongle adds 6 byte overhead
	case PHIDCONN_NETWORK:
		return MAX_OUT_PACKET_SIZE;
	default:
		MOS_PANIC("Invalid connection type");
	}
}

int
getTimeout(PhidgetDeviceHandle device) {
	do {
		assert(device);
		switch (device->connType) {
		case PHIDCONN_USB:
		case PHIDCONN_SPI:
		case PHIDCONN_LIGHTNING:
			return 1000;

		case PHIDCONN_MESH:
			return 5000;

		case PHIDCONN_NETWORK:
			return 1000;

		case PHIDCONN_VINT:
			device = device->parent;
			continue;

		default:
			assert(1);
			return -1;
		}
	} while (1);

	assert(1);
	return -1;
}

uint32_t HANDLE_DATAINTERVAL_PKT(BridgePacket *bp, uint32_t interruptRate) {
	uint32_t __di;
	__di = getBridgePacketUInt32(bp, 0);
	if (__di % interruptRate != 0) {
		__di = ((__di / interruptRate) + 1) * interruptRate;
		setBridgePacketUInt32(bp, __di, 0);
	}
	return __di;
}

static PhidgetReturnCode sendpacket(mosiop_t iop, PhidgetDeviceHandle device, const unsigned char *bufferIn,
  size_t bufferInLen, PhidgetTransactionHandle trans, int *sent);

static PhidgetReturnCode
sendpacketWithTracking(mosiop_t iop, PhidgetDeviceHandle device, const unsigned char *buf, size_t length,
  PhidgetPacketTrackerHandle packetTracker, PhidgetTransactionHandle trans, int *sent) {
	PhidgetPacketTrackerHandle tracker, tmp;
	PhidgetReturnCode res;

	res = sendpacket(iop, device, buf, length, trans, sent);

	if (res != EPHIDGET_OK)
		return res;

	if (trans) {
		//Deal with any packet trackers that are done
		// - allows packet trackers to be released before the endTransaction call so we don't run out
		MTAILQ_FOREACH_SAFE((tracker), &trans->list, link, (tmp)) {
			//wait 0 so it doesn't block
			if (waitForPendingPacket(tracker, 0) == EPHIDGET_OK) {
				getPacketReturnCode(tracker, &res);
				if (res != EPHIDGET_OK)
					trans->res = res;
				releasePacketTracker(device, tracker);
				MTAILQ_REMOVE(&trans->list, tracker, link);
				break;
			}
		}
		//Add new packet tracker
		logverbose("%s Device: %p Packet: %p Sent",
		  device->deviceInfo.UDD->type == PHIDTYPE_MESH ? "Mesh" : "Hub", device, packetTracker);
		MTAILQ_INSERT_TAIL(&trans->list, packetTracker, link);
		return (EPHIDGET_OK);
	} else {
		res = waitForPendingPacket(packetTracker, getTimeout(device));
		if (res == EPHIDGET_OK)
			getPacketReturnCode(packetTracker, &res);

		return (res);
	}
}

static PhidgetReturnCode
sendpacket(mosiop_t iop, PhidgetDeviceHandle device, const unsigned char *bufferIn, size_t bufferInLen,
  PhidgetTransactionHandle trans, int *sent) {
	PhidgetPacketTrackerHandle packetTracker;
	uint8_t buffer[MAX_OUT_PACKET_SIZE];
	PhidgetHubDeviceHandle hubDevice;
	PhidgetUSBConnectionHandle usbConn;
	PhidgetReturnCode res;
	size_t bufferLen;
	int packetID;
	PhidgetSPIConnectionHandle spiConn;
	PhidgetLightningConnectionHandle lightningConn;
	PhidgetMeshDongleDeviceHandle meshDongleDevice;

	assert(device);
	assert(bufferIn);
	assert(bufferInLen <= MAX_OUT_PACKET_SIZE);
	assert(bufferInLen <= getMaxOutPacketSize(device));

	bufferLen = sizeof(buffer);
	memset(buffer, 0, bufferLen);

	switch (device->connType) {
	case PHIDCONN_USB:
		usbConn = PhidgetUSBConnectionCast(device->conn);
		assert(usbConn);
		res = PhidgetUSBSendPacket(iop, usbConn, bufferIn, bufferInLen);
		switch (res) {
		case EPHIDGET_OK:
		case EPHIDGET_NOTATTACHED:
		case EPHIDGET_INTERRUPTED:
			break;
		case EPHIDGET_TIMEOUT:
			logerr("PhidgetUSBSendPacket() unexpected timeout (could be an ESD event)");
			PhidgetUSBError(device);
			break;
		case EPHIDGET_UNEXPECTED:
		default:
			logerr("PhidgetUSBSendPacket() returned: "PRC_FMT, PRC_ARGS(res));
			PhidgetUSBError(device);
			break;
		}
		break;

	case PHIDCONN_SPI:
		spiConn = PhidgetSPIConnectionCast(device->conn);
		assert(spiConn);
		res = PhidgetSPISendPacket(iop, spiConn, bufferIn, bufferInLen);
		switch (res) {
		case EPHIDGET_OK:
			break;
		default:
			logerr("PhidgetSPISendPacket() returned: "PRC_FMT, PRC_ARGS(res));
			MOS_ERROR(iop, res, "Failed to send SPI packet.");
			break;
		}
		break;

	case PHIDCONN_LIGHTNING:
		lightningConn = PhidgetLightningConnectionCast(device->conn);
		assert(lightningConn);
		res = PhidgetLightningSendPacket(iop, lightningConn, bufferIn, bufferInLen);
		switch (res) {
		case EPHIDGET_OK:
			break;
		default:
			logerr("PhidgetLightningSendPacket returned: "PRC_FMT, PRC_ARGS(res));
			MOS_ERROR(iop, res, "Failed to send Lightning packet.");
			break;
		}
		break;

	case PHIDCONN_MESH:
		meshDongleDevice = (PhidgetMeshDongleDeviceHandle)device->parent;
		assert(meshDongleDevice != NULL);
		assert(meshDongleDevice->phid.deviceInfo.class == PHIDCLASS_MESHDONGLE);

		res = getPacketTrackerWait((PhidgetDeviceHandle)meshDongleDevice, &packetID, &packetTracker, 1,
		  MAX_PACKET_IDS - 1, device->deviceInfo.uniqueIndex, 500);
		if (res != EPHIDGET_OK)
			return (MOS_ERROR(iop, res, "Failed to get a packet tracker."));

		logverbose("Claimed Mesh packet ID %d", packetID);

		res = PhidgetMeshDongleDevice_makePacket(meshDongleDevice, device, packetID, bufferIn,
		  bufferInLen, buffer, &bufferLen);
		if (res != EPHIDGET_OK) {
			releasePacketTracker((PhidgetDeviceHandle)meshDongleDevice, packetTracker);
			return (MOS_ERROR(iop, res, "Failed to make packet."));
		}

		res = PhidgetMeshDongleDevice_claimPacketSpace(meshDongleDevice, device, bufferLen);
		if (res != EPHIDGET_OK) {
			releasePacketTracker((PhidgetDeviceHandle)meshDongleDevice, packetTracker);
			return (MOS_ERROR(iop, res, "Failed to claim packet space."));
		}

		setPacketLength(packetTracker, bufferLen);
		res = sendpacketWithTracking(iop, (PhidgetDeviceHandle)meshDongleDevice, buffer, bufferLen, packetTracker,
		  trans, sent);
		if (res != EPHIDGET_OK || trans == NULL)
			releasePacketTracker((PhidgetDeviceHandle)meshDongleDevice, packetTracker);

		break;

	case PHIDCONN_VINT:
		hubDevice = (PhidgetHubDeviceHandle)device->parent;
		assert(hubDevice != NULL);
		assert(hubDevice->phid.deviceInfo.class == PHIDCLASS_HUB);

		//On the vint hub, the port is encoded in the packet id (range is 1 - 126)
		res = getPacketTrackerWait((PhidgetDeviceHandle)hubDevice, &packetID, &packetTracker,
			device->deviceInfo.hubPort * VINTHUB_PACKETIDS_PER_PORT + 1,
			((device->deviceInfo.hubPort + 1) * VINTHUB_PACKETIDS_PER_PORT), device->deviceInfo.hubPort, 500);
		if (res != EPHIDGET_OK)
			return (MOS_ERROR(iop, res, "Failed to get a packet tracker."));

		logverbose("Claimed Hub packet ID %d", packetID);

		res = PhidgetHubDevice_makePacket(hubDevice, device, packetID, bufferIn,
		  bufferInLen, buffer, &bufferLen);
		if (res != EPHIDGET_OK) {
			releasePacketTracker((PhidgetDeviceHandle)hubDevice, packetTracker);
			return (MOS_ERROR(iop, res, "Failed to make packet."));
		}

		res = PhidgetHubDevice_claimPacketSpace(hubDevice, device->deviceInfo.hubPort, bufferLen);
		if (res != EPHIDGET_OK) {
			releasePacketTracker((PhidgetDeviceHandle)hubDevice, packetTracker);
			return (MOS_ERROR(iop, res, "Failed to claim packet space."));
		}

		setPacketLength(packetTracker, bufferLen);
		res = sendpacketWithTracking(iop, (PhidgetDeviceHandle)hubDevice, buffer, bufferLen, packetTracker,
		  trans, sent);
		if (res != EPHIDGET_OK || trans == NULL)
			releasePacketTracker((PhidgetDeviceHandle)hubDevice, packetTracker);

		if (res != EPHIDGET_OK)
			logverbose("sendpacketWithTracking returned: "PRC_FMT, PRC_ARGS(res));
		else
			logverbose("Packet sent successfully");

		break;

	default:
		MOS_PANIC("Unexpected connection type.");
	}

	return (res);
}

PhidgetReturnCode
PhidgetDevice_sendpacket(mosiop_t iop, PhidgetDeviceHandle device, const unsigned char *bufferIn, size_t bufferInLen) {

	return (sendpacket(iop, device, bufferIn, bufferInLen, NULL, NULL));
}

PhidgetReturnCode
PhidgetDevice_sendpacketTransaction(mosiop_t iop, PhidgetDeviceHandle device, const unsigned char *bufferIn,
  size_t bufferInLen, PhidgetTransactionHandle trans, int *sent) {
	return (sendpacket(iop, device, bufferIn, bufferInLen, trans, sent));
}

PhidgetReturnCode
PhidgetChannel_beginTransaction(PhidgetChannelHandle channel, PhidgetTransactionHandle trans) {

	return (PhidgetDevice_beginTransaction(channel->parent, trans));
}

PhidgetReturnCode
PhidgetDevice_beginTransaction(PhidgetDeviceHandle device, PhidgetTransactionHandle trans) {

	logverbose("Transaction started");
	trans->res = EPHIDGET_OK;
	MTAILQ_INIT(&trans->list);
	return (EPHIDGET_OK);
}

PhidgetReturnCode
PhidgetChannel_endTransaction(PhidgetChannelHandle channel, PhidgetTransactionHandle trans) {
	PhidgetPacketTrackerHandle tracker, tmp;
	PhidgetReturnCode res, masterRes;
	PhidgetDeviceHandle device;

	// This function is only valid for VINT devices right now - because only the vint hub has packet trackers
	device = channel->parent;
	assert(device);
	assert(device->deviceInfo.class == PHIDCLASS_VINT);
	device = device->parent;
	assert(device);

	masterRes = EPHIDGET_OK;

	//TODO: Timeout should be a total timeout, not timeout*number of transactions
	MTAILQ_FOREACH_SAFE((tracker), &trans->list, link, (tmp)) {
		res = waitForPendingPacket(tracker, getTimeout(device));
		if (res == EPHIDGET_OK)
			getPacketReturnCode(tracker, &res);
		if (res != EPHIDGET_OK)
			masterRes = res;
		releasePacketTracker(device, tracker);
		MTAILQ_REMOVE(&trans->list, tracker, link);
	}

	logverbose("Transaction ended");
	return (masterRes);
}

static PhidgetReturnCode
registerChannel(mosiop_t iop, PhidgetChannelHandle channel) {
	PhidgetChannelHandle xchannel;

	assert(channel);

	PhidgetWriteLockChannels();

	/*
	 * We need to make sure we don't try to open the same port under 2 different port modes,
	 * unless the serial numbers differ.
	 */
	FOREACH_CHANNEL(xchannel) {
		if (xchannel->openInfo->hubPort == channel->openInfo->hubPort
		  && xchannel->openInfo->hubPortMode != channel->openInfo->hubPortMode) {
			if (((xchannel->openInfo->openFlags & PHIDGETOPEN_SERIAL) &&
				(channel->openInfo->openFlags & PHIDGETOPEN_SERIAL) &&
				(xchannel->openInfo->serialNumber == channel->openInfo->serialNumber)) ||
				((xchannel->openInfo->openFlags & PHIDGETOPEN_LABEL) &&
				(channel->openInfo->openFlags & PHIDGETOPEN_LABEL) &&
					(!strcmp(xchannel->openInfo->label, channel->openInfo->label)))) {
				PhidgetUnlockChannels();
				logwarn("Open was called on a port twice with different port modes. "
				  "This is not allowed unless hub serial numbers or labels are different.");
				return (MOS_ERROR(iop, EPHIDGET_BUSY, "Open was called on a port twice with different port modes. "
					"This is not allowed unless hub serial numbers or labels are different."));
			}
		}
	}

	PhidgetUnlockChannels();
	addChannel(channel);

	return (StartCentralThread());
}

static PhidgetReturnCode
Phidget_open_internal(mosiop_t iop, PhidgetChannelHandle channel, unsigned char async, uint32_t timeout) {
	PhidgetReturnCode res;
	int retained;

	/* Can't be both remote and local */
	if (channel->openInfo->isLocal && channel->openInfo->isRemote)
		return (MOS_ERROR(iop, EPHIDGET_INVALIDARG, "IsRemote and IsLocal are both set to true."));

	if (PhidgetCKandSetFlags(channel, PHIDGET_OPEN_FLAG) != EPHIDGET_OK) {
		logwarn("Open was called on an already opened Phidget handle.");
		return (EPHIDGET_OK);
	}

	channel->openInfo->openAttempts = 0;
	channel->openInfo->lastOpenRes = EPHIDGET_OK;

	/* Opening a Manager channel */
	retained = 0;
	if (PhidgetCKandCLRFlags(channel, PHIDGET_ATTACHED_FLAG) == EPHIDGET_OK) {
		retained = 1;
		PhidgetRetain(channel);
	}

	channel->openInfo->async = async;
	channel->openInfo->timeout = timeout;

	/*
	 * Determine the open mode.
	 */
	channel->openInfo->openFlags = 0;
	if (channel->openInfo->serialNumber != PHIDGET_SERIALNUMBER_ANY)
		channel->openInfo->openFlags |= PHIDGETOPEN_SERIAL;
	if (channel->openInfo->label != NULL)
		channel->openInfo->openFlags |= PHIDGETOPEN_LABEL;
	if (channel->openInfo->serverName != NULL)
		channel->openInfo->openFlags |= PHIDGETOPEN_NETWORK;
	if (channel->openInfo->isRemote)
		channel->openInfo->openFlags |= PHIDGETOPEN_NETWORK;

	/*
	 * Determine the hub port mode.
	 */
	if (channel->openInfo->isHubPort) {
		switch (channel->class) {
		case PHIDCHCLASS_DIGITALINPUT:
			channel->openInfo->hubPortMode = PORT_MODE_DIGITAL_INPUT;
			break;
		case PHIDCHCLASS_DIGITALOUTPUT:
			channel->openInfo->hubPortMode = PORT_MODE_DIGITAL_OUTPUT;
			break;
		case PHIDCHCLASS_VOLTAGEINPUT:
			channel->openInfo->hubPortMode = PORT_MODE_VOLTAGE_INPUT;
			break;
		case PHIDCHCLASS_VOLTAGERATIOINPUT:
			channel->openInfo->hubPortMode = PORT_MODE_VOLTAGE_RATIO_INPUT;
			break;
		default:
			res = EPHIDGET_INVALIDARG;
			goto bad;
		}
	} else {
		channel->openInfo->hubPortMode = PORT_MODE_VINT_PORT;
	}

	/*
	 * Checks for conflicts against already open channels, and adds the channel to the channels list.
	 */
	res = registerChannel(iop, channel);
	if (res != EPHIDGET_OK)
		goto bad;

	/*
	 * Either async is set to false explicitly, or it's unset but there isn't an attach handler.
	 */
	if (channel->openInfo->async == PFALSE) {
		res = waitForAttach(iop, channel, channel->openInfo->timeout);
		if (res != EPHIDGET_OK) {
			Phidget_close((PhidgetHandle)channel);
			goto bad;
		}
	}

	return (EPHIDGET_OK);

bad:

	PhidgetCLRFlags(channel, PHIDGET_OPEN_FLAG);
	if (retained)
		PhidgetRelease(&channel);
	return (res);
}

PhidgetReturnCode
addDevice(PhidgetDeviceHandle device) {
	PhidgetReturnCode res;

	PhidgetWriteLockDevices();
	res = _addDevice(device);
	PhidgetUnlockDevices();

	return (res);
}

PhidgetReturnCode
_addDevice(PhidgetDeviceHandle device) {

	/* callers do not always check the return value */
	MOS_ASSERT(device);

	if (!device->deviceInfo.isHubPort)
		logdebug("%"PRIphid"", device);

	MOS_ASSERT(PhidgetCKandSetFlags(device, PHIDGET_INDEVICELIST_FLAG) == EPHIDGET_OK);

	MTAILQ_INSERT_HEAD(&phidgetDevices, device, link);

	phidgetDevicesCount++;
	PhidgetRetain(device);

	return (EPHIDGET_OK);
}

PhidgetReturnCode
_removeDevice(PhidgetDeviceHandle device) {

	MOS_ASSERT(device != NULL);

	MOS_ASSERT(PhidgetCKandCLRFlags(device, PHIDGET_INDEVICELIST_FLAG) == EPHIDGET_OK);

	MTAILQ_REMOVE(&phidgetDevices, device, link);
	phidgetDevicesCount--;

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

PhidgetReturnCode
getNetworkDevice(PhidgetNetConnHandle nc, uint64_t id, PhidgetDeviceHandle *device) {
	PhidgetNetworkConnectionHandle netConn;
	NetAttachDetachEntryHandle entry;
	PhidgetDeviceHandle deviceTmp;

	PhidgetReadLockDevices();
	FOREACH_DEVICE(deviceTmp) {
		if (deviceTmp->connType != PHIDCONN_NETWORK)
			continue;

		netConn = PhidgetNetworkConnectionCast(deviceTmp->conn);
		assert(netConn);

		if (netConn->nc == nc && netConn->id == id) {
			if (device)
				*device = deviceTmp;
			PhidgetRetain(*device);
			PhidgetUnlockDevices();
			return (EPHIDGET_OK);
		}
	}
	PhidgetUnlockDevices();

	// Check in the net attach queue for the device
	PhidgetLockNetAttachDetachQueue();
	FOREACH_NET_ATTACHDETACH_QUEUE_DEVICE(entry) {
		deviceTmp = entry->dev;
		if (entry->flags != NETATTACHQUEUE_FLAG)
			continue;
		if (deviceTmp->connType != PHIDCONN_NETWORK)
			continue;

		netConn = PhidgetNetworkConnectionCast(deviceTmp->conn);
		assert(netConn);

		if (netConn->nc == nc && netConn->id == id) {
			if (device)
				*device = deviceTmp;
			PhidgetRetain(*device);
			PhidgetUnlockNetAttachDetachQueue();
			return (EPHIDGET_OK);
		}
	}
	PhidgetUnlockNetAttachDetachQueue();

	if (device)
		*device = NULL;

	return (EPHIDGET_NOENT);
}

PhidgetDeviceHandle
getDeviceById(uint64_t id) {
	PhidgetDeviceHandle dev;

	PhidgetReadLockDevices();
	FOREACH_DEVICE(dev) {
		if (id == ((uint64_t)((uintptr_t)dev)))
			break;
	}

	if (dev)
		PhidgetRetain(dev);
	PhidgetUnlockDevices();
	return (dev);
}

PhidgetReturnCode
addChannel(PhidgetChannelHandle channel) {

	MOS_ASSERT(channel);

	PhidgetWriteLockChannels();
	MTAILQ_INSERT_TAIL(&phidgetChannels, channel, link);
	phidgetChannelsCount++;
	PhidgetUnlockChannels();

	PhidgetRetain(channel);
	return (EPHIDGET_OK);
}

PhidgetReturnCode
removeChannel(PhidgetChannelHandle channel) {

	MOS_ASSERT(channel);

	PhidgetWriteLockChannels();
	MTAILQ_REMOVE(&phidgetChannels, channel, link);
	phidgetChannelsCount--;
	PhidgetUnlockChannels();

	PhidgetRelease(&channel);
	return (EPHIDGET_OK);
}

uint64_t
mkChannelId(int chindex, int chclass, int serialNumber, int vint, int port, int hubport) {
	channelid_t chid;

	chid.c_id = 0;

	chid.c_index = chindex;
	chid.c_class = chclass;
	chid.c_serial = mos_htole32(serialNumber);
	chid.c_port = port;
	if (vint)
		chid.c_flags |= CHANNELID_ISVINTDEVICE;
	if (hubport)
		chid.c_flags |= CHANNELID_ISHUBPORT;

	return (mos_htole64(chid.c_id));
}

uint64_t
getChannelId(PhidgetChannelHandle channel) {

	if (!_ISATTACHED(channel) && !_ISATTACHING(channel))
		return (0);

	if (isVintChannel(channel))
		return (mkChannelId(channel->uniqueIndex, channel->class, channel->parent->deviceInfo.serialNumber, 1,
			channel->parent->deviceInfo.hubPort, channel->parent->deviceInfo.isHubPort));
	return (mkChannelId(channel->uniqueIndex, channel->class, channel->parent->deviceInfo.serialNumber, 0, 0, 0));
}

PhidgetChannelHandle
getChannelById(uint64_t id) {
	PhidgetChannelHandle ch;

	PhidgetReadLockChannels();
	FOREACH_CHANNEL(ch) {
		if (getChannelId(ch) == id) {
			PhidgetUnlockChannels();
			PhidgetRetain(ch);
			return (ch);
		}
	}
	PhidgetUnlockChannels();

	return (NULL);
}

const char * CCONV
deviceInfo(PhidgetDeviceHandle device, char *ubuf, uint32_t buflen) {
	static char sbuf[128];
	const char *label;
	char *buf;

	buf = ubuf;
	if (buf == NULL) {
		buflen = sizeof(sbuf);
		buf = sbuf;
	}

	label = device->deviceInfo.label;
	if (label && mos_strlen(label) == 0)
		label = NULL;

	if (device->deviceInfo.class == PHIDCLASS_VINT) {
		if (label)
			snprintf(buf, buflen, "%s(%s) (%d) [%s][%d]", device->deviceInfo.UDD->name,
			  device->deviceInfo.UDD->SKU, device->deviceInfo.serialNumber, label,
			  device->deviceInfo.hubPort);
		else
			snprintf(buf, buflen, "%s(%s) (%d)[%d]", device->deviceInfo.UDD->name,
			  device->deviceInfo.UDD->SKU, device->deviceInfo.serialNumber,
			  device->deviceInfo.hubPort);
	} else {
		if (label)
			snprintf(buf, buflen, "%s(%s) (%d) [%s]", device->deviceInfo.UDD->name,
			  device->deviceInfo.UDD->SKU, device->deviceInfo.serialNumber, label);
		else
			snprintf(buf, buflen, "%s(%s) (%d)", device->deviceInfo.UDD->name,
			  device->deviceInfo.UDD->SKU, device->deviceInfo.serialNumber);
	}

	return (buf);
}

const char *
channelInfo(PhidgetChannelHandle _channel, char *ubuf, uint32_t buflen) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;
	static char sbuf[128];
	const char *label;
	char *buf;

	buf = ubuf;
	if (buf == NULL) {
		buflen = sizeof(sbuf);
		buf = sbuf;
	}

	channel = PhidgetChannelCast(_channel);
	if (channel == NULL) {
		snprintf(buf, buflen, "<not a phidget>");
		return (buf);
	}

	device = getParent(channel);
	if (!_ISATTACHED(channel) || device == NULL) {
		snprintf(buf, buflen, "%s", Phid_ChannelClassName[channel->class]);
		return (buf);
	}

	label = device->deviceInfo.label;
	if (label && mos_strlen(label) == 0)
		label = NULL;

	if (label)
		snprintf(buf, buflen, "%s (%d/%d/%d) [%s]", channel->UCD->name, device->deviceInfo.serialNumber,
		  device->deviceInfo.hubPort, channel->index, label);
	else
		snprintf(buf, buflen, "%s (%d/%d/%d)", channel->UCD->name, device->deviceInfo.serialNumber,
		  device->deviceInfo.hubPort, channel->index);

	PhidgetRelease(&device);
	return (buf);
}

PhidgetReturnCode
matchUniqueDevice(PhidgetUniqueDeviceType type, int vendorID, int productID, int interfaceNum, int version,
  int *id) {
	const PhidgetUniqueDeviceDef *pdd;
	int i;

	if (type == PHIDTYPE_USB) {
		if (!((vendorID == USBVID_PHIDGETS && productID >= USBPID_PHIDGETS_MIN &&
			productID <= USBPID_PHIDGETS_MAX) || vendorID == USBVID_OLD))
			return (EPHIDGET_UNSUPPORTED);
	}

	for (pdd = Phidget_Unique_Device_Def, i = 0; ((int)pdd->type) != END_OF_LIST; pdd++, i++) {
		if (pdd->type != type)
			continue;

		if (pdd->vendorID != vendorID || pdd->productID != productID || pdd->interfaceNum != interfaceNum)
			continue;

		if (pdd->versionLow > version || pdd->versionHigh <= version)
			continue;

		*id = i;
		return (EPHIDGET_OK);
	}

	return (EPHIDGET_NOENT);
}

PhidgetReturnCode
createPhidgetDevice(PhidgetConnectionType connType, const PhidgetUniqueDeviceDef *pdd, int version,
  const char *label, int serialNumber, PhidgetDeviceHandle *device) {
	PhidgetReturnCode res;

	res = createTypedPhidgetDeviceHandle(device, pdd->class);
	if (res != EPHIDGET_OK)
		return (res);

	(*device)->connType = connType;
	(*device)->deviceInfo.UDD = pdd;
	(*device)->deviceInfo.class = pdd->class;
	(*device)->deviceInfo.version = version;
	(*device)->deviceInfo.serialNumber = serialNumber;
	if (label)
		memcpy((*device)->deviceInfo.label, label, sizeof((*device)->deviceInfo.label));

	(*device)->vintIO = getVINTIO((*device)->deviceInfo.UDD->uid);

	return (EPHIDGET_OK);
}

PhidgetReturnCode
createPhidgetVirtualDevice(const PhidgetUniqueDeviceDef *pdd, int version, const char *label, int serialNumber,
	PhidgetDeviceHandle *device) {
	PhidgetReturnCode res;

	res = createPhidgetDevice(PHIDCONN_VIRTUAL, pdd, version, label, serialNumber, device);
	if (res != EPHIDGET_OK)
		return (res);

	res = PhidgetVirtualConnectionCreate((PhidgetVirtualConnectionHandle *)&(*device)->conn);
	if (res != EPHIDGET_OK)
		return (res);

	return (EPHIDGET_OK);
}

PhidgetReturnCode
createPhidgetVINTDevice(const PhidgetUniqueDeviceDef *pdd, int version, const char *label, int serialNumber,
	PhidgetDeviceHandle *device) {
	PhidgetReturnCode res;

	res = createPhidgetDevice(PHIDCONN_VINT, pdd, version, label, serialNumber, device);
	if (res != EPHIDGET_OK)
		return (res);

	res = PhidgetVINTConnectionCreate((PhidgetVINTConnectionHandle *)&(*device)->conn);
	if (res != EPHIDGET_OK)
		return (res);

	return (EPHIDGET_OK);
}

PhidgetReturnCode
createPhidgetMeshDevice(const PhidgetUniqueDeviceDef *pdd, int version, const char *label, int serialNumber,
	PhidgetDeviceHandle *device) {
	PhidgetReturnCode res;

	res = createPhidgetDevice(PHIDCONN_MESH, pdd, version, label, serialNumber, device);
	if (res != EPHIDGET_OK)
		return (res);

	res = PhidgetMeshConnectionCreate((PhidgetMeshConnectionHandle *)&(*device)->conn);
	if (res != EPHIDGET_OK)
		return (res);

	return (EPHIDGET_OK);
}

PhidgetReturnCode
createPhidgetSPIDevice(const PhidgetUniqueDeviceDef *pdd, int version, const char *label, int serialNumber,
  const char *skuString, PhidgetDeviceHandle *device) {
	PhidgetReturnCode res;

	res = createPhidgetDevice(PHIDCONN_SPI, pdd, version, label, serialNumber, device);
	if (res != EPHIDGET_OK)
		return (res);

	res = PhidgetSPIConnectionCreate((PhidgetSPIConnectionHandle *)&(*device)->conn);
	if (res != EPHIDGET_OK)
		return (res);   /* XXX delete the device? */

	mos_strlcpy((*device)->fwstr, skuString, sizeof((*device)->fwstr));

	return (EPHIDGET_OK);
}

PhidgetReturnCode
createPhidgetUSBDevice(const PhidgetUniqueDeviceDef *pdd, int version, const char *label, int serialNumber,
  const void *devpath, const char *productString, PhidgetDeviceHandle *device) {
	PhidgetReturnCode res;
	PhidgetUSBConnectionHandle conn;

	res = createPhidgetDevice(PHIDCONN_USB, pdd, version, label, serialNumber, device);
	if (res != EPHIDGET_OK)
		return (res);

	res = PhidgetUSBConnectionCreate((PhidgetUSBConnectionHandle *)&(*device)->conn);
	if (res != EPHIDGET_OK)
		return (res);

	conn = PhidgetUSBConnectionCast((*device)->conn);
	assert(conn);

	mos_strlcpy((*device)->fwstr, productString, sizeof((*device)->fwstr));

#ifdef _WINDOWS
	wcsncpy(conn->DevicePath, (const wchar_t *)devpath, 128);
#elif defined(_LINUX) || defined(_FREEBSD) && !defined(_ANDROID)
	strncpy(conn->uniqueName, (const char *)devpath, 20);
#endif
#if defined(_ANDROID)
	mos_strncpy(conn->dev, (const char *)devpath, sizeof(conn->dev));
#endif

	return (EPHIDGET_OK);
}

PhidgetReturnCode
createPhidgetNetDevice(const PhidgetUniqueDeviceDef *pdd, int version, const char *label, int serialNumber,
  const char *fwstr, PhidgetNetConnHandle nc, uint64_t pid, PhidgetDeviceHandle *device) {
	PhidgetNetworkConnectionHandle conn;
	PhidgetReturnCode res;

	res = createPhidgetDevice(PHIDCONN_NETWORK, pdd, version, label, serialNumber, device);
	if (res != EPHIDGET_OK)
		return (res);

	(*device)->__flags |= PHIDGET_NETWORK_FLAG;

	if (fwstr != NULL)
		mos_strlcpy((*device)->fwstr, fwstr, sizeof((*device)->fwstr));

	res = PhidgetNetworkConnectionCreate((PhidgetNetworkConnectionHandle *)&(*device)->conn);
	if (res != EPHIDGET_OK)
		return (res);

	conn = PhidgetNetworkConnectionCast((*device)->conn);
	assert(conn);
	conn->nc = nc;
	PhidgetRetain(nc);
	conn->id = pid;

	return (EPHIDGET_OK);
}

BOOL
isVintChannel(void *_phid) {
	PhidgetChannelHandle channel;

	channel = PhidgetChannelCast(_phid);
	MOS_ASSERT(channel != NULL);

	return (channel->parent->deviceInfo.UDD->class == PHIDCLASS_VINT);
}

BOOL
isNetworkPhidget(void *_phid) {
	PhidgetHandle phid;

	phid = PhidgetCast(_phid);
	MOS_ASSERT(phid != NULL);

	return (PhidgetCKFlags(phid, PHIDGET_NETWORK_FLAG) != 0);
}

uint64_t
PhidgetDeviceGetNetId(PhidgetDeviceHandle device) {
	PhidgetNetworkConnectionHandle nc;

	assert(device);

	nc = PhidgetNetworkConnectionCast(device->conn);
	assert(nc);

	return (nc->id);
}

PhidgetHandle
getPhidgetConnection(void *_phid) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;
	PhidgetHandle conn;

	device = PhidgetDeviceCast(_phid);
	if (device != NULL) {
		if (device->conn) {
			PhidgetRetain(device->conn);
			return (device->conn);
		} else {
			return (NULL);
		}
	}

	channel = PhidgetChannelCast(_phid);
	if (channel != NULL)
		device = getParent(channel);

	if (device != NULL) {
		if (device->conn) {
			conn = device->conn;
			PhidgetRetain(conn);
			PhidgetRelease(&device);
			return (conn);
		}
		PhidgetRelease(&device);
	}
	return (NULL);
}

static PhidgetReturnCode
Phidget_rebootFirmwareUpgrade_internal(mosiop_t iop, PhidgetChannelHandle channel, uint32_t upgradeTimeout) {
	PhidgetDeviceHandle device;
	PhidgetHubDeviceHandle hub;
	PhidgetReturnCode res;
	unsigned char buf[3];

	device = getParent(channel);
	TESTPTR(device);

	if (device->deviceInfo.class == PHIDCLASS_VINT) {
		hub = (PhidgetHubDeviceHandle)getParent(device);
		if (hub != NULL) {
			buf[0] = device->deviceInfo.hubPort;
			buf[1] = upgradeTimeout & 0xFF;
			buf[2] = (upgradeTimeout >> 8) & 0xFF;

			res = sendHubPacket(iop, hub, VINTHUB_HUBPACKET_UPGRADE_FIRMWARE, buf, 3);

			PhidgetRelease(&hub);

			if (res == EPHIDGET_OK) {
				res = sendVINTPacket(iop, channel, VINT_CMD_RESET, (VINTPacketType)0, NULL, 0, NULL);
				if (res != EPHIDGET_OK)
					logwarn("VINT_CMD_RESET failed in Phidget_rebootFirmwareUpgrade: "PRC_FMT, PRC_ARGS(res));
			} else {
				logwarn("VINTHUB_HUBPACKET_UPGRADE_FIRMWARE failed in Phidget_rebootFirmwareUpgrade: "PRC_FMT, PRC_ARGS(res));
			}
		} else {
			res = EPHIDGET_NOTATTACHED;
		}
	} else if (deviceSupportsGeneralPacketProtocol(device)) {
		res = (PhidgetGPP_reboot_firmwareUpgrade(iop, channel));
	} else {
		res = EPHIDGET_UNSUPPORTED;
	}

	PhidgetRelease(&device);

	return (res);
}

static PhidgetReturnCode
Phidget_reboot_internal(mosiop_t iop, PhidgetChannelHandle channel) {
	PhidgetDeviceHandle device;
	PhidgetReturnCode res;

	device = getParent(channel);
	TESTPTR(device);

	if (device->deviceInfo.class == PHIDCLASS_VINT)
		res = sendVINTPacket(iop, channel, VINT_CMD_RESET, (VINTPacketType)0, NULL, 0, NULL);
	else if (deviceSupportsGeneralPacketProtocol(device))
		res = PhidgetGPP_reboot_firmwareUpgrade(iop, channel);
	else
		res = EPHIDGET_UNSUPPORTED;

	PhidgetRelease(&device);

	return (res);
}


static PhidgetReturnCode
Phidget_writeDeviceLabel_internal(mosiop_t iop, PhidgetChannelHandle channel, const char *ubuffer) {
	char buffer2[(MAX_LABEL_SIZE * 2) + 2];
	char buffer[MAX_LABEL_STORAGE];
	PhidgetDeviceHandle device;
	PhidgetReturnCode res;
	int triedUTF8;
	size_t len;

	triedUTF8 = PFALSE;

	mos_strlcpy(buffer, ubuffer, sizeof(buffer));

	// Operate on the parent
	device = getParent(channel);
	assert(device);

	len = (MAX_LABEL_SIZE * 2);
	res = encodeLabelString(buffer, &buffer2[2], &len);
	if (res != EPHIDGET_OK) {
		logerr("Error encoding label string, not setting label.");
		PhidgetRelease(&device);
		return (res);
	}

	buffer2[0] = (char)(len + 2);	// length of descriptor
	buffer2[1] = 3;					// type of descriptor (string)

									// make sure we're not trying to set a label that will match the wrap-around bug when read back
	if (labelHasWrapError(device->deviceInfo.serialNumber, buffer2) == PTRUE) {
		logwarn("Can't set a label that would match the wraparound bug.");
		PhidgetRelease(&device);
		return (EPHIDGET_INVALIDARG);
	}

	res = PhidgetSetLabel(device, buffer2);
	if (res != EPHIDGET_OK) {
		logerr("Unexpected error setting the label. Try again.");
		PhidgetRelease(&device);
		return (res);
	}

refresh:

	// read back the label and compare it
	if ((res = PhidgetRefreshLabelString(device)) != EPHIDGET_OK) {
		logerr("Was unable to read back the label after setting.");
		goto clearlabel;
	}

	// label read back didn't match
	if (strcmp(buffer, device->deviceInfo.label) != 0) {
		/*
		* Label descriptor is longer then 16 bytes and the first 7 bytes back match;
		* almost certainly this is a problem with the wrap around bug.
		*/
		if (buffer2[0] > 16 && !strncmp(buffer, device->deviceInfo.label, 7) && triedUTF8 == PFALSE) {
			//try setting the label as UTF-8 with 0xFFFF header - we can encode up to 12 bytes
			if (strlen(buffer) <= 12) {
				logwarn("Trying to setLabel as UTF-8 because of wrap around bug.");

				//only try this once
				triedUTF8 = PTRUE;

				strcpy(&buffer2[4], buffer);
				buffer2[0] = (char)(strlen(buffer) + 4);
				buffer2[2] = 0xFF;
				buffer2[3] = 0xFF;

				if ((res = PhidgetSetLabel(device, buffer2)) == EPHIDGET_OK) {
					//go check it
					goto refresh;
				} else { //setLabel failed
					logerr("Unexpected error setting the label (UTF-8). Try again.");
					goto clearlabel;
				}
			} else {
				/*
				* Label is too long to be stored, but we have tried to write the label,
				* so we need to clear out anything stored.
				*/
				res = EPHIDGET_INVALIDARG;
				logerr("This device supports 12-bytes UTF-8 labels. "
					"Try again with a shorter string, or pure ASCII.");
				goto clearlabel;
			}
		} else { //label doesn't match and it doesn't look like the wrap around error
			res = EPHIDGET_UNEXPECTED;
			logerr("set label doesn't match read back label: \"%s\" vs. \"%s\"",
				buffer, device->deviceInfo.label);
			goto clearlabel;
		}
	}
	PhidgetRelease(&device);
	return (EPHIDGET_OK);

	/*
	* If a setLabel succeeded, but then we got an error verifying,
	* then we should just clear the label so there's nothing funky in there.
	*/

clearlabel:

	logwarn("Clearing label because of an error during set.");
	memset(buffer2, 0, (MAX_LABEL_SIZE * 2) + 2);
	buffer2[0] = 2;
	buffer2[1] = 3;
	PhidgetSetLabel(device, buffer2);

	PhidgetRelease(&device);
	return (res);
}


PhidgetReturnCode
PhidgetChannel_bridgeInput(PhidgetChannelHandle channel, BridgePacket *bp) {
	PhidgetDeviceHandle device;
	PhidgetReturnCode res;

	assert(channel->bridgeInput);

	switch (bp->vpkt) {
	case BP_ERROREVENT:
		channel->errorHandler(channel, getBridgePacketInt32(bp, 0));
		if (channel->Error)
			channel->Error((PhidgetHandle)channel, channel->ErrorCtx, getBridgePacketInt32(bp, 0), getBridgePacketString(bp, 1));
		return (EPHIDGET_OK);
	case BP_REBOOT:
		if (isNetworkPhidget(channel))
			return (EPHIDGET_OK);
		return (Phidget_reboot_internal(bp->iop, channel));
	case BP_REBOOTFIRMWAREUPGRADE:
		if (isNetworkPhidget(channel))
			return (EPHIDGET_OK);
		return (Phidget_rebootFirmwareUpgrade_internal(bp->iop, channel, getBridgePacketUInt32(bp, 0)));
	case BP_WRITELABEL:
		if (isNetworkPhidget(channel)) {
			device = getParent(channel);
			assert(device);
			mos_strncpy(device->deviceInfo.label, getBridgePacketString(bp, 0), sizeof(device->deviceInfo.label));
			return (EPHIDGET_OK);
		}
		return (Phidget_writeDeviceLabel_internal(bp->iop, channel, getBridgePacketString(bp, 0)));
	}

	if (!supportedBridgePacket(channel, bp->vpkt))
		return (EPHIDGET_UNSUPPORTED);

	res = channel->bridgeInput(channel, bp);

	if (!_HASINITIALSTATE(channel)) {
		if (channel->hasInitialState(channel))
			PhidgetSetFlags(channel, PHIDGET_HASINITIALSTATE_FLAG);
	}

	return (res);
}


/****************
 * Exported API *
 ****************/

API_PRETURN
Phidget_delete(PhidgetHandle *phidp) {
	PhidgetHandle phid;

	if (phidp == NULL || *phidp == NULL)
		return (EPHIDGET_OK);

	phid = PhidgetCast(*phidp);
	if (phid == NULL)
		return (PHID_RETURN(EPHIDGET_INVALID));

	*phidp = NULL;

	if (ISOPEN(phid))
		Phidget_close(phid);

	PhidgetRelease(&phid);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_retain(PhidgetHandle phid) {
	TESTPTR_PR(phid);
	PhidgetRetain(phid);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_release(PhidgetHandle *phid) {
	TESTPTR_PR(phid);
	PhidgetRelease(phid);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_open(PhidgetHandle phid) {
	PhidgetChannelHandle channel;
	PhidgetReturnCode res;
	mosiop_t iop;

	CHANNELNOTDEVICE_PR(channel, phid);

	iop = mos_iop_alloc();
	res = Phidget_open_internal(iop, channel, PTRUE, 0);

	return PHID_RETURN_IOP(res, iop);
}

API_PRETURN
Phidget_openWaitForAttachment(PhidgetHandle phid, uint32_t timeout) {
	PhidgetChannelHandle channel;
	PhidgetReturnCode res;
	mosiop_t iop = NULL;

	CHANNELNOTDEVICE_PR(channel, phid);

	iop = mos_iop_alloc();

	// Associate/Delete IOP with channel locked
	PLOCK(channel);
	channel->iop = iop;
	PUNLOCK(channel);

	res = Phidget_open_internal(iop, (PhidgetChannelHandle)channel, PFALSE, timeout);

	PLOCK(channel);
	channel->iop = NULL;
	PUNLOCK(channel);

	return PHID_RETURN_IOP(res, iop);
}

API_PRETURN
Phidget_close(PhidgetHandle phid) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;
	PhidgetReturnCode res;
	BridgePacket *bp;

	CHANNELNOTDEVICE_PR(channel, phid);

	if (PhidgetCKandCLRFlags(channel, PHIDGET_OPEN_FLAG) != EPHIDGET_OK) {
		// Could be .NET finalizer - always calls close even if already called.
		logverbose("Close was called on an already closed Phidget handle.");
		logStackTrace(PHIDGET_LOG_VERBOSE, "Phidget already closed");
		return (EPHIDGET_OK);
	}

	device = getParent(channel);

	if (device && PhidgetCKFlags(channel, PHIDGET_ATTACHED_FLAG | PHIDGET_DETACHING_FLAG) == PHIDGET_ATTACHED_FLAG) {
		res = createBridgePacket(&bp, BP_CLOSERESET, NULL);
		if (res == EPHIDGET_OK) {
			res = DEVBRIDGEINPUT(channel, bp);
			destroyBridgePacket(&bp);
		}
		if (res != EPHIDGET_OK)
			logerr("Failed to send BP_CLOSERESET to device: "PRC_FMT, PRC_ARGS(res));
	}

	if (isNetworkPhidget(channel))
		closeNetworkChannel(channel);

	if (PhidgetCKFlags(channel, PHIDGET_OPENBYNETCLIENT_FLAG) == 0)
		removeChannel(channel);

	if (device) {
		closeDevice(device, PFALSE);
		setChannel(device, channel->uniqueIndex, NULL);
		PhidgetRelease(&device);
	}

	channelClose(channel);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setOnPropertyChangeHandler(PhidgetHandle phid, Phidget_OnPropertyChangeCallback fptr, void *ctx) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(phid);
	TESTCHANNEL_PR(phid);

	channel = (PhidgetChannelHandle)phid;
	MOS_ASSERT(channel != NULL);

	PhidgetLock(channel);
	channel->PropertyChange = fptr;
	channel->PropertyChangeCtx = ctx;
	PhidgetUnlock(channel);

	return (EPHIDGET_OK);
}


API_PRETURN
Phidget_setOnDetachHandler(PhidgetHandle phid, Phidget_OnDetachCallback fptr, void *ctx) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(phid);
	TESTCHANNEL_PR(phid);

	channel = (PhidgetChannelHandle)phid;
	MOS_ASSERT(channel != NULL);

	PhidgetLock(channel);
	channel->Detach = fptr;
	channel->DetachCtx = ctx;
	PhidgetUnlock(channel);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setOnAttachHandler(PhidgetHandle phid, Phidget_OnAttachCallback fptr, void *ctx) {
	PhidgetChannelHandle channel;
	TESTPTR_PR(phid);
	TESTCHANNEL_PR(phid);

	channel = (PhidgetChannelHandle)phid;
	MOS_ASSERT(channel != NULL);

	PhidgetLock(channel);
	channel->Attach = fptr;
	channel->AttachCtx = ctx;
	PhidgetUnlock(channel);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setOnErrorHandler(PhidgetHandle phid, Phidget_OnErrorCallback fptr, void *ctx) {
	PhidgetChannelHandle channel;
	TESTPTR_PR(phid);
	TESTCHANNEL_PR(phid);

	channel = (PhidgetChannelHandle)phid;
	MOS_ASSERT(channel != NULL);

	PhidgetLock(channel);
	channel->Error = fptr;
	channel->ErrorCtx = ctx;
	PhidgetUnlock(channel);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceName(PhidgetHandle deviceOrChannel, const char **buffer) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(buffer);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	if ((device->deviceInfo.UDD->type == PHIDTYPE_USB || device->deviceInfo.UDD->type == PHIDTYPE_SPI)
	  && device->deviceInfo.class == PHIDCLASS_FIRMWAREUPGRADE) {
		if (!device->firmwareUpgradeName[0])
			snprintf(device->firmwareUpgradeName, 128, "%s %s", device->fwstr, device->deviceInfo.UDD->name);
		*buffer = device->firmwareUpgradeName;
	} else {
		*buffer = device->deviceInfo.UDD->name;
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceSKU(PhidgetHandle deviceOrChannel, const char **buffer) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(buffer);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*buffer = device->deviceInfo.UDD->SKU;

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceSerialNumber(PhidgetHandle deviceOrChannel, int *serialNumber) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(serialNumber);

	GETDEVICE(device, deviceOrChannel);
	channel = PhidgetChannelCast(deviceOrChannel);

	if (ISATTACHEDORDETACHING(deviceOrChannel)) {
		*serialNumber = device->deviceInfo.serialNumber;
	} else if (channel) {
		*serialNumber = channel->openInfo->serialNumber;
	} else {
		PhidgetRelease(&device);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setDeviceSerialNumber(PhidgetHandle phid, int newVal) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(phid);

	channel = (PhidgetChannelHandle)phid;
	TESTPTR_PR(channel->openInfo);

	channel->openInfo->serialNumber = newVal;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceVersion(PhidgetHandle deviceOrChannel, int *devVer) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(devVer);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*devVer = device->deviceInfo.version;

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getAttached(PhidgetHandle deviceOrChannel, int *status) {

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(status);

	*status = ISATTACHEDDONE(deviceOrChannel);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getLibraryVersion(const char **buffer) {

	TESTPTR_PR(buffer);

	*buffer = LibraryVersion;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceClassName(PhidgetHandle deviceOrChannel, const char **buffer) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(buffer);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*buffer = Phid_ClassName[device->deviceInfo.class];

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceID(PhidgetHandle deviceOrChannel, Phidget_DeviceID *deviceID) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(deviceID);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*deviceID = device->deviceInfo.UDD->id;

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceClass(PhidgetHandle deviceOrChannel, Phidget_DeviceClass *class) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(class);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*class = device->deviceInfo.class;

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceChannelCount(PhidgetHandle deviceOrChannel, Phidget_ChannelClass cls, uint32_t *count) {
	const PhidgetUniqueChannelDef *ucd;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(count);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	*count = 0;
	for (ucd = device->deviceInfo.UDD->channels; ((int)ucd->uid) != END_OF_LIST; ucd++) {
		if (cls == ucd->class || cls == PHIDCHCLASS_NOTHING)
			*count += ucd->count;
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getIsRemote(PhidgetHandle deviceOrChannel, int *isRemote) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(isRemote);

	channel = PhidgetChannelCast(deviceOrChannel);
	GETDEVICE(device, deviceOrChannel);

	if (ISATTACHEDORDETACHING(deviceOrChannel)) {
		*isRemote = isNetworkPhidget(device);
	} else if (channel) {
		*isRemote = channel->openInfo->isRemote;
	} else {
		PhidgetRelease(&device);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getIsLocal(PhidgetHandle deviceOrChannel, int *isLocal) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(isLocal);

	channel = PhidgetChannelCast(deviceOrChannel);
	GETDEVICE(device, deviceOrChannel);

	if (ISATTACHEDORDETACHING(deviceOrChannel)) {
		*isLocal = !isNetworkPhidget(device);
	} else if (channel) {
		*isLocal = channel->openInfo->isLocal;
	} else {
		PhidgetRelease(&device);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceFirmwareUpgradeString(PhidgetHandle deviceOrChannel, const char **buffer) {
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(buffer);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);

	switch (device->deviceInfo.UDD->type) {
	case PHIDTYPE_USB:
	case PHIDTYPE_SPI:
		*buffer = (char *)device->fwstr;
		break;
	case PHIDTYPE_VINT:
	case PHIDTYPE_MESH:
	case PHIDTYPE_LIGHTNING:
	case PHIDTYPE_VIRTUAL:
	default:
		*buffer = device->deviceInfo.UDD->SKU;
		break;
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getDeviceLabel(PhidgetHandle deviceOrChannel, const char **buffer) {
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(buffer);

	channel = PhidgetChannelCast(deviceOrChannel);
	GETDEVICE(device, deviceOrChannel);

	if (ISATTACHEDORDETACHING(deviceOrChannel)) {
		*buffer = (const char *)device->deviceInfo.label;
	} else if (deviceOrChannel->type == PHIDGET_CHANNEL) {
		*buffer = (const char *)channel->openInfo->label;
	} else {
		PhidgetRelease(&device);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	PhidgetRelease(&device);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setDeviceLabel(PhidgetHandle phid, const char *label) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(phid);

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);

	if (channel->openInfo->label)
		mos_free(channel->openInfo->label, mos_strlen(channel->openInfo->label) + 1);

	if (label == NULL)
		channel->openInfo->label = NULL;
	else
		channel->openInfo->label = mos_strdup(label, NULL);

	return (EPHIDGET_OK);
}

static const char *
getServerName(PhidgetDeviceHandle device) {
	PhidgetNetworkConnectionHandle netConn;
	const char *ret;

	if (!isNetworkPhidget(device))
		return ("");

	if (!ISATTACHEDORDETACHING(device))
		return ("");

	netConn = PhidgetNetworkConnectionCast(getPhidgetConnection(device));
	MOS_ASSERT(netConn != NULL);
	ret = netConn->nc->rsrvname;
	PhidgetRelease(&netConn);

	return (ret);
}

API_PRETURN
Phidget_getServerName(PhidgetHandle deviceOrChannel, const char **serverName) {
	PhidgetNetworkConnectionHandle netConn;
	PhidgetChannelHandle channel;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(serverName);

	if (ISATTACHEDORDETACHING(deviceOrChannel)) {
		GETDEVICE(device, deviceOrChannel);
		if (!isNetworkPhidget(device)) {
			PhidgetRelease(&device);
			*serverName = (NULL);
			return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
		}
		netConn = PhidgetNetworkConnectionCast(getPhidgetConnection(deviceOrChannel));
		MOS_ASSERT(netConn != NULL);
		mos_strncpy(device->deviceInfo.serverName, netConn->nc->rsrvname, sizeof(device->deviceInfo.serverName));
		*serverName = device->deviceInfo.serverName;
		PhidgetRelease(&netConn);
		PhidgetRelease(&device);
	} else if (deviceOrChannel->type == PHIDGET_CHANNEL) {
		channel = PhidgetChannelCast(deviceOrChannel);
		assert(channel != NULL);
		*serverName = channel->openInfo->serverName;
	} else {
		*serverName = (NULL);
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	}

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getServerUniqueName(PhidgetHandle deviceOrChannel, const char **serverUniqueName) {
	PhidgetNetworkConnectionHandle netConn;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(serverUniqueName);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);
	if (!isNetworkPhidget(device)) {
		PhidgetRelease(&device);
		*serverUniqueName = (NULL);
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	}
	netConn = PhidgetNetworkConnectionCast(getPhidgetConnection(deviceOrChannel));
	MOS_ASSERT(netConn != NULL);
	if (netConn->nc->private)
		mos_strncpy(device->deviceInfo.serverUniqueName, getNetworkControlEntryName(netConn->nc->private), sizeof(device->deviceInfo.serverUniqueName));
	*serverUniqueName = device->deviceInfo.serverUniqueName;
	PhidgetRelease(&netConn);
	PhidgetRelease(&device);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getServerPeerName(PhidgetHandle deviceOrChannel, const char **serverPeerName) {
	PhidgetNetworkConnectionHandle netConn;
	PhidgetDeviceHandle device;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(serverPeerName);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);
	if (!isNetworkPhidget(device)) {
		PhidgetRelease(&device);
		*serverPeerName = (NULL);
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	}
	netConn = PhidgetNetworkConnectionCast(getPhidgetConnection(deviceOrChannel));
	MOS_ASSERT(netConn != NULL);
	mos_strncpy(device->deviceInfo.serverPeerName, netConn->nc->peername, sizeof(device->deviceInfo.serverPeerName));
	*serverPeerName = device->deviceInfo.serverPeerName;
	PhidgetRelease(&netConn);
	PhidgetRelease(&device);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getServerHostname(PhidgetHandle deviceOrChannel, const char **serverHostname) {
	PhidgetNetworkConnectionHandle netConn;
	PhidgetDeviceHandle device;
	const char *tmp;

	TESTPTR_PR(deviceOrChannel);
	TESTPTR_PR(serverHostname);
	TESTATTACHEDORDETACHING_PR(deviceOrChannel);

	GETDEVICE(device, deviceOrChannel);
	if (!isNetworkPhidget(device)) {
		PhidgetRelease(&device);
		*serverHostname = (NULL);
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	}
	netConn = PhidgetNetworkConnectionCast(getPhidgetConnection(deviceOrChannel));
	MOS_ASSERT(netConn != NULL);
	tmp = clientGetHostName(netConn->nc);
	if (tmp != NULL)
		mos_strncpy(device->deviceInfo.serverHostName, tmp, sizeof(device->deviceInfo.serverHostName));
	*serverHostname = device->deviceInfo.serverHostName;
	PhidgetRelease(&netConn);
	PhidgetRelease(&device);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setServerName(PhidgetHandle phid, const char *serverName) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);

	if (channel->openInfo->serverName)
		mos_free(channel->openInfo->serverName, mos_strlen(channel->openInfo->serverName) + 1);

	if (serverName == NULL)
		channel->openInfo->serverName = NULL;
	else
		channel->openInfo->serverName = mos_strdup(serverName, NULL);

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setIsRemote(PhidgetHandle phid, int remote) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);

	if (channel->openInfo->isLocal && remote)
		return (PHID_RETURN_ERRSTR(EPHIDGET_INVALIDARG, "Can't set IsRemote to true because IsLocal is already set to true."));

	channel->openInfo->isRemote = remote;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setIsLocal(PhidgetHandle phid, int local) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);

	if (local && channel->openInfo->isRemote)
		return (PHID_RETURN_ERRSTR(EPHIDGET_INVALIDARG, "Can't set IsLocal to true because IsRemote is already set to true."));

	channel->openInfo->isLocal = local;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_writeDeviceLabel(PhidgetHandle phid, const char *buffer) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(buffer);
	CHANNELNOTDEVICE_PR(channel, phid);
	TESTATTACHED_PR(channel);
	if (!ISOPEN(channel))
		return (PHID_RETURN(EPHIDGET_CLOSED));

	return (bridgeSendToDevice(channel, BP_WRITELABEL, NULL, NULL, "%s", buffer));
}

API_PRETURN
Phidget_getErrorDescription(PhidgetReturnCode errorCode, const char **buf) {

	TESTPTR_PR(buf);

	*buf = Phidget_strerror(errorCode);
	return (EPHIDGET_OK);
}


API_PRETURN
Phidget_getIsChannel(PhidgetHandle phid, int *isChannel) {
	TESTPTR_PR(phid);
	TESTPTR_PR(isChannel);

	*isChannel = (phid->type == PHIDGET_CHANNEL) ? PTRUE : PFALSE;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getChannel(PhidgetHandle phid, int *channelNum) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(channelNum);
	CHANNELNOTDEVICE_PR(channel, phid);

	if (ISATTACHEDORDETACHING(channel))
		*channelNum = channel->index;
	else if (channel->openInfo)
		*channelNum = channel->openInfo->channel;
	else
		return (PHID_RETURN(EPHIDGET_UNEXPECTED));

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_setChannel(PhidgetHandle phid, int newVal) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTPTR_PR(channel->openInfo);
	TESTRANGE_PR(newVal, "%d", PHIDGET_CHANNEL_ANY, PHIDGET_MAXCHANNELS - 1);

	channel->openInfo->channel = newVal;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getChannelClass(PhidgetHandle phid, Phidget_ChannelClass *class) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(class);
	CHANNELNOTDEVICE_PR(channel, phid);

	*class = channel->class;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getChannelSubclass(PhidgetHandle phid, Phidget_ChannelSubclass *channelSubclass) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(channelSubclass);
	CHANNELNOTDEVICE_PR(channel, phid);
	TESTATTACHEDORDETACHING_PR(channel);

	*channelSubclass = channel->UCD->subclass;
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getChannelClassName(PhidgetHandle phid, const char **name) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(name);
	CHANNELNOTDEVICE_PR(channel, phid);

	*name = Phid_ChannelClassName[channel->class];
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_getChannelName(PhidgetHandle phid, const char **name) {
	PhidgetChannelHandle channel;

	TESTPTR_PR(name);
	CHANNELNOTDEVICE_PR(channel, phid);
	TESTATTACHEDORDETACHING_PR(phid);

	*name = channel->UCD->name;
	return (EPHIDGET_OK);
}

/*
 * This is for the control panel to get parents from manager devices.
 * The caller must release the reference to the parent.
 */
API_PRETURN
Phidget_getParent(PhidgetHandle phid, PhidgetHandle *parent) {

	TESTPTR_PR(phid);
	TESTPTR_PR(parent);

	*parent = (PhidgetHandle)getParent(phid);
	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_rebootFirmwareUpgrade(PhidgetHandle phid, uint32_t upgradeTimeout) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTATTACHED_PR(channel);

	return (bridgeSendToDevice(channel, BP_REBOOTFIRMWAREUPGRADE, NULL, NULL, "%u", upgradeTimeout));
}

API_PRETURN
Phidget_reboot(PhidgetHandle phid) {
	PhidgetChannelHandle channel;

	CHANNELNOTDEVICE_PR(channel, phid);
	TESTATTACHED_PR(channel);

	return (bridgeSendToDevice(channel, BP_REBOOT, NULL, NULL, NULL));
}

API_PRETURN
Phidget_getChildDevices(PhidgetHandle phid, PhidgetHandle *arr, size_t *arrCnt) {
	PhidgetDeviceHandle device;
	PhidgetDeviceHandle child;
	size_t cnt, cnt2;

	TESTPTR_PR(arr);
	TESTPTR_PR(arrCnt);

	// Must be NULL or a PhidgetDeviceHandle
	if (phid != NULL) {
		device = PhidgetDeviceCast(phid);
		if (device == NULL)
			return (PHID_RETURN(EPHIDGET_UNEXPECTED));
	} else {
		device = NULL;
	}

	cnt = 0;
	PhidgetReadLockDevices();
	if (phid == NULL) {
		FOREACH_DEVICE(child) {
			if ((cnt + 1) >= *arrCnt)
				break;

			if (child->parent != NULL)
				continue;

			PhidgetRetain(child);
			arr[cnt++] = (PhidgetHandle)child;
		}
	} else {
		cnt2 = 0;
		while (cnt2 < PHIDGET_MAXCHILDREN) {
			if ((cnt2 + 1) >= *arrCnt)
				break;

			// getChild retains
			child = getChild(device, (int)cnt2++);
			if (child == NULL)
				continue;

			arr[cnt++] = (PhidgetHandle)child;
		}
	}
	PhidgetUnlockDevices();

	// NULL-terminated list
	arr[cnt] = NULL;
	*arrCnt = cnt;

	return (EPHIDGET_OK);
}

API_PRETURN
Phidget_releaseDevices(PhidgetHandle *arr, size_t arrCnt) {
	size_t cnt;

	TESTPTR_PR(arr);

	cnt = 0;
	while (cnt < arrCnt) {
		if (arr[cnt] == NULL)
			break; // NULL-terminated
		PhidgetRelease(arr[cnt]);
		cnt++;
	}

	return (EPHIDGET_OK);
}
