#include <sal/crypto/pipe.hpp>
#include <sal/crypto/common.test.hpp>


namespace {


using crypto_pipe = sal_test::fixture;
using namespace sal::crypto;


inline auto certificate () noexcept
{
  auto pkcs12 = sal_test::to_der(sal_test::cert::pkcs12_no_passphrase);
  return with_certificate(import_pkcs12(pkcs12, ""));
}


TEST_F(crypto_pipe, stream_handshake)
{
  const uint8_t *send_first{};
  const uint8_t *send_last{};
  uint8_t client_buffer[8192], server_buffer[8192];

  auto sender = [&](const uint8_t *first, const uint8_t *last,
    std::error_code &) noexcept
  {
    send_first = first;
    send_last = last;
  };
  auto certificate_check = [](const certificate_t &certificate) noexcept -> bool
  {
    std::cout << "Got certificate: " << certificate << '\n';
    return true;
  };

  // generate client_hello
  std::cout << "*** client_hello ***\n";
  auto client_factory = pipe_factory(outbound, no_certificate_check);
  auto client_handshake = client_factory.make_stream_pipe(client_buffer, sender);
  ASSERT_NE(send_first, send_last);
  ASSERT_LE(client_buffer, send_first);
  ASSERT_GE(std::end(client_buffer), send_last);

  // feed client_hello to server, generate server_hello
  std::cout << "\n*** server_hello ***\n";
  auto server_factory = pipe_factory(inbound, certificate());
  auto server_handshake = server_factory.make_stream_pipe(server_buffer, sender);
  server_handshake.process(send_first, send_last);
  ASSERT_NE(send_first, send_last);
  ASSERT_LE(server_buffer, send_first);
  ASSERT_GE(std::end(server_buffer), send_last);

  // feed server_hello to client, generate key_exchange
  std::cout << "\n*** key_exchange ***\n";
  client_handshake.process(send_first, send_last);
  ASSERT_NE(send_first, send_last);
  ASSERT_LE(client_buffer, send_first);
  ASSERT_GE(std::end(client_buffer), send_last);

  // feed key_exchange to server, generate server_finished
  std::cout << "\n*** server_finished ***\n";
  server_handshake.process(send_first, send_last);
  ASSERT_NE(send_first, send_last);
  ASSERT_LE(server_buffer, send_first);
  ASSERT_GE(std::end(server_buffer), send_last);

  // feed server_finished to client, generate client finished
  std::cout << "\n*** client_finished ***\n";
  client_handshake.process(send_first, send_last);

  auto client = client_handshake.pipe();
  client.encrypt();

  auto server = server_handshake.pipe();
  server.encrypt();
}


} // namespace
