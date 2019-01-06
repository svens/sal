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


template <typename Transform>
inline std::string_view append (const std::string_view &piece,
  std::string &out,
  Transform transform) noexcept
{
  auto original_length = out.length();
  for (auto ch: piece)
  {
    out += transform(ch);
  }
  return {&out[0] + original_length, piece.length()};
}


inline std::string_view append_decoded (
  const std::string_view &piece,
  std::string &out)
{
  auto original_length = out.length();
  decode(piece.begin(), piece.end(), std::back_inserter(out));
  return {&out[0] + original_length, out.length() - original_length};
}


inline std::string_view append_decoded_and_normalized (
  std::string_view &piece,
  std::string &out)
{
  return append_decoded(piece, out);
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


inline char to_lower (char ch) noexcept
{
  return static_cast<char>(std::tolower(ch));
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
      view_.scheme = append(view_.scheme, uri_, to_lower);
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
        view_.host = append(view_.host, uri_, to_lower);
      }

      if (view_.has_port())
      {
        if (view_.port_value != scheme.default_port)
        {
          uri_ += ':';
          view_.port = append_decoded(view_.port, uri_);
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

    if (view_.has_path())
    {
      view_.path = append_decoded_and_normalized(view_.path, uri_);
    }
    else
    {
      view_.path = append_decoded(scheme.default_path, uri_);
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


std::string uri_t::to_encoded_string (std::error_code &error) const noexcept
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
