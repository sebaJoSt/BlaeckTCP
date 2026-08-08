/*
        File: BlaeckTCP.cpp
        Author: Sebastian Strobl
*/

#include "BlaeckTCP.h"

BlaeckTCP::BlaeckTCP()
{
  validatePlatformSizes();
}

BlaeckTCP::~BlaeckTCP()
{
  delete[] Signals;
  Signals = nullptr;
  delete[] Clients;
  Clients = nullptr;
}

void BlaeckTCP::_initClientMeta()
{
  for (byte i = 0; i < _maxClients; i++)
  {
    Clients[i].name[0] = '\0';
    strcpy(Clients[i].type, "unknown");
  }
}

void BlaeckTCP::_parseClientIdentity(const char *raw)
{
  // Parse identity from: BLAECK.GET_DEVICES,0,0,0,0,Name,Type
  // Skip 5 comma-separated fields (command + 4 msg_id bytes) to reach Name and Type
  const char *p = raw;
  int commaCount = 0;
  const char *nameStart = NULL;
  const char *typeStart = NULL;

  while (*p)
  {
    if (*p == ',')
    {
      commaCount++;
      if (commaCount == 5)
        nameStart = p + 1;
      else if (commaCount == 6)
        typeStart = p + 1;
    }
    p++;
  }

  if (nameStart == NULL)
    return;

  // Find client index for CommandingClient
  for (byte c = 0; c < _maxClients; c++)
  {
    if (Clients[c].connection == CommandingClient)
    {
      // Extract name (up to next comma or end)
      int len = 0;
      if (typeStart != NULL)
        len = (typeStart - 1) - nameStart;
      else
        len = strlen(nameStart);
      if (len > (int)(sizeof(Clients[c].name) - 1))
        len = sizeof(Clients[c].name) - 1;
      if (len > 0)
      {
        strncpy(Clients[c].name, nameStart, len);
        Clients[c].name[len] = '\0';
      }

      // Extract type
      if (typeStart != NULL && *typeStart != '\0')
      {
        strncpy(Clients[c].type, typeStart, sizeof(Clients[c].type) - 1);
        Clients[c].type[sizeof(Clients[c].type) - 1] = '\0';
      }

      // Log identity
      if (Clients[c].name[0] != '\0')
      {
        StreamRef->print("Client #");
        StreamRef->print(c);
        StreamRef->print(" identified (");
        StreamRef->print(Clients[c].type);
        StreamRef->print(": ");
        StreamRef->print(Clients[c].name);
        StreamRef->println(")");
      }
      break;
    }
  }
}

void BlaeckTCP::begin(Stream *streamRef, unsigned int maximumSignalCount, uint16_t port)
{
  StreamRef = (Stream *)streamRef;

  _maxClients = 1;

  _blaeckWriteDataClientMask = 1;

  _signalCapacity = maximumSignalCount;
  if (Signals != nullptr)
  {
    delete[] Signals;
    Signals = nullptr;
  }
  Signals = new Signal[maximumSignalCount];
  _signalIndex = 0;
  SignalCount = 0;
  _schemaHash = 0;
  _signalOverflowOccurred = false;
  _signalOverflowCount = 0;

  StreamRef->print("BlaeckTCP Version: ");
  StreamRef->println(BLAECKTCP_VERSION);

  StreamRef->println("Only one client (= Client 0) can connect simultaneously!");

  if (Clients != nullptr)
  {
    delete[] Clients;
    Clients = nullptr;
  }
  Clients = new BlaeckClient[_maxClients];
  _initClientMeta();
  _startServer(port);
}

void BlaeckTCP::begin(byte maxClients, Stream *streamRef, unsigned int maximumSignalCount, uint16_t port)
{
  int blaeckWriteDataClientMask = pow(2, maxClients) - 1;

  begin(maxClients, streamRef, maximumSignalCount, blaeckWriteDataClientMask, port);
}

void BlaeckTCP::begin(byte maxClients, Stream *streamRef, unsigned int maximumSignalCount, int blaeckWriteDataClientMask, uint16_t port)
{
  StreamRef = (Stream *)streamRef;

  _maxClients = maxClients;
  _blaeckWriteDataClientMask = blaeckWriteDataClientMask;

  _signalCapacity = maximumSignalCount;
  if (Signals != nullptr)
  {
    delete[] Signals;
    Signals = nullptr;
  }
  Signals = new Signal[maximumSignalCount];
  _signalIndex = 0;
  SignalCount = 0;
  _schemaHash = 0;
  _signalOverflowOccurred = false;
  _signalOverflowCount = 0;

  StreamRef->print("BlaeckTCP Version: ");
  StreamRef->println(BLAECKTCP_VERSION);

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

  if (Clients != nullptr)
  {
    delete[] Clients;
    Clients = nullptr;
  }
  Clients = new BlaeckClient[maxClients];
  _initClientMeta();
  _startServer(port);
}

void BlaeckTCP::beginBridge(byte maxClients, Stream *streamRef, Stream *bridgeStream, uint16_t port)
{
  _maxClients = maxClients;
  StreamRef = streamRef;
  _bridgeMode = true;
  BridgeStreamRef = bridgeStream;

  StreamRef->print("BlaeckTCP Version: ");
  StreamRef->println(BLAECKTCP_VERSION);
  StreamRef->println("Running in Bridge Mode");

  StreamRef->print("Max Clients allowed: ");
  StreamRef->println(maxClients);

  if (Clients != nullptr)
  {
    delete[] Clients;
    Clients = nullptr;
  }
  Clients = new BlaeckClient[maxClients];
  _initClientMeta();
  _startServer(port);
}

void BlaeckTCP::_startServer(uint16_t port)
{
  TelnetPrint = NetServer(port);
  TelnetPrint.begin();
#if defined(ESP32) || defined(ESP8266)
  TelnetPrint.setNoDelay(BLAECK_TCP_NO_DELAY_DEFAULT);
#endif
}

void BlaeckTCP::bridgePoll()
{
  // Handle new client connections
  NetClient newClient = TelnetPrint.accept();
  if (newClient)
  {
    for (byte i = 0; i < _maxClients; i++)
    {
      if (!Clients[i].connection)
      {
        StreamRef->print("Client #");
        StreamRef->print(i);
        StreamRef->print(" connected");
        IPAddress ip = newClient.remoteIP();
        if (ip != IPAddress(0, 0, 0, 0))
        {
          StreamRef->print(": ");
          StreamRef->print(ip);
          StreamRef->print(":");
          StreamRef->print(newClient.remotePort());
        }
        StreamRef->println();
        newClient.print("Hello, client number: ");
        newClient.println(i);
        Clients[i].connection = newClient;
        Clients[i].name[0] = '\0';
        strcpy(Clients[i].type, "unknown");
        if (_clientConnectedCallback != NULL)
          _clientConnectedCallback(i);
        break;
      }
    }
  }

  // Handle data from clients to bridge using buffer
  static uint8_t buffer[BLAECK_BUFFER_SIZE];

  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i].connection && Clients[i].connection.connected())
    {
      while (Clients[i].connection.available() > 0)
      {
        int bytesAvailable = min(Clients[i].connection.available(), BLAECK_BUFFER_SIZE);
        int bytesRead = Clients[i].connection.read(buffer, bytesAvailable);

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
    if (Clients[i].connection && !Clients[i].connection.connected())
    {
      Clients[i].connection.stop();
      StreamRef->print("Client #");
      StreamRef->print(i);
      StreamRef->print(" disconnected");
      if (Clients[i].name[0] != '\0')
      {
        StreamRef->print(" (");
        StreamRef->print(Clients[i].type);
        StreamRef->print(": ");
        StreamRef->print(Clients[i].name);
        StreamRef->print(")");
      }
      StreamRef->println();
      Clients[i].name[0] = '\0';
      strcpy(Clients[i].type, "unknown");
      if (_clientDisconnectedCallback != NULL)
        _clientDisconnectedCallback(i);
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
        if (Clients[client].connection.connected())
        {
          int written = 0;
          while (written < bytesRead)
          {
            int toWrite = min(64, bytesRead - written);
            Clients[client].connection.write(&buffer[written], toWrite);
            written += toWrite;
          }
        }
      }
    }
  }
}

void BlaeckTCP::addSignal(String signalName, bool *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_bool;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, byte *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_byte;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, short *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_short;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, unsigned short *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_ushort;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, int *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
#ifdef __AVR__
  Signals[_signalIndex].DataType = Blaeck_int; // 2 bytes
#else
  Signals[_signalIndex].DataType = Blaeck_long; // Treat as 4-byte long
#endif
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, unsigned int *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
#ifdef __AVR__
  Signals[_signalIndex].DataType = Blaeck_uint; // 2 bytes
#else
  Signals[_signalIndex].DataType = Blaeck_ulong; // Treat as 4-byte unsigned long
#endif
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, long *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_long;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, unsigned long *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_ulong;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, float *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_float;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, double *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
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
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::addSignal(String signalName, char *value)
{
  if (static_cast<unsigned int>(_signalIndex) >= _signalCapacity)
  {
    _signalOverflowOccurred = true;
    _signalOverflowCount++;
    return;
  }
  setSignalName(_signalIndex, signalName);
  Signals[_signalIndex].DataType = Blaeck_string;
  Signals[_signalIndex].Address = value;
  _signalIndex++;
  SignalCount = _signalIndex;
  _schemaHash = _computeSchemaHash();
}

void BlaeckTCP::deleteSignals()
{
  _signalIndex = 0;
  SignalCount = _signalIndex;
  _schemaHash = 0;
  _signalOverflowOccurred = false;
  _signalOverflowCount = 0;
}

uint16_t BlaeckTCP::_computeSchemaHash()
{
  // CRC16-CCITT (init=0x0000, poly=0x1021) over signal names + datatype codes.
  // Must match Python: binascii.crc_hqx(data, 0) & 0xFFFF
  uint16_t crc = 0x0000;
  for (int j = 0; j < _signalIndex; j++)
  {
    // Feed signal name bytes (UTF-8 / ASCII)
    const char *name = Signals[j].SignalName.c_str();
    while (*name)
    {
      byte b = (byte)*name++;
      crc ^= ((uint16_t)b << 8);
      for (byte k = 0; k < 8; k++)
      {
        if (crc & 0x8000)
          crc = (crc << 1) ^ 0x1021;
        else
          crc <<= 1;
      }
    }
    // Feed datatype code byte
    byte code = (byte)Signals[j].DataType;
    crc ^= ((uint16_t)code << 8);
    for (byte k = 0; k < 8; k++)
    {
      if (crc & 0x8000)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
    }
  }
  return crc & 0xFFFF;
}

void BlaeckTCP::setSignalName(int signalIndex, String signalName)
{
  if (signalIndex < 0 || signalIndex >= (int)_signalCapacity)
    return;

  Signals[signalIndex].SignalName = "";
  Signals[signalIndex].SignalName.reserve(signalName.length() + 1);
  Signals[signalIndex].SignalName += signalName;
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

      // Parse optional identity: <BLAECK.GET_DEVICES,0,0,0,0,Name,Type>
      _parseClientIdentity(receivedChars);

      this->writeDevices(msg_id);
    }
#if BLAECK_ENABLE_COMMAND_META
    else if (strcmp(COMMAND, "BLAECK.WRITE_COMMANDS") == 0)
    {
      unsigned long msg_id = ((unsigned long)PARAMETER[3] << 24) | ((unsigned long)PARAMETER[2] << 16) | ((unsigned long)PARAMETER[1] << 8) | ((unsigned long)PARAMETER[0]);

      this->writeCommands(msg_id);
    }
#endif
    else if (strcmp(COMMAND, "BLAECK.ACTIVATE") == 0)
    {
      if (_fixedInterval_ms == BLAECK_INTERVAL_CLIENT)
      {
        unsigned long timedInterval_ms = ((unsigned long)PARAMETER[3] << 24) | ((unsigned long)PARAMETER[2] << 16) | ((unsigned long)PARAMETER[1] << 8) | ((unsigned long)PARAMETER[0]);
        this->_setTimedDataState(true, timedInterval_ms);
      }
    }
    else if (strcmp(COMMAND, "BLAECK.DEACTIVATE") == 0)
    {
      if (_fixedInterval_ms == BLAECK_INTERVAL_CLIENT)
      {
        this->_setTimedDataState(false, _timedInterval_ms);
      }
    }

    _dispatchRegisteredHandlers();
  }
}

void BlaeckTCP::setBeforeWriteCallback(void (*callback)())
{
  _beforeWriteCallback = callback;
}

bool BlaeckTCP::onCommand(const char *command, BlaeckCommandHandler handler)
{
  if (command == nullptr || handler == nullptr || command[0] == '\0')
  {
    return false;
  }
  if (strlen(command) >= MAX_COMMAND_NAME_COUNT)
  {
    if (StreamRef != nullptr)
    {
      StreamRef->print("Command name too long for handler table: ");
      StreamRef->println(command);
    }
    return false;
  }

  // Update existing handler
  for (byte i = 0; i < MAX_COMMAND_HANDLERS; i++)
  {
    if (_commandHandlers[i].inUse && strcmp(_commandHandlers[i].command, command) == 0)
    {
      _commandHandlers[i].handler = handler;
#if BLAECK_ENABLE_COMMAND_META
      _commandHandlers[i].kind = BLAECK_CMD_PLAIN;
      _commandHandlers[i].unit = nullptr;
      _commandHandlers[i].options = nullptr;
      _commandHandlers[i].stateSignal = nullptr;
#endif
      return true;
    }
  }

  // Insert new handler
  for (byte i = 0; i < MAX_COMMAND_HANDLERS; i++)
  {
    if (!_commandHandlers[i].inUse)
    {
      strncpy(_commandHandlers[i].command, command, MAX_COMMAND_NAME_COUNT - 1);
      _commandHandlers[i].command[MAX_COMMAND_NAME_COUNT - 1] = '\0';
      _commandHandlers[i].handler = handler;
      _commandHandlers[i].inUse = true;
#if BLAECK_ENABLE_COMMAND_META
      _commandHandlers[i].kind = BLAECK_CMD_PLAIN;
      _commandHandlers[i].unit = nullptr;
      _commandHandlers[i].options = nullptr;
      _commandHandlers[i].stateSignal = nullptr;
#endif
      return true;
    }
  }

  if (StreamRef != nullptr)
  {
    StreamRef->print("Command handler table full for: ");
    StreamRef->println(command);
  }
  return false;
}

void BlaeckTCP::onAnyCommand(BlaeckAnyCommandHandler handler)
{
  _anyCommandHandler = handler;
}

void BlaeckTCP::clearAllCommandHandlers()
{
  for (byte i = 0; i < MAX_COMMAND_HANDLERS; i++)
  {
    _commandHandlers[i].inUse = false;
    _commandHandlers[i].handler = nullptr;
    _commandHandlers[i].command[0] = '\0';
#if BLAECK_ENABLE_COMMAND_META
    _commandHandlers[i].kind = BLAECK_CMD_PLAIN;
    _commandHandlers[i].unit = nullptr;
    _commandHandlers[i].options = nullptr;
    _commandHandlers[i].stateSignal = nullptr;
#endif
  }
  _anyCommandHandler = nullptr;
}

#if BLAECK_ENABLE_COMMAND_META
bool BlaeckTCP::onNumberCommand(const char *command, BlaeckCommandHandler handler,
                                const __FlashStringHelper *stateSignal,
                                float min, float max, float step,
                                const __FlashStringHelper *unit)
{
  bool ok = onCommand(command, handler);
  if (ok)
    _annotateCommand(command, BLAECK_CMD_NUMBER, stateSignal, min, max, step, unit, nullptr);
  return ok;
}

bool BlaeckTCP::onSwitchCommand(const char *command, BlaeckCommandHandler handler,
                                const __FlashStringHelper *stateSignal)
{
  bool ok = onCommand(command, handler);
  if (ok)
    _annotateCommand(command, BLAECK_CMD_SWITCH, stateSignal, 0.0f, 0.0f, 0.0f, nullptr, nullptr);
  return ok;
}

bool BlaeckTCP::onSelectCommand(const char *command, BlaeckCommandHandler handler,
                                const __FlashStringHelper *stateSignal,
                                const __FlashStringHelper *optionsCsv)
{
  bool ok = onCommand(command, handler);
  if (ok)
    _annotateCommand(command, BLAECK_CMD_SELECT, stateSignal, 0.0f, 0.0f, 0.0f, nullptr, optionsCsv);
  return ok;
}

bool BlaeckTCP::onButtonCommand(const char *command, BlaeckCommandHandler handler)
{
  bool ok = onCommand(command, handler);
  if (ok)
    _annotateCommand(command, BLAECK_CMD_BUTTON, nullptr, 0.0f, 0.0f, 0.0f, nullptr, nullptr);
  return ok;
}

bool BlaeckTCP::onTextCommand(const char *command, BlaeckCommandHandler handler,
                              const __FlashStringHelper *stateSignal,
                              unsigned int maxLength)
{
  bool ok = onCommand(command, handler);
  if (ok)
    // maxLength is stored in meta_max (reused as the text length limit).
    _annotateCommand(command, BLAECK_CMD_TEXT, stateSignal, 0.0f, (float)maxLength, 0.0f, nullptr, nullptr);
  return ok;
}

void BlaeckTCP::_annotateCommand(const char *command, uint8_t kind,
                                 const __FlashStringHelper *stateSignal,
                                 float mn, float mx, float st,
                                 const __FlashStringHelper *unit,
                                 const __FlashStringHelper *options)
{
  for (byte i = 0; i < MAX_COMMAND_HANDLERS; i++)
  {
    if (_commandHandlers[i].inUse && strcmp(_commandHandlers[i].command, command) == 0)
    {
      _commandHandlers[i].kind = kind;
      _commandHandlers[i].meta_min = mn;
      _commandHandlers[i].meta_max = mx;
      _commandHandlers[i].meta_step = st;
      _commandHandlers[i].unit = unit;
      _commandHandlers[i].options = options;
      _commandHandlers[i].stateSignal = stateSignal;
      return;
    }
  }
}

byte BlaeckTCP::_flashCsvOptionCount(const __FlashStringHelper *csv)
{
  if (csv == nullptr)
    return 0;
  PGM_P p = reinterpret_cast<PGM_P>(csv);
  byte count = 1;
  bool any = false;
  byte c;
  while ((c = pgm_read_byte(p++)) != 0)
  {
    any = true;
    if (c == ',')
      count++;
  }
  return any ? count : 0;
}

long BlaeckTCP::_flashCsvIndexOf(const __FlashStringHelper *csv, const char *value)
{
  if (csv == nullptr || value == nullptr || value[0] == '\0')
    return -1;

  PGM_P p = reinterpret_cast<PGM_P>(csv);
  long index = 0;
  const char *v = value;
  bool matching = true; // current token still matches value so far

  byte c;
  while (true)
  {
    c = pgm_read_byte(p++);
    if (c == ',' || c == '\0')
    {
      // End of a token: match if value was fully consumed too.
      if (matching && *v == '\0')
        return index;
      if (c == '\0')
        return -1;
      // Advance to next token
      index++;
      v = value;
      matching = true;
    }
    else
    {
      if (matching)
      {
        char a = (char)c;
        char b = *v;
        // Case-insensitive compare
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (b == '\0' || a != b)
          matching = false;
        else
          v++;
      }
    }
  }
}

void BlaeckTCP::_percentDecodeInPlace(char *s)
{
  if (s == nullptr)
    return;

  const char *src = s;
  char *dst = s;
  while (*src != '\0')
  {
    if (*src == '%' && src[1] != '\0' && src[2] != '\0')
    {
      char hi = src[1];
      char lo = src[2];
      int hiVal = (hi >= '0' && hi <= '9') ? hi - '0'
                  : (hi >= 'a' && hi <= 'f') ? hi - 'a' + 10
                  : (hi >= 'A' && hi <= 'F') ? hi - 'A' + 10
                                             : -1;
      int loVal = (lo >= '0' && lo <= '9') ? lo - '0'
                  : (lo >= 'a' && lo <= 'f') ? lo - 'a' + 10
                  : (lo >= 'A' && lo <= 'F') ? lo - 'A' + 10
                                             : -1;
      if (hiVal >= 0 && loVal >= 0)
      {
        *dst++ = (char)((hiVal << 4) | loVal);
        src += 3;
        continue;
      }
    }
    *dst++ = *src++;
  }
  *dst = '\0';
}
#endif

void BlaeckTCP::setClientConnectedCallback(void (*callback)(byte clientNo))
{
  _clientConnectedCallback = callback;
}

void BlaeckTCP::setClientDisconnectedCallback(void (*callback)(byte clientNo))
{
  _clientDisconnectedCallback = callback;
}

bool BlaeckTCP::isClientDataEnabled(byte clientNo) const
{
  if (clientNo >= _maxClients)
  {
    return false;
  }
  return bitRead(_blaeckWriteDataClientMask, clientNo) == 1;
}

void BlaeckTCP::_parseCommandTokens(const char *raw)
{
  _parsedCommand[0] = '\0';
  _parsedParamCount = 0;
  for (byte i = 0; i < MAX_COMMAND_PARAM_COUNT; i++)
  {
    _parsedParamPtrs[i] = nullptr;
  }

  if (raw == nullptr || raw[0] == '\0')
  {
    return;
  }

  strncpy(_parsedTokenBuffer, raw, sizeof(_parsedTokenBuffer) - 1);
  _parsedTokenBuffer[sizeof(_parsedTokenBuffer) - 1] = '\0';

  // Manual comma-scanner that preserves empty fields between consecutive commas.
  char *p = _parsedTokenBuffer;

  // Extract command (first token before the first comma)
  char *tokenStart = p;
  while (*p != ',' && *p != '\0')
    p++;
  bool hasComma = (*p == ',');
  if (hasComma)
  {
    *p = '\0';
    p++;
  }
  while (*tokenStart == ' ')
    tokenStart++;
  if (tokenStart[0] == '\0')
  {
    return;
  }
  strncpy(_parsedCommand, tokenStart, MAX_COMMAND_NAME_COUNT - 1);
  _parsedCommand[MAX_COMMAND_NAME_COUNT - 1] = '\0';

  if (!hasComma)
    return;

  // Extract parameters — empty fields (,,) produce a pointer to '\0'
  bool moreParams = true;
  while (moreParams && _parsedParamCount < MAX_COMMAND_PARAM_COUNT)
  {
    tokenStart = p;
    while (*p != ',' && *p != '\0')
      p++;
    if (*p == ',')
    {
      *p = '\0';
      p++;
    }
    else
    {
      moreParams = false;
    }
    while (*tokenStart == ' ')
      tokenStart++;
    _parsedParamPtrs[_parsedParamCount] = tokenStart;
    _parsedParamCount++;
  }
}

void BlaeckTCP::_dispatchRegisteredHandlers()
{
  _parseCommandTokens(receivedChars);
  if (_parsedCommand[0] == '\0')
  {
    return;
  }

  byte ackStatus = 1;                  // 0 = accepted, 1 = rejected
  byte ackReason = BLAECK_ACK_UNKNOWN; // reason reported when rejected
  bool matched = false;

  for (byte i = 0; i < MAX_COMMAND_HANDLERS; i++)
  {
    if (_commandHandlers[i].inUse &&
        _commandHandlers[i].handler != nullptr &&
        strcmp(_commandHandlers[i].command, _parsedCommand) == 0)
    {
      matched = true;
#if BLAECK_ENABLE_COMMAND_META
      ackReason = _validateTypedCommand(i);
#else
      ackReason = BLAECK_ACK_OK;
#endif
      if (ackReason == BLAECK_ACK_OK)
      {
        ackStatus = 0;
        _commandHandlers[i].handler(
            _parsedCommand,
            (const char *const *)_parsedParamPtrs,
            _parsedParamCount);
      }
      break;
    }
  }

  if (_anyCommandHandler != nullptr)
  {
    _anyCommandHandler(
        _parsedCommand,
        (const char *const *)_parsedParamPtrs,
        _parsedParamCount);

    if (!matched)
    {
      // Delivered to the catch-all handler: acknowledge as accepted.
      matched = true;
      ackStatus = 0;
      ackReason = BLAECK_ACK_OK;
    }
  }

  // Acknowledge every non-internal command back to the sender. BLAECK.* frames
  // are handled in read() and must not be acked here.
  if (strncmp(_parsedCommand, "BLAECK.", 7) != 0)
  {
    _writeCommandAck(receivedChars, ackStatus, ackReason);
  }
}

uint32_t BlaeckTCP::_fnv1a32(const char *s)
{
  uint32_t h = 0x811C9DC5UL; // FNV offset basis
  if (s != nullptr)
  {
    while (*s != '\0')
    {
      h ^= (uint8_t)(*s++);
      h *= 0x01000193UL; // FNV prime
    }
  }
  return h;
}

void BlaeckTCP::_writeCommandAck(const char *rawCommand, byte status, byte reasonCode)
{
  if (!CommandingClient || !CommandingClient.connected())
    return;

  CommandingClient.write("<BLAECK:");
  byte msg_key = 0xF0;
  CommandingClient.write(msg_key);
  CommandingClient.write(":");
  ulngCvt.val = _commandAckMsgId++;
  CommandingClient.write(ulngCvt.bval, 4);
  CommandingClient.write(":");

  // Payload: command hash (4 bytes, little-endian) + status (1) + reason (1).
  ulngCvt.val = _fnv1a32(rawCommand);
  CommandingClient.write(ulngCvt.bval, 4);
  CommandingClient.write(status);
  CommandingClient.write(reasonCode);

  // No CRC32 tail: acks mirror the descriptive 0xE0 frame format.
  CommandingClient.write("/BLAECK>");
  CommandingClient.write("\r\n");
}

#if BLAECK_ENABLE_COMMAND_META
byte BlaeckTCP::_validateTypedCommand(byte handlerIndex)
{
  const CommandHandlerEntry &e = _commandHandlers[handlerIndex];

  // Plain and button commands carry no value to validate.
  if (e.kind == BLAECK_CMD_PLAIN || e.kind == BLAECK_CMD_BUTTON)
    return BLAECK_ACK_OK;

  // No value supplied -> let the handler decide (e.g. query/toggle usage).
  if (_parsedParamCount < 1 || _parsedParamPtrs[0] == nullptr || _parsedParamPtrs[0][0] == '\0')
    return BLAECK_ACK_OK;

  const char *v = _parsedParamPtrs[0];

  if (e.kind == BLAECK_CMD_NUMBER)
  {
    float f = (float)atof(v);
    if (f < e.meta_min || f > e.meta_max)
    {
      if (StreamRef != nullptr)
      {
        StreamRef->print(F("Command rejected (out of range): "));
        StreamRef->print(e.command);
        StreamRef->print('=');
        StreamRef->print(v);
        StreamRef->print(F(" allowed ["));
        StreamRef->print(e.meta_min);
        StreamRef->print(F(", "));
        StreamRef->print(e.meta_max);
        StreamRef->println(F("]"));
      }
      return BLAECK_ACK_OUT_OF_RANGE;
    }
  }
  else if (e.kind == BLAECK_CMD_SWITCH)
  {
    if (!(strcmp(v, "0") == 0 || strcmp(v, "1") == 0))
    {
      if (StreamRef != nullptr)
      {
        StreamRef->print(F("Command rejected (switch expects 0/1): "));
        StreamRef->print(e.command);
        StreamRef->print('=');
        StreamRef->println(v);
      }
      return BLAECK_ACK_BAD_SWITCH;
    }
  }
  else if (e.kind == BLAECK_CMD_SELECT)
  {
    byte count = _flashCsvOptionCount(e.options);

    // Accept either an option name (case-insensitive) or a numeric index.
    long idx = _flashCsvIndexOf(e.options, v);
    if (idx < 0)
    {
      char *endp = nullptr;
      long n = strtol(v, &endp, 10);
      if (endp != v && *endp == '\0')
        idx = n;
    }

    if (idx < 0 || idx >= (long)count)
    {
      if (StreamRef != nullptr)
      {
        StreamRef->print(F("Command rejected (bad select value): "));
        StreamRef->print(e.command);
        StreamRef->print('=');
        StreamRef->print(v);
        StreamRef->print(F(" allowed [0, "));
        StreamRef->print((int)count - 1);
        StreamRef->println(F("] or an option name"));
      }
      return BLAECK_ACK_BAD_SELECT;
    }

    // Normalize to the index string so index-based handlers work whether the
    // caller sent a name (e.g. HA select) or a raw index.
    snprintf(_selectIndexScratch, sizeof(_selectIndexScratch), "%ld", idx);
    _parsedParamPtrs[0] = _selectIndexScratch;
  }
  else if (e.kind == BLAECK_CMD_TEXT)
  {
    // Percent-decode in place (SELECT-style param normalization) so the handler
    // receives raw UTF-8. The 0xF0 ack still hashes the encoded receivedChars,
    // so it keeps matching the host's hash of what it sent.
    char *decoded = (char *)_parsedParamPtrs[0];
    _percentDecodeInPlace(decoded);

    unsigned int maxLen = (unsigned int)e.meta_max;
    if (maxLen > 0 && strlen(decoded) > maxLen)
    {
      if (StreamRef != nullptr)
      {
        StreamRef->print(F("Command rejected (text too long): "));
        StreamRef->print(e.command);
        StreamRef->print(F(" len="));
        StreamRef->print((unsigned int)strlen(decoded));
        StreamRef->print(F(" max="));
        StreamRef->println(maxLen);
      }
      return BLAECK_ACK_TOO_LONG;
    }
  }

  return BLAECK_ACK_OK;
}
#endif

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
      if (!Clients[i].connection)
      {
        StreamRef->print("Client #");
        StreamRef->print(i);
        StreamRef->print(" connected");
        IPAddress ip = newClient.remoteIP();
        if (ip != IPAddress(0, 0, 0, 0))
        {
          StreamRef->print(": ");
          StreamRef->print(ip);
          StreamRef->print(":");
          StreamRef->print(newClient.remotePort());
        }
        StreamRef->println();
        newClient.print("Hello, client number: ");
        newClient.println(i);
        bool blaeckDataEnabled= bitRead(_blaeckWriteDataClientMask, i);
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
        Clients[i].connection = newClient;
        Clients[i].name[0] = '\0';
        strcpy(Clients[i].type, "unknown");
        if (_clientConnectedCallback != NULL)
          _clientConnectedCallback(i);

        break;
      }
    }
  }

  // Use a buffer
  static char tempBuffer[BLAECK_BUFFER_SIZE];

  for (byte i = 0; i < _maxClients; i++)
  {
    if (Clients[i].connection && Clients[i].connection.connected())
    {
      while (Clients[i].connection.available() > 0 && !newData)
      {
        // Read data in chunks
        int bytesToRead = min(Clients[i].connection.available(), BLAECK_BUFFER_SIZE);
        int bytesRead = Clients[i].connection.read((uint8_t *)tempBuffer, bytesToRead);

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
              CommandingClient = Clients[i].connection;
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
    if (Clients[i].connection && !Clients[i].connection.connected())
    {
      Clients[i].connection.stop();
      StreamRef->print("Client #");
      StreamRef->print(i);
      StreamRef->print(" disconnected");
      if (Clients[i].name[0] != '\0')
      {
        StreamRef->print(" (");
        StreamRef->print(Clients[i].type);
        StreamRef->print(": ");
        StreamRef->print(Clients[i].name);
        StreamRef->print(")");
      }
      StreamRef->println();
      Clients[i].name[0] = '\0';
      strcpy(Clients[i].type, "unknown");
      if (_clientDisconnectedCallback != NULL)
        _clientDisconnectedCallback(i);
    }
  }

  return newData;
}

void BlaeckTCP::parseData()
{
  // split the data into its parts
  char tempChars[sizeof(receivedChars)];
  strncpy(tempChars, receivedChars, sizeof(tempChars) - 1);
  tempChars[sizeof(tempChars) - 1] = '\0';

  // Manual comma-scanner that preserves empty fields between consecutive commas.
  char *p = tempChars;
  char *tokenStart;
  bool hasComma;

  STRING_01[0] = '\0';
  for (int i = 0; i < 10; i++)
    PARAMETER[i] = 0;

  // --- COMMAND (first token) ---
  tokenStart = p;
  while (*p != ',' && *p != '\0')
    p++;
  hasComma = (*p == ',');
  if (hasComma)
  {
    *p = '\0';
    p++;
  }
  if (tokenStart[0] != '\0')
  {
    strncpy(COMMAND, tokenStart, sizeof(COMMAND) - 1);
    COMMAND[sizeof(COMMAND) - 1] = '\0';
  }
  else
  {
    COMMAND[0] = '\0';
  }

  if (!hasComma)
    return;

  // --- STRING_01 / PARAMETER[0] ---
  tokenStart = p;
  while (*p != ',' && *p != '\0')
    p++;
  if (*p == ',')
  {
    *p = '\0';
    p++;
  }
  while (*tokenStart == ' ')
    tokenStart++;
  strncpy(STRING_01, tokenStart, 15);
  STRING_01[15] = '\0';
  PARAMETER[0] = atoi(tokenStart);

  // --- PARAMETER[1] through PARAMETER[9] ---
  for (int i = 1; i <= 9; i++)
  {
    if (*p == '\0')
      break;
    tokenStart = p;
    while (*p != ',' && *p != '\0')
      p++;
    if (*p == ',')
    {
      *p = '\0';
      p++;
    }
    while (*tokenStart == ' ')
      tokenStart++;
    PARAMETER[i] = atoi(tokenStart);
  }
}

void BlaeckTCP::_setTimedDataState(bool timedActivated, unsigned long timedInterval_ms)
{
  _timedActivated = timedActivated;

  if (_timedActivated)
  {
    _timedSetPoint_ms = timedInterval_ms;
    _timedInterval_ms = timedInterval_ms;
    _timedFirstTime = true;
  }
}

void BlaeckTCP::setIntervalMs(long interval_ms)
{
  if (interval_ms >= 0)
  {
    _fixedInterval_ms = interval_ms;
    this->_setTimedDataState(true, (unsigned long)interval_ms);
  }
  else if (interval_ms == BLAECK_INTERVAL_OFF)
  {
    _fixedInterval_ms = BLAECK_INTERVAL_OFF;
    this->_setTimedDataState(false, _timedInterval_ms);
  }
  else if (interval_ms == BLAECK_INTERVAL_CLIENT)
  {
    _fixedInterval_ms = BLAECK_INTERVAL_CLIENT;
  }
  else if (StreamRef != nullptr)
  {
    StreamRef->print("Invalid interval mode: ");
    StreamRef->println(interval_ms);
  }
}

void BlaeckTCP::writeSymbols()
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connection.connected())
    {
      this->writeSymbols(1, client);
    }
  }
}

void BlaeckTCP::writeSymbols(unsigned long msg_id)
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connection.connected())
    {
      this->writeSymbols(msg_id, client);
    }
  }
}

void BlaeckTCP::writeSymbols(unsigned long msg_id, byte i)
{
  Clients[i].connection.write("<BLAECK:");
  byte msg_key = 0xB0;
  Clients[i].connection.write(msg_key);
  Clients[i].connection.write(":");
  ulngCvt.val = msg_id;
  Clients[i].connection.write(ulngCvt.bval, 4);
  Clients[i].connection.write(":");

  for (int j = 0; j < _signalIndex; j++)
  {
    Clients[i].connection.write((byte)0);
    Clients[i].connection.write((byte)0);

    Signal signal = Signals[j];
    Clients[i].connection.print(signal.SignalName);
    Clients[i].connection.print('\0');

    switch (signal.DataType)
    {
    case (Blaeck_bool):
    {
      Clients[i].connection.write((byte)0x0);
      break;
    }
    case (Blaeck_byte):
    {
      Clients[i].connection.write(0x1);
      break;
    }
    case (Blaeck_short):
    {
      Clients[i].connection.write(0x2);
      break;
    }
    case (Blaeck_ushort):
    {
      Clients[i].connection.write(0x3);
      break;
    }
    case (Blaeck_int):
    {
      Clients[i].connection.write(0x4);
      break;
    }
    case (Blaeck_uint):
    {
      Clients[i].connection.write(0x5);
      break;
    }
    case (Blaeck_long):
    {
      Clients[i].connection.write(0x6);
      break;
    }
    case (Blaeck_ulong):
    {
      Clients[i].connection.write(0x7);
      break;
    }
    case (Blaeck_float):
    {
      Clients[i].connection.write(0x8);
      break;
    }
    case (Blaeck_double):
    {
      Clients[i].connection.write(0x9);
      break;
    }
    case (Blaeck_string):
    {
      Clients[i].connection.write(0xA);
      break;
    }
    }
  }

  Clients[i].connection.write("/BLAECK>");
  Clients[i].connection.write("\r\n");
}

#if BLAECK_ENABLE_COMMAND_META
void BlaeckTCP::writeCommands()
{
  this->writeCommands(1);
}

void BlaeckTCP::writeCommands(unsigned long msg_id)
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connection.connected())
    {
      this->writeCommands(msg_id, client);
    }
  }
}

void BlaeckTCP::writeCommands(unsigned long msg_id, byte i)
{
  // 0xE0 "Command List" frame. Per discovered command entry:
  //   msConfig(1) slaveID(1) name\0 kind(1) flags(1)
  //   [min(4) max(4) step(4)]  if flags.hasRange   (LE float)
  //   [unit\0]                 if flags.hasUnit
  //   [optionsCsv\0]           if flags.hasOptions
  //   [stateSignal\0]          if flags.hasStateSignal
  //   [maxLen(2)]              if flags.hasTextMax    (LE uint16)
  // flags bits: 0=hasRange 1=hasUnit 2=hasOptions 3=hasStateSignal 4=hasTextMax
  // All in-use entries are emitted, including plain onCommand() entries
  // (kind=BLAECK_CMD_PLAIN, flags=0, no trailing metadata). Plain entries carry
  // no Home Assistant entity, but are listed so a host can build a full command
  // palette / autocomplete of every command the device accepts.
  // TCP is always a single server device: msConfig and slaveID are hardcoded 0.
  Clients[i].connection.write("<BLAECK:");
  byte msg_key = 0xE0;
  Clients[i].connection.write(msg_key);
  Clients[i].connection.write(":");
  ulngCvt.val = msg_id;
  Clients[i].connection.write(ulngCvt.bval, 4);
  Clients[i].connection.write(":");

  for (byte j = 0; j < MAX_COMMAND_HANDLERS; j++)
  {
    CommandHandlerEntry &e = _commandHandlers[j];
    if (!e.inUse)
      continue;

    byte flags = 0;
    if (e.kind == BLAECK_CMD_NUMBER)
      flags |= 0x01;
    if (e.unit != nullptr)
      flags |= 0x02;
    if (e.kind == BLAECK_CMD_SELECT && e.options != nullptr)
      flags |= 0x04;
    if (e.stateSignal != nullptr)
      flags |= 0x08;
    if (e.kind == BLAECK_CMD_TEXT)
      flags |= 0x10;

    Clients[i].connection.write((byte)0); // msConfig
    Clients[i].connection.write((byte)0); // slaveID
    Clients[i].connection.print(e.command);
    Clients[i].connection.write((byte)0);
    Clients[i].connection.write(e.kind);
    Clients[i].connection.write(flags);

    if (flags & 0x01)
    {
      fltCvt.val = e.meta_min;
      Clients[i].connection.write(fltCvt.bval, 4);
      fltCvt.val = e.meta_max;
      Clients[i].connection.write(fltCvt.bval, 4);
      fltCvt.val = e.meta_step;
      Clients[i].connection.write(fltCvt.bval, 4);
    }
    if (flags & 0x02)
    {
      Clients[i].connection.print(e.unit);
      Clients[i].connection.write((byte)0);
    }
    if (flags & 0x04)
    {
      Clients[i].connection.print(e.options);
      Clients[i].connection.write((byte)0);
    }
    if (flags & 0x08)
    {
      Clients[i].connection.print(e.stateSignal);
      Clients[i].connection.write((byte)0);
    }
    if (flags & 0x10)
    {
      uint16_t maxLen = (uint16_t)e.meta_max;
      Clients[i].connection.write((byte)(maxLen & 0xFF));
      Clients[i].connection.write((byte)((maxLen >> 8) & 0xFF));
    }
  }

  Clients[i].connection.write("/BLAECK>");
  Clients[i].connection.write("\r\n");
}
#endif

void BlaeckTCP::writeMessage(const char *channelName, const char *text)
{
  this->writeMessage(channelName, text, _messageMsgId++);
}

void BlaeckTCP::writeMessage(const char *channelName, const char *text, unsigned long messageID)
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connection.connected())
    {
      this->writeMessage(channelName, text, messageID, client);
    }
  }
}

void BlaeckTCP::writeMessage(const char *channelName, const char *text, unsigned long messageID, byte i)
{
  // 0x90 "Message" frame: a named free-text status/log channel, device -> host.
  //   name\0  length(2, LE uint16)  text[length]
  // No CRC (like the 0xE0/0xF0 frames). The host may surface it (e.g. an
  // auto-created Home Assistant text sensor per channel); it is never treated as
  // signal/telemetry data and is not stored in the database.
  if (channelName == nullptr)
    channelName = "";
  if (text == nullptr)
    text = "";

  // 2-byte length prefix caps a single message at 65535 bytes. Widen before
  // comparing: size_t is 16 bit on AVR, where `len > 65535` can never be true
  // and warns under -Wtype-limits.
  uint32_t rawLen = (uint32_t)strlen(text);
  uint16_t len = (rawLen > 0xFFFFu) ? (uint16_t)0xFFFFu : (uint16_t)rawLen;

  Clients[i].connection.write("<BLAECK:");
  byte msg_key = 0x90;
  Clients[i].connection.write(msg_key);
  Clients[i].connection.write(":");
  ulngCvt.val = messageID;
  Clients[i].connection.write(ulngCvt.bval, 4);
  Clients[i].connection.write(":");

  // Channel name (NUL-terminated), then the UTF-8 text length-prefixed (LE uint16).
  Clients[i].connection.print(channelName);
  Clients[i].connection.write((byte)0);
  Clients[i].connection.write((byte)(len & 0xFF));
  Clients[i].connection.write((byte)((len >> 8) & 0xFF));
  Clients[i].connection.write((const uint8_t *)text, len);

  Clients[i].connection.write("/BLAECK>");
  Clients[i].connection.write("\r\n");
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

void BlaeckTCP::write(String signalName, bool value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, byte value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, short value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, unsigned short value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, int value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, unsigned int value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, long value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, unsigned long value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, float value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}
void BlaeckTCP::write(String signalName, double value, unsigned long messageID, unsigned long long timestamp)
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

void BlaeckTCP::write(int signalIndex, bool value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_bool)
    {
      *((bool *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, byte value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_byte)
    {
      *((byte *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, short value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_short)
    {
      *((short *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, unsigned short value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_ushort)
    {
      *((unsigned short *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, int value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, int stays as Blaeck_int (2 bytes)
    if (Signals[signalIndex].DataType == Blaeck_int)
    {
      *((int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
#else
    // On 32-bit platforms, int is mapped to Blaeck_long (4 bytes)
    if (Signals[signalIndex].DataType == Blaeck_long)
    {
      *((int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
#endif
  }
}

void BlaeckTCP::write(int signalIndex, unsigned int value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, int stays as Blaeck_int (2 bytes)
    if (Signals[signalIndex].DataType == Blaeck_uint)
    {
      *((unsigned int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
#else
    // On 32-bit platforms, int is mapped to Blaeck_long (4 bytes)
    if (Signals[signalIndex].DataType == Blaeck_ulong)
    {
      *((unsigned int *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
#endif
  }
}

void BlaeckTCP::write(int signalIndex, long value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_long)
    {
      *((long *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, unsigned long value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_ulong)
    {
      *((unsigned long *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, float value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_float)
    {
      *((float *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
  }
}

void BlaeckTCP::write(int signalIndex, double value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
#ifdef __AVR__
    // On AVR, double is same as float
    if (Signals[signalIndex].DataType == Blaeck_float)
    {
      *((float *)Signals[signalIndex].Address) = (float)value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
#else
    if (Signals[signalIndex].DataType == Blaeck_double)
    {
      *((double *)Signals[signalIndex].Address) = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
#endif
  }
}

// --- String signal (char*) overloads ---
void BlaeckTCP::write(String signalName, char *value)
{
  this->write(signalName, value, 1);
}
void BlaeckTCP::write(String signalName, char *value, unsigned long messageID)
{
  this->write(signalName, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(String signalName, char *value, unsigned long messageID, unsigned long long timestamp)
{
  int index = findSignalIndex(signalName);
  if (index >= 0)
  {
    this->write(index, value, messageID, timestamp);
  }
}

void BlaeckTCP::write(int signalIndex, char *value)
{
  this->write(signalIndex, value, 1);
}
void BlaeckTCP::write(int signalIndex, char *value, unsigned long messageID)
{
  this->write(signalIndex, value, messageID, getTimeStamp());
}
void BlaeckTCP::write(int signalIndex, char *value, unsigned long messageID, unsigned long long timestamp)
{
  if (signalIndex >= 0 && signalIndex < _signalIndex)
  {
    if (Signals[signalIndex].DataType == Blaeck_string)
    {
      // String values live in a user-owned buffer; repoint Address like addSignal(char*).
      Signals[signalIndex].Address = value;

      for (byte client = 0; client < _maxClients; client++)
        if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
        {
          this->writeData(messageID, client, signalIndex, signalIndex, false, timestamp);
        }
      _sendRestartFlag = false;
    }
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

void BlaeckTCP::writeAllData(unsigned long msg_id, unsigned long long timestamp)
{
  bool dataSent = false;
  for (byte client = 0; client < _maxClients; client++)
    if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      this->writeData(msg_id, client, 0, _signalIndex - 1, false, timestamp);
      dataSent = true;
    }
  if (dataSent)
    _sendRestartFlag = false;
}

void BlaeckTCP::writeUpdatedData()
{
  this->writeUpdatedData(1);
}

void BlaeckTCP::writeUpdatedData(unsigned long msg_id)
{
  this->writeUpdatedData(msg_id, getTimeStamp());
}

void BlaeckTCP::writeUpdatedData(unsigned long messageID, unsigned long long timestamp)
{
  bool dataSent = false;
  for (byte client = 0; client < _maxClients; client++)
    if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
    {
      this->writeData(messageID, client, 0, _signalIndex - 1, true, timestamp);
      dataSent = true;
    }
  if (dataSent)
  {
    _sendRestartFlag = false;
    clearAllUpdateFlags();
  }
}

void BlaeckTCP::writeData(unsigned long msg_id, byte i, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long long timestamp)
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

  Clients[i].connection.write("<BLAECK:");

  // Message Key
  byte msg_key = 0xD2;
  Clients[i].connection.write(msg_key);
  _crc.add(msg_key);

  Clients[i].connection.write(":");
  _crc.add(':');

  // Message Id
  ulngCvt.val = msg_id;
  Clients[i].connection.write(ulngCvt.bval, 4);
  _crc.add(ulngCvt.bval, 4);

  Clients[i].connection.write(":");
  _crc.add(':');

  // Restart flag
  byte restart_flag = _sendRestartFlag ? 1 : 0;
  Clients[i].connection.write(restart_flag);
  _crc.add(restart_flag);

  Clients[i].connection.write(":");
  _crc.add(':');

  // Schema hash (2 bytes, CRC16-CCITT, little-endian)
  byte hash_lo = (byte)(_schemaHash & 0xFF);
  byte hash_hi = (byte)((_schemaHash >> 8) & 0xFF);
  Clients[i].connection.write(hash_lo);
  Clients[i].connection.write(hash_hi);
  _crc.add(hash_lo);
  _crc.add(hash_hi);

  Clients[i].connection.write(":");
  _crc.add(':');

  // Timestamp mode
  byte timestamp_mode = (byte)_timestampMode;
  Clients[i].connection.write(timestamp_mode);
  _crc.add(timestamp_mode);

  // Add timestamp data if mode is not NO_TIMESTAMP
  if (_timestampMode != BLAECK_NO_TIMESTAMP && hasValidTimestampCallback())
  {
    ullCvt.val = timestamp;
    Clients[i].connection.write(ullCvt.bval, 8);
    _crc.add(ullCvt.bval, 8);
  }

  Clients[i].connection.write(":");
  _crc.add(':');

  for (int j = signalIndex_start; j <= signalIndex_end; j++)
  {
    // Skip if onlyUpdated is true and signal is not updated
    if (onlyUpdated && !Signals[j].Updated)
      continue;

    intCvt.val = j;
    Clients[i].connection.write(intCvt.bval, 2);
    _crc.add(intCvt.bval, 2);

    Signal signal = Signals[j];
    switch (signal.DataType)
    {
    case (Blaeck_bool):
    {
      boolCvt.val = *((bool *)signal.Address);
      Clients[i].connection.write(boolCvt.bval, 1);
      _crc.add(boolCvt.bval, 1);
    }
    break;
    case (Blaeck_byte):
    {
      Clients[i].connection.write(*((byte *)signal.Address));
      _crc.add(*((byte *)signal.Address));
    }
    break;
    case (Blaeck_short):
    {
      shortCvt.val = *((short *)signal.Address);
      Clients[i].connection.write(shortCvt.bval, 2);
      _crc.add(shortCvt.bval, 2);
    }
    break;
    case (Blaeck_ushort):
    {
      ushortCvt.val = *((unsigned short *)signal.Address);
      Clients[i].connection.write(ushortCvt.bval, 2);
      _crc.add(ushortCvt.bval, 2);
    }
    break;
    case (Blaeck_int):
    {
      intCvt.val = *((int *)signal.Address);
      Clients[i].connection.write(intCvt.bval, 2);
      _crc.add(intCvt.bval, 2);
    }
    break;
    case (Blaeck_uint):
    {
      uintCvt.val = *((unsigned int *)signal.Address);
      Clients[i].connection.write(uintCvt.bval, 2);
      _crc.add(uintCvt.bval, 2);
    }
    break;
    case (Blaeck_long):
    {
      lngCvt.val = *((long *)signal.Address);
      Clients[i].connection.write(lngCvt.bval, 4);
      _crc.add(lngCvt.bval, 4);
    }
    break;
    case (Blaeck_ulong):
    {
      ulngCvt.val = *((unsigned long *)signal.Address);
      Clients[i].connection.write(ulngCvt.bval, 4);
      _crc.add(ulngCvt.bval, 4);
    }
    break;
    case (Blaeck_float):
    {
      fltCvt.val = *((float *)signal.Address);
      Clients[i].connection.write(fltCvt.bval, 4);
      _crc.add(fltCvt.bval, 4);
    }
    break;
    case (Blaeck_double):
    {
      dblCvt.val = *((double *)signal.Address);
      Clients[i].connection.write(dblCvt.bval, 8);
      _crc.add(dblCvt.bval, 8);
    }
    break;
    case (Blaeck_string):
    {
      // Wire layout: 1-byte length (capped at 255) followed by that many
      // characters. A null Address is treated as an empty string.
      const char *str = (const char *)signal.Address;
      size_t rawLen = (str != nullptr) ? strlen(str) : 0;
      byte len = (rawLen > 255) ? 255 : (byte)rawLen;
      Clients[i].connection.write(len);
      _crc.add(len);
      if (len > 0)
      {
        Clients[i].connection.write((const uint8_t *)str, len);
        _crc.add((uint8_t *)str, len);
      }
    }
    break;
    }

    // Updated flags are cleared by the caller after all clients are served
  }

  // D2 tail: StatusByte + StatusPayload(4) + CRC32(4)
  byte statusByte = 0;
  byte statusPayload[4] = {0, 0, 0, 0};
  Clients[i].connection.write(statusByte);
  Clients[i].connection.write(statusPayload, 4);
  _crc.add(statusByte);
  _crc.add(statusPayload, 4);

  uint32_t crc_value = _crc.calc();
  Clients[i].connection.write((byte *)&crc_value, 4);

  Clients[i].connection.write("/BLAECK>");
  Clients[i].connection.write("\r\n");
}

void BlaeckTCP::timedWriteAllData()
{
  unsigned long id = (_fixedInterval_ms >= 0) ? 185273100 : 185273099;
  this->timedWriteAllData(id);
}

void BlaeckTCP::timedWriteAllData(unsigned long msg_id)
{
  this->timedWriteAllData(msg_id, getTimeStamp());
}

void BlaeckTCP::timedWriteAllData(unsigned long msg_id, unsigned long long timestamp)
{
  this->timedWriteData(msg_id, 0, _signalIndex - 1, false, timestamp);
}

void BlaeckTCP::timedWriteUpdatedData()
{
  unsigned long id = (_fixedInterval_ms >= 0) ? 185273100 : 185273099;
  this->timedWriteUpdatedData(id);
}

void BlaeckTCP::timedWriteUpdatedData(unsigned long msg_id)
{
  this->timedWriteUpdatedData(msg_id, getTimeStamp());
}

void BlaeckTCP::timedWriteUpdatedData(unsigned long msg_id, unsigned long long timestamp)
{
  this->timedWriteData(msg_id, 0, _signalIndex - 1, true, timestamp);
}

void BlaeckTCP::timedWriteData(unsigned long msg_id, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long long timestamp)
{
  if (_timedFirstTime == true)
    _timedFirstTimeDone_ms = millis();
  unsigned long _timedElapsedTime_ms = (millis() - _timedFirstTimeDone_ms);

  if (((_timedElapsedTime_ms >= _timedSetPoint_ms) || _timedFirstTime == true) && _timedActivated == true)
  {
    if (_timedFirstTime == false)
    {
      if (_timedInterval_ms > 0)
      {
        while (_timedSetPoint_ms <= _timedElapsedTime_ms)
          _timedSetPoint_ms += _timedInterval_ms;
      }
    }
    _timedFirstTime = false;

    bool dataSent = false;
    for (byte client = 0; client < _maxClients; client++)
      if (Clients[client].connection.connected() && bitRead(_blaeckWriteDataClientMask, client) == 1)
      {
        this->writeData(msg_id, client, signalIndex_start, signalIndex_end, onlyUpdated, timestamp);
        dataSent = true;
      }
    if (dataSent)
    {
      _sendRestartFlag = false;
      if (onlyUpdated)
        clearAllUpdateFlags();
    }
  }
}

void BlaeckTCP::writeDevices()
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connection.connected())
    {
      this->writeDevices(1, client);
    }
  }
  _serverRestarted = false;
}

void BlaeckTCP::writeDevices(unsigned long msg_id)
{
  for (byte client = 0; client < _maxClients; client++)
  {
    if (Clients[client].connection.connected())
    {
      this->writeDevices(msg_id, client);
    }
  }
  _serverRestarted = false;
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

  Clients[i].connection.write("<BLAECK:");
  byte msg_key = 0xB6;
  Clients[i].connection.write(msg_key);
  Clients[i].connection.write(":");
  ulngCvt.val = msg_id;
  Clients[i].connection.write(ulngCvt.bval, 4);
  Clients[i].connection.write(":");
  // DeviceCount = 1
  Clients[i].connection.write((byte)1);
  // Device entry
  Clients[i].connection.write((byte)0);
  Clients[i].connection.write((byte)0);
  Clients[i].connection.print(deviceName);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(deviceHWVersion);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(deviceFWVersion);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(BLAECKTCP_VERSION);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(BLAECKTCP_NAME);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(_serverRestarted);
  Clients[i].connection.print('\0');
  Clients[i].connection.print("server");
  Clients[i].connection.print('\0');
  Clients[i].connection.print("0");
  Clients[i].connection.print('\0');
  // Client trailer
  Clients[i].connection.print(clientNo);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(clientDataEnabled);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(Clients[i].name);
  Clients[i].connection.print('\0');
  Clients[i].connection.print(Clients[i].type);
  Clients[i].connection.print('\0');
  Clients[i].connection.write("/BLAECK>");
  Clients[i].connection.write("\r\n");
}

void BlaeckTCP::tickUpdated()
{
  unsigned long id = (_fixedInterval_ms >= 0) ? 185273100 : 185273099;
  this->tickUpdated(id);
}

void BlaeckTCP::tickUpdated(unsigned long msg_id)
{
  this->tick(msg_id, true);
}

void BlaeckTCP::tick()
{
  unsigned long id = (_fixedInterval_ms >= 0) ? 185273100 : 185273099;
  this->tick(id);
}

void BlaeckTCP::tick(unsigned long msg_id)
{
  this->tick(msg_id, false);
}

void BlaeckTCP::tick(unsigned long msg_id, bool onlyUpdated)
{
  this->read();
  this->timedWriteData(msg_id, 0, _signalIndex - 1, onlyUpdated, getTimeStamp());
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

  // Reset overflow tracking
  _prevMicros = 0;
  _overflowCount = 0;

  // Set default callbacks for built-in modes
  switch (mode)
  {
  case BLAECK_MICROS:
    _timestampCallback = _microsWrapper;
    break;
  case BLAECK_UNIX:
    // User must provide Unix time callback - don't override if already set
    if (_timestampCallback == _microsWrapper)
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

void BlaeckTCP::setTimestampCallback(unsigned long long (*callback)())
{
  _timestampCallback = callback;
}

bool BlaeckTCP::hasValidTimestampCallback() const
{
  return (_timestampMode != BLAECK_NO_TIMESTAMP && _timestampCallback != nullptr);
}

unsigned long long BlaeckTCP::getTimeStamp()
{
  unsigned long long timestamp = 0;

  if (_timestampMode != BLAECK_NO_TIMESTAMP && hasValidTimestampCallback())
  {
    if (_timestampMode == BLAECK_MICROS)
    {
      // Track micros() overflow: uint32 wraps every ~71 minutes
      unsigned long raw = (unsigned long)_timestampCallback();
      if (raw < _prevMicros)
      {
        _overflowCount++;
      }
      _prevMicros = raw;
      timestamp = (_overflowCount * 4294967296ULL) + raw;
    }
    else if (_timestampMode == BLAECK_UNIX)
    {
      // Callback returns microseconds since Unix epoch directly
      timestamp = _timestampCallback();
    }
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
