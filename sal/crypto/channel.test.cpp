#include <sal/crypto/channel.hpp>
#include <sal/crypto/common.test.hpp>
#include <sal/buf_ptr.hpp>

#if __sal_os_linux
  #include <openssl/opensslv.h>
#endif


namespace {


#if defined(OPENSSL_VERSION_NUMBER)
  constexpr int openssl_version = OPENSSL_VERSION_NUMBER;
#else
  // to simplify test exclusions below
  constexpr int openssl_version = 0;
#endif


std::pair<std::vector<sal::crypto::certificate_t> *, sal::crypto::private_key_t *>
  import_pkcs12 ()
{
  static sal::crypto::private_key_t private_key;
  static auto chain = sal::crypto::import_pkcs12(
    sal_test::to_der(sal_test::cert::pkcs12),
    "TestPassword",
    &private_key
  );
  return {&chain, &private_key};
}


inline auto chain ()
{
  return sal::crypto::with_chain(*import_pkcs12().first);
}


inline auto private_key ()
{
  return sal::crypto::with_private_key(import_pkcs12().second);
}


inline const std::vector<uint8_t> &expected_serial_number ()
{
  static std::vector<uint8_t> number = { 0x10, 0x01, };
  return number;
}


struct datagram_t
{
  virtual ~datagram_t () = default;

  template <typename... Option>
  static auto client_factory (
    const sal::crypto::channel_factory_option_t<Option> &...option)
  {
    return sal::crypto::datagram_client_channel_factory(option...);
  }

  template <typename... Option>
  static auto server_factory (
    const sal::crypto::channel_factory_option_t<Option> &...option)
  {
    return sal::crypto::datagram_server_channel_factory(option...);
  }
};


struct stream_t
{
  virtual ~stream_t () = default;

  template <typename... Option>
  static auto client_factory (
    const sal::crypto::channel_factory_option_t<Option> &...option)
  {
    return sal::crypto::stream_client_channel_factory(option...);
  }

  template <typename... Option>
  static auto server_factory (
    const sal::crypto::channel_factory_option_t<Option> &...option)
  {
    return sal::crypto::stream_server_channel_factory(option...);
  }
};


template <typename ChannelFactory>
struct crypto_channel
  : public sal_test::with_type<ChannelFactory>
  , public ChannelFactory
{
  static constexpr bool datagram = std::is_same_v<ChannelFactory, datagram_t>;

  template <typename... Option>
  auto make_client_channel (
    const sal::crypto::channel_option_t<Option> &...option)
  {
    return ChannelFactory::client_factory(sal::crypto::no_chain_check)
      .make_channel(option...);
  }

  template <typename... Option>
  auto make_server_channel (
    const sal::crypto::channel_option_t<Option> &...option)
  {
    return ChannelFactory::server_factory(chain(), private_key())
      .make_channel(option...);
  }

  auto make_channel_pair ()
  {
    SCOPED_TRACE("make_channel_pair");
    return std::make_pair(make_client_channel(), make_server_channel());
  }
};


template <size_t Size>
struct buffer_t final
  : public sal::crypto::channel_t::buffer_manager_t
{
  uint8_t chunk[Size];
  std::vector<uint8_t> data{};

  std::string alloc_scope{};
  int alloc_balance = 0;

  buffer_t (std::string alloc_scope)
    : alloc_scope(alloc_scope)
  {}

  ~buffer_t ()
  {
    SCOPED_TRACE(alloc_scope);
    EXPECT_EQ(0, alloc_balance);
  }

  uintptr_t alloc (uint8_t **buffer, size_t *buffer_size)
    noexcept final override
  {
    *buffer = chunk;
    *buffer_size = sizeof(chunk);
    alloc_balance++;
    return {};
  }

  void ready (uintptr_t, uint8_t *ptr, size_t size)
    noexcept final override
  {
    data.insert(data.end(), ptr, ptr + size);
    alloc_balance--;
  }

  void trash ()
  {
    if (!data.empty())
    {
      data.assign(data.size(), 0xff);
    }
  }
};


void handshake (sal::crypto::channel_t &client,
  sal::crypto::channel_t &server,
  bool require_connected = true)
{
  SCOPED_TRACE("handshake");
  ASSERT_FALSE(client.is_connected());
  ASSERT_FALSE(server.is_connected());

  buffer_t<4096> client_buf{"client"}, server_buf{"server"};

  client.handshake(sal::const_null_buf, client_buf);
  while (!client_buf.data.empty())
  {
    server.handshake(client_buf.data, server_buf);
    client_buf.data.clear();

    client.handshake(server_buf.data, client_buf);
    server_buf.data.clear();
  }

  if (require_connected)
  {
    ASSERT_TRUE(client.is_connected());
    ASSERT_TRUE(server.is_connected());
  }
}


using channel_factory_types = ::testing::Types<
  datagram_t,
  stream_t
>;

struct channel_factory_names
{
  template <typename T>
  static std::string GetName (int i)
  {
    if constexpr (std::is_same_v<T, datagram_t>)
    {
      return "dtls";
    }
    else if constexpr (std::is_same_v<T, stream_t>)
    {
      return "tls";
    }
    return std::to_string(i);
  }
};

TYPED_TEST_CASE(crypto_channel, channel_factory_types, channel_factory_names);


TYPED_TEST(crypto_channel, handshake)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);
}


TYPED_TEST(crypto_channel, handshake_already_connected)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> out{"out"};
  std::error_code error;

  error.clear();
  client.handshake(sal::const_null_buf, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  error.clear();
  server.handshake(sal::const_null_buf, out, error);
  EXPECT_EQ(std::errc::already_connected, error);
}


template <size_t Size>
void chunked_feed (sal::crypto::channel_t &channel,
  buffer_t<Size> &in,
  buffer_t<Size> &out,
  bool datagram)
{
#if __sal_os_linux

  // TODO: debug deeper what's going on here
  if (datagram)
  {
      // OpenSSL DTLS fails when fragment is less than MTU
      auto p = in.data.data(), end = in.data.data() + in.data.size();
      while (p < end)
      {
        size_t chunk_size = 1472;
        if (p + chunk_size > end)
        {
          chunk_size = end - p;
        }
        channel.handshake(sal::make_buf(p, chunk_size), out);
        p += chunk_size;
      }
      return;
  }

#else
  (void)datagram;
#endif

  // TLS
  for (auto b: in.data)
  {
    channel.handshake(sal::make_buf(&b, 1), out);
  }
}


TYPED_TEST(crypto_channel, handshake_chunked_receive)
{
  auto [client, server] = this->make_channel_pair();
  ASSERT_FALSE(client.is_connected());
  ASSERT_FALSE(server.is_connected());

  buffer_t<4096> client_buf{"client"}, server_buf{"server"};

  client.handshake(sal::const_null_buf, client_buf);
  while (!client_buf.data.empty())
  {
    chunked_feed(server, client_buf, server_buf, TestFixture::datagram);
    client_buf.data.clear();

    chunked_feed(client, server_buf, client_buf, TestFixture::datagram);
    server_buf.data.clear();
  }

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


struct failing_buffer_t final
  : public sal::crypto::channel_t::buffer_manager_t
{
  const bool null_ptr, null_size;
  uint8_t data[4096];

  failing_buffer_t (bool null_ptr, bool null_size) noexcept
    : null_ptr(null_ptr)
    , null_size(null_size)
  { }

  uintptr_t alloc (uint8_t **buffer, size_t *buffer_size)
    noexcept final override
  {
    *buffer = null_ptr ? nullptr : data;
    *buffer_size = null_size ? 0 : sizeof(data);
    return {};
  }

  void ready (uintptr_t, uint8_t *, size_t) noexcept final override
  { }
};


TYPED_TEST(crypto_channel, handshake_client_alloc_null_ptr)
{
  auto [client, server] = this->make_channel_pair();
  (void)server;

  failing_buffer_t failing_buffer(true, false);
  std::error_code error;
  client.handshake(sal::const_null_buf, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
  EXPECT_FALSE(client.is_connected());
}


TYPED_TEST(crypto_channel, handshake_server_alloc_null_ptr)
{
  auto [client, server] = this->make_channel_pair();
  std::error_code error;

  buffer_t<4096> client_hello{"client"};
  client.handshake(sal::const_null_buf, client_hello);

  failing_buffer_t failing_buffer(true, false);
  server.handshake(client_hello.data, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TYPED_TEST(crypto_channel, handshake_client_alloc_null_size)
{
  auto [client, server] = this->make_channel_pair();
  (void)server;

  failing_buffer_t failing_buffer(false, true);
  std::error_code error;
  client.handshake(sal::const_null_buf, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
  EXPECT_FALSE(client.is_connected());
}


TYPED_TEST(crypto_channel, handshake_server_alloc_null_size)
{
  auto [client, server] = this->make_channel_pair();
  std::error_code error;

  buffer_t<4096> client_hello{"client"};
  client.handshake(sal::const_null_buf, client_hello);

  failing_buffer_t failing_buffer(false, true);
  server.handshake(client_hello.data, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TYPED_TEST(crypto_channel, handshake_alloc_not_sufficient)
{
  auto [client, server] = this->make_channel_pair();
  buffer_t<1> client_buf{"client"}, server_buf{"server"};

  client.handshake(sal::const_null_buf, client_buf);
  while (!client_buf.data.empty())
  {
    server.handshake(client_buf.data, server_buf);
    client_buf.data.clear();

    client.handshake(server_buf.data, client_buf);
    server_buf.data.clear();
  }

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


// exclude tests handshake_fail_on_XXX
template <typename ChannelFactory>
bool suppress_trashed_handshake_tests (const crypto_channel<ChannelFactory> &factory)
{
#if __sal_os_macos
  // SecureTransport: due chosen trashing content, throws buffer overflow
  (void)factory;
  return false;
#elif __sal_os_linux
  // OpenSSL/DTLS: ignores trashed messages (correctly)
  return factory.datagram;
#elif __sal_os_windows
  // SChannel: accepts trashed message, asking for more instead of error
  (void)factory;
  return true;
#endif
}


TYPED_TEST(crypto_channel, handshake_fail_on_invalid_client_hello)
{
  if (suppress_trashed_handshake_tests(*this))
  {
    return;
  }

  auto [client, server] = this->make_channel_pair();

  buffer_t<4096> client_buf{"client"}, server_buf{"server"};
  client.handshake(sal::const_null_buf, client_buf);

  std::error_code error;
  client_buf.trash();
  server.handshake(client_buf.data, server_buf, error);
  EXPECT_FALSE(!error);
}


TYPED_TEST(crypto_channel, handshake_fail_on_invalid_server_hello)
{
  if (suppress_trashed_handshake_tests(*this))
  {
    return;
  }

  auto [client, server] = this->make_channel_pair();

  buffer_t<4096> client_buf{"client"}, server_buf{"server"};
  client.handshake(sal::const_null_buf, client_buf);
  server.handshake(client_buf.data, server_buf);

  std::error_code error;
  server_buf.trash();
  client.handshake(server_buf.data, client_buf, error);
  EXPECT_FALSE(!error);
}


TYPED_TEST(crypto_channel, handshake_fail_on_invalid_key_exchange)
{
  if (suppress_trashed_handshake_tests(*this))
  {
    return;
  }

  auto [client, server] = this->make_channel_pair();

  // generate client_hello
  buffer_t<4096> client_buf{"client"}, server_buf{"server"};
  client.handshake(sal::const_null_buf, client_buf);

  // server <- client_hello, generate server_hello
  server.handshake(client_buf.data, server_buf);
  client_buf.data.clear();

  // client <- server_hello, generate key_exchange
  client.handshake(server_buf.data, client_buf);
  server_buf.data.clear();

  std::error_code error;
  client_buf.trash();
  server.handshake(client_buf.data, server_buf, error);
  EXPECT_FALSE(!error);
}


TYPED_TEST(crypto_channel, client_encrypt_message)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  std::string message(secret.data.begin(), secret.data.end());
  EXPECT_EQ(std::string::npos, message.find(this->case_name));

  buffer_t<4096> plain{"plain"};
  server.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, server_encrypt_message)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};
  server.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  std::string message(secret.data.begin(), secret.data.end());
  EXPECT_EQ(std::string::npos, message.find(this->case_name));

  buffer_t<4096> plain{"plain"};
  client.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, encrypt_not_connected)
{
  auto [client, server] = this->make_channel_pair();
  buffer_t<4096> secret{"secret"};

  std::error_code error;
  client.encrypt(this->case_name, secret, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_TRUE(secret.data.empty());

  error.clear();
  server.encrypt(this->case_name, secret, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_TRUE(secret.data.empty());
}


TYPED_TEST(crypto_channel, decrypt_not_connected)
{
  auto [client, server] = this->make_channel_pair();
  buffer_t<4096> plain{"plain"};

  std::error_code error;
  client.decrypt(this->case_name, plain, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_TRUE(plain.data.empty());

  error.clear();
  server.decrypt(this->case_name, plain, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_TRUE(plain.data.empty());
}


TYPED_TEST(crypto_channel, encrypt_alloc_null_ptr)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  std::error_code error;
  failing_buffer_t failing_buffer(true, false);
  client.encrypt(this->case_name, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error) << error.message();
}


TYPED_TEST(crypto_channel, decrypt_alloc_null_ptr)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  std::error_code error;
  failing_buffer_t failing_buffer(true, false);
  server.decrypt(secret.data, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TYPED_TEST(crypto_channel, encrypt_alloc_null_size)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  std::error_code error;
  failing_buffer_t failing_buffer(false, true);
  client.encrypt(this->case_name, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error) << error.message();
}


TYPED_TEST(crypto_channel, decrypt_alloc_null_size)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  std::error_code error;
  failing_buffer_t failing_buffer(false, true);
  server.decrypt(secret.data, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TYPED_TEST(crypto_channel, encrypt_alloc_insufficient)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<1> secret{"secret"};
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  buffer_t<4096> plain{"plain"};
  server.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, decrypt_alloc_insufficient)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  buffer_t<1> plain{"plain"};
  server.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, decrypt_coalesced)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};

  auto first = this->case_name + "_first";
  client.encrypt(first, secret);
  EXPECT_FALSE(secret.data.empty());
  auto first_size = secret.data.size();

  auto second = this->case_name + "_second";
  std::reverse(second.begin(), second.end());
  client.encrypt(second, secret);
  EXPECT_FALSE(secret.data.empty());
  auto second_size = secret.data.size() - first_size;

  // first
  buffer_t<4096> plain{"plain"};
  auto used = server.decrypt(secret.data, plain);
  EXPECT_EQ(first_size, used);
  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(first, message);
  secret.data.erase(secret.data.begin(), secret.data.begin() + used);

  // second
  plain.data.clear();
  used = server.decrypt(secret.data, plain);
  EXPECT_EQ(second_size, used);
  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(second, message);
  secret.data.erase(secret.data.begin(), secret.data.begin() + used);

  EXPECT_TRUE(secret.data.empty());
}


TYPED_TEST(crypto_channel, decrypt_chunked)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"}, plain{"plain"};
  client.encrypt(this->case_name, secret);

  for (auto b: secret.data)
  {
    auto used = server.decrypt(sal::make_buf(&b, 1), plain);
    EXPECT_EQ(1U, used);
  }

  if constexpr (openssl_version && TestFixture::datagram)
  {
    // OpenSSL DTLS correctly drops each chunk i.e. should have nothing now
    EXPECT_TRUE(plain.data.empty());
  }
  else
  {
    std::string message(plain.data.begin(), plain.data.end());
    EXPECT_EQ(this->case_name, message);
  }
}


TYPED_TEST(crypto_channel, decrypt_invalid_message)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"}, plain{"plain"};
  client.encrypt(this->case_name, secret);
  secret.trash();

  std::error_code error;
  server.decrypt(secret.data, plain, error);

  if constexpr (TestFixture::datagram)
  {
    // depending on platform, invalid DTLS datagrams may be simply ignored
    EXPECT_TRUE(plain.data.empty());
  }
  else
  {
    EXPECT_FALSE(!error);
  }
}


TYPED_TEST(crypto_channel, coalesced_server_finished_and_message)
{
  auto [client, server] = this->make_channel_pair();

  buffer_t<4096> client_buf{"client"}, server_buf{"server"};

  // usual handshake except break on server_finished
  // i.e. server is connected but client is not
  client.handshake(sal::const_null_buf, client_buf);
  while (!client_buf.data.empty())
  {
    server.handshake(client_buf.data, server_buf);
    client_buf.data.clear();
    if (server.is_connected())
    {
      break;
    }

    client.handshake(server_buf.data, client_buf);
    server_buf.data.clear();
  }

  ASSERT_FALSE(client.is_connected());
  ASSERT_TRUE(server.is_connected());
  EXPECT_FALSE(server_buf.data.empty());

  // server_buf still has server_finished, append secret
  server.encrypt(this->case_name, server_buf);

  if constexpr (openssl_version && TestFixture::datagram)
  {
    // OpenSSL DTLS correctly consumes everything (i.e. server finished and
    // message) leaving message buffered in SSL engine
    auto used = client.handshake(server_buf.data, client_buf);
    EXPECT_TRUE(client.is_connected());
    EXPECT_TRUE(client_buf.data.empty());
    server_buf.data.erase(server_buf.data.begin(), server_buf.data.begin() + used);
    EXPECT_TRUE(server_buf.data.empty());

    // Here comes ugly part: at our wrapper, we can't detect there is still
    // data in SSL engine (bug in SSL_pending?)
    used = client.decrypt(sal::const_null_buf, client_buf);
    EXPECT_EQ(0U, used);
    EXPECT_FALSE(client_buf.data.empty());
    std::string message(client_buf.data.begin(), client_buf.data.end());
    EXPECT_EQ(this->case_name, message);
  }
  else
  {
    // client <- server_finished, leaving message in server_buf
    auto used = client.handshake(server_buf.data, client_buf);
    EXPECT_TRUE(client.is_connected());
    EXPECT_TRUE(client_buf.data.empty());
    server_buf.data.erase(server_buf.data.begin(), server_buf.data.begin() + used);
    EXPECT_TRUE(!server_buf.data.empty());

    // check for decrypted message
    used = client.decrypt(server_buf.data, client_buf);
    server_buf.data.erase(server_buf.data.begin(), server_buf.data.begin() + used);
    EXPECT_TRUE(server_buf.data.empty());
    std::string message(client_buf.data.begin(), client_buf.data.end());
    EXPECT_EQ(this->case_name, message);
  }
}


TYPED_TEST(crypto_channel, decrypt_half_and_one_plus_half_messages)
{
  if constexpr (openssl_version && TestFixture::datagram)
  {
    // OpenSSL DTLS correctly drops partial handshake messages
    // nothing to test
    return;
  }

  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret{"secret"};

  auto first = this->case_name + "_first";
  client.encrypt(first, secret);
  EXPECT_FALSE(secret.data.empty());
  auto secret_1st_size = secret.data.size();

  auto second = this->case_name + "_second";
  std::reverse(second.begin(), second.end());
  client.encrypt(second, secret);
  EXPECT_FALSE(secret.data.empty());
  auto secret_2nd_size = secret.data.size() - secret_1st_size;

  // split 2 encrypted messages into half and one+half
  std::vector<uint8_t> half, one_plus_half;
  half.assign(
    secret.data.begin(),
    secret.data.begin() + secret_1st_size / 2
  );
  one_plus_half.assign(
    secret.data.begin() + secret_1st_size / 2,
    secret.data.end()
  );

  // 1st half (buffered in engine but no output)
  buffer_t<4096> plain{"plain"};
  auto used_1st_half = server.decrypt(half, plain);
  EXPECT_EQ(secret_1st_size / 2, used_1st_half);
  EXPECT_TRUE(plain.data.empty());

  // 2nd half (first message as output, do not buffer second message secret)
  auto used_2nd_half = server.decrypt(one_plus_half, plain);
  EXPECT_EQ(secret_1st_size - secret_1st_size / 2, used_2nd_half);
  EXPECT_FALSE(plain.data.empty());
  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(first, message);

  // erase used data (first secret, second secret remains)
  one_plus_half.erase(one_plus_half.begin(), one_plus_half.begin() + used_2nd_half);
  EXPECT_EQ(secret_2nd_size, one_plus_half.size());

  // second message
  plain.data.clear();
  used_2nd_half = server.decrypt(one_plus_half, plain);
  EXPECT_EQ(secret_2nd_size, used_2nd_half);
  EXPECT_FALSE(plain.data.empty());
  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(second, message);
}


size_t chain_ok_count = 0;
inline bool chain_ok (const std::vector<sal::crypto::certificate_t> &chain)
  noexcept
{
  chain_ok_count++;
  EXPECT_FALSE(chain.empty());
  if (chain.size())
  {
    EXPECT_EQ(expected_serial_number(), chain[0].serial_number());
  }
  return true;
}


size_t chain_fail_count = 0;
inline bool chain_fail (const std::vector<sal::crypto::certificate_t> &chain)
  noexcept
{
  chain_fail_count++;
  EXPECT_FALSE(chain.empty());
  if (chain.size())
  {
    EXPECT_EQ(expected_serial_number(), chain[0].serial_number());
  }
  return false;
}


TYPED_TEST(crypto_channel, certificate_check_success)
{
  auto client_factory = TestFixture::client_factory(
    sal::crypto::chain_check(chain_ok)
  );
  auto client = client_factory.make_channel();
  auto server = TestFixture::make_server_channel();

  size_t expected_chain_ok_count = chain_ok_count + 1;
  handshake(client, server, true);
  EXPECT_EQ(expected_chain_ok_count, chain_ok_count);
}


TYPED_TEST(crypto_channel, certificate_check_fail)
{
  auto client_factory = TestFixture::client_factory(
    sal::crypto::chain_check(chain_fail)
  );
  auto client = client_factory.make_channel();
  auto server = TestFixture::make_server_channel();

  size_t expected_chain_fail_count = chain_fail_count + 1;
  try
  {
    handshake(client, server, false);
    FAIL() << "unexpected success";
  }
  catch (const std::system_error &)
  {
    SUCCEED();
  }
  EXPECT_EQ(expected_chain_fail_count, chain_fail_count);

  EXPECT_FALSE(client.is_connected());

#if __sal_os_linux
  // client continues handshakes even after rejecting certificate, making
  // server side succeed connecting
#elif __sal_os_macos
  EXPECT_FALSE(server.is_connected());
#elif __sal_os_windows
  // querying server provided certificate is possible only after all the
  // handshakes i.e. server is now connected, not knowing client has rejected
  // certificate
#endif
}


//
// peer_name tests are disabled by default because these reqire trusting our
// self-signed certificate chain. Ok for local tests, too much hassle on CI.
//


TYPED_TEST(crypto_channel, DISABLED_peer_name_success)
{
  auto client_factory = TestFixture::client_factory();
  auto client = client_factory.make_channel(
    // in case of CN & SAN in cert, SAN is used
    sal::crypto::peer_name("sal.alt.ee")
  );
  auto server = TestFixture::make_server_channel();
  handshake(client, server, true);
}


TYPED_TEST(crypto_channel, DISABLED_peer_name_fail)
{
  if constexpr (openssl_version && openssl_version < 0x10100000)
  {
    // not implemented yet:
    // <1.0.2: X509_check_host not supported
    // <1.1: X509_VERIFY_PARAM_set1_host not supported
    FAIL() << "not implemented for <OpenSSL-1.1";
    return;
  }

  auto client_factory = TestFixture::client_factory();
  auto client = client_factory.make_channel(
    sal::crypto::peer_name("invalid.name")
  );
  auto server = TestFixture::make_server_channel();

  EXPECT_THROW(
    handshake(client, server, false),
    std::system_error
  );

  EXPECT_FALSE(client.is_connected());
  EXPECT_FALSE(server.is_connected());
}


TYPED_TEST(crypto_channel, peer_name_not_trusted)
{
  if (!TestFixture::on_ci)
  {
    // tests above cover cases where certificate is trusted in system store
    return;
  }
  // else this test assumes certificate is not trusted in system store

  auto client_factory = TestFixture::client_factory();
  auto client = client_factory.make_channel(
    sal::crypto::peer_name("sal.alt.ee")
  );
  auto server = TestFixture::make_server_channel();

  EXPECT_THROW(
    handshake(client, server, false),
    std::system_error
  );
}


TYPED_TEST(crypto_channel, mutual_auth_success)
{
  auto client_factory = TestFixture::client_factory(
    sal::crypto::chain_check(chain_ok),
    chain(),
    private_key()
  );
  auto client = client_factory.make_channel();

  auto server_factory = TestFixture::server_factory(
    sal::crypto::chain_check(chain_ok),
    chain(),
    private_key()
  );
  auto server = server_factory.make_channel(sal::crypto::mutual_auth);

  size_t expected_chain_ok_count = chain_ok_count + 2;
  handshake(client, server, true);
  EXPECT_EQ(expected_chain_ok_count, chain_ok_count);
}


TYPED_TEST(crypto_channel, mutual_auth_client_fail)
{
  auto client_factory = TestFixture::client_factory(
    sal::crypto::chain_check(chain_fail),
    chain(),
    private_key()
  );
  auto client = client_factory.make_channel();

  auto server_factory = TestFixture::server_factory(
    sal::crypto::chain_check(chain_ok),
    chain(),
    private_key()
  );
  auto server = server_factory.make_channel(sal::crypto::mutual_auth);

  size_t original_chain_fail_count = chain_fail_count;
  try
  {
    handshake(client, server, false);
    FAIL() << "unexpected success";
  }
  catch (const std::system_error &)
  {
    SUCCEED();
  }
  EXPECT_LT(original_chain_fail_count, chain_fail_count);

  EXPECT_FALSE(client.is_connected());
}


TYPED_TEST(crypto_channel, mutual_auth_server_fail)
{
  auto client_factory = TestFixture::client_factory(
    sal::crypto::chain_check(chain_ok),
    chain(),
    private_key()
  );
  auto client = client_factory.make_channel();

  auto server_factory = TestFixture::server_factory(
    sal::crypto::chain_check(chain_fail),
    chain(),
    private_key()
  );
  auto server = server_factory.make_channel(sal::crypto::mutual_auth);

  size_t original_chain_fail_count = chain_fail_count;
  try
  {
    handshake(client, server, false);
    FAIL() << "unexpected success";
  }
  catch (const std::system_error &)
  {
    SUCCEED();
  }
  EXPECT_LT(original_chain_fail_count, chain_fail_count);

  EXPECT_FALSE(client.is_connected());
  EXPECT_FALSE(server.is_connected());
}


TYPED_TEST(crypto_channel, mutual_auth_no_client_cert)
{
  auto client_factory = TestFixture::client_factory(
    sal::crypto::chain_check(chain_ok)
  );
  auto client = client_factory.make_channel();

  auto server_factory = TestFixture::server_factory(
    sal::crypto::chain_check(chain_ok),
    chain(),
    private_key()
  );
  auto server = server_factory.make_channel(sal::crypto::mutual_auth);

  try
  {
    handshake(client, server, false);
    FAIL() << "unexpected success";
  }
  catch (const std::system_error &)
  {
    SUCCEED();
  }

  EXPECT_FALSE(client.is_connected());
  EXPECT_FALSE(server.is_connected());
}


} // namespace
