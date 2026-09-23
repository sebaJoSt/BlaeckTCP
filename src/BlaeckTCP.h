/*
        File: BlaeckTCP.h
        Author: Sebastian Strobl

    Sends binary sensor data over a network, and receives commands written as
    <HelloWorld, 12, 47>. The same library as BlaeckSerial, over TCP: a device describes
    its signals, commands, state and event channels, so a host can present them without
    being configured for it.
*/

#ifndef BLAECKTCP_H
#define BLAECKTCP_H

#define BLAECKTCP_VERSION "7.0.0"
#define BLAECKTCP_VERSION_MAJOR 7
#define BLAECKTCP_VERSION_MINOR 0
#define BLAECKTCP_VERSION_PATCH 0
#define BLAECKTCP_NAME "BlaeckTCP"

#include "BlaeckCore.h"
// NetServer and NetClient, the classes of whichever network library the sketch uses.
#include <TelnetPrint.h>

// Nagle's algorithm off on ESP32 and ESP8266, for lower latency. Set it false in
// BlaeckTCPConfig.h to favour throughput instead.
#ifndef BLAECK_TCP_NO_DELAY_DEFAULT
  #define BLAECK_TCP_NO_DELAY_DEFAULT true
#endif

// The core's names, at global scope where sketches use them.
using namespace BLAECK_CORE_NAMESPACE;

class BlaeckTCP;

// Returned by begin(): the core's table sizes, plus the number of connections. Each call
// returns this handle again, so withClients() can come anywhere in the chain.
class BlaeckTCPBeginRef : public BlaeckBeginRef
{
public:
  explicit BlaeckTCPBeginRef(BlaeckTCP *owner);

  /*!
    @brief   Sets how many connections the device accepts at once.

    Hosts and terminals together. Each connection takes a receive buffer of
    BLAECK_COMMAND_MAX_CHARS_DEFAULT bytes, 128 on a Mega. A connection beyond the
    limit is closed at once. Set it before the first read(); later it is refused.

    @param   count  1 to 255. The default is 4.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withClients(2);
    @endcode
  */
  BlaeckTCPBeginRef &withClients(byte count);

  /*!
    @brief   Sets how many signals fit in the signal table.

    Each signal takes 9 bytes of SRAM on AVR.

    @param   count  Up to 32767. A larger literal fails the build.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withSignals(50);
    @endcode
  */
  BlaeckTCPBeginRef &withSignals(unsigned int count)
  {
    BlaeckBeginRef::withSignals(count);
    return *this;
  }

  /*!
    @brief   Sets how many state channels fit in the state channel table.

    Count the channels from addStateChannel() plus one for each command that uses
    withOwnState(). Each channel takes 26 bytes of SRAM on AVR.

    @param   count  Up to 32767. A larger literal fails the build.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withStateChannels(12);
    @endcode
  */
  BlaeckTCPBeginRef &withStateChannels(unsigned int count)
  {
    BlaeckBeginRef::withStateChannels(count);
    return *this;
  }

  /*!
    @brief   Sets how many event channels fit in the event channel table.

    Each channel takes 10 bytes of SRAM on AVR.

    @param   count  Up to 32767. A larger literal fails the build.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withEventChannels(4);
    @endcode
  */
  BlaeckTCPBeginRef &withEventChannels(unsigned int count)
  {
    BlaeckBeginRef::withEventChannels(count);
    return *this;
  }

  /*!
    @brief   Sets how many event types fit, counted across all channels.

    All channels share one table of types, so give the total: four channels with
    five types each need 20. Each type takes 5 bytes of SRAM on AVR.

    @param   count  Up to 32767. A larger literal fails the build.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withEventChannels(4).withEventTypes(20);
    @endcode
  */
  BlaeckTCPBeginRef &withEventTypes(unsigned int count)
  {
    BlaeckBeginRef::withEventTypes(count);
    return *this;
  }

  /*!
    @brief   Sets how many commands fit in the command table.

    onCommand() and all the typed commands share this table. Each command takes 48
    bytes of SRAM on AVR. A command using withOwnState() also needs a state channel,
    so raise withStateChannels() to match.

    @param   count  Up to 32767. A larger literal fails the build.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withCommands(8);
    @endcode
  */
  BlaeckTCPBeginRef &withCommands(unsigned int count)
  {
    BlaeckBeginRef::withCommands(count);
    return *this;
  }

  /*!
    @brief   Sets a stream where the library reports what it rejected and why.

    Without one, problems such as a full table show only in hasRejections().

    @param   debugStream  Where to print: a serial port, or anything else that can print,
                          such as a display.
    @return  The same handle, for chaining.

    @code
      Blaeck.begin(SERVER_PORT).withSignals(50).withDebugStream(&Blaeck.Terminal);
    @endcode
  */
  BlaeckTCPBeginRef &withDebugStream(Print *debugStream)
  {
    BlaeckBeginRef::withDebugStream(debugStream);
    return *this;
  }

private:
  BlaeckTCP *_tcp;
};

// Text for every connected terminal: what Blaeck.Terminal is.
class BlaeckTerminal : public Print
{
public:
  explicit BlaeckTerminal(BlaeckTCP *owner) : _owner(owner) {}

  /*!
    @brief   Sends one byte to every connected terminal.

    Everything printed to Terminal goes through this; a sketch rarely calls it directly.

    @param   b  The byte.
    @return  1.

    @code
      Blaeck.Terminal.write('.');
    @endcode
  */
  size_t write(uint8_t b) override;

  /*!
    @brief   Sends bytes to every connected terminal.

    @param   buffer  The bytes.
    @param   size    How many.
    @return  size.

    @code
      Blaeck.Terminal.write((const uint8_t *)"ok\n", 3);
    @endcode
  */
  size_t write(const uint8_t *buffer, size_t size) override;

  using Print::write;

private:
  BlaeckTCP *_owner;
};

// Blaeck over TCP. A connection becomes a host by sending a BLAECK. command and receives
// frames from then on; every other connection is a terminal and receives text.
class BlaeckTCP : public BlaeckCore
{
public:
  BlaeckTCP();
  ~BlaeckTCP();

  /*!
    @brief   Starts the library as a TCP server on a port.

    Call it first, after the network is up. A connection becomes a host when it sends
    a BLAECK. command, such as <BLAECK.GET_DEVICES>, and receives frames from then on.
    Every other connection is a terminal: it receives the text sent to Terminal, and
    its commands run but aren't answered.

    @param   port  The TCP port to listen on.
    @return  A handle for setting the number of connections, table sizes and a debug
             stream. Each has a default, so the handle can be ignored.

    @code
      Blaeck.begin(SERVER_PORT)
          .withClients(4)
          .withSignals(50)
          .withDebugStream(&Blaeck.Terminal);
    @endcode
  */
  BlaeckTCPBeginRef begin(uint16_t port);

  /*!
    @brief   Text to every connected terminal, used like Serial.

    Hosts never receive it. Pass it to withDebugStream() to see on a terminal what
    the library refuses and which commands arrive.

    @note    A terminal that connects but never reads can fill its send buffer, and a
             write to it may then wait. Short lines don't get there.

    @code
      Blaeck.Terminal.println("LED is ON.");
    @endcode
  */
  BlaeckTerminal Terminal;

  /*!
    @brief   Sets a function to call when a connection opens.

    @param   callback  Receives the connection's slot, starting at 0.

    @code
      Blaeck.setClientConnectedCallback(onClientConnected);
    @endcode
  */
  void setClientConnectedCallback(void (*callback)(byte clientNo));

  /*!
    @brief   Sets a function to call when a connection closes.

    A connection that dies without closing, such as one whose cable was pulled, is
    noticed only when the network stack gives up on it.

    @param   callback  Receives the connection's slot, starting at 0.

    @code
      Blaeck.setClientDisconnectedCallback(onClientDisconnected);
    @endcode
  */
  void setClientDisconnectedCallback(void (*callback)(byte clientNo));

protected:
  bool _transportReady() const override;
  void _writeDirect(const byte *data, size_t len) override;
  void _flushDirect() override;
  void _sendBuffered() override;
  bool _receiveCommand() override;
  void _builtinCommandReceived() override;
  const char *_libraryName() const override { return BLAECKTCP_NAME; }
  const char *_libraryVersion() const override { return BLAECKTCP_VERSION; }

private:
  struct Connection
  {
    NetClient client;
    Receiver receiver;
    // The slot holds a connection. Tracked here because a client's own bool means
    // "connected" on some cores and "has a socket" on others.
    bool open = false;
    bool host = false;
  };

  // Allocated by the first read(), so withClients() on the begin() chain can size it.
  Connection *_connections = nullptr;
  byte _maxClients = 4;
  // The connection whose command is being handled; acks and answers go only there.
  byte _requester = 0;

  void (*_connectedCallback)(byte clientNo) = nullptr;
  void (*_disconnectedCallback)(byte clientNo) = nullptr;

  bool _ensureConnections();
  void _acceptConnection();
  void _dropClosedConnections();
  // Whether the frame being written goes to this connection.
  bool _receivesFrame(byte slot) const;
  void _setMaxClients(byte count);

  friend class BlaeckTCPBeginRef;
  friend class BlaeckTerminal;
};

inline BlaeckTCPBeginRef::BlaeckTCPBeginRef(BlaeckTCP *owner) : BlaeckBeginRef(owner), _tcp(owner) {}

inline BlaeckTCPBeginRef &BlaeckTCPBeginRef::withClients(byte count)
{
  if (_tcp != nullptr)
    _tcp->_setMaxClients(count);
  return *this;
}

#endif //  BLAECKTCP_H
