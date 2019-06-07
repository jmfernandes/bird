/* Generated: Fri May 24 2019 13:12:12 GMT-0600 (MDT) */

#include "device/accelerometerdevice.h"
#include "device/spatialdevice.h"
static void CCONV PhidgetAccelerometer_errorHandler(PhidgetChannelHandle ch,
  Phidget_ErrorEventCode code);
static PhidgetReturnCode CCONV PhidgetAccelerometer_bridgeInput(PhidgetChannelHandle phid,
  BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetAccelerometer_setStatus(PhidgetChannelHandle phid,
  BridgePacket *bp);
static PhidgetReturnCode CCONV PhidgetAccelerometer_getStatus(PhidgetChannelHandle phid,
  BridgePacket **bp);
static PhidgetReturnCode CCONV PhidgetAccelerometer_initAfterOpen(PhidgetChannelHandle phid);
static PhidgetReturnCode CCONV PhidgetAccelerometer_setDefaults(PhidgetChannelHandle phid);
static void CCONV PhidgetAccelerometer_fireInitialEvents(PhidgetChannelHandle phid);
static int CCONV PhidgetAccelerometer_hasInitialState(PhidgetChannelHandle phid);

struct _PhidgetAccelerometer {
	struct _PhidgetChannel phid;
	double acceleration[3];
	double minAcceleration[3];
	double maxAcceleration[3];
	double accelerationChangeTrigger;
	double minAccelerationChangeTrigger;
	double maxAccelerationChangeTrigger;
	int axisCount;
	uint32_t dataInterval;
	uint32_t minDataInterval;
	uint32_t maxDataInterval;
	Phidget_SpatialPrecision precision;
	double timestamp;
	PhidgetAccelerometer_OnAccelerationChangeCallback AccelerationChange;
	void *AccelerationChangeCtx;
};

static PhidgetReturnCode CCONV
_setStatus(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetAccelerometerHandle ch;
	int version;

	ch = (PhidgetAccelerometerHandle)phid;

	version = getBridgePacketUInt32ByName(bp, "_class_version_");
	if (version != 2) {
		logerr("%"PRIphid": bad version %d != 2", phid, version);
		return (EPHIDGET_BADVERSION);
	}

	memcpy(&ch->acceleration, getBridgePacketDoubleArrayByName(bp, "acceleration"),
	  sizeof (double) * 3);
	memcpy(&ch->minAcceleration, getBridgePacketDoubleArrayByName(bp, "minAcceleration"),
	  sizeof (double) * 3);
	memcpy(&ch->maxAcceleration, getBridgePacketDoubleArrayByName(bp, "maxAcceleration"),
	  sizeof (double) * 3);
	ch->accelerationChangeTrigger = getBridgePacketDoubleByName(bp, "accelerationChangeTrigger");
	ch->minAccelerationChangeTrigger = getBridgePacketDoubleByName(bp, "minAccelerationChangeTrigger");
	ch->maxAccelerationChangeTrigger = getBridgePacketDoubleByName(bp, "maxAccelerationChangeTrigger");
	ch->axisCount = getBridgePacketInt32ByName(bp, "axisCount");
	ch->dataInterval = getBridgePacketUInt32ByName(bp, "dataInterval");
	ch->minDataInterval = getBridgePacketUInt32ByName(bp, "minDataInterval");
	ch->maxDataInterval = getBridgePacketUInt32ByName(bp, "maxDataInterval");
	ch->precision = getBridgePacketInt32ByName(bp, "precision");
	ch->timestamp = getBridgePacketDoubleByName(bp, "timestamp");

	return (EPHIDGET_OK);
}

static PhidgetReturnCode CCONV
_getStatus(PhidgetChannelHandle phid, BridgePacket **bp) {
	PhidgetAccelerometerHandle ch;

	ch = (PhidgetAccelerometerHandle)phid;

	return (createBridgePacket(bp, 0, "_class_version_=%u"
	  ",acceleration=%3G"
	  ",minAcceleration=%3G"
	  ",maxAcceleration=%3G"
	  ",accelerationChangeTrigger=%g"
	  ",minAccelerationChangeTrigger=%g"
	  ",maxAccelerationChangeTrigger=%g"
	  ",axisCount=%d"
	  ",dataInterval=%u"
	  ",minDataInterval=%u"
	  ",maxDataInterval=%u"
	  ",precision=%d"
	  ",timestamp=%g"
	  ,2 /* class version */
	  ,ch->acceleration
	  ,ch->minAcceleration
	  ,ch->maxAcceleration
	  ,ch->accelerationChangeTrigger
	  ,ch->minAccelerationChangeTrigger
	  ,ch->maxAccelerationChangeTrigger
	  ,ch->axisCount
	  ,ch->dataInterval
	  ,ch->minDataInterval
	  ,ch->maxDataInterval
	  ,ch->precision
	  ,ch->timestamp
	));
}

static PhidgetReturnCode CCONV
_bridgeInput(PhidgetChannelHandle phid, BridgePacket *bp) {
	PhidgetAccelerometerHandle ch;
	PhidgetReturnCode res;

	ch = (PhidgetAccelerometerHandle)phid;
	res = EPHIDGET_OK;

	switch (bp->vpkt) {
	case BP_SETCHANGETRIGGER:
		TESTRANGE_IOP(bp->iop, "%lf", getBridgePacketDouble(bp, 0), ch->minAccelerationChangeTrigger,
		  ch->maxAccelerationChangeTrigger);
		res = DEVBRIDGEINPUT(phid, bp);
		if (res != EPHIDGET_OK) {
			break;
		}
		ch->accelerationChangeTrigger = getBridgePacketDouble(bp, 0);
		if (bridgePacketIsFromNet(bp))
			FIRE_PROPERTYCHANGE(ch, "AccelerationChangeTrigger");
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
	case BP_ACCELERATIONCHANGE:
		memcpy(&ch->acceleration, getBridgePacketDoubleArray(bp, 0), sizeof (double) * 3);
		ch->timestamp = getBridgePacketDouble(bp, 1);
		FIRECH(ch, AccelerationChange, ch->acceleration, ch->timestamp);
		break;
	default:
		logerr("%"PRIphid": unsupported bridge packet:0x%x", phid, bp->vpkt);
		res = EPHIDGET_UNSUPPORTED;
	}

	return (res);
}

static PhidgetReturnCode CCONV
_initAfterOpen(PhidgetChannelHandle phid) {
	PhidgetAccelerometerDeviceHandle parentAccelerometer;
	PhidgetSpatialDeviceHandle parentSpatial;
	PhidgetAccelerometerHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);
	ch = (PhidgetAccelerometerHandle)phid;

	ret = EPHIDGET_OK;

	parentAccelerometer = (PhidgetAccelerometerDeviceHandle)phid->parent;
	parentSpatial = (PhidgetSpatialDeviceHandle)phid->parent;

	switch (phid->UCD->uid) {
	case PHIDCHUID_1008_ACCELEROMETER_000:
		ch->dataInterval = 273;
		ch->timestamp = parentAccelerometer->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 2;
		ch->maxAcceleration[1] = 2;
		ch->maxAcceleration[2] = 2;
		ch->maxAccelerationChangeTrigger = 4;
		ch->minDataInterval = 39;
		ch->minAcceleration[0] = -2;
		ch->minAcceleration[1] = -2;
		ch->minAcceleration[2] = -2;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentAccelerometer->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentAccelerometer->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentAccelerometer->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 2;
		break;
	case PHIDCHUID_1041_ACCELEROMETER_200:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 1;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_LOW;
		break;
	case PHIDCHUID_1042_ACCELEROMETER_300:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 4;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_LOW;
		break;
	case PHIDCHUID_1043_ACCELEROMETER_300:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 1;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_HYBRID;
		break;
	case PHIDCHUID_1044_ACCELEROMETER_400:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 4;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_HYBRID;
		break;
	case PHIDCHUID_1044_ACCELEROMETER_500:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 4;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_HYBRID;
		break;
	case PHIDCHUID_1049_ACCELEROMETER_000:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 5;
		ch->maxAcceleration[1] = 5;
		ch->maxAcceleration[2] = 5;
		ch->maxAccelerationChangeTrigger = 10;
		ch->minDataInterval = 1;
		ch->minAcceleration[0] = -5;
		ch->minAcceleration[1] = -5;
		ch->minAcceleration[2] = -5;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		break;
	case PHIDCHUID_1053_ACCELEROMETER_300:
		ch->dataInterval = 256;
		ch->timestamp = parentAccelerometer->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 5;
		ch->maxAcceleration[1] = 5;
		ch->maxAcceleration[2] = 5;
		ch->maxAccelerationChangeTrigger = 10;
		ch->minDataInterval = 16;
		ch->minAcceleration[0] = -5;
		ch->minAcceleration[1] = -5;
		ch->minAcceleration[2] = -5;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentAccelerometer->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentAccelerometer->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentAccelerometer->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 2;
		break;
	case PHIDCHUID_1056_ACCELEROMETER_000:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 5;
		ch->maxAcceleration[1] = 5;
		ch->maxAcceleration[2] = 5;
		ch->maxAccelerationChangeTrigger = 10;
		ch->minDataInterval = 4;
		ch->minAcceleration[0] = -5;
		ch->minAcceleration[1] = -5;
		ch->minAcceleration[2] = -5;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_HIGH;
		break;
	case PHIDCHUID_1056_ACCELEROMETER_200:
		ch->dataInterval = 256;
		ch->timestamp = parentSpatial->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 5;
		ch->maxAcceleration[1] = 5;
		ch->maxAcceleration[2] = 5;
		ch->maxAccelerationChangeTrigger = 10;
		ch->minDataInterval = 4;
		ch->minAcceleration[0] = -5;
		ch->minAcceleration[1] = -5;
		ch->minAcceleration[2] = -5;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentSpatial->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentSpatial->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentSpatial->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_HIGH;
		break;
	case PHIDCHUID_1059_ACCELEROMETER_400:
		ch->dataInterval = 256;
		ch->timestamp = parentAccelerometer->timestamp[ch->phid.index];
		ch->maxDataInterval = 1000;
		ch->maxAcceleration[0] = 3;
		ch->maxAcceleration[1] = 3;
		ch->maxAcceleration[2] = 3;
		ch->maxAccelerationChangeTrigger = 6;
		ch->minDataInterval = 16;
		ch->minAcceleration[0] = -3;
		ch->minAcceleration[1] = -3;
		ch->minAcceleration[2] = -3;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = parentAccelerometer->acceleration[ch->phid.index][0];
		ch->acceleration[1] = parentAccelerometer->acceleration[ch->phid.index][1];
		ch->acceleration[2] = parentAccelerometer->acceleration[ch->phid.index][2];
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		break;
	case PHIDCHUID_MOT1100_ACCELEROMETER_100:
		ch->dataInterval = 250;
		ch->maxDataInterval = 60000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 20;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = PUNK_DBL;
		ch->acceleration[1] = PUNK_DBL;
		ch->acceleration[2] = PUNK_DBL;
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		break;
	case PHIDCHUID_MOT1100_ACCELEROMETER_200:
		ch->dataInterval = 250;
		ch->maxDataInterval = 60000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 10;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = PUNK_DBL;
		ch->acceleration[1] = PUNK_DBL;
		ch->acceleration[2] = PUNK_DBL;
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		break;
	case PHIDCHUID_MOT1101_ACCELEROMETER_100:
		ch->dataInterval = 250;
		ch->timestamp = PUNK_DBL;
		ch->maxDataInterval = 60000;
		ch->maxAcceleration[0] = 8;
		ch->maxAcceleration[1] = 8;
		ch->maxAcceleration[2] = 8;
		ch->maxAccelerationChangeTrigger = 16;
		ch->minDataInterval = 20;
		ch->minAcceleration[0] = -8;
		ch->minAcceleration[1] = -8;
		ch->minAcceleration[2] = -8;
		ch->minAccelerationChangeTrigger = 0;
		ch->acceleration[0] = PUNK_DBL;
		ch->acceleration[1] = PUNK_DBL;
		ch->acceleration[2] = PUNK_DBL;
		ch->accelerationChangeTrigger = 0;
		ch->axisCount = 3;
		ch->precision = SPATIAL_PRECISION_LOW;
		break;
	default:
		MOS_PANIC("Unsupported Channel");
	}


	return (ret);
}

static PhidgetReturnCode CCONV
_setDefaults(PhidgetChannelHandle phid) {
	PhidgetAccelerometerHandle ch;
	PhidgetReturnCode ret;

	TESTPTR(phid);

	ch = (PhidgetAccelerometerHandle)phid;
	ret = EPHIDGET_OK;

	switch (phid->UCD->uid) {
	case PHIDCHUID_1008_ACCELEROMETER_000:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1041_ACCELEROMETER_200:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1042_ACCELEROMETER_300:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1043_ACCELEROMETER_300:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1044_ACCELEROMETER_400:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1044_ACCELEROMETER_500:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1049_ACCELEROMETER_000:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1053_ACCELEROMETER_300:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1056_ACCELEROMETER_000:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1056_ACCELEROMETER_200:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_1059_ACCELEROMETER_400:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_MOT1100_ACCELEROMETER_100:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_MOT1100_ACCELEROMETER_200:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
		if (ret != EPHIDGET_OK) {
			break;
		}
		break;
	case PHIDCHUID_MOT1101_ACCELEROMETER_100:
		ret = bridgeSendToDevice(phid, BP_SETDATAINTERVAL, NULL, NULL, "%u", ch->dataInterval);
		if (ret != EPHIDGET_OK) {
			break;
		}
		ret = bridgeSendToDevice(phid, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
		  ch->accelerationChangeTrigger);
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
	PhidgetAccelerometerHandle ch;

	ch = (PhidgetAccelerometerHandle)phid;

	if(ch->acceleration[0] != PUNK_DBL &&
	  ch->acceleration[1] != PUNK_DBL &&
	  ch->acceleration[2] != PUNK_DBL &&
	  ch->timestamp != PUNK_DBL)
		FIRECH(ch, AccelerationChange, ch->acceleration, ch->timestamp);

}

static int CCONV
_hasInitialState(PhidgetChannelHandle phid) {
	PhidgetAccelerometerHandle ch;

	ch = (PhidgetAccelerometerHandle)phid;

	if(ch->acceleration[0] == PUNK_DBL ||
	  ch->acceleration[1] == PUNK_DBL ||
	  ch->acceleration[2] == PUNK_DBL ||
	  ch->timestamp == PUNK_DBL)
		return (PFALSE);

	return (PTRUE);
}

static void CCONV
PhidgetAccelerometer_free(PhidgetChannelHandle *ch) {

	mos_free(*ch, sizeof (struct _PhidgetAccelerometer));
}

API_PRETURN
PhidgetAccelerometer_create(PhidgetAccelerometerHandle *phidp) {

	CHANNELCREATE_BODY(Accelerometer, PHIDCHCLASS_ACCELEROMETER);
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_delete(PhidgetAccelerometerHandle *phidp) {

	return (Phidget_delete((PhidgetHandle *)phidp));
}

API_PRETURN
PhidgetAccelerometer_getAcceleration(PhidgetAccelerometerHandle ch, double (*acceleration)[3]) {

	TESTPTR_PR(ch);
	TESTPTR_PR(acceleration);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	(*acceleration)[0] = ch->acceleration[0];
	if (ch->acceleration[0] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	(*acceleration)[1] = ch->acceleration[1];
	if (ch->acceleration[1] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	(*acceleration)[2] = ch->acceleration[2];
	if (ch->acceleration[2] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getMinAcceleration(PhidgetAccelerometerHandle ch, double (*minAcceleration)[3]) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minAcceleration);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	(*minAcceleration)[0] = ch->minAcceleration[0];
	if (ch->minAcceleration[0] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	(*minAcceleration)[1] = ch->minAcceleration[1];
	if (ch->minAcceleration[1] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	(*minAcceleration)[2] = ch->minAcceleration[2];
	if (ch->minAcceleration[2] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getMaxAcceleration(PhidgetAccelerometerHandle ch, double (*maxAcceleration)[3]) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxAcceleration);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	(*maxAcceleration)[0] = ch->maxAcceleration[0];
	if (ch->maxAcceleration[0] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	(*maxAcceleration)[1] = ch->maxAcceleration[1];
	if (ch->maxAcceleration[1] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	(*maxAcceleration)[2] = ch->maxAcceleration[2];
	if (ch->maxAcceleration[2] == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_setAccelerationChangeTrigger(PhidgetAccelerometerHandle ch,
  double accelerationChangeTrigger) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETCHANGETRIGGER, NULL, NULL, "%g",
	  accelerationChangeTrigger));
}

API_PRETURN
PhidgetAccelerometer_getAccelerationChangeTrigger(PhidgetAccelerometerHandle ch,
  double *accelerationChangeTrigger) {

	TESTPTR_PR(ch);
	TESTPTR_PR(accelerationChangeTrigger);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*accelerationChangeTrigger = ch->accelerationChangeTrigger;
	if (ch->accelerationChangeTrigger == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getMinAccelerationChangeTrigger(PhidgetAccelerometerHandle ch,
  double *minAccelerationChangeTrigger) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minAccelerationChangeTrigger);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*minAccelerationChangeTrigger = ch->minAccelerationChangeTrigger;
	if (ch->minAccelerationChangeTrigger == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getMaxAccelerationChangeTrigger(PhidgetAccelerometerHandle ch,
  double *maxAccelerationChangeTrigger) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxAccelerationChangeTrigger);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*maxAccelerationChangeTrigger = ch->maxAccelerationChangeTrigger;
	if (ch->maxAccelerationChangeTrigger == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getAxisCount(PhidgetAccelerometerHandle ch, int *axisCount) {

	TESTPTR_PR(ch);
	TESTPTR_PR(axisCount);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*axisCount = ch->axisCount;
	if (ch->axisCount == (int)PUNK_INT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_setDataInterval(PhidgetAccelerometerHandle ch, uint32_t dataInterval) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETDATAINTERVAL, NULL, NULL, "%u",
	  dataInterval));
}

API_PRETURN
PhidgetAccelerometer_getDataInterval(PhidgetAccelerometerHandle ch, uint32_t *dataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(dataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*dataInterval = ch->dataInterval;
	if (ch->dataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getMinDataInterval(PhidgetAccelerometerHandle ch, uint32_t *minDataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(minDataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*minDataInterval = ch->minDataInterval;
	if (ch->minDataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getMaxDataInterval(PhidgetAccelerometerHandle ch, uint32_t *maxDataInterval) {

	TESTPTR_PR(ch);
	TESTPTR_PR(maxDataInterval);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	*maxDataInterval = ch->maxDataInterval;
	if (ch->maxDataInterval == (uint32_t)PUNK_UINT32)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_setPrecision(PhidgetAccelerometerHandle ch, Phidget_SpatialPrecision precision) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	return (bridgeSendToDevice((PhidgetChannelHandle)ch, BP_SETSPATIALPRECISION, NULL, NULL, "%d",
	  precision));
}

API_PRETURN
PhidgetAccelerometer_getPrecision(PhidgetAccelerometerHandle ch, Phidget_SpatialPrecision *precision) {

	TESTPTR_PR(ch);
	TESTPTR_PR(precision);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	switch (ch->phid.UCD->uid) {
	case PHIDCHUID_1008_ACCELEROMETER_000:
	case PHIDCHUID_1049_ACCELEROMETER_000:
	case PHIDCHUID_1053_ACCELEROMETER_300:
	case PHIDCHUID_1059_ACCELEROMETER_400:
	case PHIDCHUID_MOT1100_ACCELEROMETER_100:
	case PHIDCHUID_MOT1100_ACCELEROMETER_200:
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	default:
		break;
	}

	*precision = ch->precision;
	if (ch->precision == (Phidget_SpatialPrecision)PUNK_ENUM)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_getTimestamp(PhidgetAccelerometerHandle ch, double *timestamp) {

	TESTPTR_PR(ch);
	TESTPTR_PR(timestamp);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);
	TESTATTACHED_PR(ch);

	switch (ch->phid.UCD->uid) {
	case PHIDCHUID_MOT1100_ACCELEROMETER_100:
	case PHIDCHUID_MOT1100_ACCELEROMETER_200:
		return (PHID_RETURN(EPHIDGET_UNSUPPORTED));
	default:
		break;
	}

	*timestamp = ch->timestamp;
	if (ch->timestamp == (double)PUNK_DBL)
		return (PHID_RETURN(EPHIDGET_UNKNOWNVAL));
	return (EPHIDGET_OK);
}

API_PRETURN
PhidgetAccelerometer_setOnAccelerationChangeHandler(PhidgetAccelerometerHandle ch,
  PhidgetAccelerometer_OnAccelerationChangeCallback fptr, void *ctx) {

	TESTPTR_PR(ch);
	TESTCHANNELCLASS_PR(ch, PHIDCHCLASS_ACCELEROMETER);

	ch->AccelerationChange = fptr;
	ch->AccelerationChangeCtx = ctx;

	return (EPHIDGET_OK);
}
