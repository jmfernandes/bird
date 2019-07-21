/* Generated: Wed Jan 27 2016 14:22:18 GMT-0700 (Mountain Standard Time) */
/* Will not be regenerated. */

#include "phidgetbase.h"
#include "class/rfid.gen.h"
#include "class/rfid.gen.c"

static void
PhidgetRFID_errorHandler(PhidgetChannelHandle phid, Phidget_ErrorEventCode code) {}

static PhidgetReturnCode CCONV
PhidgetRFID_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	return (_setStatus(phid, bp));
}

static PhidgetReturnCode CCONV
PhidgetRFID_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	return (_getStatus(phid, bp));
}

static PhidgetReturnCode CCONV
PhidgetRFID_initAfterOpen(PhidgetChannelHandle phid) {
	return (_initAfterOpen(phid));
}

static PhidgetReturnCode CCONV
PhidgetRFID_setDefaults(PhidgetChannelHandle phid) {
	return (_setDefaults(phid));
}

static PhidgetReturnCode
PhidgetRFID_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetRFIDHandle ch;
	PhidgetReturnCode res;

	ch = (PhidgetRFIDHandle)phid;
	res = EPHIDGET_OK;

	switch (bp->vpkt) {
	case BP_WRITE:
		if (!supportedRFIDProtocol(phid, (PhidgetRFID_Protocol)getBridgePacketInt32(bp, 1)))
			return (MOS_ERROR(bp->iop, EPHIDGET_INVALIDARG, "Specified RFID protocol is not supported by this device."));
		TESTBOOL_IOP(bp->iop, getBridgePacketInt32(bp, 2)); //lockTag
		res = _bridgeInput(phid, bp);
		break;
	case BP_TAG:
		ch->tagPresent = PTRUE;
		FIRECH(ch, Tag, getBridgePacketString(bp, 0), (PhidgetRFID_Protocol)getBridgePacketInt32(bp, 1));
		PhidgetRunLock(ch);
		mos_strlcpy(ch->lastTagString, getBridgePacketString(bp, 0), RFIDDevice_MAX_TAG_STRING_LEN);
		ch->lastTagProtocol = (PhidgetRFID_Protocol)getBridgePacketInt32(bp, 1);
		PhidgetRunUnlock(ch);
		break;
	case BP_TAGLOST:
		ch->tagPresent = PFALSE;
		FIRECH(ch, TagLost, getBridgePacketString(bp, 0), (PhidgetRFID_Protocol)getBridgePacketInt32(bp, 1));
		break;
	case BP_SETANTENNAON:
		res = _bridgeInput(phid, bp);
		if (res == EPHIDGET_OK) {
			if (ch->tagPresent == PUNK_BOOL)
				ch->tagPresent = PFALSE;
		}
		break;
	default:
		res = _bridgeInput(phid, bp);
		break;
	}

	return (res);
}

static void
PhidgetRFID_fireInitialEvents(PhidgetChannelHandle phid) {
	_fireInitialEvents(phid);
}

static int
PhidgetRFID_hasInitialState(PhidgetChannelHandle phid) {
	return (_hasInitialState(phid));
}

API_PRETURN
PhidgetRFID_getLastTag(PhidgetRFIDHandle ch, char *tagString, size_t tagStringLen, PhidgetRFID_Protocol *protocol) {
	size_t len;

	TESTPTR_PR(ch);
	TESTPTR_PR(tagString);
	TESTPTR_PR(protocol);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_RFID);
	TESTATTACHED_PR(ch);

	PhidgetRunLock(ch);
	len = mos_strlen(ch->lastTagString);
	if (len > 0) {
		if (tagStringLen <= len)
			return (PHID_RETURN_ERRSTR(EPHIDGET_NOSPC, "tagString too short (%d / %d).", tagStringLen, len));
		mos_strlcpy(tagString, ch->lastTagString, tagStringLen);
		*protocol = ch->lastTagProtocol;
		PhidgetRunUnlock(ch);
		return (EPHIDGET_OK);
	}
	PhidgetRunUnlock(ch);
	return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
}
