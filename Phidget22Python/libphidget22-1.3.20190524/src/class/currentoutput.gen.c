/* Generated: Fri May 24 2019 13:12:12 GMT-0600 (MDT) */

static void CCONV PhidgetCurrentOutput_errorHandler(PhidgetChannelHandle ch,
  Phidget_ErrorEventCode code);
static PhidgetReturnCode CCONV PhidgetCurrentOutput_bridgeInput(PhidgetChannelHandle phid,
  BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetCurrentOutput_setStatus(PhidgetChannelHandle phid,
  BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetCurrentOutput_getStatus(PhidgetChannelHandle phid,
  BridgePacket **bp);
static PhidgetReturnCode CCONV PhidgetCurrentOutput_initAfterOpen(PhidgetChannelHandle phid);
static PhidgetReturnCode CCONV PhidgetCurrentOutput_setDefaults(PhidgetChannelHandle phid);
static void CCONV PhidgetCurrentOutput_fireInitialEvents(PhidgetChannelHandle phid);
static int CCONV PhidgetCurrentOutput_hasInitialState(PhidgetChannelHandle phid);

struct _PhidgetCurrentOutput {
	struct _PhidgetChannel phid;
	double current;
	double minCurrent;
	double maxCurrent;
	int enabled;
};

static PhidgetReturnCode CCONV
_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetCurrentOutputHandle ch;
	int version;

	ch = (PhidgetCurrentOutputHandle)phid;

	version = getBridgePacketUInt32ByName(bp, "_class_version_");
	if (version != 0) {
		logerr("%"PRIphid": bad version %d != 0", phid, version);
		return (EPHIDGET_BADVERSION);
	}

	ch->current = getBridgePacketDoubleByName(bp, "current");
	ch->minCurrent = getBridgePacketDoubleByName(bp, "minCurrent");
	ch->maxCurrent = getBridgePacketDoubleByName(bp, "maxCurrent");
	ch->enabled = getBridgePacketInt32ByName(bp, "enabled");

	return (EPHIDGET_OK);
}

static PhidgetReturnCode CCONV
_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	PhidgetCurrentOutputHandle ch;

	ch = (PhidgetCurrentOutputHandle)phid;

	return (createBridgePacket(bp, 0, "_class_version_=%u"
	  ",current=%g"
	  ",minCurrent=%g"
	  ",maxCurrent=%g"
	  ",enabled=%d"
	  ,0 /* class version */
	  ,ch->current
	  ,ch->minCurrent
	  ,ch->maxCurrent
	  ,ch->enabled
	));
}

static PhidgetReturnCode CCONV
_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetCurrentOutputHandle ch;
	PhidgetReturnCode res;

	ch = (PhidgetCurrentOutputHandle)phid;
	res = EPHIDGET_OK;

	switch (bp->vpkt) {
	case BP_SETCURRENT:
		TESTRANGE_IOP(bp->iop, "%lf", getBridgePacketDouble(bp, 0), ch->minCurrent, ch->maxCurrent);
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->current = getBridgePacketDouble(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "Current");
		break;
	case BP_SETENABLED:
		TESTBOOL_IOP(bp->iop, getBridgePacketInt32(bp, 0));
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->enabled = getBridgePacketInt32(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "Enabled");
		break;
	default:
		logerr("%"PRIphid": unsupported bridge packet:0x%x", phid, bp->vpkt);
		res = EPHIDGET_UNSUPPORTED;
	}

	return (res);
}

static PhidgetReturnCode CCONV
_initAfterOpen(PhidgetChannelHandle phid) {
	PhidgetCurrentOutputHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);
	ch = (PhidgetCurrentOutputHandle)phid;

	ret = EPHIDGET_OK;


	switch (phid->UCD->uid) {
	case PHIDCHUID_CURLOOP_CURRENTOUTPUT_100:
		ch->enabled = 0;
		ch->maxCurrent = 20;
		ch->minCurrent = 0;
		ch->current = 0;
		break;
	default:
		MOS_PANIC("Unsupported Channel");
	}


	return (ret);
}

static PhidgetReturnCode CCONV
_setDefaults(PhidgetChannelHandle phid) {
	PhidgetCurrentOutputHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);

	ch = (PhidgetCurrentOutputHandle)phid;
	ret = EPHIDGET_OK;

	switch (phid->UCD->uid) {
	case PHIDCHUID_CURLOOP_CURRENTOUTPUT_100:
		ret = bridgeSendToDevice(phid, BP_SETENABLED, NULL, NULL, "%d", ch->enabled);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCURRENT, NULL, NULL, "%g", ch->current);
		if (ret != EPHIDGET_OK) {
			break;
		}
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
PhidgetCurrentOutput_free(PhidgetChannelHandle *ch) {

	mos_free(*ch, sizeof (struct _PhidgetCurrentOutput));
}

API_PRETURN
PhidgetCurrentOutput_create(PhidgetCurrentOutputHandle *phidp) {

	CHANNELCREATE_BODY(CurrentOutput, PHIDCHCLASS_CURRENTOUTPUT);
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetCurrentOutput_delete(PhidgetCurrentOutputHandle *phidp) {

	return (Phidget_delete((PhidgetHandle *)phidp));
}

API_PRETURN
PhidgetCurrentOutput_setCurrent(PhidgetCurrentOutputHandle ch, double current) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_CURRENTOUTPUT);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETCURRENT, NULL, NULL, "%g", current));
}

API_VRETURN
PhidgetCurrentOutput_setCurrent_async(PhidgetCurrentOutputHandle ch, double current,
  Phidget_AsyncCallback fptr, void *ctx) {
	PhidgetReturnCode res;

	if (ch == NULL) {
		if (fptr) fptr((PhidgetHandle)ch, ctx, EPHIDGET_INVALIDARG);
		return;
	}
	if (ch->phid.class != PHIDCHCLASS_CURRENTOUTPUT) {
		if (fptr) fptr((PhidgetHandle)ch, ctx, EPHIDGET_WRONGDEVICE);
		return;
	}
	if (!ISATTACHED(ch)) {
		if (fptr) fptr((PhidgetHandle)ch, ctx, EPHIDGET_NOTATTACHED);
		return;
	}

	res = bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETCURRENT, fptr, ctx, "%g", current);
	if (res != EPHIDGET_OK && fptr != NULL)
		fptr((PhidgetHandle)ch, ctx, res);
}

API_PRETURN
PhidgetCurrentOutput_getCurrent(PhidgetCurrentOutputHandle ch, double *current) {

	TESTPTR_PR(ch);
	TESTPTR_PR(current);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_CURRENTOUTPUT);
	TESTATTACHED_PR(ch);

	*current = ch->current;
	if (ch->current == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetCurrentOutput_getMinCurrent(PhidgetCurrentOutputHandle ch, double *minCurrent) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minCurrent);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_CURRENTOUTPUT);
	TESTATTACHED_PR(ch);

	*minCurrent = ch->minCurrent;
	if (ch->minCurrent == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetCurrentOutput_getMaxCurrent(PhidgetCurrentOutputHandle ch, double *maxCurrent) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxCurrent);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_CURRENTOUTPUT);
	TESTATTACHED_PR(ch);

	*maxCurrent = ch->maxCurrent;
	if (ch->maxCurrent == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetCurrentOutput_setEnabled(PhidgetCurrentOutputHandle ch, int enabled) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_CURRENTOUTPUT);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETENABLED, NULL, NULL, "%d", enabled));
}

API_PRETURN
PhidgetCurrentOutput_getEnabled(PhidgetCurrentOutputHandle ch, int *enabled) {

	TESTPTR_PR(ch);
	TESTPTR_PR(enabled);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_CURRENTOUTPUT);
	TESTATTACHED_PR(ch);

	*enabled = ch->enabled;
	if (ch->enabled == (int)PUNK_BOOL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}
