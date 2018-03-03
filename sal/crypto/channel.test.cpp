#include <sal/crypto/channel.hpp>
#include <sal/crypto/common.test.hpp>
#include <sal/buf_ptr.hpp>


namespace {


std::pair<sal::crypto::certificate_t *, sal::crypto::private_key_t *> import_pkcs12 ()
{
  static sal::crypto::private_key_t private_key;
  static sal::crypto::certificate_t certificate = sal::crypto::import_pkcs12(
    sal_test::to_der(sal_test::cert::pkcs12),
    "TestPassword",
    private_key
  );
  return {&certificate, &private_key};
}


inline auto certificate ()
{
  return sal::crypto::with_certificate(*import_pkcs12().first);
}


inline auto private_key ()
{
  return sal::crypto::with_private_key(import_pkcs12().second);
}


struct datagram
{
  static constexpr const bool is_datagram = true;

  static auto client ()
  {
    auto factory = sal::crypto::datagram_client_channel_factory(
      sal::crypto::no_certificate_check
    );
    return factory.make_channel();
  }

  static auto server ()
  {
    auto factory = sal::crypto::datagram_server_channel_factory(
      certificate(),
      private_key()
    );
    return factory.make_channel();
  }
};


struct stream
{
  static constexpr const bool is_datagram = false;

  static auto client ()
  {
    auto factory = sal::crypto::stream_client_channel_factory(
      sal::crypto::no_certificate_check
    );
    return factory.make_channel();
  }

  static auto server ()
  {
    auto factory = sal::crypto::stream_server_channel_factory(
      certificate(),
      private_key()
    );
    return factory.make_channel();
  }
};


template <typename ChannelFactory>
struct crypto_channel
  : public sal_test::with_type<ChannelFactory>
{
  static constexpr const bool is_datagram = ChannelFactory::is_datagram;

  auto make_channel_pair ()
  {
    SCOPED_TRACE("make_channel_pair");
    return std::make_pair(ChannelFactory::client(), ChannelFactory::server());
  }
};


template <size_t Size>
struct buffer_t final
  : public sal::crypto::channel_t::buffer_manager_t
{
  uint8_t chunk[Size];
  std::vector<uint8_t> data{};

  uintptr_t alloc (uint8_t **buffer, size_t *buffer_size)
    noexcept final override
  {
    *buffer = chunk;
    *buffer_size = sizeof(chunk);
    return {};
  }

  void ready (uintptr_t, uint8_t *ptr, size_t size)
    noexcept final override
  {
    data.insert(data.end(), ptr, ptr + size);
  }

  void trash ()
  {
    if (!data.empty())
    {
      data.assign(data.size(), 0xff);
    }
  }
};


void handshake (sal::crypto::channel_t &client, sal::crypto::channel_t &server)
{
  SCOPED_TRACE("handshake");
  ASSERT_FALSE(client.is_connected());
  ASSERT_FALSE(server.is_connected());

  buffer_t<4096> client_buf, server_buf;

  client.handshake(sal::const_null_buf, client_buf);
  while (!client_buf.data.empty())
  {
    server.handshake(client_buf.data, server_buf);
    client_buf.data.clear();

    client.handshake(server_buf.data, client_buf);
    server_buf.data.clear();
  }

  ASSERT_TRUE(client.is_connected());
  ASSERT_TRUE(server.is_connected());
}


using channel_factory_types = ::testing::Types<
  datagram,
  stream
>;
TYPED_TEST_CASE(crypto_channel, channel_factory_types);


TYPED_TEST(crypto_channel, handshake)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);
}


TYPED_TEST(crypto_channel, handshake_after_connected)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> out;
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

  //
  // TODO: debug deeper what's going on here
  //

  if (datagram)
  {
    #if OPENSSL_VERSION_NUMBER < 0x10100000
      // OpenSSL <1.1 DTLS fails when fragment is less than MTU
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
    #else
      // OpenSSL =1.1 DTLS fail on any fragmentation
      channel.handshake(in.data, out);
    #endif
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

  buffer_t<4096> client_buf, server_buf;

  client.handshake(sal::const_null_buf, client_buf);
  while (!client_buf.data.empty())
  {
    chunked_feed(server, client_buf, server_buf, this->is_datagram);
    client_buf.data.clear();

    chunked_feed(client, server_buf, client_buf, this->is_datagram);
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


TYPED_TEST(crypto_channel, handshake_alloc_null_ptr)
{
  auto [client, server] = this->make_channel_pair();
  failing_buffer_t failing_buffer(true, false);
  std::error_code error;

  // client side
  client.handshake(sal::const_null_buf, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);

  // create client_hello before testing server side
  buffer_t<4096> client_hello;
  client.handshake(sal::const_null_buf, client_hello);

  // server side
  error.clear();
  server.handshake(client_hello.data, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TYPED_TEST(crypto_channel, handshake_alloc_null_size)
{
  auto [client, server] = this->make_channel_pair();
  failing_buffer_t failing_buffer(false, true);
  std::error_code error;

  // client side
  client.handshake(sal::const_null_buf, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);

  // create client_hello before testing server side
  buffer_t<4096> client_hello;
  client.handshake(sal::const_null_buf, client_hello);

  // server side
  error.clear();
  server.handshake(client_hello.data, failing_buffer, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TYPED_TEST(crypto_channel, handshake_alloc_not_sufficient)
{
  auto [client, server] = this->make_channel_pair();
  buffer_t<1> client_buf, server_buf;

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
constexpr bool suppress_trashed_handshake_tests (
  const crypto_channel<ChannelFactory> &factory)
{
#if __sal_os_macos
  // SecureTransport: due chosen trashing content, throws buffer overflow
  (void)factory;
  return false;
#elif __sal_os_linux
  if constexpr (factory.is_datagram)
  {
    // OpenSSL/DTLS: ignores trashed messages (correctly)
    return true;
  }
  // OpenSSL/TLS: invalid version number
  return false;
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

  buffer_t<4096> client_buf, server_buf;
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

  buffer_t<4096> client_buf, server_buf;
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
  buffer_t<4096> client_buf, server_buf;
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


#if !__sal_os_linux


TYPED_TEST(crypto_channel, client_encrypt_message)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret;
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  std::string message(secret.data.begin(), secret.data.end());
  EXPECT_EQ(std::string::npos, message.find(this->case_name));

  buffer_t<4096> plain;
  server.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, server_encrypt_message)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret;
  server.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  std::string message(secret.data.begin(), secret.data.end());
  EXPECT_EQ(std::string::npos, message.find(this->case_name));

  buffer_t<4096> plain;
  client.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, encrypt_not_connected)
{
  auto [client, server] = this->make_channel_pair();
  buffer_t<4096> secret;

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
  buffer_t<4096> plain;

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

  buffer_t<4096> secret;
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

  buffer_t<4096> secret;
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

  buffer_t<1> secret;
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  buffer_t<4096> plain;
  server.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, decrypt_alloc_insufficient)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret;
  client.encrypt(this->case_name, secret);
  EXPECT_FALSE(secret.data.empty());

  buffer_t<1> plain;
  server.decrypt(secret.data, plain);
  EXPECT_FALSE(plain.data.empty());

  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, decrypt_coalesced)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret;

  auto first = this->case_name + "_first";
  client.encrypt(first, secret);
  EXPECT_FALSE(secret.data.empty());

  auto second = this->case_name + "_second";
  std::reverse(second.begin(), second.end());
  client.encrypt(second, secret);
  EXPECT_FALSE(secret.data.empty());

  // first
  buffer_t<4096> plain;
  auto used = server.decrypt(secret.data, plain);
  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(first, message);
  secret.data.erase(secret.data.begin(), secret.data.begin() + used);

  // second
  plain.data.clear();
  used = server.decrypt(secret.data, plain);
  message.assign(plain.data.begin(), plain.data.end());
  EXPECT_EQ(second, message);
  secret.data.erase(secret.data.begin(), secret.data.begin() + used);

  EXPECT_TRUE(secret.data.empty());
}


TYPED_TEST(crypto_channel, decrypt_chunked)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret, plain;
  client.encrypt(this->case_name, secret);

  for (auto b: secret.data)
  {
    auto used = server.decrypt(sal::make_buf(&b, 1), plain);
    EXPECT_EQ(1U, used);
  }

  std::string message(plain.data.begin(), plain.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, decrypt_invalid_message)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret, plain;
  client.encrypt(this->case_name, secret);
  secret.trash();

  std::error_code error;
  server.decrypt(secret.data, plain, error);
  EXPECT_FALSE(!error);
}


TYPED_TEST(crypto_channel, coalesced_server_finished_and_message)
{
  auto [client, server] = this->make_channel_pair();

  buffer_t<4096> client_buf, server_buf;

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

  // client <- server_finished, leaving message in server_buf
  auto used = client.handshake(server_buf.data, client_buf);
  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(client_buf.data.empty());
  server_buf.data.erase(server_buf.data.begin(), server_buf.data.begin() + used);
  EXPECT_TRUE(!server_buf.data.empty());

  // check for decrypted message
  client_buf.data.clear();
  used = client.decrypt(server_buf.data, client_buf);
  server_buf.data.erase(server_buf.data.begin(), server_buf.data.begin() + used);
  EXPECT_TRUE(server_buf.data.empty());
  std::string message(client_buf.data.begin(), client_buf.data.end());
  EXPECT_EQ(this->case_name, message);
}


TYPED_TEST(crypto_channel, decrypt_half_and_one_plus_half_messages)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> secret;

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
  buffer_t<4096> plain;
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


#endif


} // namespace
