# enet.pyx
#
# DESCRIPTION
#
#   Python ENET Wrapper implemented in pyrexc.
#
# RATIONALE
#
#   Ling Lo's pyenet.c module had a problem with dropping a connection after
#   a short amount of time. Having seen other Python <-> C interfaces
#   defined in pyrexc, I decided it probably has a much better time of
#   surviving time.
#
#   Hopefully no one will be too mad with the option of choice?
#
# LICENSE
#
#   Copyright (C) 2003, Scott Robinson (scott@tranzoa.com)
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#   Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
#   Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
#   The names of its contributors may not be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
#   TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
#   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
#   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# CHANGELOG
#
#   Sat Nov  1 00:36:02 PST 2003  Scott Robinson <scott@tranzoa.com>
#     Began developing test interface after a day of coding...
#
#   Mon Nov  3 08:46:33 PST 2003  Scott Robinson <scott@tranzoa.com>
#     Added documentation to all classes, functions, and attributes.
#     While adding documentation, added accessors to a few more attributes.
#     Cleaned up a few methods to match proper pyrex behavior.
#     Removed a few, and added a couple of obvious todos for the future.
#     Fixed Address.__getattr__ extra \0s in the case .host.
#
#   Fri Feb 13 18:18:04 PST 2004  Scott Robinson <scott@tranzoa.com>
#     Added Socket class for use with select and poll.
#

import  atexit

# SECTION
#   C declarations and definitions for the interface.

cdef extern from "Python.h" :
  object  PyBuffer_FromMemory         (void *ptr, int size)
  object  PyString_FromString         (char *v)
  object  PyString_FromStringAndSize  (char *v, int len)

cdef extern from "enet/types.h" :
  ctypedef unsigned char  enet_uint8
  ctypedef unsigned short enet_uint16
  ctypedef unsigned int   enet_uint32
  ctypedef unsigned int   size_t

cdef extern from "enet/enet.h" :
  cdef enum :
    ENET_HOST_ANY = 0

  # TODO: Handle Windows situation.
  ctypedef int    ENetSocket

  ctypedef struct ENetAddress :
    enet_uint32   host
    enet_uint16   port

  ctypedef enum ENetPacketFlag :
    ENET_PACKET_FLAG_RELIABLE = (1 << 0)

  ctypedef struct ENetPacket :
    size_t        referenceCount
    enet_uint32   flags
    enet_uint8   *data
    size_t        dataLength

  ctypedef enum ENetPeerState :
    ENET_PEER_STATE_DISCONNECTED                = 0
    ENET_PEER_STATE_CONNECTING                  = 1
    ENET_PEER_STATE_ACKNOWLEDGING_CONNECT       = 2
    ENET_PEER_STATE_CONNECTED                   = 3
    ENET_PEER_STATE_DISCONNECTING               = 4
    ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT    = 5
    ENET_PEER_STATE_ZOMBIE                      = 6

  cdef enum :
    ENET_PEER_PACKET_LOSS_SCALE = (1 << 16)

  ctypedef struct ENetPeer :
    ENetAddress   address
    ENetPeerState state
    enet_uint32   packetLoss
    enet_uint32   packetThrottleAcceleration
    enet_uint32   packetThrottleDeceleration
    enet_uint32   packetThrottleInterval
    enet_uint32   roundTripTime

  ctypedef struct ENetHost :
    ENetSocket    socket
    ENetAddress   address

  ctypedef enum ENetEventType :
    ENET_EVENT_TYPE_NONE       = 0
    ENET_EVENT_TYPE_CONNECT    = 1
    ENET_EVENT_TYPE_DISCONNECT = 2
    ENET_EVENT_TYPE_RECEIVE    = 3

  ctypedef struct ENetEvent :
    ENetEventType   type
    ENetPeer       *peer
    enet_uint8      channelID
    ENetPacket     *packet

  int   enet_initialize     ()
  void  enet_deinitialize   ()

  int enet_address_set_host (ENetAddress *address, char *hostName)
  int enet_address_get_host (ENetAddress *address, char *hostName, size_t nameLength)

  ENetPacket *  enet_packet_create    (void *dataContents, size_t dataLength, enet_uint32 flags)
  void          enet_packet_destroy   (ENetPacket *packet)
  int           enet_packet_resize    (ENetPacket *packet, size_t dataLength)

  ENetHost *  enet_host_create              (ENetAddress *address, size_t peerCount, enet_uint32 incomingBandwidth, enet_uint32 outgoingBandwidth)
  void        enet_host_destroy             (ENetHost *host)
  ENetPeer *  enet_host_connect             (ENetHost *host, ENetAddress *address, size_t channelCount)
  int         enet_host_service             (ENetHost *host, ENetEvent *event, enet_uint32 timeout)
  void        enet_host_flush               (ENetHost *host)
  void        enet_host_broadcast           (ENetHost *host, enet_uint8 channelID, ENetPacket *packet)
  void        enet_host_bandwidth_limit     (ENetHost *host, enet_uint32 incomingBandwidth, enet_uint32 outgoingBandwidth)

  int             enet_peer_send                  (ENetPeer *peer, enet_uint8 channelID, ENetPacket *packet)
  ENetPacket *    enet_peer_receive               (ENetPeer *peer, enet_uint8 channelID)
  void            enet_peer_ping                  (ENetPeer *peer)
  void            enet_peer_reset                 (ENetPeer *peer)
  void            enet_peer_disconnect            (ENetPeer *peer)
  void            enet_peer_disconnect_now        (ENetPeer *peer)
  void            enet_peer_throttle_configure    (ENetPeer *peer, enet_uint32 interval, enet_uint32 acceleration, enet_uint32 deacceleration)

# SECTION
#   Enumerations and constants.

HOST_ANY              = ENET_HOST_ANY

PACKET_FLAG_RELIABLE  = ENET_PACKET_FLAG_RELIABLE

PEER_STATE_DISCONNECT                   = ENET_PEER_STATE_DISCONNECTED
PEER_STATE_CONNECTING                   = ENET_PEER_STATE_CONNECTING
PEER_STATE_ACKNOWLEDGING_CONNECT        = ENET_PEER_STATE_ACKNOWLEDGING_CONNECT 
PEER_STATE_CONNECTED                    = ENET_PEER_STATE_CONNECTED
PEER_STATE_DISCONNECTING                = ENET_PEER_STATE_DISCONNECTING
PEER_STATE_ACKNOWLEDGING_DISCONNECT     = ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT
PEER_STATE_ZOMBIE                       = ENET_PEER_STATE_ZOMBIE

PEER_PACKET_LOSS_SCALE  = ENET_PEER_PACKET_LOSS_SCALE

EVENT_TYPE_NONE       = ENET_EVENT_TYPE_NONE
EVENT_TYPE_CONNECT    = ENET_EVENT_TYPE_CONNECT
EVENT_TYPE_DISCONNECT = ENET_EVENT_TYPE_DISCONNECT
EVENT_TYPE_RECEIVE    = ENET_EVENT_TYPE_RECEIVE

# SECTION
#   Python exposed class definitions.

cdef class Socket :
  """Socket (int socket)

  DESCRIPTION

    An ENet socket.

    Can be used with select and poll."""

  cdef ENetSocket     _enet_socket

  def fileno (self) :
    return self._enet_socket

cdef class Address :
  """Address (str address, int port)

  ATTRIBUTES

    str host    Hostname referred to by the Address.
    int port    Port referred to by the Address.

  DESCRIPTION

    An ENet address and port pair.

    When instantiated, performs a resolution upon 'address'. However, if 'address' is None, enet.HOST_ANY is assumed."""

  cdef ENetAddress    _enet_address

  def __init__ (self, address, port) :
    self.host = address
    self.port = port

  def __getattr__ (self, name) :
    if name == "host" :
      if self._enet_address.host == ENET_HOST_ANY :
        return "*"
      elif self._enet_address.host :
        maxhostname = 257   # We'll follow Solaris' standard.
        host = PyString_FromStringAndSize (NULL, maxhostname)

        if enet_address_get_host (&self._enet_address, host, maxhostname) :
          raise IOError ("Resolution failure!")

        return PyString_FromString (host)
      else :
        assert (not ENET_HOST_ANY)
    elif name == "port" :
      return self._enet_address.port
    else :
      return AttributeError ("Address object has no attribute '" + name + "'")

  def __setattr__ (self, name, value) :
    if name == "host" :
      if not value or value == "*":
        self._enet_address.host = ENET_HOST_ANY
      else :
        if enet_address_set_host (&self._enet_address, value) :
          raise IOError ("Resolution failure!")
    elif name == "port" :
      self._enet_address.port = value
    else :
      return AttributeError ("Address object has no attribute '" + name + "'")

  def __str__ (self) :
      return "%s:%u" % (self.host, self.port)

cdef class Packet :
  """Packet ([dataContents, int flags])

  ATTRIBUTES

    str data                        Contains the data for the packet.
    int flags                       Flags modifying delivery of the Packet:
      enet.PACKET_FLAG_RELIABLE         Packet must be received by the target peer and resend attempts should be made until the packet is delivered.

  DESCRIPTION

    An ENet data packet that may be sent to or received from a peer."""

  cdef ENetPacket  *_enet_packet

  def __init__ (self, char *dataContents = "", flags = 0) :
    if dataContents or flags :
      self._enet_packet = enet_packet_create (dataContents, len (dataContents), flags)

      if not self._enet_packet :
        raise MemoryError ("Unable to create packet structure!")

  def __dealloc__ (self) :
    if self._enet_packet and not self._enet_packet.referenceCount :
      # WARNING: referenceCount is an internal structure. Is there a better way of doing this?
      enet_packet_destroy (self._enet_packet)

  def __getattr__ (self, name) :
    if self._enet_packet :
      if name == "flags" :
        return self._enet_packet.flags
      elif name == "data" :
        # TODO: Find out why the PyBuffer interface is cutting off data!
        #return PyBuffer_FromMemory (self._enet_packet.data, self._enet_packet.dataLength)
        return PyString_FromStringAndSize (<char *> self._enet_packet.data, self._enet_packet.dataLength)
      elif name == "dataLength" :
        return len (self.data)
      else :
        raise AttributeError ("Packet object has no attribute '" + name + "'")
    else :
      raise MemoryError ("Empty Packet object accessed!")

cdef class Peer :
  """Peer ()

  ATTRIBUTES

    Address address
    int     state                                   The peer's current state.
      enet.PEER_STATE_DISCONNECT
          .PEER_STATE_CONNECTING
          .PEER_STATE_CONNECTED
          .PEER_STATE_DISCONNECTING
          .PEER_STATE_ACKNOWLEDGING_DISCONNECT
          .PEER_STATE_ZOMBIE
    int     packetLoss                              Mean packet loss of reliable packets as a ratio with respect to the constant enet.PEER_PACKET_LOSS_SCALE.
    int     packetThrottleAcceleration
    int     packetThrottleDeceleration
    int     packetThrottleInterval
    int     roundTripTime                           Mean round trip time (RTT), in milliseconds, between sending a reliable packet and receiving its acknowledgement.

  DESCRIPTION

    An ENet peer which data packets may be sent or received from.

    This class should never be instantiated directly, but rather via enet.Host.connect or enet.Event.Peer."""

  cdef ENetPeer  *_enet_peer

  def send (self, channelID, Packet packet) :
    """send (int channelID, Packet packet)

    Queues a packet to be sent."""

    if self._enet_peer and packet._enet_packet :
      return enet_peer_send (self._enet_peer, channelID, packet._enet_packet)

  def receive (self, channelID) :
    """receive (int channelID)

    Attempts to dequeue any incoming queued packet."""

    if self._enet_peer :
      packet = Packet ()
      (<Packet> packet)._enet_packet = enet_peer_receive (self._enet_peer, channelID)

      if packet._enet_packet :
        return packet
      else :
        return None

  def reset (self) :
    """reset ()

    Forcefully disconnects a peer."""

    if self._enet_peer :
      enet_peer_reset (self._enet_peer)

  def ping (self) :
    """ping ()

    Sends a ping request to a peer."""

    if self._enet_peer :
      enet_peer_ping (self._enet_peer)

  def disconnect (self) :
    """disconnect ()

    Request a disconnection from a peer."""

    if self._enet_peer :
      enet_peer_disconnect (self._enet_peer)

  def __getattr__ (self, name) :
    if self._enet_peer :
      if name == "address" :
        address = Address (0, 0)
        (<Address> address)._enet_address = self._enet_peer.address

        return address
      elif name == "state" :
        return self._enet_peer.state
      elif name == "packetLoss" :
        return self._enet_peer.packetLoss
      elif name == "packetThrottleInterval" :
        return self._enet_peer.packetThrottleInterval
      elif name == "packetThrottleAcceleration" :
        return self._enet_peer.packetThrottleAcceleration
      elif name == "packetThrottleDeceleration" :
        return self._enet_peer.packetThrottleDeceleration
      elif name == "roundTripTime" :
        return self._enet_peer.roundTripTime
      else :
        raise AttributeError ("Peer object has no attribute '" + name + "'")
    else :
      raise MemoryError ("Empty Peer object accessed!")

  def __setattr__ (self, name, value) :
    if self._enet_peer :
      if name == "packetThrottleInterval" : 
        enet_peer_throttle_configure (self._enet_peer, value, self._enet_peer.packetThrottleAcceleration, self._enet_peer.packetThrottleDeceleration)
      elif name == "packetThrottleAcceleration" :
        enet_peer_throttle_configure (self._enet_peer, self._enet_peer.packetThrottleInterval, value, self._enet_peer.packetThrottleDeceleration)
      elif name == "packetThrottleDeceleration" :
        enet_peer_throttle_configure (self._enet_peer, self._enet_peer.packetThrottleInterval, self._enet_peer.packetThrottleAcceleration, value)
      else :
        raise AttributeError ("Peer object has no attribute '" + name + "'")
    else :
      raise MemoryError ("Empty Peer object accessed!")

cdef class Event :
  """Event ()

  ATTRIBUTES

    int     type        Type of the event.
      enet.EVENT_TYPE_NONE
          .EVENT_TYPE_CONNECT
          .EVENT_TYPE_DISCONNECT
          .EVENT_TYPE_RECEIVE
    Peer    peer        Peer that generated a connect, disconnect or receive event.
    int     channelID
    Packet  packet

  DESCRIPTION

    An ENet event as returned by enet.Host.service.

    This class should never be instantiated directly."""

  cdef ENetEvent  _enet_event

  def __getattr__ (self, name) :
    if name == "type" :
      return self._enet_event.type
    elif name == "peer" :
      peer = Peer ()
      (<Peer> peer)._enet_peer = self._enet_event.peer

      return peer
    elif name == "channelID" :
      return self._enet_event.channelID
    elif name == "packet" :
      packet = Packet ()
      (<Packet> packet)._enet_packet = self._enet_event.packet

      return packet
    else :
      raise AttributeError ("Event object has no attribute '" + name + "'")

  def __setattr__ (self, name, value) :
    if name == "type" or name == "peer" or name == "channelID" or name == "packet" : 
      raise AttributeError ("Attribute '" + name +"' on Event object is read-only.")
    else :
      raise AttributeError ("Event object has no attribute '" + name + "'")

cdef class Host :
  """Host (Address address, int peerCount, int incomingBandwidth, int outgoingBandwidth)

  ATTRIBUTES

    Address address             Internet address of the host.
    Socket  socket              The socket the host services.
    int     incomingBandwidth   Downstream bandwidth of the host.
    int     outgoingBandwidth   Upstream bandwidth of the host.

  DESCRIPTION

    An ENet host for communicating with peers.

    If 'address' is None, then the Host will be client only."""

  cdef ENetHost    *_enet_host
  cdef enet_uint32  _enet_incomingBandwidth
  cdef enet_uint32  _enet_outgoingBandwidth

  def __init__ (self, Address address = None, peerCount = 0, incomingBandwidth = 0, outgoingBandwidth = 0) :
    (self._enet_incomingBandwidth, self._enet_outgoingBandwidth) = (incomingBandwidth, outgoingBandwidth)

    if address :
      self._enet_host = enet_host_create (&address._enet_address, peerCount, incomingBandwidth, outgoingBandwidth)
    else :
      self._enet_host = enet_host_create (NULL, peerCount, incomingBandwidth, outgoingBandwidth)

    if not self._enet_host :
      raise MemoryError ("Unable to create host structure!")

  def __dealloc__ (self) :
    if self._enet_host :
      enet_host_destroy (self._enet_host)

  def connect (self, Address address, channelCount) :
    """Peer connect (Address address, int channelCount)

    Initiates a connection to a foreign host."""

    if self._enet_host :
      peer = Peer ()
      (<Peer> peer)._enet_peer = enet_host_connect (self._enet_host, &address._enet_address, channelCount)

      if not (<Peer> peer)._enet_peer :
        raise IOError ("Connection failure!")

      return peer

  def service (self, timeout) :
    """Event service (int timeout)

    Waits for events on the host specified and shuttles packets between the host and its peers."""

    if self._enet_host :
      event = Event ()
      result = enet_host_service (self._enet_host, &(<Event> event)._enet_event, timeout)

      if result < 0 :
        raise IOError ("Servicing error - probably disconnected.")
      else :
        return event

  def flush (self) :
    """flush ()

    Sends any queued packets on the host specified to its designated peers."""

    if self._enet_host :
      enet_host_flush (self._enet_host)

  def broadcast (self, channelID, Packet packet) :
    """broadcast (int channelID, Packet packet)

    Queues a packet to be sent to all peers associated with the host."""

    if self._enet_host and packet._enet_packet :
      enet_host_broadcast (self._enet_host, channelID, packet._enet_packet)

  def __getattr__ (self, name) :
    # TODO: Add 'peers'.
    if name == "address" and self._enet_host :
      address = Address (0, 0)
      (<Address> address)._enet_address = self._enet_host.address

      return address
    elif name == "incomingBandwidth" :
      return self._enet_incomingBandwidth
    elif name == "outgoingBandwidth" :
      return self._enet_outgoingBandwidth
    elif name == "socket" :
      socket = Socket ()
      (<Socket> socket)._enet_socket = self._enet_host.socket

      return socket
    else :
      raise AttributeError ("Host object has no attribute '" + name + "'")

  def __setattr__ (self, name, value) :
    if name == "incomingBandwidth" :
      self._enet_incomingBandwidth = value
      enet_host_bandwidth_limit (self._enet_host, self._enet_incomingBandwidth, self._enet_outgoingBandwidth)
    elif name == "outgoingBandwidth" :
      self._enet_outgoingBandwidth = value
      enet_host_bandwidth_limit (self._enet_host, self._enet_incomingBandwidth, self._enet_outgoingBandwidth)
    else :
      raise AttributeError ("Host object has no attribute '" + name + "'")

# SECTION
#   Testing
#
# TODO
#   Switch to using the unittest framework.

class test :
  """test ()

  DESCRIPTION

    A very simple testing class that will change between releases. This is for the maintainer only."""

  def check (self, host) :

    event = host.service (0)

    if event.type == EVENT_TYPE_NONE :
      pass
    elif event.type == EVENT_TYPE_CONNECT :
      print "%s connected to %s via %s." % (host, event.peer.address, event.peer)
    elif event.type == EVENT_TYPE_DISCONNECT :
      print "%s disconnected from %s via %s." % (host, event.peer.address, event.peer)
    elif event.type == EVENT_TYPE_RECEIVE :
      print "%s received %s containing '%s' from %s via %s." % (host, event.packet, event.packet.data, event.peer.address, event.peer)
    else :
      print "%s received invalid event %s of type %u." % (host, event, event.type)

  def test (self) :
    print "Starting services..."

    host1 = Host (None, 1, 0, 0)
    host2 = Host (Address ("localhost", 6666), 1, 0, 0)

    print "Connecting %s (client) to %s (server)..." % (host1, host2)

    peer1 = host1.connect (Address ("localhost", 6666), 1)

    print "Entering service loop..."

    count = 0

    while 1 :
      self.check (host1)
      self.check (host2)

      count = count + 1

      if not (count % 10000) :
        print "Sending broadcast..."
        host1.broadcast (0, Packet ("SuperJoe"))

# SECTION
#   Ensure ENET is properly initialized and de-initialized.

def _enet_atexit () :
  enet_deinitialize ()

enet_initialize ()
atexit.register (_enet_atexit)
