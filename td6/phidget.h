// phidget parse code lifted from cphidgetinterfacekit.c
// constants from a few headers
#define IFKIT_MAXINPUTS 				32
#define IFKIT_MAXOUTPUTS 				32
#define IFKIT_MAXSENSORS 				8

#define MAX_OUT_PACKET_SIZE				64

#define IFKIT_MAXSENSORCHANGE 			1000 //BL: Had to check for this, might as well use a define

//usually it is <=8, but could be bigger if a packet gets missed.
#define IFKIT_MAX_DATA_PER_PACKET		16
//in milliseconds - this is the fastest hardware rate of any device
#define IFKIT_MAX_DATA_RATE 			1
//1 second is the longest between events that we support
#define IFKIT_MIN_DATA_RATE 			1000
//add 200ms for timing differences (late events, etc) - should be plenty
#define IFKIT_DATA_BUFFER_SIZE ((IFKIT_MIN_DATA_RATE + 200)/IFKIT_MAX_DATA_RATE)
#define PFALSE							0x00	/** < False. Used for boolean values. */
#define PTRUE							0x01	/**< True. Used for boolean values. */

/** \name Phidget States
 * Returned by getStatus() functions
 * @{
 */
#define PHIDGET_ATTACHED				0x1 /**< Phidget attached */
#define PHIDGET_NOTATTACHED				0x0 /**< Phidget not attached */
/** @} */

//Adding error codes: Update .NET, COM, Python, Java
/** \name Phidget Error Codes
 * Returned by all C API calls
 * @{
 */
#define	PHIDGET_ERROR_CODE_COUNT		20
#define EPHIDGET_OK						0	/**< Function completed successfully. */
#define EPHIDGET_NOTFOUND				1	/**< Phidget not found. "A Phidget matching the type and or serial number could not be found." */
#define EPHIDGET_NOMEMORY				2	/**< No memory. "Memory could not be allocated." */
#define EPHIDGET_UNEXPECTED				3	/**< Unexpected. "Unexpected Error. Contact Phidgets Inc. for support." */
#define EPHIDGET_INVALIDARG				4	/**< Invalid argument. "Invalid argument passed to function." */
#define EPHIDGET_NOTATTACHED			5	/**< Phidget not attached. "Phidget not physically attached." */
#define EPHIDGET_INTERRUPTED			6	/**< Interrupted. "Read/Write operation was interrupted." This code is not currently used. */
#define EPHIDGET_INVALID				7	/**< Invalid error code. "The Error Code is not defined." */
#define EPHIDGET_NETWORK				8	/**< Network. "Network Error." */
#define EPHIDGET_UNKNOWNVAL				9	/**< Value unknown. "Value is Unknown (State not yet received from device, or not yet set by user)." */
#define EPHIDGET_BADPASSWORD			10	/**< Authorization exception. "No longer used. Replaced by EEPHIDGET_BADPASSWORD" */
#define EPHIDGET_UNSUPPORTED			11	/**< Unsupported. "Not Supported." */
#define EPHIDGET_DUPLICATE				12	/**< Duplicate request. "Duplicated request." */
#define EPHIDGET_TIMEOUT				13	/**< Timeout. "Given timeout has been exceeded." */
#define EPHIDGET_OUTOFBOUNDS			14	/**< Out of bounds. "Index out of Bounds." */
#define EPHIDGET_EVENT					15	/**< Event. "A non-null error code was returned from an event handler." This code is not currently used. */
#define EPHIDGET_NETWORK_NOTCONNECTED	16	/**< Network not connected. "A connection to the server does not exist." */
#define EPHIDGET_WRONGDEVICE			17	/**< Wrong device. "Function is not applicable for this device." */
#define EPHIDGET_CLOSED					18	/**< Phidget Closed. "Phidget handle was closed." */
#define EPHIDGET_BADVERSION				19	/**< Version Mismatch. "No longer used. Replaced by EEPHIDGET_BADVERSION" */
/** @} */

//Adding error codes: Update .NET, COM, Python, Java
/** \name Phidget Error Event Codes
 * Returned in the Phidget error event
 * @{
 */
#define EEPHIDGET_EVENT_ERROR(code) (0x8000 + code)


//Library errors
#define EEPHIDGET_NETWORK		EEPHIDGET_EVENT_ERROR(0x0001)	/**< Network Error (asynchronous). */  
#define EEPHIDGET_BADPASSWORD	EEPHIDGET_EVENT_ERROR(0x0002)	/**< Authorization Failed. */
#define EEPHIDGET_BADVERSION	EEPHIDGET_EVENT_ERROR(0x0003)	/**< Webservice and Client protocol versions don't match. Update to newest release. */

//Errors streamed back from firmware
#define EEPHIDGET_OK			EEPHIDGET_EVENT_ERROR(0x1000)	/**< An error state has ended - see description for details. */
#define EEPHIDGET_OVERRUN		EEPHIDGET_EVENT_ERROR(0x1002)	/**< A sampling overrun happend in firmware. */
#define EEPHIDGET_PACKETLOST	EEPHIDGET_EVENT_ERROR(0x1003)	/**< One or more packets were lost. */
#define EEPHIDGET_WRAP			EEPHIDGET_EVENT_ERROR(0x1004)	/**< A variable has wrapped around. */
#define EEPHIDGET_OVERTEMP		EEPHIDGET_EVENT_ERROR(0x1005)	/**< Overtemperature condition detected. */
#define EEPHIDGET_OVERCURRENT	EEPHIDGET_EVENT_ERROR(0x1006)	/**< Overcurrent condition detected. */
#define EEPHIDGET_OUTOFRANGE	EEPHIDGET_EVENT_ERROR(0x1007)	/**< Out of range condition detected. */
#define EEPHIDGET_BADPOWER		EEPHIDGET_EVENT_ERROR(0x1008)	/**< Power supply problem detected. */

/** @} */

/** \name Phidget Unknown Constants
 * Data values will be set to these constants when a call fails with \ref EPHIDGET_UNKNOWNVAL.
 * @{
 */
#define PUNK_BOOL	0x02					/**< Unknown Boolean (unsigned char) */
#define PUNK_SHRT	0x7FFF					/**< Unknown Short	 (16-bit) */
#define PUNK_INT	0x7FFFFFFF				/**< Unknown Integer (32-bit) */
#define PUNK_INT64	0x7FFFFFFFFFFFFFFFLL	/**< Unknown Integer (64-bit) */
#define PUNK_DBL	1e300					/**< Unknown Double */
#define PUNK_FLT	1e30					/**< Unknown Float */

#define TESTPTR(phidname) if (!phidname) return EPHIDGET_INVALIDARG;
#define TESTPTRS(vname1,vname2) if ((!vname1) || (!vname2)) return EPHIDGET_INVALIDARG;
#define ZEROMEM(var,size) memset(var, 0, size);

struct _ifkit{
	int numInputs;
	int numSensors;
	int numOutputs;
};

struct _attr {
	struct _ifkit ifkit;
};

struct _phid{
	struct _attr attr;
	int outputReportByteLength;
};

struct PhidgetInterfaceKit {
	struct _phid phid;
	unsigned char *physicalState;
	unsigned char *outputEchoStates;
	unsigned char *outputStates;
	
	unsigned char *changedOutputs;
	unsigned char *nextOutputStates;
	
	int sensorValue[IFKIT_MAXSENSORS][IFKIT_MAX_DATA_PER_PACKET];
	int dataSinceAttach;
	int lastPacketCount;
};

// Function to initialize initialise a Phidget InterfaceKit structure
struct PhidgetInterfaceKit * phidget_ifkit_init(void);

void phidget_ifkit_free(struct PhidgetInterfaceKit *phidget);

int phidget_parse_packet(struct PhidgetInterfaceKit *phidget, unsigned char* buffer, int length);