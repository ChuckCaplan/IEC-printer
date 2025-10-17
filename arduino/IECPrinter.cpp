// IEC Printer Device for Arduino UNO R4 WiFi
// Based on IECDevice library by David Hansel

#include "IECPrinter.h"

#define PRINT_TIMEOUT_MS 60000
#define MIN_PRINT_SIZE 10

IECPrinter::IECPrinter() : IECDevice(4)
{
  m_channel = 0;
  m_inPrintJob = false;
  m_needsConnection = false;
  m_connecting = false;
  m_lastDataTime = 0;
  m_chunkPos = 0;
  m_totalBytes = 0;
  m_server = NULL;
  m_port = 0;
}

void IECPrinter::setServerInfo(const char* host, int port)
{
  m_server = host;
  m_port = port;
}

void IECPrinter::begin()
{
  Serial.println("IEC Printer initialized");
}

void IECPrinter::listen(uint8_t secondary)
{
  m_channel = secondary & 0x0F;
  
  if (m_channel != 15) {
    if (!m_inPrintJob && !m_needsConnection) {
      m_needsConnection = true;
    }
    m_lastDataTime = millis();
  }
}

void IECPrinter::unlisten()
{
  if (m_inPrintJob) {
    m_lastDataTime = millis();
  }
}

void IECPrinter::talk(uint8_t secondary)
{
  m_channel = secondary & 0x0F;
}

int8_t IECPrinter::canWrite()
{
  if (!m_inPrintJob) return 1;
  
  if (!m_client.connected()) {
    Serial.println("Connection lost during print!");
    endPrintJob();
    return 0;
  }
  
  return 1;
}

void IECPrinter::write(uint8_t data, bool eoi)
{
  if (!m_inPrintJob) {
    Serial.println("[DEBUG] write() called but not in print job!");
    return;
  }
  
  m_chunk[m_chunkPos++] = data;
  m_totalBytes++;
  m_lastDataTime = millis();
  
  if (m_chunkPos >= CHUNK_SIZE) {
    flushChunk();
  }
}

int8_t IECPrinter::canRead()
{
  if (m_channel == 15) {
    return 2;
  }
  return 0;
}

uint8_t IECPrinter::read()
{
  static const char status[] = "00,OK\r";
  static uint8_t statusIdx = 0;
  
  uint8_t c = status[statusIdx++];
  if (statusIdx >= sizeof(status) - 1) {
    statusIdx = 0;
  }
  
  return c;
}

void IECPrinter::task()
{
  if (m_needsConnection && !m_inPrintJob && !m_connecting) {
    m_connecting = true;
    unsigned long startTime = millis();
    setActive(false);
    startPrintJob();
    setActive(true);
    unsigned long elapsed = millis() - startTime;
    Serial.print("WiFi connection took: ");
    Serial.print(elapsed);
    Serial.println(" ms");
    m_connecting = false;
    m_needsConnection = false;
  }
  
  if (m_inPrintJob && millis() - m_lastDataTime > PRINT_TIMEOUT_MS) {
    if (m_totalBytes >= MIN_PRINT_SIZE) {
      flushChunk();
      endPrintJob();
    } else if (m_totalBytes > 0) {
      Serial.print("Ignoring init job: ");
      Serial.print(m_totalBytes);
      Serial.println(" bytes");
      m_client.stop();
      m_inPrintJob = false;
      m_chunkPos = 0;
      m_totalBytes = 0;
    }
  }
}

void IECPrinter::startPrintJob()
{
  if (!m_server || m_port == 0) {
    Serial.println("Server not configured!");
    return;
  }
  
  Serial.println("\n=== STARTING PRINT JOB ===");
  Serial.print("Connecting to ");
  Serial.print(m_server);
  Serial.print(":");
  Serial.println(m_port);
  
  if (m_client.connect(m_server, m_port)) {
    Serial.println("Connected - streaming data...");
    m_inPrintJob = true;
    m_chunkPos = 0;
    m_totalBytes = 0;
    m_lastDataTime = millis();
  } else {
    Serial.println("Connection failed!");
  }
}

void IECPrinter::flushChunk()
{
  if (m_chunkPos > 0 && m_client.connected()) {
    m_client.write(m_chunk, m_chunkPos);
    m_chunkPos = 0;
  }
}

void IECPrinter::endPrintJob()
{
  if (m_inPrintJob) {
    Serial.print("Print job complete: ");
    Serial.print(m_totalBytes);
    Serial.println(" bytes");
    m_client.stop();
    m_inPrintJob = false;
    m_chunkPos = 0;
    m_totalBytes = 0;
  }
}
