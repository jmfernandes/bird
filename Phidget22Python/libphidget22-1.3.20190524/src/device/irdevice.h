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

#ifndef __CPHIDGETIRDEVICE
#define __CPHIDGETIRDEVICE

typedef struct _PhidgetIRDevice *PhidgetIRDeviceHandle;
PhidgetReturnCode PhidgetIRDevice_create(PhidgetIRDeviceHandle *phid);

#define IR_MAX_REPEAT_LENGTH		26											/* Maximum array size for a repeat code */

#define IR_DATA_ARRAY_SIZE		2048
#define IR_MAX_CODE_DATA_LENGTH		(IR_MAX_CODE_BIT_COUNT / 8)		/* Maximum array size needed to hold the longest code */

#define IR_DATASIZE(bitCount) ((bitCount / 8) + ((bitCount % 8) ? 1 : 0))

#define IR_MAX_DATA_PER_PACKET	31

#define IR_DATA_ARRAY_MASK		0x7ff

//for transmitting / receiving raw data
#define IR_MAX_DATA_us			327670

//this is just actual gap, not the gap that includes data
#define IR_MAX_GAP_LENGTH		100000 //us
#define IR_MIN_GAP_LENGTH		20000 //us

#define IR_DEFINEDATA_PACKET	0

#define IR_STOP_RX_WHILE_TX_FLAG	0x01

#define IR_RAW_DATA_WS_KEYS_MAX		100

#include "mos/mos_time.h"

struct _PhidgetIRDevice {
#undef devChannelCnts
#define devChannelCnts	phid.deviceInfo.UDD->channelCnts.ir
	PhidgetDevice phid;

	uint32_t dataBuffer[IR_DATA_ARRAY_SIZE];
	uint32_t dataBufferNormalized[IR_DATA_ARRAY_SIZE];
	unsigned dataReadPtr, dataWritePtr;
	unsigned learnReadPtr; //for the learning function

	uint8_t polarity;

	uint8_t lastSentCode[IR_MAX_CODE_DATA_LENGTH];
	PhidgetIR_CodeInfo lastSentCodeInfo;

	uint32_t lastGap;

	uint8_t lastCodeKnown;
	PhidgetIR_CodeInfo lastCodeInfo;
	char lastCodeStr[IR_MAX_CODE_STR_LENGTH];
	uint8_t lastRepeat;

	uint8_t lastLearnedCodeKnown;
	char lastLearnedCodeStr[IR_MAX_CODE_STR_LENGTH];
	PhidgetIR_CodeInfo lastLearnedCodeInfo;

	mostime_t lastDataTime;
	uint8_t delayCode;
} typedef PhidgetIRDeviceInfo;

#endif
