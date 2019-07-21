/* Generated: Wed Jun 05 2019 14:05:37 GMT-0600 (MDT) */

#include "device/genericdevice.h"
static void CCONV PhidgetGeneric_errorHandler(PhidgetChannelHandle ch, Phidget_ErrorEventCode code);
static PhidgetReturnCode CCONV PhidgetGeneric_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetGeneric_setStatus(PhidgetChannelHandle phid, BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetGeneric_getStatus(PhidgetChannelHandle phid, BridgePacket **bp);
static PhidgetReturnCode CCONV PhidgetGeneric_initAfterOpen(PhidgetChannelHandle phid);
static PhidgetReturnCode CCONV PhidgetGeneric_setDefaults(PhidgetChannelHandle phid);
static void CCONV PhidgetGeneric_fireInitialEvents(PhidgetChannelHandle phid);
static int CCONV PhidgetGeneric_hasInitialState(PhidgetChannelHandle phid);

struct _PhidgetGeneric {
	struct _PhidgetChannel phid;
	uint32_t INPacketLength;
	uint32_t OUTPacketLength;
	PhidgetGeneric_OnPacketCallback Packet;
	void *PacketCtx;
};

static PhidgetReturnCode CCONV
_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetGenericHandle ch;
	int version;

	ch = (PhidgetGenericHandle)phid;

	version = getBridgePacketUInt32ByName(bp, "_class_version_");
	if (version != 0) {
		loginfo("%"PRIphid": server/client class version mismatch: %d != 0 - functionality may be limited.", phid, version);
	}

	if(version >= 0)
		ch->INPacketLength = getBridgePacketUInt32ByName(bp, "INPacketLength");
	if(version >= 0)
		ch->OUTPacketLength = getBridgePacketUInt32ByName(bp, "OUTPacketLength");

	return (EPHIDGET_OK);
}

static PhidgetReturnCode CCONV
_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	PhidgetGenericHandle ch;

	ch = (PhidgetGenericHandle)phid;

	return (createBridgePacket(bp, 0, "_class_version_=%u"
	  ",INPacketLength=%u"
	  ",OUTPacketLength=%u"
	  ,0 /* class version */
	  ,ch->INPacketLength
	  ,ch->OUTPacketLength
	));
}

static PhidgetReturnCode CCONV
_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetReturnCode res;

	res = EPHIDGET_OK;

	switch (bp->vpkt) {
	case BP_SENDPACKET:
		res = DEVBRIDGEINPUT(phid, bp);
		break;
	default:
		logerr("%"PRIphid": unsupported bridge packet:0x%x", phid, bp->vpkt);
		res = EPHIDGET_UNSUPPORTED;
	}

	return (res);
}

static PhidgetReturnCode CCONV
_initAfterOpen(PhidgetChannelHandle phid) {
	PhidgetGenericDeviceHandle parentGeneric;
	PhidgetGenericHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);
	ch = (PhidgetGenericHandle)phid;

	ret = EPHIDGET_OK;

	parentGeneric = (PhidgetGenericDeviceHandle)phid->parent;

	switch (phid->UCD->uid) {
	case PHIDCHUID_USB_GENERIC:
		ch->INPacketLength = parentGeneric->INPacketLength[ch->phid.index];
		ch->OUTPacketLength = parentGeneric->OUTPacketLength[ch->phid.index];
		break;
	case PHIDCHUID_VINT_GENERIC:
		break;
	case PHIDCHUID_USB_UNKNOWN:
		ch->INPacketLength = parentGeneric->INPacketLength[ch->phid.index];
		ch->OUTPacketLength = parentGeneric->OUTPacketLength[ch->phid.index];
		break;
	case PHIDCHUID_VINT_UNKNOWN:
		break;
	default:
		MOS_PANIC("Unsupported Channel");
	}


	return (ret);
}

static PhidgetReturnCode CCONV
_setDefaults(PhidgetChannelHandle phid) {
	PhidgetReturnCode ret;

	TESTPTR(phid);

	ret = EPHIDGET_OK;

	switch (phid->UCD->uid) {
	case PHIDCHUID_USB_GENERIC:
		break;
	case PHIDCHUID_VINT_GENERIC:
		break;
	case PHIDCHUID_USB_UNKNOWN:
		break;
	case PHIDCHUID_VINT_UNKNOWN:
		break;
	default:
		MOS_PANIC("Unsupported Channel");
	}

	return (ret);
}

static void CCONV
_fireInitialEvents(PhidgetChannelHandle phid) {

}

static int CCONV
_hasInitialState(PhidgetChannelHandle phid) {

	return (PTRUE);
}

static void CCONV
PhidgetGeneric_free(PhidgetChannelHandle *ch) {

	mos_free(*ch, sizeof (struct _PhidgetGeneric));
}

API_PRETURN
PhidgetGeneric_create(PhidgetGenericHandle *phidp) {

	CHANNELCREATE_BODY(Generic, PHIDCHCLASS_GENERIC);
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetGeneric_delete(PhidgetGenericHandle *phidp) {

	return (Phidget_delete((PhidgetHandle *)phidp));
}

API_PRETURN
PhidgetGeneric_sendPacket(PhidgetGenericHandle ch, const uint8_t *packet, size_t packetLen) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_GENERIC);
	TESTATTACHED_PR(ch);

	return bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SENDPACKET, NULL, NULL, "%*R", packetLen,
	  packet);
}

API_VRETURN
PhidgetGeneric_sendPacket_async(PhidgetGenericHandle ch, const uint8_t *packet, size_t packetLen,
  Phidget_AsyncCallback fptr, void *ctx) {
	PhidgetReturnCode res;

	if (ch == NULL) {
		if (fptr) fptr((PhidgetHandle)ch, ctx, EPHIDGET_INVALIDARG);
		return;
	}
	if (ch->phid.class != PHIDCHCLASS_GENERIC) {
		if (fptr) fptr((PhidgetHandle)ch, ctx, EPHIDGET_WRONGDEVICE);
		return;
	}
	if (!ISATTACHED(ch)) {
		if (fptr) fptr((PhidgetHandle)ch, ctx, EPHIDGET_NOTATTACHED);
		return;
	}

	res = bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SENDPACKET, fptr, ctx, "%*R", packetLen,
	  packet);

	if (res != EPHIDGET_OK && fptr != NULL)
		fptr((PhidgetHandle)ch, ctx, res);
}

API_PRETURN
PhidgetGeneric_getINPacketLength(PhidgetGenericHandle ch, uint32_t *INPacketLength) {

	TESTPTR_PR(ch);
	TESTPTR_PR(INPacketLength);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_GENERIC);
	TESTATTACHED_PR(ch);

	switch (ch->phid.UCD->uid) {
	case PHIDCHUID_VINT_GENERIC:
	case PHIDCHUID_VINT_UNKNOWN:
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	default:
		break;
	}

	*INPacketLength = ch->INPacketLength;
	if (ch->INPacketLength == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetGeneric_getOUTPacketLength(PhidgetGenericHandle ch, uint32_t *OUTPacketLength) {

	TESTPTR_PR(ch);
	TESTPTR_PR(OUTPacketLength);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_GENERIC);
	TESTATTACHED_PR(ch);

	switch (ch->phid.UCD->uid) {
	case PHIDCHUID_VINT_GENERIC:
	case PHIDCHUID_VINT_UNKNOWN:
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	default:
		break;
	}

	*OUTPacketLength = ch->OUTPacketLength;
	if (ch->OUTPacketLength == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetGeneric_setOnPacketHandler(PhidgetGenericHandle ch, PhidgetGeneric_OnPacketCallback fptr,
  void *ctx) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_GENERIC);

	ch->Packet = fptr;
	ch->PacketCtx = ctx;

	return (EPHIDGET_OK);
}
