#include <sal/uri/uri.hpp>
#include <sal/uri/encoding.hpp>
#include <cctype>
#include <unordered_map>


__sal_begin


namespace uri {


namespace {


std::unordered_map<std::string_view, const scheme_t *> schemes_ =
{
  { "ftp", &ftp_scheme() },
  { "http", &http_scheme() },
  { "https", &https_scheme() },
  { "mailto", &mailto_scheme() },
};


inline const scheme_t &get_scheme (const view_t &uri) noexcept
{
  auto it = schemes_.find(uri.scheme);
  if (it != schemes_.end())
  {
    return *it->second;
  }
  return generic_scheme();
}


inline std::string_view append (const std::string_view &piece, std::string &out)
  noexcept
{
  auto original_length = out.length();
  out += piece;
  return {&out[0] + original_length, out.length() - original_length};
}


inline std::string_view append_decoded (
  const std::string_view &piece,
  std::string &out)
{
  auto original_length = out.length();
  decode(piece.begin(), piece.end(), std::back_inserter(out));
  return {&out[0] + original_length, out.length() - original_length};
}


inline std::string_view to_lower (std::string_view view) noexcept
{
  // ugly stuff but we know we own it
  auto first = const_cast<char *>(view.data());
  auto last = first + view.length();
  std::transform(first, last, first,
    [](char ch)
    {
      return static_cast<char>(std::tolower(ch));
    }
  );
  return view;
}


constexpr bool starts_with (std::string_view data, std::string_view prefix)
  noexcept
{
  return data.size() >= prefix.size()
      && data.compare(0, prefix.size(), prefix) == 0;
}


inline void pop_last_segment (std::string &path, size_t original_length) noexcept
{
  auto last_segment_start = path.rfind('/');
  if (last_segment_start != std::string::npos
    && last_segment_start >= original_length)
  {
    path.erase(last_segment_start);
  }
  else
  {
    path.erase(original_length);
  }
}


inline void copy_first_segment (std::string &output, std::string_view &input)
  noexcept
{
  while (!input.empty())
  {
    output.push_back(input.front());
    input.remove_prefix(1);
    if (!input.empty() && input.front() == '/')
    {
      return;
    }
  }
}


inline std::string_view append_decoded_and_normalized (
  std::string_view &piece,
  std::string &output)
{
  // https://tools.ietf.org/html/rfc3986#section-5.2.4

  // 1
  std::string tmp;
  tmp.reserve(piece.length());
  auto input = append_decoded(piece, tmp);

  // 2
  auto original_length = output.length();
  while (!input.empty())
  {
    // A
    if (starts_with(input, "../"))
    {
      input.remove_prefix(3);
    }
    else if (starts_with(input, "./"))
    {
      input.remove_prefix(2);
    }

    // B
    else if (starts_with(input, "/./"))
    {
      input.remove_prefix(2);
    }
    else if (input == "/.")
    {
      input = "/";
    }

    // C
    else if (starts_with(input, "/../"))
    {
      input.remove_prefix(3);
      pop_last_segment(output, original_length);
    }
    else if (input == "/..")
    {
      input = "/";
      pop_last_segment(output, original_length);
    }

    // D
    else if (input == "." || input == "..")
    {
      input = {};
    }

    // E
    else
    {
      copy_first_segment(output, input);
    }
  }

  // 3
  return {&output[0] + original_length, output.length() - original_length};
}


inline size_t estimated_length (const view_t &uri) noexcept
{
  return uri.scheme.length()
    + uri.user_info.length()
    + uri.host.length()
    + uri.port.length()
    + uri.path.length()
    + uri.query.length()
    + uri.fragment.length()
    + sizeof("://@:/?#")
  ;
}


} // namespace


void uri_t::register_scheme (std::string_view name, const scheme_t &scheme)
{
  schemes_.insert_or_assign(name, &scheme);
}


void uri_t::init (std::error_code &error) noexcept
{
  // * during entry, view_ points to original URI
  // * while copying components to uri_, view_ will be updated
  // * on any error, view_ will be in undefined state

  try
  {
    // uri_ can only shrink due decoding 3B -> 1B
    uri_.reserve(estimated_length(view_));

    if (view_.has_scheme())
    {
      view_.scheme = to_lower(append(view_.scheme, uri_));
      uri_ += ':';
    }
    const auto &scheme = get_scheme(view_);

    if (view_.has_authority())
    {
      uri_ += "//";

      if (view_.has_user_info())
      {
        view_.user_info = append_decoded(view_.user_info, uri_);
        uri_ += '@';
      }

      if (view_.has_host())
      {
        view_.host = to_lower(append_decoded(view_.host, uri_));
      }

      if (view_.has_port())
      {
        if (!view_.port.empty() && view_.port_value != scheme.default_port)
        {
          uri_ += ':';
          view_.port = append(view_.port, uri_);
        }
        else
        {
          view_.port = {};
        }
      }
      else
      {
        view_.port_value = scheme.default_port;
      }
    }

    view_.path = append_decoded_and_normalized(view_.path, uri_);
    if (!view_.path.empty())
    {
      if (scheme.case_insensitive_path)
      {
        to_lower(view_.path);
      }
    }
    else
    {
      view_.path = append(scheme.default_path, uri_);
    }

    if (view_.has_query())
    {
      uri_ += '?';
      view_.query = append_decoded(view_.query, uri_);
    }

    if (view_.has_fragment())
    {
      uri_ += '#';
      view_.fragment = append_decoded(view_.fragment, uri_);
    }
  }
  catch (const std::system_error &ex)
  {
    error = ex.code();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
  }
}


std::string uri_t::encoded_string (std::error_code &error) const noexcept
{
  std::string result;

  try
  {
    // assume everything is percent encoded
    result.reserve(3 * estimated_length(view_));
    auto result_append = std::back_inserter(result);

    if (view_.has_scheme())
    {
      result += view_.scheme;
      result += ':';
    }

    if (view_.has_authority())
    {
      result += "//";

      if (view_.has_user_info())
      {
        encode_user_info(
          view_.user_info.begin(),
          view_.user_info.end(),
          result_append
        );
        result += '@';
      }

      if (view_.has_host())
      {
        result += view_.host;
      }

      if (view_.has_port())
      {
        result += ':';
        result += view_.port;
      }
    }

    if (view_.has_path())
    {
      encode_path(
        view_.path.begin(),
        view_.path.end(),
        result_append
      );
    }

    if (view_.has_query())
    {
      result += '?';
      encode_query(
        view_.query.begin(),
        view_.query.end(),
        result_append
      );
    }

    if (view_.has_fragment())
    {
      result += '#';
      encode_fragment(
        view_.fragment.begin(),
        view_.fragment.end(),
        result_append
      );
    }

    result.shrink_to_fit();
  }
  catch (const std::bad_alloc &)
  {
    error = std::make_error_code(std::errc::not_enough_memory);
    result.clear();
  }

  return result;
}


} // namespace uri


__sal_end
