#include <sal/crypto/pipe.hpp>
#include <sal/crypto/common.test.hpp>
#include <sal/buf_ptr.hpp>


namespace {


using namespace sal::crypto;


struct crypto_pipe
  : public sal_test::fixture
{


  std::pair<pipe_t, pipe_t> handshake (pipe_t &&client, pipe_t &&server)
  {
    SCOPED_TRACE("handshake");
    EXPECT_FALSE(client.is_connected());
    EXPECT_FALSE(server.is_connected());

    uint8_t client_buf[2048], server_buf[2048];

    // generate client_hello
    auto [consumed, produced] = client.handshake(sal::const_null_buf, server_buf);
    EXPECT_EQ(0U, consumed);
    EXPECT_NE(0U, produced);

    // server <- client_hello
    // generate server_hello
    auto expected_consumed = produced;
    std::tie(consumed, produced) = server.handshake(
      sal::make_buf(server_buf, produced),
      client_buf
    );
    EXPECT_EQ(expected_consumed, consumed);
    EXPECT_NE(0U, produced);

    // client <- server_hello
    // generate key_exchange
    expected_consumed = produced;
    std::tie(consumed, produced) = client.handshake(
      sal::make_buf(client_buf, produced),
      server_buf
    );
    EXPECT_EQ(expected_consumed, consumed);
    EXPECT_NE(0U, produced);

    // server <- key_exchange
    // generate server_fnished
    expected_consumed = produced;
    std::tie(consumed, produced) = server.handshake(
      sal::make_buf(server_buf, produced),
      client_buf
    );
    EXPECT_EQ(expected_consumed, consumed);
    EXPECT_NE(0U, produced);
    EXPECT_TRUE(server.is_connected());

    // client <- server_finished
    expected_consumed = produced;
    std::tie(consumed, produced) = client.handshake(
      sal::make_buf(client_buf, produced),
      server_buf
    );
    EXPECT_EQ(expected_consumed, consumed);
    EXPECT_EQ(0U, produced);
    EXPECT_TRUE(client.is_connected());

    return {std::move(client), std::move(server)};
  }


  std::pair<pipe_t, pipe_t> make_pipe_pair (
    client_pipe_factory_t &&client_factory,
    server_pipe_factory_t &&server_factory,
    bool stream_oriented)
  {
    if (stream_oriented)
    {
      SCOPED_TRACE("make_stream_pipe");
      return handshake(
        client_factory.make_stream_pipe(),
        server_factory.make_stream_pipe()
      );
    }
    else
    {
      SCOPED_TRACE("make_datagram_pipe");
      return handshake(
        client_factory.make_datagram_pipe(),
        server_factory.make_datagram_pipe()
      );
    }
  }
};


inline auto certificate () noexcept
{
  auto pkcs12 = sal_test::to_der(sal_test::cert::pkcs12);
  return with_certificate(import_pkcs12(pkcs12, "TestPassword"));
}


TEST_F(crypto_pipe, stream_handshake)
{
  (void)make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );
}


TEST_F(crypto_pipe, datagram_handshake)
{
  (void)make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    false
  );
}


TEST_F(crypto_pipe, stream_handshake_after_connected)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );
  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());

  uint8_t in[2048], out[2048];
  std::error_code error;

  client.handshake(in, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  server.handshake(in, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


TEST_F(crypto_pipe, datagram_handshake_after_connected)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    false
  );
  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());

  uint8_t in[2048], out[2048];
  std::error_code error;

  client.handshake(in, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  server.handshake(in, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


void chunked_receive (const char phase[],
  pipe_t &receiver,
  const uint8_t *in_ptr, size_t in_size)
{
  SCOPED_TRACE(phase);

  while (in_size)
  {
    auto [consumed, produced] = receiver.handshake(
      sal::make_buf(in_ptr, 1),
      sal::null_buf
    );
    EXPECT_EQ(1U, consumed);
    EXPECT_EQ(0U, produced);
    in_ptr++, in_size--;
  }
}


TEST_F(crypto_pipe, stream_handshake_chunked_receive)
{
  uint8_t client_buf[2048], server_buf[2048];
  auto client = client_pipe_factory(no_certificate_check).make_stream_pipe();
  auto server = server_pipe_factory(certificate()).make_stream_pipe();

  auto [consumed, produced] = client.handshake(sal::const_null_buf, server_buf);
  EXPECT_EQ(0U, consumed);
  EXPECT_FALSE(client.is_connected());
  chunked_receive("client_hello", server, server_buf, produced);
  EXPECT_FALSE(server.is_connected());

  std::tie(consumed, produced) = server.handshake(sal::const_null_buf, client_buf);
  EXPECT_EQ(0U, consumed);
  EXPECT_FALSE(server.is_connected());
  chunked_receive("server_hello", client, client_buf, produced);
  EXPECT_FALSE(client.is_connected());

  std::tie(consumed, produced) = client.handshake(sal::const_null_buf, server_buf);
  EXPECT_EQ(0U, consumed);
  EXPECT_FALSE(client.is_connected());
  chunked_receive("key_exchange", server, server_buf, produced);
  EXPECT_FALSE(server.is_connected());

  std::tie(consumed, produced) = server.handshake(sal::const_null_buf, client_buf);
  EXPECT_EQ(0U, consumed);
  chunked_receive("server_finished", client, client_buf, produced);
  EXPECT_TRUE(client.is_connected());

#if !__sal_os_macos
  // SecureTransport bug? If during key_exchange feeding server side fails to
  // generate output (due errSecWouldBlock, for example), server does not
  // proceed to connected state.
  EXPECT_TRUE(server.is_connected());
#endif
}


void chunked_send (const char phase[], pipe_t &receiver, pipe_t &sender)
{
  SCOPED_TRACE(phase);

  // do not let receiver form response, next chunked send will do it
  uint8_t buffer[2048];
  for (auto it = buffer;  it != buffer + sizeof(buffer);  ++it)
  {
    auto [consumed, produced] = sender.handshake(
      sal::const_null_buf,
      sal::make_buf(it, 1)
    );
    EXPECT_EQ(0U, consumed);
    if (!produced)
    {
      receiver.handshake(sal::make_buf(buffer, it - buffer), sal::null_buf);
      return;
    }
    EXPECT_EQ(1U, produced);
  }
}


TEST_F(crypto_pipe, stream_handshake_chunked_send)
{
  auto client = client_pipe_factory(no_certificate_check).make_stream_pipe();
  auto server = server_pipe_factory(certificate()).make_stream_pipe();

  chunked_send("server <- client_hello", server, client);
  chunked_send("client <- server_hello", client, server);
  chunked_send("server <- key_exchange", server, client);
  chunked_send("client <- server_finished", client, server);
  EXPECT_TRUE(client.is_connected());

#if !__sal_os_macos
  // SecureTransport bug? If during key_exchange feeding server side fails to
  // generate output (due errSecWouldBlock, for example), server does not
  // proceed to connected state.
  EXPECT_TRUE(server.is_connected());
#endif
}


void trash (uint8_t *ptr, size_t size)
{
  while (size)
  {
    *ptr = 0xff;
    ptr++, size--;
  }
}


TEST_F(crypto_pipe, handshake_fail_on_invalid_client_hello)
{
  uint8_t client_buf[2048], server_buf[2048];
  auto client = client_pipe_factory(no_certificate_check).make_stream_pipe();
  auto server = server_pipe_factory(certificate()).make_stream_pipe();

  auto [consumed, produced] = client.handshake(sal::const_null_buf, server_buf);
  trash(server_buf, produced);
  (void)consumed;

  std::error_code error;
  server.handshake(sal::make_buf(server_buf, produced), client_buf, error);
  EXPECT_FALSE(!error);
}


TEST_F(crypto_pipe, handshake_fail_on_invalid_server_hello)
{
  uint8_t client_buf[2048], server_buf[2048];
  auto client = client_pipe_factory(no_certificate_check).make_stream_pipe();
  auto server = server_pipe_factory(certificate()).make_stream_pipe();

  // server <- client_hello
  auto [consumed, produced] = client.handshake(sal::const_null_buf, server_buf);
  std::tie(consumed, produced) = server.handshake(
    sal::make_buf(server_buf, produced),
    client_buf
  );
  trash(client_buf, produced);

  std::error_code error;
  client.handshake(sal::make_buf(client_buf, produced), server_buf, error);
  EXPECT_FALSE(!error);
}


TEST_F(crypto_pipe, handshake_fail_on_invalid_key_exchange)
{
  uint8_t client_buf[2048], server_buf[2048];
  auto client = client_pipe_factory(no_certificate_check).make_stream_pipe();
  auto server = server_pipe_factory(certificate()).make_stream_pipe();

  // server <- client_hello
  auto [consumed, produced] = client.handshake(sal::const_null_buf, server_buf);
  std::tie(consumed, produced) = server.handshake(
    sal::make_buf(server_buf, produced),
    client_buf
  );

  // client <- server_hello, generate key_exchange
  std::tie(consumed, produced) = client.handshake(
    sal::make_buf(client_buf, produced),
    server_buf
  );
  trash(server_buf, produced);

  std::error_code error;
  server.handshake(sal::make_buf(server_buf, produced), client_buf, error);
  EXPECT_FALSE(!error);
}


TEST_F(crypto_pipe, stream_client_encrypt_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  EXPECT_EQ(case_name.size(), consumed);
  EXPECT_LT(0U, produced);

  std::string message(secret, secret + produced);
  EXPECT_EQ(std::string::npos, message.find(case_name));

  uint8_t plain[2048];
  std::tie(consumed, produced) = server.decrypt(message, plain);
  EXPECT_EQ(case_name, std::string(plain, plain + produced));
  EXPECT_EQ(consumed, message.size());
}


TEST_F(crypto_pipe, datagram_client_encrypt_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    false
  );

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  EXPECT_EQ(case_name.size(), consumed);
  EXPECT_LT(0U, produced);

  std::string message(secret, secret + produced);
  EXPECT_EQ(std::string::npos, message.find(case_name));

  uint8_t plain[2048];
  std::tie(consumed, produced) = server.decrypt(message, plain);
  EXPECT_EQ(case_name, std::string(plain, plain + produced));
  EXPECT_EQ(consumed, message.size());
}


TEST_F(crypto_pipe, stream_server_encrypt_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );

  uint8_t secret[2048];
  auto [consumed, produced] = server.encrypt(case_name, secret);
  EXPECT_EQ(case_name.size(), consumed);
  EXPECT_LT(0U, produced);

  std::string message(secret, secret + produced);
  EXPECT_EQ(std::string::npos, message.find(case_name));

  uint8_t plain[2048];
  std::tie(consumed, produced) = client.decrypt(message, plain);
  EXPECT_EQ(case_name, std::string(plain, plain + produced));
  EXPECT_EQ(consumed, message.size());
}


TEST_F(crypto_pipe, datagram_server_encrypt_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    false
  );

  uint8_t secret[2048];
  auto [consumed, produced] = server.encrypt(case_name, secret);
  EXPECT_EQ(case_name.size(), consumed);
  EXPECT_LT(0U, produced);

  std::string message(secret, secret + produced);
  EXPECT_EQ(std::string::npos, message.find(case_name));

  uint8_t plain[2048];
  std::tie(consumed, produced) = client.decrypt(message, plain);
  EXPECT_EQ(case_name, std::string(plain, plain + produced));
  EXPECT_EQ(consumed, message.size());
}


TEST_F(crypto_pipe, stream_client_encrypt_not_connected)
{
  auto client = client_pipe_factory(no_certificate_check).make_stream_pipe();
  char buffer[2048];
  std::error_code error;
  auto [consumed, produced] = client.encrypt(case_name, buffer, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_EQ(0U, consumed);
  EXPECT_EQ(0U, produced);
}


TEST_F(crypto_pipe, stream_server_encrypt_not_connected)
{
  auto server = server_pipe_factory(certificate()).make_stream_pipe();
  char buffer[2048];
  std::error_code error;
  auto [consumed, produced] = server.encrypt(case_name, buffer, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_EQ(0U, consumed);
  EXPECT_EQ(0U, produced);
}


TEST_F(crypto_pipe, datagram_client_encrypt_not_connected)
{
  auto client = client_pipe_factory(no_certificate_check).make_datagram_pipe();
  char buffer[2048];
  std::error_code error;
  auto [consumed, produced] = client.encrypt(case_name, buffer, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_EQ(0U, consumed);
  EXPECT_EQ(0U, produced);
}


TEST_F(crypto_pipe, datagram_server_encrypt_not_connected)
{
  auto server = server_pipe_factory(certificate()).make_datagram_pipe();
  char buffer[2048];
  std::error_code error;
  auto [consumed, produced] = server.encrypt(case_name, buffer, error);
  EXPECT_EQ(std::errc::not_connected, error);
  EXPECT_EQ(0U, consumed);
  EXPECT_EQ(0U, produced);
}


TEST_F(crypto_pipe, stream_encrypt_decrypt_coalesced_messages)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );

  uint8_t secret[2048];
  std::string first("first");
  auto [consumed_1, produced_1] = client.encrypt(first, secret);
  EXPECT_EQ(first.size(), consumed_1);
  EXPECT_LT(0U, produced_1);

  std::string second("second");
  auto [consumed_2, produced_2] = client.encrypt(second,
    sal::make_buf(secret + produced_1, sizeof(secret) - produced_1)
  );
  EXPECT_EQ(second.size(), consumed_2);
  EXPECT_LT(0U, produced_2);

  uint8_t plain[2048];
  auto [consumed_3, produced_3] = server.decrypt(
    sal::make_buf(secret, produced_1 + produced_2),
    plain
  );
  EXPECT_EQ(produced_1, consumed_3);
  EXPECT_EQ(first.size(), produced_3);

  auto [consumed_4, produced_4] = server.decrypt(
    sal::make_buf(secret + consumed_3, produced_2),
    sal::make_buf(plain + produced_3, sizeof(plain) - produced_3)
  );
  EXPECT_EQ(produced_2, consumed_4);
  EXPECT_EQ(second.size(), produced_4);

  EXPECT_EQ(first + second, std::string(plain, plain + produced_3 + produced_4));
}


TEST_F(crypto_pipe, datagram_encrypt_decrypt_coalesced_messages)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    false
  );

  uint8_t secret[2048];
  std::string first("first");
  auto [consumed_1, produced_1] = client.encrypt(first, secret);
  EXPECT_EQ(first.size(), consumed_1);
  EXPECT_LT(0U, produced_1);

  std::string second("second");
  auto [consumed_2, produced_2] = client.encrypt(second,
    sal::make_buf(secret + produced_1, sizeof(secret) - produced_1)
  );
  EXPECT_EQ(second.size(), consumed_2);
  EXPECT_LT(0U, produced_2);

  uint8_t plain[2048];
  auto [consumed_3, produced_3] = server.decrypt(
    sal::make_buf(secret, produced_1 + produced_2),
    plain
  );
  EXPECT_EQ(produced_1, consumed_3);
  EXPECT_EQ(first.size(), produced_3);

  auto [consumed_4, produced_4] = server.decrypt(
    sal::make_buf(secret + consumed_3, produced_2),
    sal::make_buf(plain + produced_3, sizeof(plain) - produced_3)
  );
  EXPECT_EQ(produced_2, consumed_4);
  EXPECT_EQ(second.size(), produced_4);

  EXPECT_EQ(first + second, std::string(plain, plain + produced_3 + produced_4));
}


TEST_F(crypto_pipe, stream_encrypt_decrypt_chunked)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  EXPECT_EQ(case_name.size(), consumed);
  EXPECT_LT(0U, produced);

  uint8_t plain[2048];
  for (auto it = secret;  it != secret + produced;  ++it)
  {
    auto [chunk_consume, chunk_produce] = server.decrypt(
      sal::make_buf(it, 1),
      plain
    );
    EXPECT_EQ(1U, chunk_consume);
    if (chunk_produce)
    {
      EXPECT_EQ(produced, it - secret + 1);
      EXPECT_EQ(case_name.size(), chunk_produce);
      EXPECT_EQ(case_name, std::string(plain, plain + chunk_produce));
    }
  }
}


TEST_F(crypto_pipe, datagram_encrypt_decrypt_chunked)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  EXPECT_EQ(case_name.size(), consumed);
  EXPECT_LT(0U, produced);

  uint8_t plain[2048];
  for (auto it = secret;  it != secret + produced;  ++it)
  {
    auto [chunk_consume, chunk_produce] = server.decrypt(
      sal::make_buf(it, 1),
      plain
    );
    EXPECT_EQ(1U, chunk_consume);
    if (chunk_produce)
    {
      EXPECT_EQ(produced, it - secret + 1);
      EXPECT_EQ(case_name.size(), chunk_produce);
      EXPECT_EQ(case_name, std::string(plain, plain + chunk_produce));
    }
  }
}


} // namespace
