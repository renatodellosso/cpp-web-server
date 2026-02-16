#include <memory>
#include <thread>

#include "boost/beast.hpp"
#include "boost/asio.hpp"

#include "server.hpp"

namespace beast = boost::beast;
namespace asio = boost::asio;

void handleConnection(asio::ip::tcp::socket& socket, std::shared_ptr<std::string const> const& docRoot) {

}

void startServer(const char* addressRaw, unsigned short port, const char* docRootRaw) {
  const auto address = asio::ip::make_address(addressRaw);
  const auto docRoot = std::make_shared<std::string>(docRootRaw);

  asio::io_context ctx {
    1
  };

  asio::ip::tcp::acceptor acceptor {
    ctx, { address, port }
  };

  while (true) {
    asio::ip::tcp::socket socket { ctx };

    // Block until connection
    acceptor.accept(socket);

    // Handle on other thread
    std::thread {
      std::bind(&handleConnection, std::move(socket), docRoot)
    }.detach();
  }
}