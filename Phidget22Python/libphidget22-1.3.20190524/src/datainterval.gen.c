#include "phidgetbase.h"
#include "phidget.h"
#include "mos/mos_os.h"
#include "class/voltageratioinput.gen.h"
#include "class/rcservo.gen.h"
#include "class/accelerometer.gen.h"
#include "class/voltageinput.gen.h"
#include "class/capacitivetouch.gen.h"
#include "class/gyroscope.gen.h"
#include "class/magnetometer.gen.h"
#include "class/spatial.gen.h"
#include "class/temperaturesensor.gen.h"
#include "class/encoder.gen.h"
#include "class/frequencycounter.gen.h"
#include "class/phsensor.gen.h"
#include "class/dcmotor.gen.h"
#include "class/currentinput.gen.h"
#include "class/stepper.gen.h"
#include "class/motorpositioncontroller.gen.h"
#include "class/bldcmotor.gen.h"
#include "class/distancesensor.gen.h"
#include "class/humiditysensor.gen.h"
#include "class/lightsensor.gen.h"
#include "class/pressuresensor.gen.h"
#include "class/soundsensor.gen.h"
#include "class/resistanceinput.gen.h"

API_PRETURN
Phidget_getDataInterval(PhidgetHandle ch, uint32_t *di) {
	Phidget_ChannelClass cclass;
	PhidgetReturnCode res;

	res = Phidget_getChannelClass(ch, &cclass);
	if (res != EPHIDGET_OK)
		return (PHID_RETURN(res));

	switch (cclass) {
	case PHIDCHCLASS_VOLTAGERATIOINPUT:
		return (PhidgetVoltageRatioInput_getDataInterval((PhidgetVoltageRatioInputHandle)ch, di));
	case PHIDCHCLASS_RCSERVO:
		return (PhidgetRCServo_getDataInterval((PhidgetRCServoHandle)ch, di));
	case PHIDCHCLASS_ACCELEROMETER:
		return (PhidgetAccelerometer_getDataInterval((PhidgetAccelerometerHandle)ch, di));
	case PHIDCHCLASS_VOLTAGEINPUT:
		return (PhidgetVoltageInput_getDataInterval((PhidgetVoltageInputHandle)ch, di));
	case PHIDCHCLASS_CAPACITIVETOUCH:
		return (PhidgetCapacitiveTouch_getDataInterval((PhidgetCapacitiveTouchHandle)ch, di));
	case PHIDCHCLASS_GYROSCOPE:
		return (PhidgetGyroscope_getDataInterval((PhidgetGyroscopeHandle)ch, di));
	case PHIDCHCLASS_MAGNETOMETER:
		return (PhidgetMagnetometer_getDataInterval((PhidgetMagnetometerHandle)ch, di));
	case PHIDCHCLASS_SPATIAL:
		return (PhidgetSpatial_getDataInterval((PhidgetSpatialHandle)ch, di));
	case PHIDCHCLASS_TEMPERATURESENSOR:
		return (PhidgetTemperatureSensor_getDataInterval((PhidgetTemperatureSensorHandle)ch, di));
	case PHIDCHCLASS_ENCODER:
		return (PhidgetEncoder_getDataInterval((PhidgetEncoderHandle)ch, di));
	case PHIDCHCLASS_FREQUENCYCOUNTER:
		return (PhidgetFrequencyCounter_getDataInterval((PhidgetFrequencyCounterHandle)ch, di));
	case PHIDCHCLASS_PHSENSOR:
		return (PhidgetPHSensor_getDataInterval((PhidgetPHSensorHandle)ch, di));
	case PHIDCHCLASS_DCMOTOR:
		return (PhidgetDCMotor_getDataInterval((PhidgetDCMotorHandle)ch, di));
	case PHIDCHCLASS_CURRENTINPUT:
		return (PhidgetCurrentInput_getDataInterval((PhidgetCurrentInputHandle)ch, di));
	case PHIDCHCLASS_STEPPER:
		return (PhidgetStepper_getDataInterval((PhidgetStepperHandle)ch, di));
	case PHIDCHCLASS_MOTORPOSITIONCONTROLLER:
		return (PhidgetMotorPositionController_getDataInterval((PhidgetMotorPositionControllerHandle)ch,
		  di));
	case PHIDCHCLASS_BLDCMOTOR:
		return (PhidgetBLDCMotor_getDataInterval((PhidgetBLDCMotorHandle)ch, di));
	case PHIDCHCLASS_DISTANCESENSOR:
		return (PhidgetDistanceSensor_getDataInterval((PhidgetDistanceSensorHandle)ch, di));
	case PHIDCHCLASS_HUMIDITYSENSOR:
		return (PhidgetHumiditySensor_getDataInterval((PhidgetHumiditySensorHandle)ch, di));
	case PHIDCHCLASS_LIGHTSENSOR:
		return (PhidgetLightSensor_getDataInterval((PhidgetLightSensorHandle)ch, di));
	case PHIDCHCLASS_PRESSURESENSOR:
		return (PhidgetPressureSensor_getDataInterval((PhidgetPressureSensorHandle)ch, di));
	case PHIDCHCLASS_SOUNDSENSOR:
		return (PhidgetSoundSensor_getDataInterval((PhidgetSoundSensorHandle)ch, di));
	case PHIDCHCLASS_RESISTANCEINPUT:
		return (PhidgetResistanceInput_getDataInterval((PhidgetResistanceInputHandle)ch, di));
	default:
		return (PHID_RETURN_ERRSTR(EPHIDGET_UNSUPPORTED,
		  "Data interval is not supported by this channel class."));
	}
}

API_PRETURN
Phidget_setDataInterval(PhidgetHandle ch, uint32_t di) {
	Phidget_ChannelClass cclass;
	PhidgetReturnCode res;

	res = Phidget_getChannelClass(ch, &cclass);
	if (res != EPHIDGET_OK)
		return (PHID_RETURN(res));

	switch (cclass) {
	case PHIDCHCLASS_VOLTAGERATIOINPUT:
		return (PhidgetVoltageRatioInput_setDataInterval((PhidgetVoltageRatioInputHandle)ch, di));
	case PHIDCHCLASS_RCSERVO:
		return (PhidgetRCServo_setDataInterval((PhidgetRCServoHandle)ch, di));
	case PHIDCHCLASS_ACCELEROMETER:
		return (PhidgetAccelerometer_setDataInterval((PhidgetAccelerometerHandle)ch, di));
	case PHIDCHCLASS_VOLTAGEINPUT:
		return (PhidgetVoltageInput_setDataInterval((PhidgetVoltageInputHandle)ch, di));
	case PHIDCHCLASS_CAPACITIVETOUCH:
		return (PhidgetCapacitiveTouch_setDataInterval((PhidgetCapacitiveTouchHandle)ch, di));
	case PHIDCHCLASS_GYROSCOPE:
		return (PhidgetGyroscope_setDataInterval((PhidgetGyroscopeHandle)ch, di));
	case PHIDCHCLASS_MAGNETOMETER:
		return (PhidgetMagnetometer_setDataInterval((PhidgetMagnetometerHandle)ch, di));
	case PHIDCHCLASS_SPATIAL:
		return (PhidgetSpatial_setDataInterval((PhidgetSpatialHandle)ch, di));
	case PHIDCHCLASS_TEMPERATURESENSOR:
		return (PhidgetTemperatureSensor_setDataInterval((PhidgetTemperatureSensorHandle)ch, di));
	case PHIDCHCLASS_ENCODER:
		return (PhidgetEncoder_setDataInterval((PhidgetEncoderHandle)ch, di));
	case PHIDCHCLASS_FREQUENCYCOUNTER:
		return (PhidgetFrequencyCounter_setDataInterval((PhidgetFrequencyCounterHandle)ch, di));
	case PHIDCHCLASS_PHSENSOR:
		return (PhidgetPHSensor_setDataInterval((PhidgetPHSensorHandle)ch, di));
	case PHIDCHCLASS_DCMOTOR:
		return (PhidgetDCMotor_setDataInterval((PhidgetDCMotorHandle)ch, di));
	case PHIDCHCLASS_CURRENTINPUT:
		return (PhidgetCurrentInput_setDataInterval((PhidgetCurrentInputHandle)ch, di));
	case PHIDCHCLASS_STEPPER:
		return (PhidgetStepper_setDataInterval((PhidgetStepperHandle)ch, di));
	case PHIDCHCLASS_MOTORPOSITIONCONTROLLER:
		return (PhidgetMotorPositionController_setDataInterval((PhidgetMotorPositionControllerHandle)ch,
		  di));
	case PHIDCHCLASS_BLDCMOTOR:
		return (PhidgetBLDCMotor_setDataInterval((PhidgetBLDCMotorHandle)ch, di));
	case PHIDCHCLASS_DISTANCESENSOR:
		return (PhidgetDistanceSensor_setDataInterval((PhidgetDistanceSensorHandle)ch, di));
	case PHIDCHCLASS_HUMIDITYSENSOR:
		return (PhidgetHumiditySensor_setDataInterval((PhidgetHumiditySensorHandle)ch, di));
	case PHIDCHCLASS_LIGHTSENSOR:
		return (PhidgetLightSensor_setDataInterval((PhidgetLightSensorHandle)ch, di));
	case PHIDCHCLASS_PRESSURESENSOR:
		return (PhidgetPressureSensor_setDataInterval((PhidgetPressureSensorHandle)ch, di));
	case PHIDCHCLASS_SOUNDSENSOR:
		return (PhidgetSoundSensor_setDataInterval((PhidgetSoundSensorHandle)ch, di));
	case PHIDCHCLASS_RESISTANCEINPUT:
		return (PhidgetResistanceInput_setDataInterval((PhidgetResistanceInputHandle)ch, di));
	default:
		return (PHID_RETURN_ERRSTR(EPHIDGET_UNSUPPORTED,
		  "Data interval is not supported by this channel class."));
	}
}
