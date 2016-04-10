#include <sal/view.hpp>


namespace sal {
__sal_cpp_begin


namespace __bits {


namespace {

constexpr auto max_float_repr = 26;

} // namespace


char *copy_v (float value, char *first, char *last) noexcept
{
  if (last - first > max_float_repr)
  {
    // happy path, result fits directly into view
    return first + std::snprintf(first, last - first, "%g", value);
  }

  // suspicious, result may fit, go through temporary buffer
  char data[max_float_repr + 1];
  auto end = data + std::snprintf(data, sizeof(data), "%g", value);
  return copy_str(data, end, first, last);
}


char *copy_v (double value, char *first, char *last) noexcept
{
  if (last - first > max_float_repr)
  {
    // happy path, result fits directly into view
    return first + std::snprintf(first, last - first, "%g", value);
  }

  // suspicious, result may fit, go through temporary buffer
  char data[max_float_repr + 1];
  auto end = data + std::snprintf(data, sizeof(data), "%g", value);
  return copy_str(data, end, first, last);
}


char *copy_v (const long double &value, char *first, char *last) noexcept
{
  if (last - first > max_float_repr)
  {
    // happy path, result fits directly into view
    return first + std::snprintf(first, last - first, "%Lg", value);
  }

  // suspicious, result may fit, go through temporary buffer
  char data[max_float_repr + 1];
  auto end = data + std::snprintf(data, sizeof(data), "%Lg", value);
  return copy_str(data, end, first, last);
}


} // namespace __bits


__sal_cpp_end
} // namespace sal
