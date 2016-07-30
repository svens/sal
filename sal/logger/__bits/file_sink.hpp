#pragma once

#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/assert.hpp>
#include <sal/file.hpp>
#include <sal/spinlock.hpp>
#include <mutex>
#include <string>


namespace sal { namespace logger {
__sal_begin


namespace __bits {


template <int Tag, typename T>
struct file_sink_option_t
{
  T value;

  explicit file_sink_option_t (const T &value)
    : value(value)
  {}
};


using file_dir = file_sink_option_t<1, std::string>;
using file_max_size = file_sink_option_t<2, size_t>;
using file_buffer_size = file_sink_option_t<3, size_t>;
using file_utc_time = file_sink_option_t<4, bool>;


class file_sink_t final
  : public sink_t
{
public:

  template <typename... Options>
  file_sink_t (const std::string &label, Options &&...options)
    : suffix_('_' + label + ".log")
  {
    bool unused[] = { set_option(std::forward<Options>(options))..., false };
    (void)unused;

    auto file = make_file();
    swap_file(file);
  }


  virtual ~file_sink_t ()
  {
    flush();
  }


private:

  using mutex_t = spinlock_t;
  using lock_t = std::lock_guard<mutex_t>;
  mutex_t mutex_;

  file_t file_;
  const std::string suffix_;
  std::string dir_ = ".";
  std::unique_ptr<std::string> buffer_{};
  bool utc_time_ = true;

  // if max_size_ == 0, size_ has undefined value
  size_t max_size_ = 0, size_ = 0;


  file_t make_file ();


  void sink_event_init (event_t &event, const std::string &channel_name)
    final override
  {
    event.time = utc_time_ ? now() : sink_t::local_now();
    sink_t::init(event, channel_name);
  }


  void sink_event_write (event_t &event) final override;


  bool set_option (file_dir &&option)
  {
    dir_ = std::move(option.value);
    return false;
  }


  bool set_option (file_max_size &&option)
  {
    max_size_ = option.value;
    return false;
  }


  bool set_option (file_utc_time &&option)
  {
    utc_time_ = option.value;
    return false;
  }


  bool set_option (file_buffer_size &&option)
  {
    sal_assert(buffer_ == nullptr);

    if (option.value)
    {
      constexpr auto min_buffer_size = 2 * event_t::max_message_size;
      if (option.value < min_buffer_size)
      {
        option.value = min_buffer_size;
      }

      buffer_ = std::make_unique<std::string>();
      buffer_->reserve(option.value);
    }

    return false;
  }


  void flush ()
  {
    if (buffer_ && buffer_->size())
    {
      file_.write(buffer_->data(), buffer_->size());
      buffer_->clear();
    }
  }


  void swap_file (file_t &file)
  {
    swap(file_, file);
  }


  void rotate ()
  {
    if (auto file = make_file())
    {
      flush();
      swap_file(file);
    }
  }
};


} // namespace __bits


__sal_end
}} // namespace sal::logger
