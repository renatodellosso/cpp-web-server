#include <iostream>
#include <boost/beast.hpp>
#include "client.hpp"

namespace beast = boost::beast;
namespace asio = boost::asio;

bool request(char* host, char* port, char* target) {
  asio::io_context ctx;
  asio::ip::tcp::resolver resolver(ctx);
  beast::tcp_stream stream(ctx);

  // Resolve host -> IP
  const auto results = resolver.resolve(host, port);

  stream.connect(results);

  beast::http::request<beast::http::string_body> req {
    beast::http::verb::get,
    target,
    10 // HTTP Version 1.0
  };
  req.set(beast::http::field::host, host);
  req.set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // Send the request
  beast::http::write(stream, req);

  // Read response
  beast::flat_buffer buffer;
  beast::http::response<beast::http::dynamic_body> res;
  beast::http::read(stream, buffer, res);

  std::cout << res << std::endl;

  // Close socket
  beast::error_code error;
  stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, error);

  if (error && error != beast::errc::not_connected)
    throw beast::system_error {
      error
    };

  return true;
}