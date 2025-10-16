// IEC Printer Device for Arduino Uno R4 WiFi
// Based on IECDevice library by David Hansel

#ifndef IECPRINTER_H
#define IECPRINTER_H

#include <Arduino.h>
#include <IECDevice.h>
#include <WiFiS3.h>

#define CHUNK_SIZE 512

class IECPrinter : public IECDevice
{
public:
  IECPrinter();
  
  void setServerInfo(const char* host, int port);
  void task();

protected:
  virtual void begin();
  virtual void listen(uint8_t secondary);
  virtual void unlisten();
  virtual void talk(uint8_t secondary);
  virtual int8_t canWrite();
  virtual void write(uint8_t data, bool eoi);
  virtual int8_t canRead();
  virtual uint8_t read();

private:
  void startPrintJob();
  void endPrintJob();
  void flushChunk();
  
  uint8_t m_chunk[CHUNK_SIZE];
  uint16_t m_chunkPos;
  uint8_t m_channel;
  bool m_inPrintJob;
  bool m_needsConnection;
  bool m_connecting;
  unsigned long m_lastDataTime;
  uint32_t m_totalBytes;
  
  const char* m_server;
  int m_port;
  WiFiClient m_client;
};

#endif
