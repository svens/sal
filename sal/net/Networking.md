<!--- \defgroup net Networking -->

\tableofcontents


Overview {#overview}
========

sal::net module provides low-level networking services. Socket operations may
be performed in synchronous or asynchonous manner. Here is the conceptual
overview how different pieces work together.


Synchronous operations {#sync-mode}
----------------------

In synchronous API, sockets' API follows usual BSD API. \ref error-handling is
documented separately.

Example client side session:
```{.cpp}
// Connect to remote server that listens at server_endpoint
sal::net::ip::tcp_t::socket_t channel;
channel.connect(server_endpoint);

// Send data to server ('hello\0')
auto sent_size = channel.send("hello");

// Receive data from server (up to 1024B)
char data[1024];
auto received_size = channel.receive(data);
```

Same session from server side:
```{.cpp}
// Listen for incoming connections at listen_endpoint
sal::net::ip::tcp_t::acceptor_t acceptor(listen_endpoint, true);

// Accept new incoming connection
auto channel = acceptor.accept();

// Echo data from client back to it
char data[1024];
auto size = channel.receive(data);
channel.send(sal::make_buf(data, size));
```


Asynchronous operations {#async-mode}
-----------------------

For asynchronous API, there are multiple parts working together:
  - sal::net::async::service_t wraps OS-specific proactor (IOCP) or reactor
    (epoll/kqueue) into unified proactor-based class
  - sal::net::async::io_t represents single asynchronous operation and related
    I/O buffer (size above MTU size but below 2kB)

There is usually one sal::net::async::service_t instance per application. Each
worker thread that wants to receive I/O completion notifications, does it by
invoking repeatedly sal::net::async::service_t::wait() (or similar related
methods). This method returns one completed operation or if returned object
bool cast returns false, there is none. This instance is generic completed
asynchronous opoerations. To detect which operation exactly finished, use
sal::net::async::io_t::get_if<ResultType>() that returns pointer to result
data or ```nullptr``` if completed operation is not ResultType. Possible
ResultType types can be found in specific socket class as nested structs with
name ```OperationType_t``` (for example,
sal::net::basic_datagram_socket_t::receive_from_t, etc).

Sample usage (pseudocode):

```{.cpp}
// global I/O completion poller
sal::net::async::service_t io_svc;

// create socket and associate it with service to support asynchronous I/O
using socket_t = sal::net::ip::udp_t::socket_t;
socket_t socket;
socket.associate(io_svc);

// allocate asynchronous I/O handle (application is owner)
sal::net::async::io_t io = io_svc.make_io();

// start asynchronous recvfrom (io_svc is owner)
socket.start_receive_from(std::move(io));

// handle completions while not stopped
while (!stopped)
{
  // block until thre are completions to handle
  // (or if any operation is already completed, return immediately)
  if (auto io = io_svc.wait())
  {
    // application owns io

    // detect and handle completed I/O type
    if (auto result = io.get_if<socket_t::receive_from_t>())
    {
      // received data from result->remote_endpoint
      // in [io.data(), io.data() + result->transferred)
    }
    else if (auto result = io.get_if<socket_t::send_to_t>())
    {
      // ...
    }

    // application owns io. It can be reused for different operation. If not
    // reused, on leaving scope it is automatically released into pool (io_svc
    // becomes owner)
  }
}
```


Error handling {#error-handling}
==============

Synchronous methods {#error-handling-sync}
-------------------

Each networking library synchronous method provide two overloads, one that
throws an exception, and another that sets an ```std::error_code&``` (last
parameter). This supports common use cases:
  - cases where system errors are truly exceptional and indicate serious
    failure. In this case, throwing an exception is the most appropriate
    response.
  - cases where syscall errors are routine and do not necessarily represent
    failure. Returning an error code in the most appropriate response. This
    allows application-specific error handling, including simply ignoring
    errors.

Methods not having last argument ```std::error_code&``` report errors as
follows:
  - when underlying calls result in error that prevents method succeeding, the
    method exists via an exception of type ```std::system_error```
  - destructors throw nothing

Methods with last argument ```std::error_code&``` report errors as follows:
  - when underlying calls result in error that prevents method succeeding, the
    method sets passed error variable reference to result error and exits
    immediately. Otherwise, returned error is set such that ```!error``` is
    ```true```.


Asynchronous methods {#error-handing-async}
--------------------

In asynchronous invocations, errors are not returned/thrown immediately (even
if detected) but during operation result handling. This simplifies
application's completions and errors handling.

Error handling is similar to \ref error-handling-sync except error code is
returned during operation type detection call.
