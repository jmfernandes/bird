/* Generated: Wed Jun 05 2019 14:05:37 GMT-0600 (MDT) */

#include "device/spatialdevice.h"
static void CCONV PhidgetSpatial_errorHandler(PhidgetChannelHandle ch, Phidget_ErrorEventCode code);
static PhidgetReturnCode CCONV PhidgetSpatial_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetSpatial_setStatus(PhidgetChannelHandle phid, BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetSpatial_getStatus(PhidgetChannelHandle phid, BridgePacket **bp);
static PhidgetReturnCode CCONV PhidgetSpatial_initAfterOpen(PhidgetChannelHandle phid);
static PhidgetReturnCode CCONV PhidgetSpatial_setDefaults(PhidgetChannelHandle phid);
static void CCONV PhidgetSpatial_fireInitialEvents(PhidgetChannelHandle phid);
static int CCONV PhidgetSpatial_hasInitialState(PhidgetChannelHandle phid);

struct _PhidgetSpatial {
	struct _PhidgetChannel phid;
	Phidget_SpatialAlgorithm algorithm;
	double algorithmMagnetometerGain;
	uint32_t dataInterval;
	uint32_t minDataInterval;
	uint32_t maxDataInterval;
	Phidget_SpatialPrecision precision;
	PhidgetSpatial_OnAlgorithmDataCallback AlgorithmData;
	void *AlgorithmDataCtx;
	PhidgetSpatial_OnSpatialDataCallback SpatialData;
	void *SpatialDataCtx;
};

static PhidgetReturnCode CCONV
_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetSpatialHandle ch;
	int version;

	ch = (PhidgetSpatialHandle)phid;

	version = getBridgePacketUInt32ByName(bp, "_class_version_");
	if (version != 3) {
		loginfo("%"PRIphid": server/client class version mismatch: %d != 3 - functionality may be limited.", phid, version);
	}

	if(version >= 3)
		ch->algorithm = getBridgePacketInt32ByName(bp, "algorithm");
	if(version >= 3)
		ch->algorithmMagnetometerGain = getBridgePacketDoubleByName(bp, "algorithmMagnetometerGain");
	if(version >= 0)
		ch->dataInterval = getBridgePacketUInt32ByName(bp, "dataInterval");
	if(version >= 0)
		ch->minDataInterval = getBridgePacketUInt32ByName(bp, "minDataInterval");
	if(version >= 0)
		ch->maxDataInterval = getBridgePacketUInt32ByName(bp, "maxDataInterval");
	if(version >= 2)
		ch->precision = getBridgePacketInt32ByName(bp, "precision");

	return (EPHIDGET_OK);
}

static PhidgetReturnCode CCONV
_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	PhidgetSpatialHandle ch;

	ch = (PhidgetSpatialHandle)phid;

	return (createBridgePacket(bp, 0, "_class_version_=%u"
	  ",algorithm=%d"
	  ",algorithmMagnetometerGain=%g"
	  ",dataInterval=%u"
	  ",minDataInterval=%u"
	  ",maxDataInterval=%u"
	  ",precision=%d"
	  ,3 /* class version */
	  ,ch->algorithm
	  ,ch->algorithmMagnetometerGain
	  ,ch->dataInterval
	  ,ch->minDataInterval
	  ,ch->maxDataInterval
	  ,ch->precision
	));
}

static PhidgetReturnCode CCONV
_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetSpatialHandle ch;
	PhidgetReturnCode res;

	ch = (PhidgetSpatialHandle)phid;
	res = EPHIDGET_OK;

	switch (bp->vpkt) {
	case BP_SETCORRECTIONPARAMETERS:
		res = DEVBRIDGEINPUT(phid, bp);
		break;
	case BP_RESETCORRECTIONPARAMETERS:
		res = DEVBRIDGEINPUT(phid, bp);
		break;
	case BP_SAVECORRECTIONPARAMETERS:
		res = DEVBRIDGEINPUT(phid, bp);
		break;
	case BP_ZEROSPATIALALGORITHM:
		res = DEVBRIDGEINPUT(phid, bp);
		break;
	case BP_ZERO:
		res = DEVBRIDGEINPUT(phid, bp);
		break;
	case BP_SETSPATIALALGORITHM:
		if (!supportedSpatialAlgorithm(phid, (Phidget_SpatialAlgorithm)getBridgePacketInt32(bp, 0)))
			return (MOS_ERROR(bp->iop, EPHIDGET_INVALIDARG,
			  "Specified SpatialAlgorithm is unsupported by this device."));
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->algorithm = getBridgePacketInt32(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "Algorithm");
		break;
	case BP_SETSPATIALALGORITHMMAGGAIN:
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->algorithmMagnetometerGain = getBridgePacketDouble(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "AlgorithmMagnetometerGain");
		break;
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
	case BP_SETSPATIALPRECISION:
		if (!supportedSpatialPrecision(phid, (Phidget_SpatialPrecision)getBridgePacketInt32(bp, 0)))
			return (MOS_ERROR(bp->iop, EPHIDGET_INVALIDARG,
			  "Specified SpatialPrecision is unsupported by this device."));
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->precision = getBridgePacketInt32(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "Precision");
		break;
	case BP_SPATIALALGDATA:
		FIRECH(ch, AlgorithmData, getBridgePacketDoubleArray(bp, 0), getBridgePacketDouble(bp, 1));
		break;
	case BP_SPATIALDATA:
		FIRECH(ch, SpatialData, getBridgePacketDoubleArray(bp, 0), getBridgePacketDoubleArray(bp, 1),
		  getBridgePacketDoubleArray(bp, 2), getBridgePacketDouble(bp, 3));
		break;
	default:
		logerr("%"PRIphid": unsupported bridge packet:0x%x", phid, bp->vpkt);
		res = EPHIDGET_UNSUPPORTED;
	}

	return (res);
}

static PhidgetReturnCode CCONV
_initAfterOpen(PhidgetChannelHandle phid) {
	PhidgetSpatialHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);
	ch = (PhidgetSpatialHandle)phid;

	ret = EPHIDGET_OK;


	switch (phid->UCD->uid) {
	case PHIDCHUID_1042_SPATIAL_300:
		ch->dataInterval = 256;
		ch->maxDataInterval = 1000;
		ch->minDataInterval = 4;
		ch->precision = SPATIAL_PRECISION_LOW;
		ch->algorithm = SPATIAL_ALGORITHM_NONE;
		break;
	case PHIDCHUID_1044_SPATIAL_400:
		ch->dataInterval = 256;
		ch->maxDataInterval = 1000;
		ch->minDataInterval = 4;
		ch->precision = SPATIAL_PRECISION_HYBRID;
		ch->algorithm = SPATIAL_ALGORITHM_NONE;
		break;
	case PHIDCHUID_1044_SPATIAL_500:
		ch->dataInterval = 256;
		ch->maxDataInterval = 1000;
		ch->minDataInterval = 4;
		ch->precision = SPATIAL_PRECISION_HYBRID;
		ch->algorithm = SPATIAL_ALGORITHM_AHRS;
		ch->algorithmMagnetometerGain = 0.005;
		break;
	case PHIDCHUID_1056_SPATIAL_000:
		ch->dataInterval = 256;
		ch->maxDataInterval = 1000;
		ch->minDataInterval = 4;
		ch->precision = SPATIAL_PRECISION_HIGH;
		ch->algorithm = SPATIAL_ALGORITHM_NONE;
		break;
	case PHIDCHUID_1056_SPATIAL_200:
		ch->dataInterval = 256;
		ch->maxDataInterval = 1000;
		ch->minDataInterval = 4;
		ch->precision = SPATIAL_PRECISION_HIGH;
		ch->algorithm = SPATIAL_ALGORITHM_NONE;
		break;
	case PHIDCHUID_MOT1101_SPATIAL_100:
		ch->dataInterval = 250;
		ch->maxDataInterval = 60000;
		ch->minDataInterval = 20;
		ch->precision = SPATIAL_PRECISION_LOW;
		ch->algorithm = SPATIAL_ALGORITHM_NONE;
		break;
	default:
		MOS_PANIC("Unsupported Channel");
	}


	return (ret);
}

static PhidgetReturnCode CCONV
_setDefaults(PhidgetChannelHandle phid) {
	PhidgetSpatialHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);

	ch = (PhidgetSpatialHandle)phid;
	ret = EPHIDGET_OK;

	switch (phid->UCD->uid) {
	case PHIDCHUID_1042_SPATIAL_300:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1044_SPATIAL_400:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1044_SPATIAL_500:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETSPATIALALGORITHMMAGGAIN, NULL, NULL, "%g",
		  ch->algorithmMagnetometerGain);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1056_SPATIAL_000:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1056_SPATIAL_200:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_MOT1101_SPATIAL_100:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
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
PhidgetSpatial_free(PhidgetChannelHandle *ch) {

	mos_free(*ch, sizeof (struct _PhidgetSpatial));
}

API_PRETURN
PhidgetSpatial_create(PhidgetSpatialHandle *phidp) {

	CHANNELCREATE_BODY(Spatial, PHIDCHCLASS_SPATIAL);
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_delete(PhidgetSpatialHandle *phidp) {

	return (Phidget_delete((PhidgetHandle *)phidp));
}

API_PRETURN
PhidgetSpatial_setMagnetometerCorrectionParameters(PhidgetSpatialHandle ch, double magneticField,
  double offset0, double offset1, double offset2, double gain0, double gain1, double gain2, double T0, double T1, double T2, double T3, double T4, double T5) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETCORRECTIONPARAMETERS, NULL, NULL,
	  "%g%g%g%g%g%g%g%g%g%g%g%g%g", magneticField, offset0, offset1, offset2, gain0, gain1, gain2, T0, T1, T2, T3, T4, T5);
}

API_PRETURN
PhidgetSpatial_resetMagnetometerCorrectionParameters(PhidgetSpatialHandle ch) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return bridgeSendToDevice((PhidgetChannelHandle)ch, BP_RESETCORRECTIONPARAMETERS, NULL, NULL,
	  NULL);
}

API_PRETURN
PhidgetSpatial_saveMagnetometerCorrectionParameters(PhidgetSpatialHandle ch) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SAVECORRECTIONPARAMETERS, NULL, NULL, NULL);
}

API_PRETURN
PhidgetSpatial_zeroAlgorithm(PhidgetSpatialHandle ch) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return bridgeSendToDevice((PhidgetChannelHandle)ch, BP_ZEROSPATIALALGORITHM, NULL, NULL, NULL);
}

API_PRETURN
PhidgetSpatial_zeroGyro(PhidgetSpatialHandle ch) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return bridgeSendToDevice((PhidgetChannelHandle)ch, BP_ZERO, NULL, NULL, NULL);
}

API_PRETURN
PhidgetSpatial_setAlgorithm(PhidgetSpatialHandle ch, Phidget_SpatialAlgorithm algorithm) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETSPATIALALGORITHM, NULL, NULL, "%d",
	  algorithm));
}

API_PRETURN
PhidgetSpatial_getAlgorithm(PhidgetSpatialHandle ch, Phidget_SpatialAlgorithm *algorithm) {

	TESTPTR_PR(ch);
	TESTPTR_PR(algorithm);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	*algorithm = ch->algorithm;
	if (ch->algorithm == (Phidget_SpatialAlgorithm)PUNK_ENUM)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_setAlgorithmMagnetometerGain(PhidgetSpatialHandle ch, double algorithmMagnetometerGain) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETSPATIALALGORITHMMAGGAIN, NULL, NULL, "%g",
	  algorithmMagnetometerGain));
}

API_PRETURN
PhidgetSpatial_getAlgorithmMagnetometerGain(PhidgetSpatialHandle ch,
  double *algorithmMagnetometerGain) {

	TESTPTR_PR(ch);
	TESTPTR_PR(algorithmMagnetometerGain);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	switch (ch->phid.UCD->uid) {
	case PHIDCHUID_1042_SPATIAL_300:
	case PHIDCHUID_1044_SPATIAL_400:
	case PHIDCHUID_1056_SPATIAL_000:
	case PHIDCHUID_1056_SPATIAL_200:
	case PHIDCHUID_MOT1101_SPATIAL_100:
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	default:
		break;
	}

	*algorithmMagnetometerGain = ch->algorithmMagnetometerGain;
	if (ch->algorithmMagnetometerGain == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_setDataInterval(PhidgetSpatialHandle ch, uint32_t dataInterval) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETDATAINTERVAL, NULL, NULL, "%u",
	  dataInterval));
}

API_PRETURN
PhidgetSpatial_getDataInterval(PhidgetSpatialHandle ch, uint32_t *dataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(dataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	*dataInterval = ch->dataInterval;
	if (ch->dataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_getMinDataInterval(PhidgetSpatialHandle ch, uint32_t *minDataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minDataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	*minDataInterval = ch->minDataInterval;
	if (ch->minDataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_getMaxDataInterval(PhidgetSpatialHandle ch, uint32_t *maxDataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxDataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	*maxDataInterval = ch->maxDataInterval;
	if (ch->maxDataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_setPrecision(PhidgetSpatialHandle ch, Phidget_SpatialPrecision precision) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETSPATIALPRECISION, NULL, NULL, "%d",
	  precision));
}

API_PRETURN
PhidgetSpatial_getPrecision(PhidgetSpatialHandle ch, Phidget_SpatialPrecision *precision) {

	TESTPTR_PR(ch);
	TESTPTR_PR(precision);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);
	TESTATTACHED_PR(ch);

	*precision = ch->precision;
	if (ch->precision == (Phidget_SpatialPrecision)PUNK_ENUM)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_setOnAlgorithmDataHandler(PhidgetSpatialHandle ch,
  PhidgetSpatial_OnAlgorithmDataCallback fptr, void *ctx) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);

	ch->AlgorithmData = fptr;
	ch->AlgorithmDataCtx = ctx;

	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetSpatial_setOnSpatialDataHandler(PhidgetSpatialHandle ch, PhidgetSpatial_OnSpatialDataCallback fptr,
  void *ctx) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_SPATIAL);

	ch->SpatialData = fptr;
	ch->SpatialDataCtx = ctx;

	return (EPHIDGET_OK);
}
