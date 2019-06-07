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
#include "util/packettracker.h"
#include "mos/mos_time.h"

PhidgetPacketTrackersHandle
mallocPhidgetPacketTrackers(void) {
	PhidgetPacketTrackersHandle item;
	int i;

	item = (PhidgetPacketTrackersHandle)mos_zalloc(sizeof (PhidgetPacketTrackers));

	for (i = 0; i < MAX_PACKET_IDS; i++) {
		mos_cond_init(&item->packetTracker[i]._cond);
		mos_mutex_init(&item->packetTracker[i]._lock);
	}

	return (item);
}

void
freePhidgetPacketTrackers(PhidgetPacketTrackersHandle item) {
	int i;

	assert(item != NULL);

	for (i = 0; i < MAX_PACKET_IDS; i++) {
		mos_cond_destroy(&item->packetTracker[i]._cond);
		mos_mutex_destroy(&item->packetTracker[i]._lock);
	}
	mos_free(item, sizeof (PhidgetPacketTrackers));
}

void
setPacketLength(PhidgetPacketTrackerHandle packetTracker, size_t len) {

	mos_mutex_lock(&packetTracker->_lock);
	packetTracker->len = len;
	mos_mutex_unlock(&packetTracker->_lock);
}

PhidgetReturnCode
setPacketReturnCode(PhidgetPacketTrackerHandle packetTracker, PhidgetReturnCode res) {

	mos_mutex_lock(&packetTracker->_lock);
	if ((packetTracker->_state & PACKETTRACKER_INUSE) == 0 || packetTracker->_state & PACKETTRACKER_SIGNALLED) {
		mos_mutex_unlock(&packetTracker->_lock);
		return (EPHIDGET_INVALID);
	}

	packetTracker->_returnCode = res;
	packetTracker->_state |= PACKETTRACKER_SIGNALLED;
	mos_cond_broadcast(&packetTracker->_cond);
	mos_mutex_unlock(&packetTracker->_lock);

	return (EPHIDGET_OK);
}

void
_setPacketsReturnCode(PhidgetDeviceHandle device, int child, PhidgetReturnCode res) {
	PhidgetPacketTrackerHandle packetTracker;
	int i;

	for (i = 0; i < MAX_PACKET_IDS; i++) {
		packetTracker = &device->packetTracking->packetTracker[i];

		if (!(packetTracker->_state & PACKETTRACKER_INUSE))
			continue;
		if (packetTracker->childIndex != child)
			continue;

		mos_mutex_lock(&packetTracker->_lock);
		packetTracker->_returnCode = res;
		packetTracker->_state |= PACKETTRACKER_SIGNALLED;
		mos_cond_broadcast(&packetTracker->_cond);
		mos_mutex_unlock(&packetTracker->_lock);
	}
}

void
setPacketsReturnCode(PhidgetDeviceHandle device, int child, PhidgetReturnCode res) {
	PhidgetLock(device);
	_setPacketsReturnCode(device, child, res);
	PhidgetUnlock(device);
}

PhidgetReturnCode
getPacketReturnCode(PhidgetPacketTrackerHandle packetTracker, PhidgetReturnCode *res) {

	mos_mutex_lock(&packetTracker->_lock);
	if ((packetTracker->_state & PACKETTRACKER_SIGNALLED) == 0) {
		mos_mutex_unlock(&packetTracker->_lock);
		return (EPHIDGET_INVALID);
	}
	*res = packetTracker->_returnCode;
	mos_mutex_unlock(&packetTracker->_lock);
	return (EPHIDGET_OK);
}

PhidgetReturnCode
waitForPendingPacket(PhidgetPacketTrackerHandle packetTracker, uint32_t ms) {
	PhidgetReturnCode res;
	mostime_t now;
	mostime_t tm;

	assert(packetTracker != NULL);
	assert(packetTracker->_state & PACKETTRACKER_INUSE);

	tm = mos_gettime_usec() + (ms * 1000);

	mos_mutex_lock(&packetTracker->_lock);
	if (packetTracker->_state & PACKETTRACKER_SIGNALLED) {
		mos_mutex_unlock(&packetTracker->_lock);
		return (EPHIDGET_OK);
	}

	res = EPHIDGET_OK;
	while ((packetTracker->_state & PACKETTRACKER_SIGNALLED) == 0) {
		now = mos_gettime_usec();
		if (now > tm) {
			if (ms > 0)
				logdebug("Packet tracker waitForPendingPacket timeout (%dms).", ms);
			res = EPHIDGET_TIMEOUT;
			break;
		}

		mos_cond_timedwait(&packetTracker->_cond, &packetTracker->_lock, ms * 1000000);
	}

	mos_mutex_unlock(&packetTracker->_lock);
	return (res);
}

void
waitForPendingPackets(PhidgetDeviceHandle device, int child) {
	PhidgetPacketTrackerHandle packetTracker;
	int stillSomeLeft;
	mostime_t tm;
	int i;

	tm = mos_gettime_usec() + (10 * 1000000);

	do {
		stillSomeLeft = 0;
		for (i = 0; i < MAX_PACKET_IDS; i++) {
			packetTracker = &device->packetTracking->packetTracker[i];
			if (packetTracker->childIndex != child)
				continue;

			if (!(packetTracker->_state & PACKETTRACKER_INUSE))
				continue;

			if (!(packetTracker->_state & PACKETTRACKER_SIGNALLED))
				continue;

			stillSomeLeft++;
		}
		if (stillSomeLeft) {
			if (mos_gettime_usec() > tm)
				break;
			mos_usleep(10000);
		}
	} while (stillSomeLeft);
}

void
waitForAllPendingPackets(PhidgetDeviceHandle device) {
	PhidgetPacketTrackerHandle packetTracker;
	int stillSomeLeft;
	mostime_t tm;
	int i;

	tm = mos_gettime_usec() + (10 * 1000000);

	do {
		stillSomeLeft = 0;
		PhidgetLock(device);
		for (i = 0; i < MAX_PACKET_IDS; i++) {
			packetTracker = &device->packetTracking->packetTracker[i];

			if (!(packetTracker->_state & PACKETTRACKER_INUSE))
				continue;

			if (!(packetTracker->_state & PACKETTRACKER_SIGNALLED))
				continue;

			stillSomeLeft++;
		}
		PhidgetUnlock(device);
		if (stillSomeLeft) {
			if (mos_gettime_usec() > tm)
				break;
			mos_usleep(10000);
		}
	} while (stillSomeLeft);
}

PhidgetReturnCode
getPacketTrackerWait(PhidgetDeviceHandle device, int *packetID, PhidgetPacketTrackerHandle *packetTracker,
	int min, int max, int childIndex, uint32_t timeout_ms) {
	PhidgetReturnCode ret;
	mostime_t tm;

	tm = mos_gettime_usec() + (timeout_ms * 1000);

	while (1) {

		ret = getPacketTracker(device, packetID, packetTracker, min, max, childIndex);

		if (ret == EPHIDGET_OK)
			break;

		if (ret != EPHIDGET_NOENT)
			break;

		if (mos_gettime_usec() > tm) {
			ret = EPHIDGET_TIMEOUT;
			break;
		}

		mos_usleep(10000);
	}

	//loginfo("getPacketTrackerWait returning %d after %d ms", ret, (mos_gettime_usec() - tm + (timeout_ms * 1000)) / 1000);

	return (ret);
}

PhidgetReturnCode
getPacketTracker(PhidgetDeviceHandle device, int *packetID, PhidgetPacketTrackerHandle *packetTracker,
  int min, int max, int childIndex) {
	int i;

	assert(device != NULL);
	assert(device->packetTracking != NULL);
	assert(min >= 0);
	assert(max < MAX_PACKET_IDS);

	PhidgetLock(device);

	for (i = min; i <= max; i++) {
		if (device->packetTracking->packetTracker[i]._state & PACKETTRACKER_INUSE)
			continue;

		device->packetTracking->packetTracker[i]._state |= PACKETTRACKER_INUSE;
		device->packetTracking->packetTracker[i]._returnCode = EPHIDGET_UNKNOWNVAL;
		device->packetTracking->packetTracker[i].len = 0;
		device->packetTracking->packetTracker[i].childIndex = childIndex;
		*packetID = i;
		*packetTracker = &device->packetTracking->packetTracker[i];

		PhidgetUnlock(device);
		return (EPHIDGET_OK);
	}

	// All trackers in use - wait

	PhidgetUnlock(device);
	return (EPHIDGET_NOENT);
}

void
releasePacketTracker(PhidgetDeviceHandle device, PhidgetPacketTrackerHandle packetTracker) {

	assert(device != NULL);
	assert(device->packetTracking != NULL);

	PhidgetLock(device);
	packetTracker->_state = 0;
	PhidgetUnlock(device);
}