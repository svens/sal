#pragma once

#include <sal/config.hpp>
#include <sal/logger/fwd.hpp>
#include <sal/logger/event.hpp>
#include <sal/logger/sink.hpp>
#include <sal/assert.hpp>
#include <sal/file.hpp>
#include <string>


namespace sal { namespace logger {
__sal_begin


namespace __bits {


template <int Tag, typename T>
struct file_sink_option_t
{
  T value;

  file_sink_option_t (const T &value)
    : value(value)
  {}
};


using file_dir = file_sink_option_t<1, std::string>;
using file_max_size = file_sink_option_t<3, size_t>;
using file_buffer_size = file_sink_option_t<2, size_t>;


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

    file_t file = make_file();
    swap_file(file);
  }


  virtual ~file_sink_t ()
  {
    flush();
  }


private:

  file_t file_;

  const std::string suffix_;
  std::string dir_ = ".";
  size_t max_size_ = 0;
  std::unique_ptr<std::string> buffer_{};


  file_t make_file ();


  void event_write (event_t &event) final override;


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
};


} // namespace __bits


__sal_end
}} // namespace sal::logger
