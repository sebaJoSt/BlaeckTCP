/*
        File: BlaeckTCP.cpp
        Author: Sebastian Strobl
*/

#include <Arduino.h>
#include <TelnetPrint.h>
#include "BlaeckTCP.h"

BlaeckTCP::BlaeckTCP()
{
  validatePlatformSizes();
}

BlaeckTCP::~BlaeckTCP()
{
  delete Signals;
  delete Clients;
}

void BlaeckTCP::begin(Stream *streamRef, unsigned int maximumSignalCount)
{
  StreamRef = (Stream *)streamRef;

  _maxClients = 1;

  _blaeckWriteDataClientMask = 1;

  Signals = new Signal[maximumSignalCount];

  StreamRef->print("BlaeckTCP Version: ");
  StreamRef->println(LIBRARY_VERSION);

  StreamRef->println("Only one client (= Client 0) can connect simultaneously!");

  Clients = new NetClient[_maxClients];
}

void BlaeckTCP::begin(byte maxClients, Stream *streamRef, unsigned int maximumSignalCount)
{
  int blaeckWriteDataClientMask = pow(2, maxClients) - 1;

  begin(maxClients, streamRef, maximumSignalCount, blaeckWriteDataClientMask);
}

void BlaeckTCP::begin(byte maxClients, Stream *streamRef, unsigned int maximumSignalCount, int blaeckWriteDataClientMask)
{
  StreamRef = (Stream *)streamRef;

  _maxClients = maxClients;
  _blaeckWriteDataClientMask = blaeckWriteDataClientMask;

  Signals = new Signal[maximumSignalCount];

  StreamRef->print("BlaeckTCP Version: ");
  StreamRef->println(LIBRARY_VERSION);

  StreamRef->print("Max Clients allowed: ");
  StreamRef->println(maxClients);

  StreamRef->print("Clients receiving data messages: ");
  byte allowedClientsCount = 0;
  for (int client = 0; client < maxClients; client++)
  {
    if (bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      if (allowedClientsCount == 0)
      {
        StreamRef->print("#");
        StreamRef->print(client);
      }
      if (allowedClientsCount > 0)
      {
        StreamRef->print(", #");
        StreamRef->print(client);
      }
      allowedClientsCount++;
    }
  }
  if (allowedClientsCount == 0)
  {
    StreamRef->print("none");
  }
  StreamRef->println();

  Clients = new NetClient[maxClients];
}

void BlaeckTCP::beginBridge(byte maxClients, Stream *streamRef, Stream *bridgeStream)
{
  _maxClients = maxClients;
  StreamRef = streamRef;
  _bridgeMode = true;
  BridgeStreamRef = bridgeStream;

  StreamRef->print("BlaeckTCP Version: ");
  StreamRef->println(LIBRARY_VERSION);
  StreamRef->println("Running in Bridge Mode");

  StreamRef->print("Max Clients allowed: ");
  StreamRef->println(maxClients);

  Clients = new NetClient[maxClients];
}

void BlaeckTCP::bridgePoll()
{
  // Handle new client connections
  NetClient newClient = TelnetPrint.accept();
  if (newClient)
  {
    for (byte i = 0; i < _maxClients; i++)
    {
      if (!Clients[i])
      {
        StreamRef->print("We have a new client #");
        StreamRef->println(i);
        newClient.print("Hello, client number: ");
        newClient.println(i);
        Clients[i] = newClient;
        break;
      }
    }
  }

  // Handle data from clients to bridge using buffer
  static uint8_t buffer[BLAECK_BUFFER_SIZE];

  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i] && Clients[i].connected())
    {
      while (Clients[i].available() > 0)
      {
        int bytesAvailable = min(Clients[i].available(), BLAECK_BUFFER_SIZE);
        int bytesRead = Clients[i].read(buffer, bytesAvailable);

        if (bytesRead > 0)
        {
          // Write data to bridge in chunks to avoid overwhelming it
          int written = 0;
          while (written < bytesRead)
          {
            int toWrite = min(64, bytesRead - written); // Write in smaller chunks
            BridgeStreamRef->write(&buffer[written], toWrite);
            written += toWrite;
          }
        }
      }
    }
  }

  // Handle disconnected clients
  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i] && !Clients[i].connected())
    {
      Clients[i].stop();
      StreamRef->print("Client #");
      StreamRef->print(i);
      StreamRef->println(" disconnected");
    }
  }

  // Forward data from bridge stream to clients
  if (BridgeStreamRef->available())
  {
    int bytesAvailable = min(BridgeStreamRef->available(), BLAECK_BUFFER_SIZE);
    int bytesRead = BridgeStreamRef->readBytes(buffer, bytesAvailable);

    if (bytesRead > 0)
    {
      // Send to all connected clients
      for (byte client = 0; client < _maxClients; client++)
      {
        if (Clients[client].connected())
        {
          int written = 0;
          while (written < bytesRead)
          {
            int toWrite = min(64, bytesRead - written);
            Clients[client].write(&buffer[written], toWrite);
            written += toWrite;
          }
        }
      }
    }
  }
}

void BlaeckTCP::addSignal(String signalName, bool *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_bool;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, byte *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_byte;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, short *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_short;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, unsigned short *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_ushort;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, int *value)
{
  Signals[_signalIndex].SignalName = signalName;
#ifdef __AVR__
  Signals[_signalIndex].DataType = Blaeck_int; // 2 bytes
#else
  Signals[_signalIndex].DataType = Blaeck_long; // Treat as 4-byte long
#endif
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, unsigned int *value)
{
  Signals[_signalIndex].SignalName = signalName;
#ifdef __AVR__
  Signals[_signalIndex].DataType = Blaeck_uint; // 2 bytes
#else
  Signals[_signalIndex].DataType = Blaeck_ulong; // Treat as 4-byte unsigned long
#endif
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, long *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_long;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, unsigned long *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_ulong;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, float *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_float;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::addSignal(String signalName, double *value)
{
  Signals[_signalIndex].SignalName = signalName;
#ifdef __AVR__
  /*On the Uno and other ATMEGA based boards, the double implementation occupies 4 bytes
  and is exactly the same as the float, with no gain in precision.*/
  Signals[_signalIndex].DataType = Blaeck_float;
#else
  Signals[_signalIndex].DataType = Blaeck_double;
#endif
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
}

void BlaeckTCP::deleteSignals()
{
  _signalIndex = 0;
  SignalCount = _signalIndex;
}

void BlaeckTCP::read()
{

  if (recvWithStartEndMarkers() == true)
  {
    parseData();
    StreamRef->print("<");
    StreamRef->print(receivedChars);
    StreamRef->println(">");

    if (strcmp(COMMAND, "BLAECK.WRITE_SYMBOLS") == 0)
    {
      unsigned long msg_id = ((unsigned long)PARAMETER[3] << 24) | ((unsigned long)PARAMETER[2] << 16) | ((unsigned long)PARAMETER[1] << 8) | ((unsigned long)PARAMETER[0]);

      this->writeSymbols(msg_id);
    }
    else if (strcmp(COMMAND, "BLAECK.WRITE_DATA") == 0)
    {
      unsigned long msg_id = ((unsigned long)PARAMETER[3] << 24) | ((unsigned long)PARAMETER[2] << 16) | ((unsigned long)PARAMETER[1] << 8) | ((unsigned long)PARAMETER[0]);

      this->writeAllData(msg_id);
    }
    else if (strcmp(COMMAND, "BLAECK.GET_DEVICES") == 0)
    {
      unsigned long msg_id = ((unsigned long)PARAMETER[3] << 24) | ((unsigned long)PARAMETER[2] << 16) | ((unsigned long)PARAMETER[1] << 8) | ((unsigned long)PARAMETER[0]);

      this->writeDevices(msg_id);
    }
    else if (strcmp(COMMAND, "BLAECK.ACTIVATE") == 0)
    {
      unsigned long timedInterval_ms = ((unsigned long)PARAMETER[3] << 24) | ((unsigned long)PARAMETER[2] << 16) | ((unsigned long)PARAMETER[1] << 8) | ((unsigned long)PARAMETER[0]);
      this->setTimedData(true, timedInterval_ms);
    }
    else if (strcmp(COMMAND, "BLAECK.DEACTIVATE") == 0)
    {
      this->setTimedData(false, _timedInterval_ms);
    }

    if (_commandCallback != NULL)
      _commandCallback(COMMAND, PARAMETER, STRING_01);
  }
}

void BlaeckTCP::setCommandCallback(void (*callback)(char *command, int *parameter, char *string_01))
{
  _commandCallback = callback;
}

void BlaeckTCP::setBeforeWriteCallback(void (*callback)())
{
  _beforeWriteCallback = callback;
}

bool BlaeckTCP::recvWithStartEndMarkers()
{
  bool newData = false;
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';

  NetClient newClient = TelnetPrint.accept();

  if (newClient)
  {

    for (byte i = 0; i < _maxClients; i++)
    {
      if (!Clients[i])
      {
        StreamRef->print("We have a new client #");
        StreamRef->println(i);
        newClient.print("Hello, client number: ");
        newClient.println(i);
        bool blaeckDataEnabled = bitRead(_blaeckWriteDataClientMask, i);
        if (blaeckDataEnabled)
        {
          newClient.println("You are enabled to receive data messages.");
        }
        else
        {
          newClient.println("Receiving data messages was disabled for you.");
        }
        // Once we "accept", the client is no longer tracked by the server
        // so we must store it into our list of clients
        Clients[i] = newClient;

        break;
      }
    }
  }

  // Use a buffer
  static char tempBuffer[BLAECK_BUFFER_SIZE];

  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i] && Clients[i].connected())
    {
      while (Clients[i].available() > 0 && !newData)
      {
        // Read data in chunks
        int bytesToRead = min(Clients[i].available(), BLAECK_BUFFER_SIZE);
        int bytesRead = Clients[i].read((uint8_t *)tempBuffer, bytesToRead);

        // Process each character in the buffer
        for (int j = 0; j < bytesRead && !newData; j++)
        {
          char rc = tempBuffer[j];

          if (recvInProgress)
          {
            if (rc != endMarker)
            {
              if (ndx < MAXIMUM_CHAR_COUNT - 1)
              {
                receivedChars[ndx] = rc;
                ndx++;
              }
            }
            else
            {
              receivedChars[ndx] = '\0';
              recvInProgress = false;
              ndx = 0;
              newData = true;
              ActiveClient = Clients[i];
            }
          }
          else if (rc == startMarker)
          {
            recvInProgress = true;
          }
        }
      }
    }
  }

  // stop any clients which disconnect
  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i] && !Clients[i].connected())
    {
      Clients[i].stop();
      StreamRef->print("Client #");
      StreamRef->print(i);
      StreamRef->println(" disconnected");
    }
  }

  return newData;
}

void BlaeckTCP::parseData()
{
  // split the data into its parts
  char tempChars[sizeof(receivedChars)];
  strcpy(tempChars, receivedChars);
  char *strtokIndx;
  strtokIndx = strtok(tempChars, ",");
  strcpy(COMMAND, strtokIndx);

  strtokIndx = strtok(NULL, ",");
  if (strtokIndx != NULL)
  {
    // PARAMETER 1 is stored in PARAMETER_01 & STRING_01 (if PARAMETER 1 is a string)

    // Only copy first 15 chars
    strncpy(STRING_01, strtokIndx, 15);
    // 16th Char = Null Terminator
    STRING_01[15] = '\0';
    PARAMETER[0] = atoi(strtokIndx);
  }
  else
  {
    STRING_01[0] = '\0';
    PARAMETER[0] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[1] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[1] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[2] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[2] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[3] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[3] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[4] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[4] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[5] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[5] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[6] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[6] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[7] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[7] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[8] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[8] = 0;
  }

  strtokIndx = strtok(NULL, ", ");
  if (strtokIndx != NULL)
  {
    PARAMETER[9] = atoi(strtokIndx);
  }
  else
  {
    PARAMETER[9] = 0;
  }
}

void BlaeckTCP::setTimedData(bool timedActivated, unsigned long timedInterval_ms)
{
  _timedActivated = timedActivated;

  if (_timedActivated)
  {
    _timedSetPoint_ms = timedInterval_ms;
    _timedInterval_ms = timedInterval_ms;
    _timedFirstTime = true;
  }
}

void BlaeckTCP::writeSymbols()
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connected())
    {
      this->writeSymbols(1, client);
    }
  }
}

void BlaeckTCP::writeSymbols(unsigned long msg_id)
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connected())
    {
      this->writeSymbols(msg_id, client);
    }
  }
}

void BlaeckTCP::writeSymbols(unsigned long msg_id, byte i)
{
  Clients[i].write("<BLAECK:");
  byte msg_key = 0xB0;
  Clients[i].write(msg_key);
  Clients[i].write(":");
  ulngCvt.val = msg_id;
  Clients[i].write(ulngCvt.bval, 4);
  Clients[i].write(":");

  for (int j = 0; j < _signalIndex; j++)
  {
    Clients[i].write((byte)0);
    Clients[i].write((byte)0);

    Signal signal = Signals[j];
    Clients[i].print(signal.SignalName);
    Clients[i].print('\0');

    switch (signal.DataType)
    {
    case (Blaeck_bool):
    {
      Clients[i].write((byte)0x0);
      break;
    }
    case (Blaeck_byte):
    {
      Clients[i].write(0x1);
      break;
    }
    case (Blaeck_short):
    {
      Clients[i].write(0x2);
      break;
    }
    case (Blaeck_ushort):
    {
      Clients[i].write(0x3);
      break;
    }
    case (Blaeck_int):
    {
      Clients[i].write(0x4);
      break;
    }
    case (Blaeck_uint):
    {
      Clients[i].write(0x5);
      break;
    }
    case (Blaeck_long):
    {
      Clients[i].write(0x6);
      break;
    }
    case (Blaeck_ulong):
    {
      Clients[i].write(0x7);
      break;
    }
    case (Blaeck_float):
    {
      Clients[i].write(0x8);
      break;
    }
    case (Blaeck_double):
    {
      Clients[i].write(0x9);
      break;
    }
    }
  }

  Clients[i].write("/BLAECK>");
  Clients[i].write("\r\n");
}

void BlaeckTCP::update(int signalIndex, bool value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_bool)
    {
      *((bool *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, byte value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_byte)
    {
      *((byte *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, short value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_short)
    {
      *((short *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, unsigned short value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_ushort)
    {
      *((unsigned short *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, int value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    if (Signals[signalIndex].DataType == Blaeck_int)
    {
      *((int *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
#else
    if (Signals[signalIndex].DataType == Blaeck_long)
    {
      *((int *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
#endif
  }
}

void BlaeckTCP::update(int signalIndex, unsigned int value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    if (Signals[signalIndex].DataType == Blaeck_uint)
    {
      *((unsigned int *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
#else
    if (Signals[signalIndex].DataType == Blaeck_ulong)
    {
      *((unsigned int *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
#endif
  }
}

void BlaeckTCP::update(int signalIndex, long value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_long)
    {
      *((long *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, unsigned long value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_ulong)
    {
      *((unsigned long *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, float value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_float)
    {
      *((float *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
  }
}

void BlaeckTCP::update(int signalIndex, double value)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, double is same as float
    if (Signals[signalIndex].DataType == Blaeck_float)
    {
      *((float *)Signals[signalIndex].Address) = (float)value;
      Signals[signalIndex].Updated = true;
    }
#else
    if (Signals[signalIndex].DataType == Blaeck_double)
    {
      *((double *)Signals[signalIndex].Address) = value;
      Signals[signalIndex].Updated = true;
    }
#endif
  }
}

void BlaeckTCP::update(String signalName, bool value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, byte value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, short value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, unsigned short value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, int value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, unsigned int value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, long value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, unsigned long value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, float value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::update(String signalName, double value)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    update(index, value);
  }
}

void BlaeckTCP::write(String signalName, bool value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, byte value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, short value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, unsigned short value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, int value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, unsigned int value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, long value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, unsigned long value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, float value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, double value)
{
  this->write(signalName, value, 1);
}

void BlaeckTCP::write(String signalName, bool value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, byte value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, short value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, unsigned short value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, int value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, unsigned int value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, long value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, unsigned long value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, float value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, double value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(String signalName, bool value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, byte value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, short value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, unsigned short value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, int value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, unsigned int value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, long value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, unsigned long value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, float value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, double value, unsigned long messageID, unsigned long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}

void BlaeckTCP::write(int signalIndex, bool value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, byte value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, short value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, unsigned short value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, int value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, unsigned int value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, long value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, unsigned long value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, float value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, double value)
{
  this->write(signalIndex, value, 1);
}

void BlaeckTCP::write(int signalIndex, bool value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, byte value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, short value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, unsigned short value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, int value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, unsigned int value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, long value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, unsigned long value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, float value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, double value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}

void BlaeckTCP::write(int signalIndex, bool value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_bool)
    {
      *((bool *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, byte value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_byte)
    {
      *((byte *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, short value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_short)
    {
      *((short *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, unsigned short value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_ushort)
    {
      *((unsigned short *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, int value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, int stays as Blaeck_int (2 bytes)
    if (Signals[signalIndex].DataType == Blaeck_int)
    {
      *((int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
#else
    // On 32-bit platforms, int is mapped to Blaeck_long (4 bytes)
    if (Signals[signalIndex].DataType == Blaeck_long)
    {
      *((int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
#endif
  }
}

void BlaeckTCP::write(int signalIndex, unsigned int value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, int stays as Blaeck_int (2 bytes)
    if (Signals[signalIndex].DataType == Blaeck_uint)
    {
      *((unsigned int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
#else
    // On 32-bit platforms, int is mapped to Blaeck_long (4 bytes)
    if (Signals[signalIndex].DataType == Blaeck_ulong)
    {
      *((unsigned int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
#endif
  }
}

void BlaeckTCP::write(int signalIndex, long value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_long)
    {
      *((long *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, unsigned long value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_ulong)
    {
      *((unsigned long *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, float value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_float)
    {
      *((float *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
  }
}

void BlaeckTCP::write(int signalIndex, double value, unsigned long messageID, unsigned long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, double is same as float
    if (Signals[signalIndex].DataType == Blaeck_float)
    {
      *((float *)Signals[signalIndex].Address) = (float)value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
#else
    if (Signals[signalIndex].DataType == Blaeck_double)
    {
      *((double *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
    }
#endif
  }
}

int BlaeckTCP::findSignalIndex(String signalName)
{
  for (int i = 0; i < _signalIndex; i++)
  {
    if (Signals[i].SignalName == signalName)
    {
      return i;
    }
  }
  return -1; // Not found
}

void BlaeckTCP::writeAllData()
{
  this->writeAllData(1);
}

void BlaeckTCP::writeAllData(unsigned long msg_id)
{
  this->writeAllData(msg_id, getTimeStamp());
}

void BlaeckTCP::writeAllData(unsigned long msg_id, unsigned long timestamp)
{
  for (byte client = 0; client < _maxClients; client++)
    if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      this->writeData(msg_id, client, 0, _signalIndex - 1, false, timestamp);
    }
}

void BlaeckTCP::writeUpdatedData()
{
  this->writeUpdatedData(1);
}

void BlaeckTCP::writeUpdatedData(unsigned long msg_id)
{
  this->writeUpdatedData(msg_id, getTimeStamp());
}

void BlaeckTCP::writeUpdatedData(unsigned long messageID, unsigned long timestamp)
{
  for (byte client = 0; client < _maxClients; client++)
    if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      this->writeData(messageID, client, 0, _signalIndex - 1, true, timestamp);
    }
}

void BlaeckTCP::writeData(unsigned long msg_id, byte i, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long timestamp)
{
  if (onlyUpdated && !hasUpdatedSignals())
    return; // No updated signals

  // Bounds checking
  if (signalIndex_start < 0)
    signalIndex_start = 0;
  if (signalIndex_end >= _signalIndex)
    signalIndex_end = _signalIndex - 1;
  if (signalIndex_start > signalIndex_end)
    return; // No valid range

  if (_beforeWriteCallback != NULL)
    _beforeWriteCallback();

  _crc.setPolynome(0x04C11DB7);
  _crc.setInitial(0xFFFFFFFF);
  _crc.setXorOut(0xFFFFFFFF);
  _crc.setReverseIn(true);
  _crc.setReverseOut(true);
  _crc.restart();

  Clients[i].write("<BLAECK:");

  // Message Key
  byte msg_key = 0xD1;
  Clients[i].write(msg_key);
  _crc.add(msg_key);

  Clients[i].write(":");
  _crc.add(':');

  // Message Id
  ulngCvt.val = msg_id;
  Clients[i].write(ulngCvt.bval, 4);
  _crc.add(ulngCvt.bval, 4);

  Clients[i].write(":");
  _crc.add(':');

  // Restart flag
  byte restart_flag = _sendRestartFlag ? 1 : 0;
  Clients[i].write(restart_flag);
  _crc.add(restart_flag);
  _sendRestartFlag = false; // Clear the flag after first transmission

  Clients[i].write(":");
  _crc.add(':');

  // Timestamp mode
  byte timestamp_mode = (byte)_timestampMode;
  Clients[i].write(timestamp_mode);
  _crc.add(timestamp_mode);

  // Add timestamp data if mode is not NO_TIMESTAMP
  if (_timestampMode != BLAECK_NO_TIMESTAMP && hasValidTimestampCallback())
  {
    ulngCvt.val = timestamp;
    Clients[i].write(ulngCvt.bval, 4);
    _crc.add(ulngCvt.bval, 4);
  }

  Clients[i].write(":");
  _crc.add(':');

  for (int j = signalIndex_start; j <= signalIndex_end; j++)
  {
    // Skip if onlyUpdated is true and signal is not updated
    if (onlyUpdated && !Signals[j].Updated)
      continue;

    intCvt.val = j;
    Clients[i].write(intCvt.bval, 2);
    _crc.add(intCvt.bval, 2);

    Signal signal = Signals[j];
    switch (signal.DataType)
    {
    case (Blaeck_bool):
    {
      boolCvt.val = *((bool *)signal.Address);
      Clients[i].write(boolCvt.bval, 1);
      _crc.add(boolCvt.bval, 1);
    }
    break;
    case (Blaeck_byte):
    {
      Clients[i].write(*((byte *)signal.Address));
      _crc.add(*((byte *)signal.Address));
    }
    break;
    case (Blaeck_short):
    {
      shortCvt.val = *((short *)signal.Address);
      Clients[i].write(shortCvt.bval, 2);
      _crc.add(shortCvt.bval, 2);
    }
    break;
    case (Blaeck_ushort):
    {
      ushortCvt.val = *((unsigned short *)signal.Address);
      Clients[i].write(ushortCvt.bval, 2);
      _crc.add(ushortCvt.bval, 2);
    }
    break;
    case (Blaeck_int):
    {
      intCvt.val = *((int *)signal.Address);
      Clients[i].write(intCvt.bval, 2);
      _crc.add(intCvt.bval, 2);
    }
    break;
    case (Blaeck_uint):
    {
      uintCvt.val = *((unsigned int *)signal.Address);
      Clients[i].write(uintCvt.bval, 2);
      _crc.add(uintCvt.bval, 2);
    }
    break;
    case (Blaeck_long):
    {
      lngCvt.val = *((long *)signal.Address);
      Clients[i].write(lngCvt.bval, 4);
      _crc.add(lngCvt.bval, 4);
    }
    break;
    case (Blaeck_ulong):
    {
      ulngCvt.val = *((unsigned long *)signal.Address);
      Clients[i].write(ulngCvt.bval, 4);
      _crc.add(ulngCvt.bval, 4);
    }
    break;
    case (Blaeck_float):
    {
      fltCvt.val = *((float *)signal.Address);
      Clients[i].write(fltCvt.bval, 4);
      _crc.add(fltCvt.bval, 4);
    }
    break;
    case (Blaeck_double):
    {
      dblCvt.val = *((double *)signal.Address);
      Clients[i].write(dblCvt.bval, 8);
      _crc.add(dblCvt.bval, 8);
    }
    break;
    }

    // Clear the updated flag after transmission if we're only sending updated signals (onlyUpdated)
    if (onlyUpdated)
    {
      Signals[j].Updated = false;
    }
  }

  // StatusByte 0: Normal transmission
  // StatusByte + CRC First Byte + CRC Second Byte + CRC Third Byte + CRC Fourth Byte
  Clients[i].write((byte)0);

  uint32_t crc_value = _crc.calc();
  Clients[i].write((byte *)&crc_value, 4);

  Clients[i].write("/BLAECK>");
  Clients[i].write("\r\n");
}

void BlaeckTCP::timedWriteAllData()
{
  this->timedWriteAllData(185273099);
}

void BlaeckTCP::timedWriteAllData(unsigned long msg_id)
{
  this->timedWriteAllData(msg_id, getTimeStamp());
}

void BlaeckTCP::timedWriteAllData(unsigned long msg_id, unsigned long timestamp)
{
  this->timedWriteData(msg_id, 0, _signalIndex - 1, false, timestamp);
}

void BlaeckTCP::timedWriteUpdatedData()
{
  this->timedWriteUpdatedData(185273099);
}

void BlaeckTCP::timedWriteUpdatedData(unsigned long msg_id)
{
  this->timedWriteUpdatedData(185273099, getTimeStamp());
}

void BlaeckTCP::timedWriteUpdatedData(unsigned long msg_id, unsigned long timestamp)
{
  this->timedWriteData(msg_id, 0, _signalIndex - 1, true, timestamp);
}

void BlaeckTCP::timedWriteData(unsigned long msg_id, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long timestamp)
{
  if (_timedFirstTime == true)
    _timedFirstTimeDone_ms = millis();
  unsigned long _timedElapsedTime_ms = (millis() - _timedFirstTimeDone_ms);

  if (((_timedElapsedTime_ms >= _timedSetPoint_ms) || _timedFirstTime == true) && _timedActivated == true)
  {
    if (_timedFirstTime == false)
      _timedSetPoint_ms += _timedInterval_ms;
    _timedFirstTime = false;

    for (byte client = 0; client < _maxClients; client++)
      if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
      {
        this->writeData(msg_id, client, signalIndex_start, signalIndex_end, onlyUpdated, timestamp);
      }
  }
}

void BlaeckTCP::writeDevices()
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connected())
    {
      this->writeDevices(1, client);
    }
  }
}

void BlaeckTCP::writeDevices(unsigned long msg_id)
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connected())
    {
      this->writeDevices(msg_id, client);
    }
  }
}

void BlaeckTCP::writeDevices(unsigned long msg_id, byte i)
{
  String deviceName = DeviceName;
  if (deviceName == "")
    deviceName = "Unknown";

  String deviceHWVersion = DeviceHWVersion;
  if (deviceHWVersion == "")
    deviceHWVersion = "n/a";

  String deviceFWVersion = DeviceFWVersion;
  if (deviceFWVersion == "")
    deviceFWVersion = "n/a";

  byte clientNo = i;
  byte clientDataEnabled = bitRead(_blaeckWriteDataClientMask, clientNo);

  Clients[i].write("<BLAECK:");
  byte msg_key = 0xB5;
  Clients[i].write(msg_key);
  Clients[i].write(":");
  ulngCvt.val = msg_id;
  Clients[i].write(ulngCvt.bval, 4);
  Clients[i].write(":");
  Clients[i].write((byte)0);
  Clients[i].write((byte)0);
  Clients[i].print(deviceName);
  Clients[i].print('\0');
  Clients[i].print(deviceHWVersion);
  Clients[i].print('\0');
  Clients[i].print(deviceFWVersion);
  Clients[i].print('\0');
  Clients[i].print(LIBRARY_VERSION);
  Clients[i].print('\0');
  Clients[i].print(LIBRARY_NAME);
  Clients[i].print('\0');
  Clients[i].print(clientNo);
  Clients[i].print('\0');
  Clients[i].print(clientDataEnabled);
  Clients[i].print('\0');
  Clients[i].print(_serverRestarted);
  Clients[i].print('\0');
  Clients[i].write("/BLAECK>");
  Clients[i].write("\r\n");

  if (_serverRestarted)
  {
    _serverRestarted = false;
  }
}

void BlaeckTCP::tickUpdated()
{
  this->tickUpdated(185273099);
}

void BlaeckTCP::tickUpdated(unsigned long msg_id)
{
  this->tickUpdated(msg_id, getTimeStamp());
}

void BlaeckTCP::tickUpdated(unsigned long msg_id, unsigned long timestamp)
{
  this->tick(msg_id, true, timestamp);
}

void BlaeckTCP::tick()
{
  this->tick(185273099);
}

void BlaeckTCP::tick(unsigned long msg_id)
{
  this->tick(msg_id, getTimeStamp());
}

void BlaeckTCP::tick(unsigned long msg_id, unsigned long timestamp)
{
  this->tick(msg_id, false, timestamp);
}

void BlaeckTCP::tick(unsigned long msg_id, bool onlyUpdated, unsigned long timestamp)
{
  this->read();
  this->timedWriteData(msg_id, 0, _signalIndex - 1, onlyUpdated, timestamp);
}

void BlaeckTCP::markSignalUpdated(int signalIndex)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    Signals[signalIndex].Updated = true;
  }
}

void BlaeckTCP::markSignalUpdated(String signalName)
{
  for (int i = 0; i < _signalIndex; i++)
  {
    if (Signals[i].SignalName == signalName)
    {
      Signals[i].Updated = true;
      break;
    }
  }
}

void BlaeckTCP::markAllSignalsUpdated()
{
  for (int i = 0; i < _signalIndex; i++)
  {
    Signals[i].Updated = true;
  }
}

void BlaeckTCP::clearAllUpdateFlags()
{
  for (int i = 0; i < _signalIndex; i++)
  {
    Signals[i].Updated = false;
  }
}

bool BlaeckTCP::hasUpdatedSignals()
{
  for (int i = 0; i < _signalIndex; i++)
  {
    if (Signals[i].Updated)
    {
      return true;
    }
  }
  return false;
}

void BlaeckTCP::setTimestampMode(BlaeckTimestampMode mode)
{
  _timestampMode = mode;

  // Set default callbacks for built-in modes
  switch (mode)
  {
  case BLAECK_MICROS:
    _timestampCallback = micros;
    break;
  case BLAECK_RTC:
    // User must provide RTC callback - don't override if already set
    if (_timestampCallback == micros)
    {
      _timestampCallback = nullptr;
    }
    break;
  case BLAECK_NO_TIMESTAMP:
  default:
    _timestampCallback = nullptr;
    break;
  }
}

void BlaeckTCP::setTimestampCallback(unsigned long (*callback)())
{
  _timestampCallback = callback;
}

bool BlaeckTCP::hasValidTimestampCallback() const
{
  return (_timestampMode != BLAECK_NO_TIMESTAMP && _timestampCallback != nullptr);
}

unsigned long BlaeckTCP::getTimeStamp()
{
  unsigned long timestamp = 0;

  // Add timestamp data if mode is not NO_TIMESTAMP
  if (_timestampMode != BLAECK_NO_TIMESTAMP && hasValidTimestampCallback())
  {
    timestamp = _timestampCallback();
  }

  return timestamp;
}

void BlaeckTCP::validatePlatformSizes()
{
#ifdef __AVR__
  // AVR (8-bit) platform checks
  static_assert(sizeof(int) == 2, "BlaeckTCP: Expected 2-byte int on AVR");
  static_assert(sizeof(unsigned int) == 2, "BlaeckTCP: Expected 2-byte unsigned int on AVR");
  static_assert(sizeof(double) == 4, "BlaeckTCP: Expected 4-byte double on AVR");
  static_assert(sizeof(double) == sizeof(float), "BlaeckTCP: double should equal float on AVR");
#else
  // 32-bit platform checks
  static_assert(sizeof(int) == 4, "BlaeckTCP: Expected 4-byte int on 32-bit platforms");
  static_assert(sizeof(unsigned int) == 4, "BlaeckTCP: Expected 4-byte unsigned int on 32-bit platforms");
  static_assert(sizeof(double) == 8, "BlaeckTCP: Expected 8-byte double on 32-bit platforms");
  static_assert(sizeof(double) != sizeof(float), "BlaeckTCP: double should differ from float on 32-bit platforms");
  static_assert(sizeof(int) == sizeof(long), "BlaeckTCP: int/long size mismatch breaks type remapping");
  static_assert(sizeof(unsigned int) == sizeof(unsigned long), "BlaeckTCP: uint/ulong size mismatch breaks type remapping");
#endif

  // Universal checks (should be same on ALL Arduino platforms)
  static_assert(sizeof(bool) == 1, "BlaeckTCP: Expected 1-byte bool");
  static_assert(sizeof(byte) == 1, "BlaeckTCP: Expected 1-byte byte");
  static_assert(sizeof(short) == 2, "BlaeckTCP: Expected 2-byte short");
  static_assert(sizeof(unsigned short) == 2, "BlaeckTCP: Expected 2-byte unsigned short");
  static_assert(sizeof(long) == 4, "BlaeckTCP: Expected 4-byte long");
  static_assert(sizeof(unsigned long) == 4, "BlaeckTCP: Expected 4-byte unsigned long");
  static_assert(sizeof(float) == 4, "BlaeckTCP: Expected 4-byte float");
}