/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright © 2019 Fragcolor Pte. Ltd. */

// ASIO must go first!!
#include <boost/asio.hpp>

#include "../runtime.hpp"
#include "shared.hpp"
#include "utility.hpp"
#include <boost/lockfree/queue.hpp>
#include <thread>

using boost::asio::ip::udp;

namespace chainblocks {
namespace Network {
constexpr uint32_t SocketCC = 'netS';

struct SocketData {
  udp::socket *socket;
  udp::endpoint *endpoint;
};

struct NetworkBase {
  BlocksVar _blks{};

  static inline Type SocketInfo{
      {CBType::Object, {.object = {.vendorId = CoreCC, .typeId = SocketCC}}}};

  static inline boost::asio::io_context _io_context;
  static inline int64_t _io_context_refc = 0;

  ParamVar _addr{Var("localhost")};
  ParamVar _port{Var(9191)};

  // Every server/client will share same context, so sharing the same recv
  // buffer is possible and nice!
  ThreadShared<std::array<char, 0xFFFF>> _recv_buffer;

  CBVar *_socketVar = nullptr;
  SocketData _socket{};

  static inline ParamsInfo params = ParamsInfo(
      ParamsInfo::Param(
          "Address", CBCCSTR("The local bind address or the remote address."),
          CoreInfo::StringOrStringVar),
      ParamsInfo::Param(
          "Port",
          CBCCSTR("The port to bind if server or to connect to if client."),
          CoreInfo::IntOrIntVar),
      ParamsInfo::Param(
          "Receive", CBCCSTR("The flow to execute when a packet is received."),
          CoreInfo::BlocksOrNone));

  static CBParametersInfo parameters() { return CBParametersInfo(params); }

  static void setup() {
    if (_io_context_refc == 0) {
      auto worker = std::thread([] {
        // Force run to run even without work
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
            g = boost::asio::make_work_guard(_io_context);
        try {
          // CBLOG_DEBUG("Boost asio context running...");
          _io_context.run();
        } catch (...) {
          CBLOG_ERROR("Boost asio context run failed.");
        }
        // CBLOG_DEBUG("Boost asio context exiting...");
      });
      worker.detach();
    }
    _io_context_refc++;
  }

  void destroy() {
    // defer all in the context or we will crash!
    if (_io_context_refc > 0) {
      boost::asio::post(_io_context, []() {
        _io_context_refc--;
        if (_io_context_refc == 0) {
          // allow end/thread exit
          _io_context.stop();
        }
      });
    }
  }

  void cleanup() {
    // defer all in the context or we will crash!
    if (_socket.socket) {
      auto socket = _socket.socket;
      boost::asio::post(_io_context, [socket]() {
        if (socket) {
          socket->close();
          delete socket;
        }
      });

      _socket.socket = nullptr;
      _socket.endpoint = nullptr;
    }

    // clean context vars
    if (_socketVar) {
      releaseVariable(_socketVar);
      _socketVar = nullptr;
    }

    _blks.cleanup();
    _addr.cleanup();
    _port.cleanup();
  }

  void warmup(CBContext *context) {
    _addr.warmup(context);
    _port.warmup(context);
    _blks.warmup(context);
  }

  static CBTypesInfo inputTypes() { return CoreInfo::AnyType; }

  static CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

  CBTypeInfo compose(CBInstanceData &data) {
    // inject our special context vars
    auto endpointInfo =
        ExposedInfo::Variable("Network.Socket", CBCCSTR("The active socket."),
                              CBTypeInfo(SocketInfo));
    chainblocks::arrayPush(data.shared, endpointInfo);
    _blks.compose(data);
    return data.inputType;
  }

  void setParam(int index, const CBVar &value) {
    switch (index) {
    case 0:
      _addr = value;
      break;
    case 1:
      _port = value;
      break;
    case 2:
      _blks = value;
      break;
    default:
      break;
    }
  }

  CBVar getParam(int index) {
    switch (index) {
    case 0:
      return _addr;
    case 1:
      return _port;
    case 2:
      return _blks;
    default:
      return Var::Empty;
    }
  }

  struct Reader {
    char *buffer;
    size_t offset;
    size_t max;

    Reader(char *buf, size_t size) : buffer(buf), offset(0), max(size) {}

    void operator()(uint8_t *buf, size_t size) {
      auto newOffset = offset + size;
      if (newOffset > max) {
        throw ActivationError("Overflow requested");
      }
      memcpy(buf, buffer + offset, size);
      offset = newOffset;
    }
  };

  struct Writer {
    char *buffer;
    size_t offset;
    size_t max;

    Writer(char *buf, size_t size) : buffer(buf), offset(0), max(size) {}

    void operator()(const uint8_t *buf, size_t size) {
      auto newOffset = offset + size;
      if (newOffset > max) {
        throw ActivationError("Overflow requested");
      }
      memcpy(buffer + offset, buf, size);
      offset = newOffset;
    }
  };

  void setSocket(CBContext *context) {
    if (!_socketVar) {
      _socketVar = referenceVariable(context, "Network.Socket");
    }
    auto rc = _socketVar->refcount;
    auto rcflag = _socketVar->flags & CBVAR_FLAGS_REF_COUNTED;
    *_socketVar = Var::Object(&_socket, CoreCC, SocketCC);
    _socketVar->refcount = rc;
    _socketVar->flags |= rcflag;
  }
};

struct Server : public NetworkBase {
  struct ClientPkt {
    udp::endpoint *remote;
    CBVar payload;
  };

  boost::lockfree::queue<ClientPkt> _queue{16};
  boost::lockfree::queue<ClientPkt> _empty_queue{16};
  udp::endpoint _sender;

  void destroy() {
    // drain queues first
    while (!_queue.empty()) {
      ClientPkt pkt;
      if (_queue.pop(pkt)) {
        delete pkt.remote;
        Serialization::varFree(pkt.payload);
      }
    }

    while (!_empty_queue.empty()) {
      ClientPkt pkt;
      if (_empty_queue.pop(pkt)) {
        delete pkt.remote;
        Serialization::varFree(pkt.payload);
      }
    }

    NetworkBase::destroy();
  }

  Serialization deserial;

  void do_receive() {
    _socket.socket->async_receive_from(
        boost::asio::buffer(&_recv_buffer().front(), _recv_buffer().size()),
        _sender, [this](boost::system::error_code ec, std::size_t bytes_recvd) {
          if (!ec && bytes_recvd > 0) {
            ClientPkt pkt{};

            // try reuse vars internal memory smartly
            if (!_empty_queue.empty()) {
              _empty_queue.pop(pkt);
              *pkt.remote = _sender;
            } else {
              pkt.remote = new udp::endpoint(_sender);
            }

            // deserialize from buffer
            Reader r(&_recv_buffer().front(), bytes_recvd);
            deserial.reset();
            deserial.deserialize(r, pkt.payload);

            // add ready packet to queue
            _queue.push(pkt);

            // keep receiving
            do_receive();
          }
        });
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    if (!_socket.socket) {
      // first activation, let's init
      _socket.socket = new udp::socket(
          _io_context, udp::endpoint(udp::v4(), _port.get().payload.intValue));

      // start receiving
      boost::asio::post(_io_context, [this]() { do_receive(); });
    }

    setSocket(context);

    // receive from ringbuffer and run chains
    while (!_queue.empty()) {
      ClientPkt pkt;
      if (_queue.pop(pkt)) {
        CBVar output{};
        // update remote as pops in context variable
        _socket.endpoint = pkt.remote;
        activateBlocks(CBVar(_blks).payload.seqValue, context, pkt.payload,
                       output);
        // release the var once done
        // will recycle internal buffers
        _empty_queue.push(pkt);
      }
    }

    return input;
  }
};

// Register
RUNTIME_BLOCK(Network, Server);
RUNTIME_BLOCK_setup(Server);
RUNTIME_BLOCK_cleanup(Server);
RUNTIME_BLOCK_warmup(Server);
RUNTIME_BLOCK_destroy(Server);
RUNTIME_BLOCK_inputTypes(Server);
RUNTIME_BLOCK_outputTypes(Server);
RUNTIME_BLOCK_parameters(Server);
RUNTIME_BLOCK_setParam(Server);
RUNTIME_BLOCK_getParam(Server);
RUNTIME_BLOCK_activate(Server);
RUNTIME_BLOCK_compose(Server);
RUNTIME_BLOCK_END(Server);

struct Client : public NetworkBase {
  ExposedInfo _exposedInfo{};

  boost::lockfree::queue<CBVar> _queue{16};
  boost::lockfree::queue<CBVar> _empty_queue{16};

  void destroy() {
    // drain queues first
    while (!_queue.empty()) {
      CBVar v;
      if (_queue.pop(v)) {
        Serialization::varFree(v);
      }
    }

    while (!_empty_queue.empty()) {
      CBVar v;
      if (_empty_queue.pop(v)) {
        Serialization::varFree(v);
      }
    }

    NetworkBase::destroy();
  }

  Serialization deserial;

  void do_receive() {
    _socket.socket->async_receive_from(
        boost::asio::buffer(&_recv_buffer().front(), _recv_buffer().size()),
        _server, [this](boost::system::error_code ec, std::size_t bytes_recvd) {
          if (!ec && bytes_recvd > 0) {
            CBVar v{};

            // try reuse vars internal memory smartly
            if (!_empty_queue.empty()) {
              _empty_queue.pop(v);
            }

            // deserialize from buffer
            Reader r(&_recv_buffer().front(), bytes_recvd);
            deserial.reset();
            deserial.deserialize(r, v);

            // process a packet
            _queue.push(v);

            // keep receiving
            do_receive();
          }
        });
  }

  CBVar activate(CBContext *context, const CBVar &input) {
    if (!_socket.socket) {
      // first activation, let's init
      _socket.socket =
          new udp::socket(_io_context, udp::endpoint(udp::v4(), 0));

      boost::asio::io_service io_service;
      udp::resolver resolver(io_service);
      auto sport = std::to_string(_port.get().payload.intValue);
      udp::resolver::query query(udp::v4(), _addr.get().payload.stringValue,
                                 sport);
      _server = *resolver.resolve(query);

      // start receiving
      boost::asio::post(_io_context, [this]() { do_receive(); });
    }

    setSocket(context);
    // in the case of client we actually set the remote here
    _socket.endpoint = &_server;

    // receive from ringbuffer and run chains
    while (!_queue.empty()) {
      CBVar v;
      if (_queue.pop(v)) {
        CBVar output{};
        activateBlocks(CBVar(_blks).payload.seqValue, context, v, output);
        // release the var once done
        // will recycle internal buffers
        _empty_queue.push(v);
      }
    }

    return input;
  }

  udp::endpoint _server;
};

// Register
RUNTIME_BLOCK(Network, Client);
RUNTIME_BLOCK_setup(Client);
RUNTIME_BLOCK_cleanup(Client);
RUNTIME_BLOCK_warmup(Client);
RUNTIME_BLOCK_destroy(Client);
RUNTIME_BLOCK_inputTypes(Client);
RUNTIME_BLOCK_outputTypes(Client);
RUNTIME_BLOCK_parameters(Client);
RUNTIME_BLOCK_setParam(Client);
RUNTIME_BLOCK_getParam(Client);
RUNTIME_BLOCK_activate(Client);
RUNTIME_BLOCK_compose(Client);
RUNTIME_BLOCK_END(Client);

struct Send {
  // Must take an optional seq of SocketData, to be used properly by server
  // This way we get also a easy and nice broadcast

  ThreadShared<std::array<char, 0xFFFF>> _send_buffer;

  CBVar *_socketVar = nullptr;

  void cleanup() {
    // clean context vars
    if (_socketVar) {
      releaseVariable(_socketVar);
      _socketVar = nullptr;
    }
  }

  SocketData *getSocket(CBContext *context) {
    if (!_socketVar) {
      _socketVar = referenceVariable(context, "Network.Socket");
    }
    assert(_socketVar->payload.objectVendorId == CoreCC);
    assert(_socketVar->payload.objectTypeId == SocketCC);
    return reinterpret_cast<SocketData *>(_socketVar->payload.objectValue);
  }

  static CBTypesInfo inputTypes() { return CoreInfo::AnyType; }
  static CBTypesInfo outputTypes() { return CoreInfo::AnyType; }

  Serialization serializer;

  CBVar activate(CBContext *context, const CBVar &input) {
    auto socket = getSocket(context);
    NetworkBase::Writer w(&_send_buffer().front(), _send_buffer().size());
    serializer.reset();
    auto size = serializer.serialize(input, w);
    // use async, avoid syscalls!
    socket->socket->send_to(boost::asio::buffer(&_send_buffer().front(), size),
                            *socket->endpoint);
    return input;
  }
};

// Register
RUNTIME_BLOCK(Network, Send);
RUNTIME_BLOCK_cleanup(Send);
RUNTIME_BLOCK_inputTypes(Send);
RUNTIME_BLOCK_outputTypes(Send);
RUNTIME_BLOCK_activate(Send);
RUNTIME_BLOCK_END(Send);
}; // namespace Network

void registerNetworkBlocks() {
  REGISTER_BLOCK(Network, Server);
  REGISTER_BLOCK(Network, Client);
  REGISTER_BLOCK(Network, Send);
}
}; // namespace chainblocks
