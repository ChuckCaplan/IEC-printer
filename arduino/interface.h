#ifndef INTERFACE_H
#define INTERFACE_H

#include "iec_driver.h"

class Interface
{
public:
	Interface(IEC& iec);
	virtual ~Interface() {}

	// The handler returns the current IEC state, see the iec_driver.hpp for possible states.
	IEC::ATNCheck handler(void);

    bool isPrintJobActive() { return printJobActive; }
    void printJobHandled() { printJobActive = false; }

private:
	void reset(void);

	// handler helpers.
	void handleATNCmdCodeDataTalk();
	void handleATNCmdCodeDataListen();

	IEC& m_iec;
	IEC::ATNCmd m_cmd;
    bool printJobActive = false;
};

#endif