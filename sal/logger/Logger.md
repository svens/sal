<!--- \defgroup logger Logging utilities -->

# Overview

sal::logger implements very simple logging package. Unlike most logging
libraries, it has no levels and logging thresholds. Instead, it uses channel
concept. Each channel has own prefix can be independently turned on/off. This
approach keeps logging library simpler and same time lets application layer
define own levels as necessary. For example, application has `auth` module. To
split log events into prioritised categories, application can define
`auth.error`, `auth.info` and `auth.debug` channels.

There are three participants in logging event data flow:
  * `sal::logger::sink_t`: event message output device (file, console etc).
  * `sal::logger::channel_t`: logging destination of grouped events.
  * `sal::logger::basic_worker_t`: maintains lifecycle and mappings between
    sinks and channels.


## Worker (`sal::logger::basic_worker_t`)

Maintains mapping between channels and sinks. All channels in application are
owned by some worker. SAL provides two worker implementations:
  * `sal::logger::worker_t`: simple worker that does no allocations and logs
    event messages to sink in caller thread context. If there is no much
    logging expected, this is simple and sufficient approach.
  * `sal::logger::async_worker_t`: threaded worker. This worker creates thread
    that spins on event queue. On new event, it's message is written to sink
    and event record is put into freelist from where next event can reuse
    it's storage. This worker is recommended for multithreaded applications
    that might log quite much.

Application can instantiate multiple workers for different submodules to
distribute logging contention if necessary. Each channel belongs to single
worker that created it but sinks can be shared between multiple channels and
workers (assuming sink itself has proper internal synchronisation in place).


## Channel (`sal::logger::channel_t`)

Application can create number of channels for it's logging categorising,
grouping etc purposes. Each channel is identified by application assigned
name. Channels can be independently turned on/off. This is main API for
application to check if logging is enabled and create new event record if it
is (sal/logger/logger.hpp provides macros for more convenient usage).

For simplicity and thread-safety, channels settings are set during
construction and are immutable later except turning enabled flag on/off. Also,
once channel is created, there is no methods to remove it again.

Each channel has tied to single sink where it sends it's events.


## Sink (`sal::logger::sink_t`)

Sink's responsibility is to do initial event formatting during logging and
once message is completed, to write it out to final destination. `sink_t`
itself if abstract base class. Inherited implementations must provide actual
writing functionality (for example, `sal::logger::file()`). Single sink can be
shared between multiple channels and/or workers. See class documentation for
more information how to handle threading issues when sink is shared between
channels that belong to different workers.


# Usage

Simplest usage with default worker and channel for commmand-line applications
that log to console:
```
sal_print << "hello, world";
```

Redirect default logging to different sink (to std::cout in this example):
```
sal::logger::make_default_worker(
  sal::logger::set_channel_sink(std::cout)
);
sal_print << "hello, world";
```

Add new channel to default worker using default settings:
```
auto channel = sal::logger::default_worker().make_channel("app");
sal_log(channel) << "hello, world";
```

Add new channel to default worker using new sink:
```
auto channel = sal::logger::default_worker().make_channel("app",
  sal::logger::set_channel_sink(std::cout)
);
sal_log(channel) << "hello, world";
```

Create new worker with default setting for all new channels that do not set
own settings:
```
sal::logger::worker_t worker(
  sal::logger::set_channel_sink(std::cout)
);
auto channel = worker.make_channel("app");
sal_log(channel) << "hello, world";
```

Create new worker and add multiple channels with own settings:
```
sal::logger::worker_t worker;

auto app_info = worker.make_channel("app.info",
  sal::logger::set_channel_sink(std::cout)
);
auto app_error = worker.make_channel("app.error",
  sal::logger::set_channel_sink(std::cerr)
);

sal_log(app_info) << "app info";
sal_log(app_error) << "app error";
```
