/*
        File: BlaeckTCP.h
        Author: Sebastian Strobl
*/

#ifndef BLAECKTCP_H
#define BLAECKTCP_H

#ifndef BLAECK_BUFFER_SIZE
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#define BLAECK_BUFFER_SIZE 1024
#elif defined(ARDUINO_ARCH_AVR)
#define BLAECK_BUFFER_SIZE 32
#else
#define BLAECK_BUFFER_SIZE 64
#endif
#endif

#include <Arduino.h>
#include <Ethernet.h>
#include <TelnetPrint.h>
#include <CRC32.h>

typedef enum DataType
{
  Blaeck_bool,
  Blaeck_byte,
  Blaeck_short,
  Blaeck_ushort,
  Blaeck_int,
  Blaeck_uint,
  Blaeck_long,
  Blaeck_ulong,
  Blaeck_float,
  Blaeck_double
} dataType;

struct Signal
{
  String SignalName;
  dataType DataType;
  void *Address;
  bool Updated = false;
};

enum BlaeckTimestampMode
{
  BLAECK_NO_TIMESTAMP = 0,
  BLAECK_MICROS = 1,
  BLAECK_RTC = 2
};

class BlaeckTCP
{
public:
  // ----- Constructor -----
  BlaeckTCP();

  // ----- Destructor -----
  ~BlaeckTCP();

  // ----- Initialize ----
  void begin(Stream *streamRef, unsigned int size);
  void begin(byte maxClients, Stream *streamRef, unsigned int size);
  void begin(byte maxClients, Stream *streamRef, unsigned int size, int blaeckWriteDataClientMask);
  void beginBridge(byte maxClients, Stream *streamRef, Stream *bridgeStream);

  // Set these variables in your Arduino sketch
  String DeviceName;
  String DeviceHWVersion;
  String DeviceFWVersion;

  const String LIBRARY_NAME = "BlaeckTCP";
  const String LIBRARY_VERSION = "5.0.1";

  NetClient *Clients;
  // ActiveClient is the client, which sent the command
  NetClient ActiveClient;

  // ----- Signals -----
  // Add a Signal
  void addSignal(String signalName, bool *value);
  void addSignal(String signalName, byte *value);
  void addSignal(String signalName, short *value);
  void addSignal(String signalName, unsigned short *value);
  void addSignal(String signalName, int *value);
  void addSignal(String signalName, unsigned int *value);
  void addSignal(String signalName, long *value);
  void addSignal(String signalName, unsigned long *value);
  void addSignal(String signalName, float *value);
  void addSignal(String signalName, double *value);

  // Delete all Signals
  void deleteSignals();

  // Signal Count
  int SignalCount;

  // ----- Devices -----
  void writeDevices();
  void writeDevices(unsigned long messageID);

  // ----- Symbols -----
  void writeSymbols();
  void writeSymbols(unsigned long messageID);

  // ----- Data Write -----
  // Update value and write directly - by name
  void write(String signalName, bool value);
  void write(String signalName, byte value);
  void write(String signalName, short value);
  void write(String signalName, unsigned short value);
  void write(String signalName, int value);
  void write(String signalName, unsigned int value);
  void write(String signalName, long value);
  void write(String signalName, unsigned long value);
  void write(String signalName, float value);
  void write(String signalName, double value);

  void write(String signalName, bool value, unsigned long messageID);
  void write(String signalName, byte value, unsigned long messageID);
  void write(String signalName, short value, unsigned long messageID);
  void write(String signalName, unsigned short value, unsigned long messageID);
  void write(String signalName, int value, unsigned long messageID);
  void write(String signalName, unsigned int value, unsigned long messageID);
  void write(String signalName, long value, unsigned long messageID);
  void write(String signalName, unsigned long value, unsigned long messageID);
  void write(String signalName, float value, unsigned long messageID);
  void write(String signalName, double value, unsigned long messageID);

  void write(String signalName, bool value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, byte value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, short value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, unsigned short value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, int value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, unsigned int value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, long value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, unsigned long value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, float value, unsigned long messageID, unsigned long timestamp);
  void write(String signalName, double value, unsigned long messageID, unsigned long timestamp);

  // Update value and write directly - by index
  void write(int signalIndex, bool value);
  void write(int signalIndex, byte value);
  void write(int signalIndex, short value);
  void write(int signalIndex, unsigned short value);
  void write(int signalIndex, int value);
  void write(int signalIndex, unsigned int value);
  void write(int signalIndex, long value);
  void write(int signalIndex, unsigned long value);
  void write(int signalIndex, float value);
  void write(int signalIndex, double value);

  void write(int signalIndex, bool value, unsigned long messageID);
  void write(int signalIndex, byte value, unsigned long messageID);
  void write(int signalIndex, short value, unsigned long messageID);
  void write(int signalIndex, unsigned short value, unsigned long messageID);
  void write(int signalIndex, int value, unsigned long messageID);
  void write(int signalIndex, unsigned int value, unsigned long messageID);
  void write(int signalIndex, long value, unsigned long messageID);
  void write(int signalIndex, unsigned long value, unsigned long messageID);
  void write(int signalIndex, float value, unsigned long messageID);
  void write(int signalIndex, double value, unsigned long messageID);

  void write(int signalIndex, bool value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, byte value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, short value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, unsigned short value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, int value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, unsigned int value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, long value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, unsigned long value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, float value, unsigned long messageID, unsigned long timestamp);
  void write(int signalIndex, double value, unsigned long messageID, unsigned long timestamp);

  // ----- Data Update -----
  // Update value and mark Signal as updated - by name
  void update(String signalName, bool value);
  void update(String signalName, byte value);
  void update(String signalName, short value);
  void update(String signalName, unsigned short value);
  void update(String signalName, int value);
  void update(String signalName, unsigned int value);
  void update(String signalName, long value);
  void update(String signalName, unsigned long value);
  void update(String signalName, float value);
  void update(String signalName, double value);

  // Update value and mark Signal as updated - by index
  void update(int signalIndex, bool value);
  void update(int signalIndex, byte value);
  void update(int signalIndex, short value);
  void update(int signalIndex, unsigned short value);
  void update(int signalIndex, int value);
  void update(int signalIndex, unsigned int value);
  void update(int signalIndex, long value);
  void update(int signalIndex, unsigned long value);
  void update(int signalIndex, float value);
  void update(int signalIndex, double value);

  // ----- Mark Signals as Updated -----
  // Use these mark functions for cases where you don't want to change the value
  void markSignalUpdated(int signalIndex);
  void markSignalUpdated(String signalName);
  void markAllSignalsUpdated();
  void clearAllUpdateFlags();
  // Check if any Signals are marked as updated
  bool hasUpdatedSignals();

  // ----- Data Write All -----
  void writeAllData();
  void writeAllData(unsigned long messageID);
  void writeAllData(unsigned long messageID, unsigned long timestamp);
  void timedWriteAllData();
  void timedWriteAllData(unsigned long messageID);
  void timedWriteAllData(unsigned long messageID, unsigned long timestamp);

  // ----- Data Write Updated -----
  void writeUpdatedData();
  void writeUpdatedData(unsigned long messageID);
  void writeUpdatedData(unsigned long messageID, unsigned long timestamp);
  void timedWriteUpdatedData();
  void timedWriteUpdatedData(unsigned long messageID);
  void timedWriteUpdatedData(unsigned long messageID, unsigned long timestamp);

  // ----- Tick -----
  void tick();
  void tick(unsigned long messageID);
  void tickUpdated();
  void tickUpdated(unsigned long messageID);

  // ----- Timed Data configuruation -----
  void setTimedData(bool timedActivated, unsigned long timedInterval_ms);

  // ----- Read  -----
  void read();

  // ----- Command callback  -----
  void setCommandCallback(void (*callback)(char *command, int *parameter, char *string_01));

  // ----- Before data write callback  -----
  void setBeforeWriteCallback(void (*callback)());

  /**
  Handles bidirectional data transfer between TCP and UART interface. This function
  should be called in the main loop to maintain communication flow.
  Data received from TCP is forwarded to UART and responses are sent back.
  */
  void bridgePoll();

  // Timestamp configuration methods
  void setTimestampMode(BlaeckTimestampMode mode);
  void setTimestampCallback(unsigned long (*callback)());
  BlaeckTimestampMode getTimestampMode() const { return _timestampMode; }
  bool hasValidTimestampCallback() const;

private:
  unsigned long getTimeStamp();
  int findSignalIndex(String signalName);

  void timedWriteData(unsigned long msg_id, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long timestamp);
  void tick(unsigned long messageID, bool onlyUpdated);

  void writeData(unsigned long msg_id, byte i, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long timestamp);

  void writeDevices(unsigned long messageID, byte client);

  void writeSymbols(unsigned long messageID, byte client);

  static void validatePlatformSizes();

  Stream *StreamRef;
  int _blaeckWriteDataClientMask;
  byte _maxClients = 0;
  Stream *BridgeStreamRef;
  bool _bridgeMode;

  Signal *Signals;
  int _signalIndex = 0;

  bool _serverRestarted = true;
  bool _sendRestartFlag = true;

  bool _timedActivated = false;
  bool _timedFirstTime = true;
  unsigned long _timedFirstTimeDone_ms = 0;
  unsigned long _timedSetPoint_ms = 0;
  unsigned long _timedInterval_ms = 1000;

  static const int MAXIMUM_CHAR_COUNT = 64;
  char receivedChars[MAXIMUM_CHAR_COUNT];
  char COMMAND[MAXIMUM_CHAR_COUNT] = {0};
  int PARAMETER[10];
  // STRING_01: Max. 15 chars allowed  + Null Terminator '\0' = 16
  // In case more than 15 chars are sent, the rest is cut off in function void parseData()
  char STRING_01[16];

  CRC32 _crc;

  void (*_commandCallback)(char *command, int *parameter, char *string01) = nullptr;
  bool recvWithStartEndMarkers();
  void parseData();

  void (*_beforeWriteCallback)() = nullptr;

  BlaeckTimestampMode _timestampMode = BLAECK_NO_TIMESTAMP;
  unsigned long (*_timestampCallback)() = nullptr;

  union
  {
    bool val;
    byte bval[1];
  } boolCvt;

  union
  {
    short val;
    byte bval[2];
  } shortCvt;

  union
  {
    unsigned short val;
    byte bval[2];
  } ushortCvt;

  union
  {
    int val;
    byte bval[2];
  } intCvt;

  union
  {
    unsigned int val;
    byte bval[2];
  } uintCvt;

  union
  {
    long val;
    byte bval[4];
  } lngCvt;

  union
  {
    unsigned long val;
    byte bval[4];
  } ulngCvt;

  union
  {
    float val;
    byte bval[4];
  } fltCvt;

  union
  {
    double val;
    byte bval[8];
  } dblCvt;
};

#endif //  BLAECKTCP_H