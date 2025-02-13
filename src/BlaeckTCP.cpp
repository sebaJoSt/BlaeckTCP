/*
        File: BlaeckTCP.cpp
        Author: Sebastian Strobl
*/

#include <Arduino.h>
#include <TelnetPrint.h>
#include "BlaeckTCP.h"

BlaeckTCP::BlaeckTCP()
{
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
  static uint8_t buffer[BLAECK_INPUT_BUFFER_SIZE];

  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i] && Clients[i].connected())
    {
      while (Clients[i].available() > 0)
      {
        int bytesAvailable = min(Clients[i].available(), BLAECK_INPUT_BUFFER_SIZE);
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

            // Only yield if we're processing a large amount of data
            if (bytesRead > 64)
            {
              yield(); // Allow other processes to run
            }
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
    int bytesAvailable = min(BridgeStreamRef->available(), BLAECK_INPUT_BUFFER_SIZE);
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

            // Only yield if we're processing a large amount of data
            if (bytesRead > 64)
            {
              yield(); // Allow other processes to run
            }
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
}

void BlaeckTCP::addSignal(String signalName, byte *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_byte;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, short *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_short;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, unsigned short *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_ushort;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, int *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_int;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, unsigned int *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_uint;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, long *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_long;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, unsigned long *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_ulong;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
}

void BlaeckTCP::addSignal(String signalName, float *value)
{
  Signals[_signalIndex].SignalName = signalName;
  Signals[_signalIndex].DataType = Blaeck_float;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
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
}

void BlaeckTCP::deleteSignals()
{
  _signalIndex = 0;
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

      this->writeData(msg_id);
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

    if (_readCallback != NULL)
      _readCallback(COMMAND, PARAMETER, STRING_01);
  }
}

void BlaeckTCP::attachRead(void (*readCallback)(char *command, int *parameter, char *string_01))
{
  _readCallback = readCallback;
}

void BlaeckTCP::attachUpdate(void (*updateCallback)())
{
  _updateCallback = updateCallback;
}

bool BlaeckTCP::recvWithStartEndMarkers()
{
  bool newData = false;
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

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

  // Use a buffer to read chunks of data
  static char tempBuffer[BLAECK_INPUT_BUFFER_SIZE];

  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i] && Clients[i].connected())
    {
      while (Clients[i].available() > 0 && !newData)
      {
        // Read data in chunks
        int bytesToRead = min(Clients[i].available(), BLAECK_INPUT_BUFFER_SIZE);
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

        // Give other processes a chance to run
        if (bytesRead >= BLAECK_CHUNK_SIZE)
        {
          yield();
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

void BlaeckTCP::writeData()
{
  if (_updateCallback != NULL)
    _updateCallback();

  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      this->writeData(1, client);
    }
  }
}

void BlaeckTCP::writeData(unsigned long msg_id)
{
  if (_updateCallback != NULL)
    _updateCallback();

  for (byte client = 0; client < _maxClients; client++)
    if (Clients[client].connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      this->writeData(msg_id, client);
    }
}

void BlaeckTCP::writeData(unsigned long msg_id, byte i)
{
  _crc.setPolynome(0x04C11DB7);
  _crc.setInitial(0xFFFFFFFF);
  _crc.setXorOut(0xFFFFFFFF);
  _crc.setReverseIn(true);
  _crc.setReverseOut(true);
  _crc.restart();

  // Buffer for chunked writing
  const size_t bufferSize = 256;
  uint8_t buffer[bufferSize];
  size_t bufferIndex = 0;

  // Helper function to flush buffer
  auto flushBuffer = [&]()
  {
    if (bufferIndex > 0)
    {
      Clients[i].write(buffer, bufferIndex);
      if (bufferIndex >= BLAECK_CHUNK_SIZE)
      {
        yield();
      }
      bufferIndex = 0;
    }
  };

  // Helper function to add to buffer
  auto addToBuffer = [&](const uint8_t *data, size_t len)
  {
    while (len > 0)
    {
      size_t chunk = min(len, bufferSize - bufferIndex);
      memcpy(buffer + bufferIndex, data, chunk);
      bufferIndex += chunk;
      len -= chunk;

      if (bufferIndex >= bufferSize)
      {
        flushBuffer();
      }
    }
  };

  // Write header
  uint8_t header[] = {'<', 'B', 'L', 'A', 'E', 'C', 'K', ':', 0xB1, ':'};
  addToBuffer(header, sizeof(header));

  // Add message ID
  ulngCvt.val = msg_id;
  addToBuffer(ulngCvt.bval, 4);
  addToBuffer((const uint8_t *)":", 1);

  // Update CRC with header
  _crc.add(0xB1); // msg_key
  _crc.add(':');
  _crc.add(ulngCvt.bval, 4);
  _crc.add(':');

  // Write data for each signal
  for (int j = 0; j < _signalIndex; j++)
  {
    // Write signal index
    intCvt.val = j;
    addToBuffer(intCvt.bval, 2);
    _crc.add(intCvt.bval, 2);

    Signal signal = Signals[j];
    switch (signal.DataType)
    {
    case Blaeck_bool:
    {
      boolCvt.val = *((bool *)signal.Address);
      addToBuffer(boolCvt.bval, 1);
      _crc.add(boolCvt.bval, 1);
      break;
    }

    case Blaeck_byte:
    {
      uint8_t byteVal = *((byte *)signal.Address);
      addToBuffer(&byteVal, 1);
      _crc.add(byteVal);
      break;
    }

    case Blaeck_short:
    {
      shortCvt.val = *((short *)signal.Address);
      addToBuffer(shortCvt.bval, 2);
      _crc.add(shortCvt.bval, 2);
      break;
    }

    case Blaeck_ushort:
    {
      ushortCvt.val = *((unsigned short *)signal.Address);
      addToBuffer(ushortCvt.bval, 2);
      _crc.add(ushortCvt.bval, 2);
      break;
    }

    case Blaeck_int:
    {
      intCvt.val = *((int *)signal.Address);
      addToBuffer(intCvt.bval, 2);
      _crc.add(intCvt.bval, 2);
      break;
    }

    case Blaeck_uint:
    {
      uintCvt.val = *((unsigned int *)signal.Address);
      addToBuffer(uintCvt.bval, 2);
      _crc.add(uintCvt.bval, 2);
      break;
    }

    case Blaeck_long:
    {
      lngCvt.val = *((long *)signal.Address);
      addToBuffer(lngCvt.bval, 4);
      _crc.add(lngCvt.bval, 4);
      break;
    }

    case Blaeck_ulong:
    {
      ulngCvt.val = *((unsigned long *)signal.Address);
      addToBuffer(ulngCvt.bval, 4);
      _crc.add(ulngCvt.bval, 4);
      break;
    }

    case Blaeck_float:
    {
      fltCvt.val = *((float *)signal.Address);
      addToBuffer(fltCvt.bval, 4);
      _crc.add(fltCvt.bval, 4);
      break;
    }

    case Blaeck_double:
    {
      dblCvt.val = *((double *)signal.Address);
      addToBuffer(dblCvt.bval, 8);
      _crc.add(dblCvt.bval, 8);
      break;
    }
    }

    // Yield periodically during processing
    if (j % 10 == 0)
    {
      yield();
    }
  }

  // Write status byte (0 for normal transmission)
  uint8_t statusByte = 0;
  addToBuffer(&statusByte, 1);

  // Calculate and write CRC
  uint32_t crc_value = _crc.calc();
  addToBuffer((uint8_t *)&crc_value, 4);

  // Write footer
  const char *footer = "/BLAECK>\r\n";
  addToBuffer((const uint8_t *)footer, strlen(footer));

  // Ensure all remaining data is flushed
  flushBuffer();
}

void BlaeckTCP::timedWriteData()
{
  this->timedWriteData(185273099);
}

void BlaeckTCP::timedWriteData(unsigned long msg_id)
{

  if (_timedFirstTime == true)
    _timedFirstTimeDone_ms = millis();
  unsigned long _timedElapsedTime_ms = (millis() - _timedFirstTimeDone_ms);

  if (((_timedElapsedTime_ms >= _timedSetPoint_ms) || _timedFirstTime == true) && _timedActivated == true)
  {
    if (_timedFirstTime == false)
      _timedSetPoint_ms += _timedInterval_ms;
    _timedFirstTime = false;

    this->writeData(msg_id);
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

void BlaeckTCP::tick()
{
  this->tick(185273099);
}

void BlaeckTCP::tick(unsigned long msg_id)
{
  this->read();
  this->timedWriteData(msg_id);
}