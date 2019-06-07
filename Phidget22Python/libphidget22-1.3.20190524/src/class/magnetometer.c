/* Generated: Wed Jan 27 2016 14:22:18 GMT-0700 (Mountain Standard Time) */
/* Will not be regenerated. */

#include "phidgetbase.h"
#include "class/magnetometer.gen.h"
#include "class/magnetometer.gen.c"

static void
PhidgetMagnetometer_errorHandler(PhidgetChannelHandle phid, Phidget_ErrorEventCode code) {
	PhidgetMagnetometerHandle ch = (PhidgetMagnetometerHandle)phid;
	switch (code) {
	case EEPHIDGET_SATURATION:
		ch->magneticField[0] = PUNK_DBL;
		ch->magneticField[1] = PUNK_DBL;
		ch->magneticField[2] = PUNK_DBL;
		ch->timestamp = PUNK_DBL;
		break;
	}
}

static PhidgetReturnCode CCONV
PhidgetMagnetometer_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	return (_setStatus(phid, bp));
}

static PhidgetReturnCode CCONV
PhidgetMagnetometer_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	return (_getStatus(phid, bp));
}

static PhidgetReturnCode CCONV
PhidgetMagnetometer_initAfterOpen(PhidgetChannelHandle phid) {
	return (_initAfterOpen(phid));
}

static PhidgetReturnCode CCONV
PhidgetMagnetometer_setDefaults(PhidgetChannelHandle phid) {
	return (_setDefaults(phid));
}

static PhidgetReturnCode
PhidgetMagnetometer_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	return (_bridgeInput(phid, bp));
}

static void
PhidgetMagnetometer_fireInitialEvents(PhidgetChannelHandle phid) {
	_fireInitialEvents(phid);
}

static int
PhidgetMagnetometer_hasInitialState(PhidgetChannelHandle phid) {
	return (_hasInitialState(phid));
}
