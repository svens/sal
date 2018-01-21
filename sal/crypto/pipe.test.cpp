#include <sal/crypto/pipe.hpp>
#include <sal/crypto/common.test.hpp>


namespace {


using namespace sal::crypto;


struct crypto_pipe
  : public sal_test::fixture
{
  void handshake (pipe_t &client, pipe_t &server)
  {
    // client
    uint8_t client_buf[8192], server_buf[8192];
    client.send_buffer(server_buf).process();
    server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();
    client.receive_buffer(server.send_buffer()).send_buffer(server_buf).process();
    client.send_buffer(server_buf).process();
    server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();
    client.receive_buffer(server.send_buffer()).send_buffer(server_buf).process();
    server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();
  }


  std::pair<pipe_t, pipe_t> make_pipe_pair (
    client_pipe_factory_t &&client_factory,
    server_pipe_factory_t &&server_factory,
    bool stream_oriented)
  {
    if (stream_oriented)
    {
      auto client = client_factory.make_stream_pipe();
      auto server = server_factory.make_stream_pipe();
      handshake(client, server);
      return {std::move(client), std::move(server)};
    }
    else
    {
      auto client = client_factory.make_datagram_pipe();
      auto server = server_factory.make_datagram_pipe();
      handshake(client, server);
      return {std::move(client), std::move(server)};
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
  // client
  uint8_t client_buf[8192];
  auto client_factory = client_pipe_factory(no_certificate_check);
  auto client = client_factory.make_stream_pipe();
  EXPECT_FALSE(client.is_connected());

  // server
  uint8_t server_buf[8192];
  auto server_factory = server_pipe_factory(certificate());
  auto server = server_factory.make_stream_pipe();
  EXPECT_FALSE(server.is_connected());

  // client -> server: client_hello
  client.send_buffer(server_buf).process();
  server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();

  // client <- server: server_hello
  client.receive_buffer(server.send_buffer()).send_buffer(server_buf).process();

  // client -> server: key_exchange
  client.send_buffer(server_buf).process();
  server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();

  // client_finished
  client.receive_buffer(server.send_buffer()).send_buffer(server_buf).process();

  // server_finished
  server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


TEST_F(crypto_pipe, datagram_handshake)
{
  // client
  uint8_t client_buf[8192];
  auto client_factory = client_pipe_factory(no_certificate_check);
  auto client = client_factory.make_datagram_pipe();
  EXPECT_FALSE(client.is_connected());

  // server
  uint8_t server_buf[8192];
  auto server_factory = server_pipe_factory(certificate());
  auto server = server_factory.make_datagram_pipe();
  EXPECT_FALSE(server.is_connected());

  // client -> server: client_hello
  client.send_buffer(server_buf).process();
  server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();

  // client <- server: server_hello
  client.receive_buffer(server.send_buffer()).send_buffer(server_buf).process();

  // client -> server: key_exchange
  client.send_buffer(server_buf).process();
  server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();

  // client_finished
  client.receive_buffer(server.send_buffer()).send_buffer(server_buf).process();

  // server_finished
  server.receive_buffer(client.send_buffer()).send_buffer(client_buf).process();

  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());
}


TEST_F(crypto_pipe, exchange_messages)
{
  auto [client, server] = make_pipe_pair(
    client_pipe_factory(no_certificate_check),
    server_pipe_factory(certificate()),
    true
  );
  EXPECT_TRUE(client.is_connected());
  EXPECT_TRUE(server.is_connected());

  uint8_t /*client_buf[8192],*/ server_buf[8192];
  client.receive_buffer("hello").send_buffer(server_buf).process();
  auto [begin, end] = client.send_buffer();
  std::cout << (end - begin) << '\n';
}


} // namespace
