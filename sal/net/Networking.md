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
auto sent_size = channel.send(sal::make_buf("hello"));

// Receive data from server (up to 1024B)
char data[1024];
auto received_size = channel.receive(sal::make_buf(data));
```

Same session from server side:
```{.cpp}
// Listen for incoming connections at listen_endpoint
sal::net::ip::tcp_t::acceptor_t acceptor(listen_endpoint, true);

// Accept new incoming connection
auto channel = acceptor.accept();

// Echo data from client back to it
char data[1024];
auto size = channel.receive(sal::make_buf(data));
channel.send(sal::make_buf(data, size));
```



Asynchronous operations {#async-mode}
-----------------------

For asynchronous API, there are multiple parts working together:
  - sal::net::async_service_t provides internal OS-specific underlying
    functionality (IOCP/epoll/kqueue handle, used by
    sal::net::async_service_t::context_t, not application directly).
  - sal::net::async_service_t::context_t maintains thread-specific
    resources and forwards asynchronous calls completions to application
    domain
  - sal::net::async_service_t::io_t represents single asynchronous
    operations and related resources.

There is usually one sal::net::async_service_t per application. Each
thread that invokes callbacks from OS to application, does it by invoking
repeatedly method sal::net::async_service_t::context_t::poll that
returns one completed operation or ```nullptr``` if there is none. Returned
sal::net::io_ptr (std::unique_ptr to sal::net::async_service_t::io_t) is
generic completed asynchronous operation. To detect which exactly,
call ```SOCKET_TYPE::async_OPERATION_result```. If buffer represents given
operation, pointer to result data is returned or ```nullptr``` otherwise.

Sample usage (pseudocode):
```{.cpp}
// global I/O service
sal::net::async_service_t svc;

// for each thread
sal::net::async_service_t::context_t ctx = svc.make_context();

// create channel associated with async service
using socket_t = sal::net::ip::udp_t::socket_t;
socket_t channel;
channel.associate(svc);

// allocate asynchronous operation handle (application owns io)
sal::net::io_ptr io = ctx.make_io();

// start asynchronous receive_from (networking library owns io)
channel.async_receive_from(std::move(io));

// handle completions while not stopped
while (!stopped)
{
  // block and wait until there are completions to handle
  // (if some operation has already completed, returns immediately)
  if (auto io = ctx.poll())
  {
    if (auto *result = socket_t::async_receive_from_result(io))
    {
      // handle completed receive from, result provides information about it
    }
    // else: list other operations' result handlers
  }

  // application owns io: it can be reused or simply dropped, in which
  // case ctx will automatically return it to internal free list
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
