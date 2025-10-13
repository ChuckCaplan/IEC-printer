#include "interface.h"
#include "globals.h"
#include <Arduino.h>

static const bool DEBUG_IEC = true;

static void dumpBytes(const byte* p, size_t n) {
    for (size_t i=0;i<n;i++) {
        if (i) Serial.print(' ');
        if (p[i] < 16) Serial.print('0');
        Serial.print(p[i], HEX);
    }
}

Interface::Interface(IEC& iec) : m_iec(iec) {
    reset();
}

void Interface::reset(void) {
    printJobActive = false;
    channelOpen = false;
    currentSA = 0xFF;
    printDataBuffer.clear();
}

IEC::ATNCheck Interface::handler(void) {
    noInterrupts();
    IEC::ATNCheck retATN = m_iec.checkATN(m_cmd);
    interrupts();

    if (DEBUG_IEC && retATN != IEC::ATN_IDLE) {
        if (retATN == IEC::ATN_ERROR) {
            Serial.println("ATN ERROR!");
        } else {
            Serial.print("ATN ret="); Serial.print((int)retATN);
            Serial.print(" code=0x"); Serial.print(m_cmd.code, HEX);
            Serial.print(" len="); Serial.print(m_cmd.strLen);
            Serial.print(" str=[");
            dumpBytes(m_cmd.str, min((int)m_cmd.strLen, 6));
            if (m_cmd.strLen > 6) Serial.print(" ...");
            Serial.println("]");
        }
    }

    if (retATN == IEC::ATN_ERROR) return retATN;
    if (retATN == IEC::ATN_IDLE)  return retATN;

    // ATN_CMD means command string was received ending with UNLISTEN
    // The command string contains the actual print data
    if (retATN == IEC::ATN_CMD) {
        if (DEBUG_IEC) {
            Serial.print("ATN_CMD: Adding "); Serial.print(m_cmd.strLen);
            Serial.println(" bytes from command string to buffer");
        }
        // Add command string data to print buffer
        for (byte i = 0; i < m_cmd.strLen; i++) {
            printDataBuffer.push_back(m_cmd.str[i]);
        }
    }

    // OPEN: mark channel and capture any data in command string
    if ((m_cmd.code & 0xF0) == IEC::ATN_CODE_OPEN) {
        channelOpen = true;
        currentSA = (m_cmd.code & 0x0F);
        if (DEBUG_IEC) { Serial.print("OPEN channel SA="); Serial.println(currentSA); }
    }

    // End-of-job
    if (retATN == IEC::ATN_CMD || m_cmd.code == IEC::ATN_CODE_UNLISTEN || (m_cmd.code & 0xF0) == IEC::ATN_CODE_CLOSE) {
        if (DEBUG_IEC) {
            Serial.print("END - buffer has ");
            Serial.print(printDataBuffer.size());
            Serial.println(" bytes");
        }
        if (!printDataBuffer.empty()) {
            printJobActive = true;
        }
        channelOpen = false;
        currentSA = 0xFF;
        return retATN;
    }

    // DATA/TALK/LISTEN handling
    switch (m_cmd.code & 0xF0) {
        case IEC::ATN_CODE_TALK:
            handleATNCmdCodeDataTalk();
            break;
        case IEC::ATN_CODE_DATA:
            if (retATN == IEC::ATN_CMD_LISTEN) {
                handleATNCmdCodeDataListen();
                // For printer: EOI means end of job, trigger print
                if (m_iec.state() & IEC::eoiFlag) {
                    if (DEBUG_IEC) {
                        Serial.print("EOI - Print job complete, buffer has ");
                        Serial.print(printDataBuffer.size());
                        Serial.println(" bytes");
                    }
                    // Only print if buffer has substantial data (filter out init commands)
                    if (printDataBuffer.size() > 100) {
                        printJobActive = true;
                        channelOpen = false;
                        currentSA = 0xFF;
                    } else {
                        if (DEBUG_IEC) Serial.println("Skipping small buffer (likely init command)");
                        printDataBuffer.clear();
                    }
                }
            }
            break;
        default:
            break;
    }

    return retATN;
}

void Interface::handleATNCmdCodeDataTalk() {
    m_iec.send(0x00);
    delayMicroseconds(100);
}

void Interface::handleATNCmdCodeDataListen() {
    bool done = false;
    byte data;
    size_t byteCount = 0;

    do {
        noInterrupts();
        data = m_iec.receive();
        interrupts();
        if (!(m_iec.state() & IEC::errorFlag)) {
            printDataBuffer.push_back(data);
            byteCount++;
        }
        done = (m_iec.state() & (IEC::eoiFlag | IEC::errorFlag));
        
        if (m_iec.state() & IEC::atnFlag) {
            if (DEBUG_IEC) Serial.println("ATN detected during data reception");
            done = true;
        }
    } while (!done);

    if (DEBUG_IEC) {
        Serial.print("LISTEN received ");
        Serial.print(byteCount);
        Serial.println(" bytes");
    }
}
