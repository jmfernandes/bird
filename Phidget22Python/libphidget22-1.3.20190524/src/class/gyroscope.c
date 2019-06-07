/* Generated: Wed Jan 27 2016 14:22:18 GMT-0700 (Mountain Standard Time) */
/* Will not be regenerated. */

#include "phidgetbase.h"
#include "class/gyroscope.gen.h"
#include "class/gyroscope.gen.c"

static void
PhidgetGyroscope_errorHandler(PhidgetChannelHandle phid, Phidget_ErrorEventCode code) {
	PhidgetGyroscopeHandle ch = (PhidgetGyroscopeHandle)phid;
	switch (code) {
	case EEPHIDGET_SATURATION:
		ch->angularRate[0] = PUNK_DBL;
		ch->angularRate[1] = PUNK_DBL;
		ch->angularRate[2] = PUNK_DBL;
		ch->timestamp = PUNK_DBL;
		break;
	}
}

static PhidgetReturnCode CCONV
PhidgetGyroscope_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	return (_setStatus(phid, bp));
}

static PhidgetReturnCode CCONV
PhidgetGyroscope_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	return (_getStatus(phid, bp));
}

static PhidgetReturnCode CCONV
PhidgetGyroscope_initAfterOpen(PhidgetChannelHandle phid) {
	return (_initAfterOpen(phid));
}

static PhidgetReturnCode CCONV
PhidgetGyroscope_setDefaults(PhidgetChannelHandle phid) {
	return (_setDefaults(phid));
}

static PhidgetReturnCode
PhidgetGyroscope_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	return (_bridgeInput(phid, bp));
}

static void
PhidgetGyroscope_fireInitialEvents(PhidgetChannelHandle phid) {
	_fireInitialEvents(phid);
}

static int
PhidgetGyroscope_hasInitialState(PhidgetChannelHandle phid) {
	return (_hasInitialState(phid));
}
