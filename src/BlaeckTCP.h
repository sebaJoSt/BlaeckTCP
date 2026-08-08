/*
        File: BlaeckTCP.h
        Author: Sebastian Strobl
*/

#ifndef BLAECKTCP_H
#define BLAECKTCP_H

#define BLAECKTCP_VERSION "7.0.0"
#define BLAECKTCP_VERSION_MAJOR 7
#define BLAECKTCP_VERSION_MINOR 0
#define BLAECKTCP_VERSION_PATCH 0
#define BLAECKTCP_NAME "BlaeckTCP"

// Must come before the compile-time defaults below: the AVR handler-limit
// gate tests RAMEND, which only exists once <avr/io.h> has been pulled in via
// Arduino.h. Including it later let the gate resolve differently in the
// library and sketch translation units, giving the class two layouts (ODR).
#include <Arduino.h>

// Allow user overrides of the defaults below, e.g.:
//   #define BLAECK_COMMAND_MAX_CHARS_DEFAULT 128
//
// IMPORTANT: an override MUST reach every translation unit - this sketch AND
// BlaeckTCP.cpp. The values below size members of class BlaeckTCP, so a
// setting seen by only one of them gives the class two different layouts
// (an ODR violation) and corrupts memory silently. All-or-nothing, never half.
//
//   PlatformIO
//     build_flags = -DBLAECK_COMMAND_MAX_CHARS_DEFAULT=128
//     Reaches every unit. Nothing else to do.
//
//   Arduino IDE / arduino-cli
//     A BlaeckTCPConfig.h in your sketch folder is NOT found by default:
//     the sketch folder is not on the compiler's include path, so the
//     __has_include below fails and your settings are silently ignored.
//     There is no IDE preference and no sketch.yaml key for compiler flags;
//     only the core's own config files can add them. Two ways round it:
//
//     a) Simplest, no setup: put the config next to this header, at
//        libraries/BlaeckTCP/src/BlaeckTCPConfig.h. That folder is already
//        on the include path, so every unit sees it. The catch is that a
//        library update overwrites it.
//
//     b) Keep it per sketch: put the sketch folder on the include path via
//        platform.local.txt next to platform.txt in your core (or
//        boards.local.txt to scope it to one board), containing:
//
//          build.extra_flags=-I{build.source.path}
//
//        e.g. ...\packages\arduino\hardware\avr\1.8.8\platform.local.txt
//        This is per core - repeat it for esp32, samd, renesas_uno, ...
//        With arduino-cli you can pass it per build instead:
//          --build-property "build.extra_flags=-I{build.source.path}"
//
// If you are unsure whether your override took effect, compare
// configFingerprint() against BLAECK_CONFIG_FINGERPRINT from your sketch;
// see the note further down.
#if defined __has_include
  #if __has_include(<BlaeckTCPConfig.h>)
    #include <BlaeckTCPConfig.h>
  #endif
#endif

#ifndef BLAECK_BUFFER_SIZE
  #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    #define BLAECK_BUFFER_SIZE 1024
  #elif defined(ARDUINO_ARCH_AVR)
    #define BLAECK_BUFFER_SIZE 32
  #else
    #define BLAECK_BUFFER_SIZE 64
  #endif
#endif

#ifndef BLAECK_COMMAND_MAX_CHARS_DEFAULT
  #if defined(__AVR__)
    #define BLAECK_COMMAND_MAX_CHARS_DEFAULT 48
  #else
    #define BLAECK_COMMAND_MAX_CHARS_DEFAULT 96
  #endif
#endif

#ifndef BLAECK_COMMAND_MAX_HANDLERS_DEFAULT
  #if defined(__AVR__)
    // Scale with available SRAM: each handler entry costs roughly
    // MAX_COMMAND_NAME_COUNT + a function pointer (~28 bytes on AVR).
    // Larger-SRAM AVRs (Mega 2560, ATmega1284, ...) get a generous limit;
    // small ones (Uno/Nano/Leonardo) get a modest one to conserve SRAM.
    #if defined(RAMEND) && (RAMEND >= 0x10FF)
      #define BLAECK_COMMAND_MAX_HANDLERS_DEFAULT 12
    #else
      #define BLAECK_COMMAND_MAX_HANDLERS_DEFAULT 6
    #endif
  #else
    #define BLAECK_COMMAND_MAX_HANDLERS_DEFAULT 12
  #endif
#endif

#ifndef BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT
  #if defined(__AVR__)
    #define BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT 24
  #else
    #define BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT 40
  #endif
#endif

#ifndef BLAECK_COMMAND_MAX_PARAMS_DEFAULT
  #define BLAECK_COMMAND_MAX_PARAMS_DEFAULT 10
#endif

// Command metadata (Home Assistant discovery catalog).
// When ON, the typed command registration helpers (onNumberCommand/
// onSwitchCommand/onSelectCommand/onButtonCommand) store parameter metadata and
// the device can emit a 0xE0 "Command List" frame in response to
// BLAECK.WRITE_COMMANDS. Turn OFF to save flash on tiny targets; the typed
// helpers then behave exactly like plain onCommand() (no metadata, no 0xE0).
// Override via BlaeckTCPConfig.h or build flag.
#ifndef BLAECK_ENABLE_COMMAND_META
  #define BLAECK_ENABLE_COMMAND_META 1
#endif

// Fingerprint of every setting above that sizes a member of class BlaeckTCP.
// Two translation units that disagree here disagree on the class layout, which
// is undefined behaviour. Use it to verify an override actually reached the
// library; see configFingerprint() / configMatchesLibrary().
#define BLAECK_CONFIG_FINGERPRINT                       \
  ((unsigned long)(BLAECK_COMMAND_MAX_CHARS_DEFAULT) * 1000003UL +      \
   (unsigned long)(BLAECK_COMMAND_MAX_HANDLERS_DEFAULT) * 10007UL +     \
   (unsigned long)(BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT) * 101UL +     \
   (unsigned long)(BLAECK_COMMAND_MAX_PARAMS_DEFAULT) * 7UL +           \
   (unsigned long)(BLAECK_ENABLE_COMMAND_META))

// Disable Nagle's algorithm for lower latency on ESP32/ESP8266.
// Set to false in BlaeckTCPConfig.h if you prefer throughput over latency.
#ifndef BLAECK_TCP_NO_DELAY_DEFAULT
  #define BLAECK_TCP_NO_DELAY_DEFAULT true
#endif

#include <TelnetPrint.h>
#include <CRC.h>

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
  Blaeck_double,
  Blaeck_string
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
  BLAECK_UNIX = 2,
  BLAECK_RTC = BLAECK_UNIX // Deprecated alias
};

enum BlaeckIntervalMode
{
  BLAECK_INTERVAL_CLIENT = -1,
  BLAECK_INTERVAL_OFF = -2
};

struct BlaeckClient {
    NetClient connection;
    char name[20];
    char type[8];
};

typedef void (*BlaeckCommandHandler)(const char *command, const char *const *params, byte paramCount);
typedef void (*BlaeckAnyCommandHandler)(const char *command, const char *const *params, byte paramCount);

// Command kind for Home Assistant discovery (0xE0 Command List frame).
enum BlaeckCommandKind
{
  BLAECK_CMD_PLAIN = 0,  // registered via onCommand(): no HA entity, but listed in 0xE0 for command palettes
  BLAECK_CMD_NUMBER = 1, // HA number   (value in [min,max])
  BLAECK_CMD_SWITCH = 2, // HA switch   (0/1)
  BLAECK_CMD_SELECT = 3, // HA select   (index into optionsCsv)
  BLAECK_CMD_BUTTON = 4, // HA button   (no value)
  BLAECK_CMD_TEXT = 5    // HA text     (free text, percent-encoded on the wire)
};

// Acknowledgement reason for the 0xF0 Command Ack frame. Sent back to the
// commanding client after a command is dispatched so a host can confirm receipt
// and surface accept/reject feedback. status = 0 accepted, 1 rejected.
enum BlaeckCommandAckReason
{
  BLAECK_ACK_OK = 0,           // accepted: delivered to a handler, validation passed
  BLAECK_ACK_UNKNOWN = 1,      // rejected: no handler registered for this command
  BLAECK_ACK_OUT_OF_RANGE = 2, // rejected: number outside [min, max]
  BLAECK_ACK_BAD_SWITCH = 3,   // rejected: switch value not 0/1
  BLAECK_ACK_BAD_SELECT = 4,   // rejected: select value not a valid index/option
  BLAECK_ACK_TOO_LONG = 5      // rejected: text value longer than the advertised max length
};

class BlaeckTCP
{
public:
  // ----- Constructor -----
  BlaeckTCP();

  // ----- Destructor -----
  ~BlaeckTCP();

  // ----- Config consistency -----
  // configFingerprint() returns BLAECK_CONFIG_FINGERPRINT as BlaeckTCP.cpp was
  // compiled. configMatchesLibrary() compares it against the value seen where
  // you call it: the default argument is evaluated at the call site, so calling
  // it from your sketch compares your settings against the library's.
  //
  // Returns false when a config override reached only one of them - the class
  // then has two layouts and the program is already in undefined behaviour.
  // Worth a guard in setup() if you override any of the settings:
  //
  //   if (!BlaeckTCP.configMatchesLibrary()) { /* halt, log, blink */ }
  //
  // It cannot be checked automatically: the constructor lives in the .cpp and
  // so only ever sees the library's own values.
  unsigned long configFingerprint() const;
  bool configMatchesLibrary(unsigned long sketchFingerprint = BLAECK_CONFIG_FINGERPRINT) const;

  // ----- Initialize ----
  void begin(Stream *streamRef, unsigned int size, uint16_t port);
  void begin(byte maxClients, Stream *streamRef, unsigned int size, uint16_t port);
  void begin(byte maxClients, Stream *streamRef, unsigned int size, int blaeckWriteDataClientMask, uint16_t port);
  void beginBridge(byte maxClients, Stream *streamRef, Stream *bridgeStream, uint16_t port);

  // Set these variables in your Arduino sketch
  String DeviceName;
  String DeviceHWVersion;
  String DeviceFWVersion;

  BlaeckClient *Clients = nullptr;
  // CommandingClient is the client which sent the parsed command
  NetClient CommandingClient;

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
  // String signal: value points to a user-owned, null-terminated char buffer.
  // The buffer is read (not copied) at transmit time; keep it valid and updated
  // in place. Emitted on the wire as a 1-byte length (capped at 255) + bytes,
  // so keep strings short - especially on RAM-constrained targets.
  void addSignal(String signalName, char *value);

  // Delete all Signals
  void deleteSignals();
  bool hasSignalOverflow() const { return _signalOverflowOccurred; }
  uint16_t getSignalOverflowCount() const { return _signalOverflowCount; }

  // Signal Count
  int SignalCount;

  // ----- Devices -----
  void writeDevices();
  void writeDevices(unsigned long messageID);

  // ----- Symbols -----
  void writeSymbols();
  void writeSymbols(unsigned long messageID);

#if BLAECK_ENABLE_COMMAND_META
  // ----- Commands (Home Assistant discovery catalog, 0xE0) -----
  void writeCommands();
  void writeCommands(unsigned long messageID);
#endif

  // ----- Messages (Home Assistant text/log channel, 0x90) -----
  // Send a free-text status/log message on a named channel to every connected
  // client. Fire-and-forget: a host may surface it (e.g. a Home Assistant text
  // sensor auto-created per channel name) but it is never stored as signal data.
  // The frame carries no CRC (like the 0xE0/0xF0 frames). Text longer than
  // 65535 bytes is truncated.
  void writeMessage(const char *channelName, const char *text);
  void writeMessage(const char *channelName, const char *text, unsigned long messageID);

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
  void write(String signalName, char *value);

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
  void write(String signalName, char *value, unsigned long messageID);

  void write(String signalName, bool value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, byte value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, short value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, unsigned short value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, int value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, unsigned int value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, long value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, unsigned long value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, float value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, double value, unsigned long messageID, unsigned long long timestamp);
  void write(String signalName, char *value, unsigned long messageID, unsigned long long timestamp);

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
  void write(int signalIndex, char *value);

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
  void write(int signalIndex, char *value, unsigned long messageID);

  void write(int signalIndex, bool value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, byte value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, short value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, unsigned short value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, int value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, unsigned int value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, long value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, unsigned long value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, float value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, double value, unsigned long messageID, unsigned long long timestamp);
  void write(int signalIndex, char *value, unsigned long messageID, unsigned long long timestamp);

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
  void writeAllData(unsigned long messageID, unsigned long long timestamp);
  void timedWriteAllData();
  void timedWriteAllData(unsigned long messageID);
  void timedWriteAllData(unsigned long messageID, unsigned long long timestamp);

  // ----- Data Write Updated -----
  void writeUpdatedData();
  void writeUpdatedData(unsigned long messageID);
  void writeUpdatedData(unsigned long messageID, unsigned long long timestamp);
  void timedWriteUpdatedData();
  void timedWriteUpdatedData(unsigned long messageID);
  void timedWriteUpdatedData(unsigned long messageID, unsigned long long timestamp);

  // ----- Tick -----
  void tick();
  void tick(unsigned long messageID);
  void tickUpdated();
  void tickUpdated(unsigned long messageID);

  // ----- Timed Data configuruation -----
  // interval_ms semantics:
  //   >= 0                    fixed interval lock in ms (ACTIVATE/DEACTIVATE ignored)
  //   BLAECK_INTERVAL_OFF     timed data locked off (ACTIVATE ignored)
  //   BLAECK_INTERVAL_CLIENT  client-controlled mode (default)
  // Invalid values are rejected and the previous mode remains active.
  void setIntervalMs(long interval_ms);
  long getIntervalMs() const { return _fixedInterval_ms; }

  // ----- Read  -----
  void read();

  // ----- Command callback  -----
  // Deprecated: use onCommand(...) / onAnyCommand(...)
  void setCommandCallback(void (*callback)(char *command, int *parameter, char *string_01));
  bool onCommand(const char *command, BlaeckCommandHandler handler);
  void onAnyCommand(BlaeckAnyCommandHandler handler);
  void clearAllCommandHandlers();

  // ----- Typed command registration (Home Assistant discovery metadata) -----
  // Same runtime behavior as onCommand(), but attach metadata so the device can
  // describe the command in a 0xE0 "Command List" frame (BLAECK.WRITE_COMMANDS).
  // stateSignal (nullable): name of the signal that mirrors this command's value
  // (closed-loop -> HA state_topic + logged); pass nullptr for an optimistic /
  // open-loop control. All metadata strings must be F()/PROGMEM literals with
  // program lifetime (stored as pointers, never copied).
  // Number values outside [min,max], bad select indices and non-0/1 switch
  // values are rejected (handler skipped) and reported on the debug stream.
  // step is HA display resolution only; the firmware does not round.
  bool onNumberCommand(const char *command, BlaeckCommandHandler handler,
                       const __FlashStringHelper *stateSignal,
                       float min, float max, float step,
                       const __FlashStringHelper *unit = nullptr);
  bool onSwitchCommand(const char *command, BlaeckCommandHandler handler,
                       const __FlashStringHelper *stateSignal);
  bool onSelectCommand(const char *command, BlaeckCommandHandler handler,
                       const __FlashStringHelper *stateSignal,
                       const __FlashStringHelper *optionsCsv);
  bool onButtonCommand(const char *command, BlaeckCommandHandler handler);
  // Registers a free-text command shown as a Home Assistant "text" entity.
  // The value travels percent-encoded on the wire (so it can carry commas,
  // angle brackets and non-ASCII); the library decodes it in place before the
  // handler runs, so the handler receives the raw UTF-8 text. maxLength is the
  // advertised limit (values longer than this are rejected).
  bool onTextCommand(const char *command, BlaeckCommandHandler handler,
                     const __FlashStringHelper *stateSignal,
                     unsigned int maxLength = 255);

  // ----- Before data write callback  -----
  void setBeforeWriteCallback(void (*callback)());
  void setClientConnectedCallback(void (*callback)(byte clientNo));
  void setClientDisconnectedCallback(void (*callback)(byte clientNo));
  bool isClientDataEnabled(byte clientNo) const;

  /**
  Handles bidirectional data transfer between TCP and UART interface. This function
  should be called in the main loop to maintain communication flow.
  Data received from TCP is forwarded to UART and responses are sent back.
  */
  void bridgePoll();

  // Timestamp configuration methods
  void setTimestampMode(BlaeckTimestampMode mode);
  void setTimestampCallback(unsigned long long (*callback)());
  BlaeckTimestampMode getTimestampMode() const { return _timestampMode; }
  bool hasValidTimestampCallback() const;

private:
  unsigned long long getTimeStamp();
  int findSignalIndex(String signalName);
  void setSignalName(int signalIndex, String signalName);
  void _setTimedDataState(bool timedActivated, unsigned long timedInterval_ms);
  void _parseCommandTokens(const char *raw);
  void _dispatchRegisteredHandlers();
  // Send a 0xF0 Command Ack frame (cmdHash + status + reason) to CommandingClient.
  void _writeCommandAck(const char *rawCommand, byte status, byte reasonCode);
  // FNV-1a 32-bit hash of a NUL-terminated string; correlation id for acks.
  static uint32_t _fnv1a32(const char *s);
  // Monotonic message id stamped into the 0xF0 ack frame header.
  unsigned long _commandAckMsgId = 0;

  // Send a 0x90 Message frame (channel name + length-prefixed UTF-8 text) to one client.
  void writeMessage(const char *channelName, const char *text, unsigned long messageID, byte client);
  // Monotonic message id stamped into the 0x90 message frame header.
  unsigned long _messageMsgId = 0;

  void timedWriteData(unsigned long msg_id, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long long timestamp);
  void tick(unsigned long messageID, bool onlyUpdated);

  void writeData(unsigned long msg_id, byte i, int signalIndex_start, int signalIndex_end, bool onlyUpdated, unsigned long long timestamp);

  void writeDevices(unsigned long messageID, byte client);

  void writeSymbols(unsigned long messageID, byte client);

#if BLAECK_ENABLE_COMMAND_META
  void writeCommands(unsigned long messageID, byte client);
  void _annotateCommand(const char *command, uint8_t kind,
                        const __FlashStringHelper *stateSignal,
                        float mn, float mx, float st,
                        const __FlashStringHelper *unit,
                        const __FlashStringHelper *options);
  byte _validateTypedCommand(byte handlerIndex);
  static byte _flashCsvOptionCount(const __FlashStringHelper *csv);
  static long _flashCsvIndexOf(const __FlashStringHelper *csv, const char *value);
  // Percent-decodes a command value (e.g. from a HA text entity) in place.
  // "%XX" triple -> the byte 0xXX; other characters are copied unchanged.
  static void _percentDecodeInPlace(char *s);
#endif

  uint16_t _computeSchemaHash();

  static void validatePlatformSizes();

  void _initClientMeta();
  void _parseClientIdentity(const char *raw);
  void _startServer(uint16_t port);

  Stream *StreamRef = nullptr;
  int _blaeckWriteDataClientMask;
  byte _maxClients = 0;
  Stream *BridgeStreamRef = nullptr;
  bool _bridgeMode = false;

  Signal *Signals = nullptr;
  int _signalIndex = 0;
  unsigned int _signalCapacity = 0;
  bool _signalOverflowOccurred = false;
  uint16_t _signalOverflowCount = 0;
  uint16_t _schemaHash = 0;

  bool _serverRestarted = true;
  bool _sendRestartFlag = true;

  // Micros overflow tracking for D2 (uint64 timestamp)
  unsigned long _prevMicros = 0;
  unsigned long long _overflowCount = 0;

  bool _timedActivated = false;
  bool _timedFirstTime = true;
  unsigned long _timedFirstTimeDone_ms = 0;
  unsigned long _timedSetPoint_ms = 0;
  unsigned long _timedInterval_ms = 1000;
  long _fixedInterval_ms = BLAECK_INTERVAL_CLIENT;

  static const int MAXIMUM_CHAR_COUNT = BLAECK_COMMAND_MAX_CHARS_DEFAULT;
  static const byte MAX_COMMAND_HANDLERS = BLAECK_COMMAND_MAX_HANDLERS_DEFAULT;
  static const byte MAX_COMMAND_PARAM_COUNT = BLAECK_COMMAND_MAX_PARAMS_DEFAULT;
  static const byte MAX_COMMAND_NAME_COUNT = BLAECK_COMMAND_MAX_NAME_CHARS_DEFAULT;
  char receivedChars[MAXIMUM_CHAR_COUNT];
  char COMMAND[MAXIMUM_CHAR_COUNT] = {0};
  int PARAMETER[10];
  // STRING_01: Max. 15 chars allowed  + Null Terminator '\0' = 16
  // In case more than 15 chars are sent, the rest is cut off in function void parseData()
  char STRING_01[16];

  CRC32 _crc;

  void (*_commandCallback)(char *command, int *parameter, char *string01) = nullptr;
  bool _commandCallbackDeprecationWarned = false;
  struct CommandHandlerEntry
  {
    char command[MAX_COMMAND_NAME_COUNT];
    BlaeckCommandHandler handler = nullptr;
    bool inUse = false;
#if BLAECK_ENABLE_COMMAND_META
    uint8_t kind = BLAECK_CMD_PLAIN;
    float meta_min = 0.0f;
    float meta_max = 0.0f;
    float meta_step = 0.0f;
    const __FlashStringHelper *unit = nullptr;
    const __FlashStringHelper *options = nullptr;
    const __FlashStringHelper *stateSignal = nullptr;
#endif
  };
  CommandHandlerEntry _commandHandlers[MAX_COMMAND_HANDLERS];
  BlaeckAnyCommandHandler _anyCommandHandler = nullptr;
  char _parsedTokenBuffer[MAXIMUM_CHAR_COUNT] = {0};
  char _parsedCommand[MAX_COMMAND_NAME_COUNT] = {0};
  const char *_parsedParamPtrs[MAX_COMMAND_PARAM_COUNT] = {0};
  byte _parsedParamCount = 0;
#if BLAECK_ENABLE_COMMAND_META
  // Scratch buffer holding a select command's normalized index string, so a
  // name payload (e.g. from a Home Assistant select) is handed to index-based
  // handlers as its numeric index.
  char _selectIndexScratch[8] = {0};
#endif
  bool recvWithStartEndMarkers();
  void parseData();

  void (*_beforeWriteCallback)() = nullptr;
  void (*_clientConnectedCallback)(byte clientNo) = nullptr;
  void (*_clientDisconnectedCallback)(byte clientNo) = nullptr;

  static unsigned long long _microsWrapper()
  {
    return (unsigned long long)micros();
  }

  BlaeckTimestampMode _timestampMode = BLAECK_NO_TIMESTAMP;
  unsigned long long (*_timestampCallback)() = nullptr;

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
    unsigned long long val;
    byte bval[8];
  } ullCvt;

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
