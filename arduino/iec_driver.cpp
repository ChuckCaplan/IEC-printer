#include "iec_driver.h"

/******************************************************************************
 *                                                                             *
 *                                TIMING SETUP                                 *
 *                                                                             *
 ******************************************************************************/


// IEC protocol timing consts:
#define TIMING_BIT          70  // bit clock hi/lo time     (us)
#define TIMING_NO_EOI       20  // delay before bits        (us)
#define TIMING_EOI_WAIT     200 // delay to signal EOI      (us)
#define TIMING_EOI_THRESH   20  // threshold for EOI detect (*10 us approx)
#define TIMING_STABLE_WAIT  20  // line stabilization       (us)
#define TIMING_ATN_PREDELAY 50  // delay required in atn    (us)
#define TIMING_ATN_DELAY    100 // delay required after atn (us)

// See timeoutWait below.
#define TIMEOUT  65000

IEC::IEC(byte deviceNumber) :
	m_state(noFlags), m_deviceNumber(deviceNumber),
	m_atnPin(0), m_dataPin(0), m_clockPin(0)
{
} // ctor


byte IEC::timeoutWait(byte waitBit, boolean whileHigh)
{
	word t = 0;
	boolean c;

	while(t < TIMEOUT) {
		// Check the waiting condition:
		c = readPIN(waitBit);

		if(whileHigh)
			c = !c;

		if(c)
			return false;

		delayMicroseconds(2); // The aim is to make the loop at least 3 us
		t++;
	}

	// If down here, we have had a timeout.
	// Release lines and go to inactive state with error flag
	writeCLOCK(false);
	writeDATA(false);

	m_state = errorFlag;

	// Wait for ATN release, problem might have occured during attention
	while(!readATN());

	// Note: The while above is without timeout. If ATN is held low forever,
	//       the CBM is out in the woods and needs a reset anyways.

	return true;
} // timeoutWait


// IEC Recieve byte standard function
byte IEC::receiveByte(void)
{
	m_state = noFlags;

	// Wait for talker ready
	if(timeoutWait(m_clockPin, false))
		return 0;

	// Say we're ready
	writeDATA(false);

	// Record how long CLOCK is high, more than 200 us means EOI
	byte n = 0;
	while(readCLOCK() && (n < 20)) {
		delayMicroseconds(10);  // this loop should cycle in about 10 us...
		n++;
	}

	if(n >= TIMING_EOI_THRESH) {
		// EOI intermission
		m_state |= eoiFlag;

		// Acknowledge by pull down data more than 60 us
		writeDATA(true);
		delayMicroseconds(TIMING_BIT);
		writeDATA(false);

		// but still wait for clk
		if(timeoutWait(m_clockPin, true))
			return 0;
	}

	// Sample ATN
	if(false == readATN())
		m_state |= atnFlag;

	byte data = 0;
	// Get the bits, sampling on clock rising edge:
	for(n = 0; n < 8; n++) {
		data >>= 1;
		if(timeoutWait(m_clockPin, false))
			return 0;
		data |= (readDATA() ? (1 << 7) : 0);
		if(timeoutWait(m_clockPin, true))
			return 0;
	}

	// Signal we accepted data:
	writeDATA(true);

	return data;
} // receiveByte


// IEC Send byte standard function
boolean IEC::sendByte(byte data, boolean signalEOI)
{
	// Listener must have accepted previous data
	if(timeoutWait(m_dataPin, true))
		return false;

	// Say we're ready
	writeCLOCK(false);

	// Wait for listener to be ready
	if(timeoutWait(m_dataPin, false))
		return false;

	if(signalEOI) {
		// Signal eoi by waiting 200 us
		delayMicroseconds(TIMING_EOI_WAIT);

		// get eoi acknowledge:
		if(timeoutWait(m_dataPin, true))
			return false;

		if(timeoutWait(m_dataPin, false))
			return false;
	}

	delayMicroseconds(TIMING_NO_EOI);

	// Send bits
	for(byte n = 0; n < 8; n++) {
		writeCLOCK(true);
		// set data
		writeDATA((data & 1) ? false : true);

		delayMicroseconds(TIMING_BIT);
		writeCLOCK(false);
		delayMicroseconds(TIMING_BIT);

		data >>= 1;
	}

	writeCLOCK(true);
	writeDATA(false);

	// Line stabilization delay
	delayMicroseconds(TIMING_STABLE_WAIT);

	// Wait for listener to accept data
	if(timeoutWait(m_dataPin, true))
		return false;

	return true;
} // sendByte


// IEC turnaround
boolean IEC::turnAround(void)
{
	// Wait until clock is released
	if(timeoutWait(m_clockPin, false))
		return false;

	writeDATA(false);
	delayMicroseconds(TIMING_BIT);
	writeCLOCK(true);
	delayMicroseconds(TIMING_BIT);

	return true;
} // turnAround


// this routine will set the direction on the bus back to normal
boolean IEC::undoTurnAround(void)
{
	writeDATA(true);
	delayMicroseconds(TIMING_BIT);
	writeCLOCK(false);
	delayMicroseconds(TIMING_BIT);

	// wait until the computer releases the clock line
	if(timeoutWait(m_clockPin, true))
		return false;

	return true;
} // undoTurnAround


/******************************************************************************
 *                                                                             *
 *                               Public functions                              *
 *                                                                             *
 ******************************************************************************/

IEC::ATNCheck IEC::checkATN(ATNCmd& cmd)
{
	ATNCheck ret = ATN_IDLE;
	byte i = 0;

	if(!readATN()) {
		// Attention line is active, go to listener mode and get message.
		writeDATA(true);
		writeCLOCK(false);
		delayMicroseconds(TIMING_ATN_PREDELAY);

		// Get first ATN byte, it is either LISTEN or TALK
		ATNCommand c = (ATNCommand)receiveByte();
		if(m_state & errorFlag)
			return ATN_ERROR;

		if(c == (ATN_CODE_LISTEN | m_deviceNumber)) {
			// Okay, we will listen.
			c = (ATNCommand)receiveByte();
			if (m_state & errorFlag) return ATN_ERROR;
			cmd.code = c;

			if((c & 0xF0) == ATN_CODE_DATA) {
				ret = ATN_CMD_LISTEN;
			} else if(c != ATN_CODE_UNLISTEN) {
				// Some other command. Record the cmd string until UNLISTEN is sent
				for(;;) {
					c = (ATNCommand)receiveByte();
					if(m_state & errorFlag) return ATN_ERROR;
					if((m_state & atnFlag) && (ATN_CODE_UNLISTEN == c)) break;
					if(i >= ATN_CMD_MAX_LENGTH) return ATN_ERROR; // overflow
					cmd.str[i++] = c;
				}
				ret = ATN_CMD;
			}
		} else if (c == (ATN_CODE_TALK | m_deviceNumber)) {
			// Okay, we will talk soon
			c = (ATNCommand)receiveByte();
			if(m_state & errorFlag) return ATN_ERROR;
			cmd.code = c;

			while(!readATN()) {
				if(readCLOCK()) {
					c = (ATNCommand)receiveByte();
					if(m_state & errorFlag) return ATN_ERROR;
					if(i >= ATN_CMD_MAX_LENGTH) return ATN_ERROR; // overflow
					cmd.str[i++] = c;
				}
			}

			if(!turnAround()) return ATN_ERROR;
			ret = ATN_CMD_TALK;

		} else {
			// Not for us.
			delayMicroseconds(TIMING_ATN_DELAY);
			writeDATA(false);
			writeCLOCK(false);
			while(!readATN());
		}
	} else {
		// No ATN, keep lines in a released state.
		writeDATA(false);
		writeCLOCK(false);
	}

	delayMicroseconds(TIMING_ATN_DELAY);
	cmd.strLen = i;
	return ret;
} // checkATN

byte IEC::receive() { return receiveByte(); }
boolean IEC::send(byte data) { return sendByte(data, false); }
boolean IEC::sendEOI(byte data) {
	if(sendByte(data, true)) {
		if(undoTurnAround()) return true;
	}
	return false;
}

boolean IEC::init() {
	// Set the initial output state to LOW for all pins. This ensures that when
	// we switch pinMode to OUTPUT, the pin will drive LOW immediately.
	digitalWrite(m_atnPin, LOW);
	digitalWrite(m_dataPin, LOW);
	digitalWrite(m_clockPin, LOW);

	// Set the initial pin mode to INPUT to be passive on the bus.
	pinMode(m_atnPin, INPUT);
	pinMode(m_dataPin, INPUT);
	pinMode(m_clockPin, INPUT);
	m_state = noFlags;
	return true;
}

byte IEC::deviceNumber() const { return m_deviceNumber; }
void IEC::setDeviceNumber(const byte deviceNumber) { m_deviceNumber = deviceNumber; }
void IEC::setPins(byte atn, byte clock, byte data) {
	m_atnPin = atn;
	m_clockPin = clock;
	m_dataPin = data;
}
IEC::IECState IEC::state() const { return static_cast<IECState>(m_state); }