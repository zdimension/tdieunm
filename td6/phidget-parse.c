#include <linux/kernel.h>
#include <linux/slab.h>

#include "phidget.h"

struct PhidgetInterfaceKit * phidget_ifkit_init() 
{
	struct PhidgetInterfaceKit * phidget = NULL;
	
	phidget = kmalloc(sizeof(struct PhidgetInterfaceKit), GFP_KERNEL);
	if (phidget != NULL) {
		phidget->physicalState = kzalloc(8 * sizeof(char), GFP_KERNEL);
		phidget->outputEchoStates = kzalloc(8*sizeof(char), GFP_KERNEL);
		phidget->phid.attr.ifkit.numInputs = 8;
		phidget->phid.attr.ifkit.numSensors = 8;
		phidget->phid.attr.ifkit.numOutputs = 8;
	}
	return phidget;
}

void phidget_ifkit_free(struct PhidgetInterfaceKit * phidget)
{
	if (phidget != NULL) {
		kfree(phidget->physicalState);
		kfree(phidget->outputEchoStates);
		kfree(phidget);
	}
}

int phidget_parse_packet(struct PhidgetInterfaceKit *phid, unsigned char* buffer, int length)
{	
	unsigned char outputState[IFKIT_MAXOUTPUTS], lastOutputState[IFKIT_MAXOUTPUTS];
	unsigned char inputState[IFKIT_MAXINPUTS], lastInputState[IFKIT_MAXINPUTS];
	int sensorRawValue[IFKIT_MAXSENSORS][IFKIT_MAX_DATA_PER_PACKET];
	int sensorDataCount[IFKIT_MAXSENSORS];
	unsigned char ratiometricEcho = PUNK_BOOL;
	
	int overrunBits, overrunPtr, countPtr, packetCount, channelCount[IFKIT_MAXSENSORS], overrunCount[IFKIT_MAXSENSORS];
	unsigned char overcurrentFlag = 0;
	int datacount = 0;
	int flip, bufindx;

	int j = 0, i = 0;
	
	if (length<0) return EPHIDGET_INVALIDARG;
	//TESTPTR(phid);
	TESTPTR(buffer);
	
	for (j = 0; j<phid->phid.attr.ifkit.numInputs; j++)
	{
		inputState[j] = PUNK_BOOL;
		lastInputState[j] = phid->physicalState[j];
	}
	for (j = 0; j<phid->phid.attr.ifkit.numSensors; j++)
	{
		for(i=0;i<IFKIT_MAX_DATA_PER_PACKET;i++)
		{
			sensorRawValue[j][i] = PUNK_INT;
			phid->sensorValue[j][i] = PUNK_INT;
		}
		sensorDataCount[j] = 0;
	}
	for (j = 0; j<phid->phid.attr.ifkit.numOutputs; j++)
	{
		outputState[j] = PUNK_BOOL;
		lastOutputState[j] = phid->outputEchoStates[j];
	}

	//counters, etc.
	packetCount = (buffer[0] >> 6) & 0x03;
	overcurrentFlag = (buffer[0] >> 5) & 0x01;
	ratiometricEcho = (buffer[0] >> 4) & 0x01;
	overrunBits = buffer[0] & 0x0f;

	//Inputs
	for (i = 0, j = 0x01; i < phid->phid.attr.ifkit.numInputs; i++, j <<= 1)
	{
		if (buffer[1] & j)
			inputState[i] = PFALSE;
		else
			inputState[i] = PTRUE;
	}

	//Outputs
	for (i = 0, j = 0x01; i < phid->phid.attr.ifkit.numOutputs; i++, j <<= 1)
	{
		if ((buffer[2] & j) == 0)
			outputState[i] = PFALSE;
		else
			outputState[i] = PTRUE;
	}

	//Sensors
	//Overruns
	overrunPtr = 3;
	for (i = 0; i<phid->phid.attr.ifkit.numSensors; i++)
	{
		overrunCount[i] = 0;
	}
	if(overrunBits & 0x01)
	{
		overrunCount[0] = buffer[overrunPtr] >> 4;
		overrunCount[1] = buffer[overrunPtr] & 0x0f;
		overrunPtr++;
	}
	if(overrunBits & 0x02)
	{
		overrunCount[2] = buffer[overrunPtr] >> 4;
		overrunCount[3] = buffer[overrunPtr] & 0x0f;
		overrunPtr++;
	}
	if(overrunBits & 0x04)
	{
		overrunCount[4] = buffer[overrunPtr] >> 4;
		overrunCount[5] = buffer[overrunPtr] & 0x0f;
		overrunPtr++;
	}
	if(overrunBits & 0x08)
	{
		overrunCount[6] = buffer[overrunPtr] >> 4;
		overrunCount[7] = buffer[overrunPtr] & 0x0f;
		overrunPtr++;
	}

	//Counts
	countPtr = overrunPtr;
	for (i = 0; i<phid->phid.attr.ifkit.numSensors; i++)
	{
		if(i%2)
		{
			channelCount[i] = buffer[countPtr] & 0x0F;
			countPtr++;
		}
		else
		{
			channelCount[i] = buffer[countPtr] >> 4;
		}
		datacount+=channelCount[i];
	}

	//Data
	j=0;
	flip = 0;
	bufindx = countPtr;
	while(datacount>0)
	{
		for (i = 0; i<phid->phid.attr.ifkit.numSensors; i++)
		{
			if(channelCount[i]>j)
			{
				if(!flip)
				{
					sensorRawValue[i][j] = ((unsigned char)buffer[bufindx] + (((unsigned char)buffer[bufindx+1] & 0xf0) << 4));
					bufindx+=2;
				}
				else
				{
					sensorRawValue[i][j] = ((unsigned char)buffer[bufindx] + (((unsigned char)buffer[bufindx-1] & 0x0f) << 8));
					bufindx++;
				}
				//compensating for resistors, etc. - on earlier versions, this was done in Firmware.
				/**
				// NOTE: anything double is commented because no double in the kernel
				**/
				//sensorRawValue[i][j] = round(sensorRawValue[i][j] * 1.001);
				if(sensorRawValue[i][j] > 0xfff)
					sensorRawValue[i][j] = 0xfff;
				phid->sensorValue[i][j] = /*round((double)sensorRawValue[i][j] / 4.095);*/sensorRawValue[i][j];
				sensorDataCount[i]++;
				flip^=0x01;
				datacount--;
			}
		}
		j++;
	}
	if(datacount < 0)
		printk(KERN_INFO "PHIDGET_LOG_DEBUG Datacount error");

	//Send out some errors - overruns/lost packets
	for (i = 0; i<phid->phid.attr.ifkit.numSensors; i++)
	{
		if(overrunCount[i])
		{
			if(phid->dataSinceAttach >= 10)
			{
				printk(KERN_ERR "EEPHIDGET_OVERRUN, Channel %d: %d sample overrun detected.", i, overrunCount[i]);
			}
		}
	}
	if((phid->lastPacketCount >= 0) && ((phid->lastPacketCount+1)&0x03) != packetCount)
	{
		printk(KERN_ERR "EEPHIDGET_PACKETLOST, One or more data packets were lost");
	}
	if(overcurrentFlag)
	{
		printk(KERN_ERR "EEPHIDGET_OVERCURRENT, Analog input overcurrent detected.");
	}

	phid->lastPacketCount = packetCount;

	//break;
	
	return 0;
}
