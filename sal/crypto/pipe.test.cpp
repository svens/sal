#include <sal/crypto/pipe.hpp>
#include <sal/crypto/common.test.hpp>
#include <sal/buf_ptr.hpp>


namespace {


using namespace sal::crypto;


struct pipe
  : public sal_test::with_value<bool>
{
  std::pair<pipe_t, pipe_t> make_pipe_pair (
    client_pipe_factory_t &&client_factory,
    server_pipe_factory_t &&server_factory,
    bool stream_oriented)
  {
    if (stream_oriented)
    {
      SCOPED_TRACE("make_stream_pipe");
      return
      {
        client_factory.make_stream_pipe(),
        server_factory.make_stream_pipe()
      };
    }
    else
    {
      SCOPED_TRACE("make_datagram_pipe");
      return
      {
        client_factory.make_datagram_pipe(),
        server_factory.make_datagram_pipe()
      };
    }
  }


  void handshake (pipe_t &client, pipe_t &server)
  {
    SCOPED_TRACE("handshake");
    ASSERT_FALSE(client.is_connected());
    ASSERT_FALSE(server.is_connected());

    uint8_t client_buf[2048], server_buf[2048];
    auto [consumed, produced] = client.handshake(
      sal::const_null_buf,
      server_buf
    );

    while (produced > 0)
    {
      std::tie(consumed, produced) = server.handshake(
        sal::make_buf(server_buf, produced),
        client_buf
      );
      std::tie(consumed, produced) = client.handshake(
        sal::make_buf(client_buf, produced),
        server_buf
      );
    }

    ASSERT_TRUE(client.is_connected());
    ASSERT_TRUE(server.is_connected());
  }
};


inline auto certificate ()
{
  static auto cert = import_pkcs12(
    sal_test::to_der(sal_test::cert::pkcs12),
    "TestPassword"
  );
  return with_certificate(cert);
}


INSTANTIATE_TEST_CASE_P(crypto, pipe, ::testing::Bool());


TEST_P(pipe, handshake)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);
}


TEST_P(pipe, handshake_after_connected)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t in[2048], out[2048];
  std::error_code error;

  client.handshake(in, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  server.handshake(in, out, error);
  EXPECT_EQ(std::errc::already_connected, error);

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


template <size_t Size>
size_t chunked_receive (pipe_t &receiver,
  const uint8_t *in_ptr, size_t in_size,
  uint8_t (&out)[Size],
  bool is_stream)
{
  auto out_ptr = out;
  auto out_size = Size;

  std::error_code error;
  while (in_size && !::testing::Test::HasFailure())
  {
    size_t chunk_size = 1U;

#if __sal_os_windows
    if (!is_stream)
    {
      // special case for SChannel DTLS handshake that fails
      // on 1B fragments client_hello during first 13B
      chunk_size = 13;
      if (chunk_size > in_size)
      {
        chunk_size = in_size;
      }
    }
#else
    (void)is_stream;
#endif

    auto [consumed, produced] = receiver.handshake(
      sal::make_buf(in_ptr, chunk_size),
      sal::make_buf(out_ptr, out_size),
      error
    );
    EXPECT_TRUE(!error) << error.message();

    in_ptr += consumed;
    in_size -= consumed;

    out_ptr += produced;
    out_size -= produced;
  }

  return out_ptr - out;
}


TEST_P(pipe, handshake_chunked_receive)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  ASSERT_FALSE(client.is_connected());
  ASSERT_FALSE(server.is_connected());

  uint8_t client_buf[2048], server_buf[2048];
  auto [consumed, produced] = client.handshake(
    sal::const_null_buf,
    server_buf
  );
  EXPECT_EQ(0U, consumed);
  ASSERT_NE(0U, produced);

  while (produced > 0)
  {
    produced = chunked_receive(server, server_buf, produced, client_buf, GetParam());
    produced = chunked_receive(client, client_buf, produced, server_buf, GetParam());
  }

  ASSERT_TRUE(client.is_connected());
  ASSERT_TRUE(server.is_connected());
}


#if __sal_os_windows

//
// While it is possible to support chunked send with SChannel, it creates
// unnecessary overhead. For that reason, we push the responsibility of
// providing sufficiently sized buffer to application layer
//

TEST_P(pipe, handshake_no_output_buffer)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  char buffer[2048];
  std::error_code error;

  // client side
  client.handshake(sal::const_null_buf, sal::null_buf, error);
  EXPECT_EQ(std::errc::no_buffer_space, error);

  // server side (but first we need proper client_hello)
  auto [consumed, produced] = client.handshake(sal::const_null_buf, buffer);
  std::tie(consumed, produced) = server.handshake(
    sal::make_buf(buffer, produced),
    sal::null_buf,
    error
  );
  EXPECT_EQ(std::errc::no_buffer_space, error);
}


TEST_P(pipe, handshake_output_buffer_too_small)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  char buffer[2048];
  std::error_code error;

  // client side

  auto [consumed, produced] = client.handshake(
    sal::const_null_buf,
    sal::make_buf(buffer, 1),
    error
  );
  EXPECT_EQ(std::errc::no_buffer_space, error);

  // server side (first we need proper client_hello)
  std::tie(consumed, produced) = client.handshake(sal::const_null_buf, buffer);
  std::tie(consumed, produced) = server.handshake(
    sal::make_buf(buffer, produced),
    sal::make_buf(buffer, 1),
    error
  );
  EXPECT_EQ(std::errc::no_buffer_space, error);
}

#else

//
// MacOS & Linux will buffer overflowing output themselves
//

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


TEST_P(pipe, handshake_chunked_send)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

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

#endif // !__sal_os_windows


inline void trash (uint8_t *ptr, size_t size) noexcept
{
  while (size)
  {
    *ptr++ = 0xff;
    size--;
  }
}


#if __sal_os_windows


//
// SChannel is ok with trashed message, asking for more instead of error
//


#else


TEST_P(pipe, handshake_fail_on_invalid_client_hello)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  uint8_t client_buf[2048], server_buf[2048];
  auto [consumed, produced] = client.handshake(sal::const_null_buf, server_buf);
  trash(server_buf, produced);
  (void)consumed;

  std::error_code error;
  server.handshake(sal::make_buf(server_buf, produced), client_buf, error);
  EXPECT_FALSE(!error);
}


TEST_P(pipe, handshake_fail_on_invalid_server_hello)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  // server <- client_hello
  uint8_t client_buf[2048], server_buf[2048];
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


TEST_P(pipe, handshake_fail_on_invalid_key_exchange)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  // server <- client_hello
  uint8_t client_buf[2048], server_buf[2048];
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


#endif // !__sal_os_windows


TEST_P(pipe, client_encrypt_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  ASSERT_EQ(case_name.size(), consumed);
  ASSERT_NE(0U, produced);

  std::string message(secret, secret + produced);
  EXPECT_EQ(std::string::npos, message.find(case_name));

  uint8_t plain[2048];
  std::tie(consumed, produced) = server.decrypt(message, plain);
  EXPECT_EQ(case_name, std::string(plain, plain + produced));
  EXPECT_EQ(consumed, message.size());
}


TEST_P(pipe, server_encrypt_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t secret[2048];
  auto [consumed, produced] = server.encrypt(case_name, secret);
  ASSERT_EQ(case_name.size(), consumed);
  ASSERT_NE(0U, produced);

  std::string message(secret, secret + produced);
  EXPECT_EQ(std::string::npos, message.find(case_name));

  uint8_t plain[2048];
  std::tie(consumed, produced) = client.decrypt(message, plain);
  EXPECT_EQ(case_name, std::string(plain, plain + produced));
  EXPECT_EQ(consumed, message.size());
}


TEST_P(pipe, encrypt_not_connected)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  char buffer[2048];
  std::error_code error;

  {
    auto [consumed, produced] = client.encrypt(case_name, buffer, error);
    EXPECT_EQ(std::errc::not_connected, error);
    EXPECT_EQ(0U, consumed);
    EXPECT_EQ(0U, produced);
  }

  {
    auto [consumed, produced] = server.encrypt(case_name, buffer, error);
    EXPECT_EQ(std::errc::not_connected, error);
    EXPECT_EQ(0U, consumed);
    EXPECT_EQ(0U, produced);
  }
}


TEST_P(pipe, decrypt_not_connected)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );

  char buffer[2048];
  std::error_code error;

  {
    auto [consumed, produced] = client.decrypt(case_name, buffer, error);
    EXPECT_EQ(std::errc::not_connected, error);
    EXPECT_EQ(0U, consumed);
    EXPECT_EQ(0U, produced);
  }

  {
    auto [consumed, produced] = server.decrypt(case_name, buffer, error);
    EXPECT_EQ(std::errc::not_connected, error);
    EXPECT_EQ(0U, consumed);
    EXPECT_EQ(0U, produced);
  }
}


TEST_P(pipe, decrypt_coalesced)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t secret[2048];
  std::string first("first");
  auto [consumed_1, produced_1] = client.encrypt(first, secret);
  ASSERT_EQ(first.size(), consumed_1);
  ASSERT_NE(0U, produced_1);

  std::string second("second");
  auto [consumed_2, produced_2] = client.encrypt(second,
    sal::make_buf(secret + produced_1, sizeof(secret) - produced_1)
  );
  ASSERT_EQ(second.size(), consumed_2);
  ASSERT_NE(0U, produced_2);

  uint8_t plain[2048];
  auto [consumed_3, produced_3] = server.decrypt(
    sal::make_buf(secret, produced_1 + produced_2),
    plain
  );
  ASSERT_EQ(produced_1, consumed_3);
  ASSERT_EQ(first.size(), produced_3);

  auto [consumed_4, produced_4] = server.decrypt(
    sal::make_buf(secret + consumed_3, produced_2),
    sal::make_buf(plain + produced_3, sizeof(plain) - produced_3)
  );
  ASSERT_EQ(produced_2, consumed_4);
  ASSERT_EQ(second.size(), produced_4);

  EXPECT_EQ(first + second, std::string(plain, plain + produced_3 + produced_4));
}


TEST_P(pipe, decrypt_chunked)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  ASSERT_EQ(case_name.size(), consumed);
  ASSERT_NE(0U, produced);

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
      ASSERT_EQ(produced, it - secret + 1);
      ASSERT_EQ(case_name.size(), chunk_produce);
      EXPECT_EQ(case_name, std::string(plain, plain + chunk_produce));
      return;
    }
  }

  FAIL() << "no message decrypted";
}


TEST_P(pipe, client_decrypt_trashed_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t secret[2048];
  auto [consumed, produced] = server.encrypt(case_name, secret);
  ASSERT_EQ(case_name.size(), consumed);
  ASSERT_NE(0U, produced);

  trash(secret, produced);
  std::string message(secret, secret + produced);

  uint8_t plain[2048];
  std::error_code error;
  std::tie(consumed, produced) = client.decrypt(message, plain, error);
  EXPECT_FALSE(!error);
}


TEST_P(pipe, server_decrypt_trashed_message)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    GetParam()
  );
  handshake(client, server);

  uint8_t secret[2048];
  auto [consumed, produced] = client.encrypt(case_name, secret);
  ASSERT_EQ(case_name.size(), consumed);
  ASSERT_NE(0U, produced);

  trash(secret, produced);
  std::string message(secret, secret + produced);

  uint8_t plain[2048];
  std::error_code error;
  std::tie(consumed, produced) = server.decrypt(message, plain, error);
  EXPECT_FALSE(!error);
}


TEST_P(pipe, DISABLED_coalesced_server_finished_and_message)
{
  FAIL() << "TODO";
}


} // namespace
