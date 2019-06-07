/* Generated: Fri May 24 2019 13:12:12 GMT-0600 (MDT) */

static void CCONV PhidgetPressureSensor_errorHandler(PhidgetChannelHandle ch,
  Phidget_ErrorEventCode code);
static PhidgetReturnCode CCONV PhidgetPressureSensor_bridgeInput(PhidgetChannelHandle phid,
  BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetPressureSensor_setStatus(PhidgetChannelHandle phid,
  BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetPressureSensor_getStatus(PhidgetChannelHandle phid,
  BridgePacket **bp);
static PhidgetReturnCode CCONV PhidgetPressureSensor_initAfterOpen(PhidgetChannelHandle phid);
static PhidgetReturnCode CCONV PhidgetPressureSensor_setDefaults(PhidgetChannelHandle phid);
static void CCONV PhidgetPressureSensor_fireInitialEvents(PhidgetChannelHandle phid);
static int CCONV PhidgetPressureSensor_hasInitialState(PhidgetChannelHandle phid);

struct _PhidgetPressureSensor {
	struct _PhidgetChannel phid;
	uint32_t dataInterval;
	uint32_t minDataInterval;
	uint32_t maxDataInterval;
	double pressure;
	double minPressure;
	double maxPressure;
	double pressureChangeTrigger;
	double minPressureChangeTrigger;
	double maxPressureChangeTrigger;
	PhidgetPressureSensor_OnPressureChangeCallback PressureChange;
	void *PressureChangeCtx;
};

static PhidgetReturnCode CCONV
_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetPressureSensorHandle ch;
	int version;

	ch = (PhidgetPressureSensorHandle)phid;

	version = getBridgePacketUInt32ByName(bp, "_class_version_");
	if (version != 0) {
		logerr("%"PRIphid": bad version %d != 0", phid, version);
		return (EPHIDGET_BADVERSION);
	}

	ch->dataInterval = getBridgePacketUInt32ByName(bp, "dataInterval");
	ch->minDataInterval = getBridgePacketUInt32ByName(bp, "minDataInterval");
	ch->maxDataInterval = getBridgePacketUInt32ByName(bp, "maxDataInterval");
	ch->pressure = getBridgePacketDoubleByName(bp, "pressure");
	ch->minPressure = getBridgePacketDoubleByName(bp, "minPressure");
	ch->maxPressure = getBridgePacketDoubleByName(bp, "maxPressure");
	ch->pressureChangeTrigger = getBridgePacketDoubleByName(bp, "pressureChangeTrigger");
	ch->minPressureChangeTrigger = getBridgePacketDoubleByName(bp, "minPressureChangeTrigger");
	ch->maxPressureChangeTrigger = getBridgePacketDoubleByName(bp, "maxPressureChangeTrigger");

	return (EPHIDGET_OK);
}

static PhidgetReturnCode CCONV
_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	PhidgetPressureSensorHandle ch;

	ch = (PhidgetPressureSensorHandle)phid;

	return (createBridgePacket(bp, 0, "_class_version_=%u"
	  ",dataInterval=%u"
	  ",minDataInterval=%u"
	  ",maxDataInterval=%u"
	  ",pressure=%g"
	  ",minPressure=%g"
	  ",maxPressure=%g"
	  ",pressureChangeTrigger=%g"
	  ",minPressureChangeTrigger=%g"
	  ",maxPressureChangeTrigger=%g"
	  ,0 /* class version */
	  ,ch->dataInterval
	  ,ch->minDataInterval
	  ,ch->maxDataInterval
	  ,ch->pressure
	  ,ch->minPressure
	  ,ch->maxPressure
	  ,ch->pressureChangeTrigger
	  ,ch->minPressureChangeTrigger
	  ,ch->maxPressureChangeTrigger
	));
}

static PhidgetReturnCode CCONV
_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetPressureSensorHandle ch;
	PhidgetReturnCode res;

	ch = (PhidgetPressureSensorHandle)phid;
	res = EPHIDGET_OK;

	switch (bp->vpkt) {
	case BP_SETDATAINTERVAL:
		TESTRANGE_IOP(bp->iop, "%"PRIu32, getBridgePacketUInt32(bp, 0), ch->minDataInterval,
		  ch->maxDataInterval);
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->dataInterval = getBridgePacketUInt32(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "DataInterval");
		break;
	case BP_SETCHANGETRIGGER:
		TESTRANGE_IOP(bp->iop, "%lf", getBridgePacketDouble(bp, 0), ch->minPressureChangeTrigger,
		  ch->maxPressureChangeTrigger);
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->pressureChangeTrigger = getBridgePacketDouble(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "PressureChangeTrigger");
		break;
	case BP_PRESSURECHANGE:
		ch->pressure = getBridgePacketDouble(bp, 0);
		FIRECH(ch, PressureChange, ch->pressure);
		break;
	default:
		logerr("%"PRIphid": unsupported bridge packet:0x%x", phid, bp->vpkt);
		res = EPHIDGET_UNSUPPORTED;
	}

	return (res);
}

static PhidgetReturnCode CCONV
_initAfterOpen(PhidgetChannelHandle phid) {
	PhidgetPressureSensorHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);
	ch = (PhidgetPressureSensorHandle)phid;

	ret = EPHIDGET_OK;


	switch (phid->UCD->uid) {
	case PHIDCHUID_PRE1000_PRESSURESENSOR_100:
		ch->dataInterval = 250;
		ch->maxDataInterval = 60000;
		ch->maxPressure = 110;
		ch->maxPressureChangeTrigger = 60;
		ch->minDataInterval = 100;
		ch->minPressure = 50;
		ch->minPressureChangeTrigger = 0;
		ch->pressure = PUNK_DBL;
		ch->pressureChangeTrigger = 0;
		break;
	default:
		MOS_PANIC("Unsupported Channel");
	}


	return (ret);
}

static PhidgetReturnCode CCONV
_setDefaults(PhidgetChannelHandle phid) {
	PhidgetPressureSensorHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);

	ch = (PhidgetPressureSensorHandle)phid;
	ret = EPHIDGET_OK;

	switch (phid->UCD->uid) {
	case PHIDCHUID_PRE1000_PRESSURESENSOR_100:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->pressureChangeTrigger);
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
	PhidgetPressureSensorHandle ch;

	ch = (PhidgetPressureSensorHandle)phid;

	if(ch->pressure != PUNK_DBL)
		FIRECH(ch, PressureChange, ch->pressure);

}

static int CCONV
_hasInitialState(PhidgetChannelHandle phid) {
	PhidgetPressureSensorHandle ch;

	ch = (PhidgetPressureSensorHandle)phid;

	if(ch->pressure == PUNK_DBL)
		return (PFALSE);

	return (PTRUE);
}

static void CCONV
PhidgetPressureSensor_free(PhidgetChannelHandle *ch) {

	mos_free(*ch, sizeof (struct _PhidgetPressureSensor));
}

API_PRETURN
PhidgetPressureSensor_create(PhidgetPressureSensorHandle *phidp) {

	CHANNELCREATE_BODY(PressureSensor, PHIDCHCLASS_PRESSURESENSOR);
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_delete(PhidgetPressureSensorHandle *phidp) {

	return (Phidget_delete((PhidgetHandle *)phidp));
}

API_PRETURN
PhidgetPressureSensor_setDataInterval(PhidgetPressureSensorHandle ch, uint32_t dataInterval) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETDATAINTERVAL, NULL, NULL, "%u",
	  dataInterval));
}

API_PRETURN
PhidgetPressureSensor_getDataInterval(PhidgetPressureSensorHandle ch, uint32_t *dataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(dataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*dataInterval = ch->dataInterval;
	if (ch->dataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getMinDataInterval(PhidgetPressureSensorHandle ch, uint32_t *minDataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minDataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*minDataInterval = ch->minDataInterval;
	if (ch->minDataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getMaxDataInterval(PhidgetPressureSensorHandle ch, uint32_t *maxDataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxDataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*maxDataInterval = ch->maxDataInterval;
	if (ch->maxDataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getPressure(PhidgetPressureSensorHandle ch, double *pressure) {

	TESTPTR_PR(ch);
	TESTPTR_PR(pressure);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*pressure = ch->pressure;
	if (ch->pressure == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getMinPressure(PhidgetPressureSensorHandle ch, double *minPressure) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minPressure);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*minPressure = ch->minPressure;
	if (ch->minPressure == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getMaxPressure(PhidgetPressureSensorHandle ch, double *maxPressure) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxPressure);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*maxPressure = ch->maxPressure;
	if (ch->maxPressure == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_setPressureChangeTrigger(PhidgetPressureSensorHandle ch,
  double pressureChangeTrigger) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
	  pressureChangeTrigger));
}

API_PRETURN
PhidgetPressureSensor_getPressureChangeTrigger(PhidgetPressureSensorHandle ch,
  double *pressureChangeTrigger) {

	TESTPTR_PR(ch);
	TESTPTR_PR(pressureChangeTrigger);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*pressureChangeTrigger = ch->pressureChangeTrigger;
	if (ch->pressureChangeTrigger == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getMinPressureChangeTrigger(PhidgetPressureSensorHandle ch,
  double *minPressureChangeTrigger) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minPressureChangeTrigger);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*minPressureChangeTrigger = ch->minPressureChangeTrigger;
	if (ch->minPressureChangeTrigger == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_getMaxPressureChangeTrigger(PhidgetPressureSensorHandle ch,
  double *maxPressureChangeTrigger) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxPressureChangeTrigger);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);
	TESTATTACHED_PR(ch);

	*maxPressureChangeTrigger = ch->maxPressureChangeTrigger;
	if (ch->maxPressureChangeTrigger == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetPressureSensor_setOnPressureChangeHandler(PhidgetPressureSensorHandle ch,
  PhidgetPressureSensor_OnPressureChangeCallback fptr, void *ctx) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_PRESSURESENSOR);

	ch->PressureChange = fptr;
	ch->PressureChangeCtx = ctx;

	return (EPHIDGET_OK);
}
