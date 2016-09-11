#pragma once

/**
 * \file sal/logger/file_sink.hpp
 * Logging sink that writes event messages into file.
 *
 * \addtogroup logger
 * \{
 */


#include <sal/config.hpp>
#include <sal/logger/__bits/file_sink.hpp>


namespace sal { namespace logger {
__sal_begin


/**
 * Return option to configure file sink directory where logfiles are stored.
 * If not configured, default is current working directory.
 */
inline auto set_file_dir (const std::string &dir)
{
  return __bits::file_dir(dir);
}


/**
 * Return options to configure maximum logfile size (in MB). Whenever
 * \a size is reached, file sink closes existing file and starts new file.
 * See file() documentation about file naming schema. If not set, max size is
 * not limited.
 */
inline auto set_file_max_size_mb (size_t size) noexcept
{
  return __bits::file_max_size(1024 * 1024 * size);
}


/**
 * Return option to configure file sink buffering. If \a size is zero,
 * event messages are written to file immediately. Otherwise events are
 * temporarily gathered into buffer with \a size kB and written only when
 * buffer is full.
 *
 * Buffering improves performance but has certain disadvantages:
 *   - performance boost is penalised during buffer flushing
 *   - when application crashes, buffer remains unflushed
 *
 * Choose appropriate buffering strategy as required by application. If not
 * set, default is not to buffer.
 *
 * \todo Catch crash signals and flush buffers.
 */
inline auto set_file_buffer_size_kb (size_t size) noexcept
{
  return __bits::file_buffer_size(1024 * size);
}


/**
 * Return option to configure whether file sink uses local or UTC time. If not
 * set, UTC time is used.
 */
inline auto set_file_utc_time (bool on) noexcept
{
  return __bits::file_utc_time(on);
}


/**
 * Create new file sink with \a label and \a options.
 *
 * Argument \a label is used to create actual logfile name (in directory
 * configured with set_file_dir()):
 * \code{.txt}
 * {YYYY}-{MM}-{DD}T{hh}{mm}{ss}_{label}.log
 * \endcode
 *
 * Possible \a options:
 *   - set_file_dir(): set directory where logfiles are created
 *   - set_file_max_size_mb(): maximum single file size (in kB)
 *   - set_file_buffer_size_kb(): configure file buffering
 *   - set_file_utc_time(): configure whether to use UTC or local time
 *
 * Logfile is closed and new is started whenever current size reaches
 * configured maximum size. If file already exists with given name and size
 * exceeds maximum size, numeric index will be appended to filename (in range
 * .0 - .999). If sizes of all those files exceed maximum size, filename with
 * index .999 will be forced to use (regardless of it's size)
 *
 * Also, log file is rotated every midnight (using UTC or local time,
 * depending on how set_file_utc_time() is set).
 */
template <typename... Options>
sink_ptr file (const std::string &label, Options &&...options)
{
  return std::make_shared<__bits::file_sink_t>(label,
    std::forward<Options>(options)...
  );
}


__sal_end
}} // namespace sal::logger

/// \}
