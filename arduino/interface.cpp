#include "interface.h"
#include "globals.h" // For access to printDataBuffer

Interface::Interface(IEC& iec) : m_iec(iec) {
	reset();
}

void Interface::reset(void) {
    printJobActive = false;
    printDataBuffer.clear(); // Clear any partial data on reset/error.
}

IEC::ATNCheck Interface::handler(void) {
	noInterrupts();
	IEC::ATNCheck retATN = m_iec.checkATN(m_cmd);
	interrupts();
	
	if(retATN != IEC::ATN_IDLE && retATN != IEC::ATN_ERROR) {
		Serial.print("Handler: cmd.code=0x");
		Serial.println(m_cmd.code, HEX);
	}

	if(retATN == IEC::ATN_ERROR) {
		reset();
	} else if(retATN != IEC::ATN_IDLE) {
		m_cmd.str[m_cmd.strLen] = '\0';

		// Handle specific, full command codes first.
		if (m_cmd.code == IEC::ATN_CODE_UNLISTEN) {
			Serial.println("UNLISTEN command received");
			// An UNLISTEN command signifies the end of data transfer.
			// If we have data in the buffer, we can consider the job complete.
			if (printDataBuffer.size() > 0) {
				printJobActive = true;
			}
		}
		switch(m_cmd.code & 0xF0) {
			case IEC::ATN_CODE_DATA:
				if(retATN == IEC::ATN_CMD_TALK) {
					handleATNCmdCodeDataTalk();
				} else if(retATN == IEC::ATN_CMD_LISTEN) {
					handleATNCmdCodeDataListen();
				}
				break;

			case IEC::ATN_CODE_CLOSE:
                Serial.println("CLOSE command received");
                // A CLOSE command signifies the end of the print job. If we have data, send it.
                // The check for device number is implicit; the C64 KERNAL handles it.
                if (printDataBuffer.size() > 0) {
                    printJobActive = true;
                }
				break;

            // Other cases not handled for a simple printer device
			case IEC::ATN_CODE_OPEN:
			case IEC::ATN_CODE_LISTEN:
			case IEC::ATN_CODE_TALK:
			case IEC::ATN_CODE_UNTALK:
				break;
		}
	}
	return retATN;
}

void Interface::handleATNCmdCodeDataTalk() {
    // The C64 wants to read from us. A real printer would send status.
    // We'll just send a simple "OK" message.
    m_iec.send('O');
    m_iec.sendEOI('K');
}

void Interface::handleATNCmdCodeDataListen() {
    bool done = false;
    byte data;
    int byteCount = 0;

    Serial.println("Receiving data...");
    // DO NOT clear the buffer here. A single print job may consist of multiple
    // data blocks, and we need to append them. The buffer is cleared in the
    // main loop after the job is successfully sent to the server.
    noInterrupts();
    do {
        data = m_iec.receive();

        if (!(m_iec.state() & IEC::errorFlag)) {
            printDataBuffer.push_back(data);
            byteCount++;
        }
        done = (m_iec.state() & IEC::eoiFlag) || (m_iec.state() & IEC::errorFlag);
    } while(!done);
    interrupts();
    Serial.print("Received ");
    Serial.print(byteCount);
    Serial.println(" bytes");

    // DO NOT set printJobActive here. EOI only signals the end of a data block,
    // not the end of the entire print job. We must wait for a CLOSE or UNLISTEN.
}