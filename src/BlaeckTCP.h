/*
        File: BlaeckTCP.h
        Author: Sebastian Strobl
*/

#ifndef BLAECKTCP_H
#define BLAECKTCP_H

#include <Arduino.h>
#include <Ethernet.h>
#include <TelnetPrint.h>
#include <CRC32.h>

#if __has_include(<WiFiS3.h>)
#define MULTI_CLIENTS 0
#else
#define MULTI_CLIENTS 1
#endif

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
};

class BlaeckTCP
{
public:
  // ----- Constructor -----
  BlaeckTCP();

  // ----- Destructor -----
  ~BlaeckTCP();

  // ----- Initialize
  void begin(Stream *streamRef, unsigned int size);
#ifdef MULTI_CLIENTS
#if (MULTI_CLIENTS == 1)
  void begin(byte maxClients, Stream *streamRef, unsigned int size);
  void begin(byte maxClients, Stream *streamRef, unsigned int size, int blaeckWriteDataClientMask);
#endif
#endif

  /**
           @brief Set these variables in your Arduino sketch
    */
  String DeviceName;
  String DeviceHWVersion;
  String DeviceFWVersion;

  const String LIBRARY_NAME = "BlaeckTCP";
  const String LIBRARY_VERSION = "2.2.0";

  NetClient *Clients;
  // ActiveClient is the client, which sent the command
  NetClient ActiveClient;

  // ----- Signals -----
  // add or delete signals
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

  void deleteSignals();

  // ----- Devices -----
  void writeDevices();
  void writeDevices(unsigned long messageID);
  void writeDevices(unsigned long messageID, byte client);

  // ----- Symbols -----
  void writeSymbols();
  void writeSymbols(unsigned long messageID);
  void writeSymbols(unsigned long messageID, byte client);

  // ----- Data -----
  void writeData();
  void writeData(unsigned long messageID);
  void writeData(unsigned long messageID, byte client);

  // ----- Timed Data -----
  /**
           @brief Call this function every some milliseconds for writing timed data; default messageId = 185273099
    */
  void timedWriteData();
  /**
           @brief Call this function every some milliseconds for writing timed data
           @param messageId --> A unique message ID which echoes back to transmitter to indicate a response to a message.
    */
  void timedWriteData(unsigned long messageID);
  /**
           @brief Call this function for timed data settings
    */
  void setTimedData(bool timedActivated, unsigned long timedInterval_ms);

  // ----- Update before data write Callback function  -----
  /**
          @brief Attach a function that will be called just before transmitting data.
    */
  void attachUpdate(void (*updateCallback)());

  // ----- Read  -----
  /**
           @brief Call this function every some milliseconds for reading TCP input
    */
  void read();
  /**
          @brief Attach a function that will be called when a valid message was received;
    */
  void attachRead(void (*readCallback)(char *command, int *parameter, char *string_01));

  // ----- All-in-one -----
  /**
          @brief Call this function every some milliseconds for reading TCP input
           and writing timed data; default messageId = 185273099
    */
  void tick();
  /**
          @brief Call this function every some milliseconds for reading TCP input
           and writing timed data with messageID;
          @param messageId --> A unique message ID which echoes back to transmitter to indicate a response to a message.
    */
  void tick(unsigned long messageID);

private:
  Stream *StreamRef;
  int _blaeckWriteDataClientMask;
  byte _maxClients = 0;

  Signal *Signals;
  int _signalIndex = 0;

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

  void (*_readCallback)(char *command, int *parameter, char *string01);
  bool recvWithStartEndMarkers();
  void parseData();

  void (*_updateCallback)();

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
    short val;
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