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
  static constexpr auto protocol () noexcept
  {
    return sal::crypto::datagram_oriented;
  }
};


struct stream
{
  static constexpr auto protocol () noexcept
  {
    return sal::crypto::stream_oriented;
  }
};


template <typename ChannelFactory>
struct crypto_channel
  : public sal_test::with_type<ChannelFactory>
{
  auto make_channel_pair ()
  {
    SCOPED_TRACE("make_channel_pair");
    auto client_context = sal::crypto::channel_context(
      ChannelFactory::protocol(),
      sal::crypto::no_certificate_check
    );
    auto server_context = sal::crypto::channel_context(
      ChannelFactory::protocol(),
      certificate(),
      private_key()
    );
    return std::make_pair(
      client_context.connect(),
      server_context.accept()
    );
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

  client.handshake(server_buf.data, client_buf);
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


using channel_context_types = ::testing::Types<
  datagram,
  stream
>;
TYPED_TEST_CASE(crypto_channel, channel_context_types);


TYPED_TEST(crypto_channel, handshake)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);
}


TYPED_TEST(crypto_channel, handshake_after_connected)
{
  auto [client, server] = this->make_channel_pair();
  handshake(client, server);

  buffer_t<4096> in, out;
  std::error_code error;

  error.clear();
  client.handshake(in.data, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  error.clear();
  server.handshake(in.data, out, error);
  EXPECT_EQ(std::errc::already_connected, error);
}


template <size_t Size>
void chunked_feed (sal::crypto::channel_t &channel,
  buffer_t<Size> &in,
  buffer_t<Size> &out)
{
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

  client.handshake(server_buf.data, client_buf);
  while (!client_buf.data.empty())
  {
    chunked_feed(server, client_buf, server_buf);
    client_buf.data.clear();

    chunked_feed(client, server_buf, client_buf);
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


void chunked_send (const char phase[],
  sal::crypto::channel_t &receiver,
  sal::crypto::channel_t &sender)
{
  SCOPED_TRACE(phase);

  buffer_t<1> buffer;
  std::vector<uint8_t> response;
  for (;;)
  {
    sender.handshake(sal::const_null_buf, buffer);
    if (!buffer.data.empty())
    {
      // while sender generates chunks, keep gathering it
      response.insert(response.end(), buffer.data.begin(), buffer.data.end());
      buffer.data.clear();
    }
    else
    {
      // no more, now feed as whole to receiver
      std::error_code error;
      failing_buffer_t failing_buffer(true, true);
      receiver.handshake(response, failing_buffer, error);
      if (!receiver.is_connected())
      {
        // while not connected, must want to send data
        EXPECT_EQ(std::errc::no_buffer_space, error) << error.message();
      }
      return;
    }
  }
}


TYPED_TEST(crypto_channel, handshake_chunked_send)
{
  auto [client, server] = this->make_channel_pair();
  chunked_send("server <- client_hello", server, client);
  chunked_send("client <- server_hello", client, server);
  chunked_send("server <- key_exchange", server, client);
  chunked_send("client <- server_finished", client, server);

  EXPECT_TRUE(client.is_connected());

#if !__sal_os_macos
  // SecureTransport bug? If during key_exchange feeding server side fails to
  // generate output (due errSecWouldBlock, for example), server does not
  // proceed to connected state
  EXPECT_TRUE(server.is_connected());
#endif
}


TYPED_TEST(crypto_channel, handshake_fail_on_invalid_client_hello)
{
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

  // generate client_hello
  client.handshake(server_buf.data, client_buf);

  // server <- client_hello, generate server_hello
  server.handshake(client_buf.data, server_buf);
  client_buf.data.clear();

  // client <- server_hello, generate key_exchange
  client.handshake(server_buf.data, client_buf);
  server_buf.data.clear();

  // server <- key_exchange, generate server_finished
  server.handshake(client_buf.data, server_buf);
  EXPECT_TRUE(server.is_connected());
  client_buf.data.clear();

  // append message to server_finished
  server.encrypt(this->case_name, server_buf);

  // client <- server_finished, leaving message in server_buf
  auto used = client.handshake(server_buf.data, client_buf);
  EXPECT_TRUE(client.is_connected());
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


} // namespace
