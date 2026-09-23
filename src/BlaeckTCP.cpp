/*
        File: BlaeckTCP.cpp
        Author: Sebastian Strobl
*/

#include <Arduino.h>
#include "BlaeckTCP.h"

BlaeckTCP::BlaeckTCP() : Terminal(this)
{
}

BlaeckTCP::~BlaeckTCP()
{
  delete[] _connections;
}

BlaeckTCPBeginRef BlaeckTCP::begin(uint16_t port)
{
  TelnetPrint = NetServer(port);
  TelnetPrint.begin();
#if defined(ESP32) || defined(ESP8266)
  TelnetPrint.setNoDelay(BLAECK_TCP_NO_DELAY_DEFAULT);
#endif
  _beginCore();
  return BlaeckTCPBeginRef(this);
}

void BlaeckTCP::setClientConnectedCallback(void (*callback)(byte clientNo))
{
  _connectedCallback = callback;
}

void BlaeckTCP::setClientDisconnectedCallback(void (*callback)(byte clientNo))
{
  _disconnectedCallback = callback;
}

void BlaeckTCP::_setMaxClients(byte count)
{
  if (_connections != nullptr)
  {
    if (_debugStream != nullptr)
      _debugStream->println(F("withClients() ignored: the connections are already set up."));
    return;
  }
  _maxClients = count > 0 ? count : 1;
}

bool BlaeckTCP::_ensureConnections()
{
  if (_connections == nullptr)
    _connections = new (std::nothrow) Connection[_maxClients];
  return _connections != nullptr;
}

// ----- Frames out -----

bool BlaeckTCP::_receivesFrame(byte slot) const
{
  const Connection &c = _connections[slot];
  if (!c.open || !c.host || !const_cast<NetClient &>(c.client).connected())
    return false;
  return _frameAudience != AUDIENCE_REQUESTER || slot == _requester;
}

bool BlaeckTCP::_transportReady() const
{
  if (_connections == nullptr)
    return false;
  for (byte i = 0; i < _maxClients; i++)
  {
    const Connection &c = _connections[i];
    if (c.open && c.host && const_cast<NetClient &>(c.client).connected())
      return true;
  }
  return false;
}

void BlaeckTCP::_writeDirect(const byte *data, size_t len)
{
  for (byte i = 0; i < _maxClients; i++)
  {
    if (_receivesFrame(i))
      _connections[i].client.write(data, len);
  }
}

// Nothing to do: a TCP client has no send buffer to push out, and on older ESP32 cores
// flush() threw away received data instead.
void BlaeckTCP::_flushDirect()
{
}

void BlaeckTCP::_sendBuffered()
{
  for (byte i = 0; i < _maxClients; i++)
  {
    if (_receivesFrame(i))
      _connections[i].client.write(_frameBuf, _framePos);
  }
}

// ----- Connections and commands in -----

void BlaeckTCP::_acceptConnection()
{
  NetClient incoming = TelnetPrint.accept();
  if (!incoming)
    return;

  for (byte i = 0; i < _maxClients; i++)
  {
    Connection &c = _connections[i];
    if (c.open)
      continue;

    c.client = incoming;
    c.open = true;
    c.receiver = Receiver();
    c.host = false;
    if (_debugStream != nullptr)
    {
      _debugStream->print(F("Client #"));
      _debugStream->print(i);
      _debugStream->print(F(" connected: "));
      _debugStream->print(incoming.remoteIP());
      _debugStream->print(':');
      _debugStream->println(incoming.remotePort());
    }
    if (_connectedCallback != nullptr)
      _connectedCallback(i);
    return;
  }

  // Every slot is taken.
  incoming.stop();
}

void BlaeckTCP::_dropClosedConnections()
{
  for (byte i = 0; i < _maxClients; i++)
  {
    Connection &c = _connections[i];
    if (!c.open || c.client.connected())
      continue;

    c.client.stop();
    c.open = false;
    c.host = false;
    c.receiver = Receiver();
    if (_debugStream != nullptr)
    {
      _debugStream->print(F("Client #"));
      _debugStream->print(i);
      _debugStream->println(F(" disconnected"));
    }
    if (_disconnectedCallback != nullptr)
      _disconnectedCallback(i);
  }
}

bool BlaeckTCP::_receiveCommand()
{
  if (!_ensureConnections())
    return false;

  _acceptConnection();
  _dropClosedConnections();

  // Start after the connection served last, so one busy connection can't starve the rest.
  for (byte k = 1; k <= _maxClients; k++)
  {
    byte i = (byte)((_requester + k) % _maxClients);
    Connection &c = _connections[i];
    if (!c.open)
      continue;

    // One byte at a time, so whatever follows a complete command stays in the socket for
    // the next read().
    while (c.client.available() > 0)
    {
      if (_receiveByte(c.receiver, (char)c.client.read()))
      {
        _receiver = c.receiver;
        _requester = i;
        return true;
      }
    }
  }
  return false;
}

void BlaeckTCP::_builtinCommandReceived()
{
  Connection &c = _connections[_requester];
  if (c.host)
    return;

  c.host = true;
  if (_debugStream != nullptr)
  {
    _debugStream->print(F("Client #"));
    _debugStream->print(_requester);
    _debugStream->println(F(" is a host"));
  }
}

// ----- Terminal -----

size_t BlaeckTerminal::write(uint8_t b)
{
  return write(&b, 1);
}

size_t BlaeckTerminal::write(const uint8_t *buffer, size_t size)
{
  if (_owner->_connections == nullptr)
    return size;
  for (byte i = 0; i < _owner->_maxClients; i++)
  {
    BlaeckTCP::Connection &c = _owner->_connections[i];
    if (c.open && !c.host && c.client.connected())
      c.client.write(buffer, size);
  }
  return size;
}
