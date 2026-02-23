#include <memory>
#include <thread>
#include <iostream>

#include "boost/beast.hpp"
#include "boost/asio.hpp"

#include "server.hpp"

// Based on https://www.boost.org/doc/libs/latest/libs/beast/example/http/server/sync/http_server_sync.cpp

namespace beast = boost::beast;
namespace asio = boost::asio;

void logResponse(std::string target,
                 beast::http::status status)
{
  if (SERVER_LOGGING_ENABLED)
    std::cout << status << ": " << target << "\n";
}

void fail(beast::error_code error, const char *what)
{
  if (SERVER_LOGGING_ENABLED)
    std::cerr << what << ": " << error.message() << "\n";
}

beast::string_view mimeType(beast::string_view path)
{
  const auto ext = [&path]
  {
    const auto pos = path.rfind(".");
    if (pos == beast::string_view::npos)
      return path;
    return path.substr(pos);
  }();

  // iequals is case-insensitive equals
  if (beast::iequals(ext, ".htm"))
    return "text/html";
  if (beast::iequals(ext, ".html"))
    return "text/html";
  if (beast::iequals(ext, ".php"))
    return "text/html";
  if (beast::iequals(ext, ".css"))
    return "text/css";
  if (beast::iequals(ext, ".txt"))
    return "text/plain";
  if (beast::iequals(ext, ".js"))
    return "application/javascript";
  if (beast::iequals(ext, ".json"))
    return "application/json";
  if (beast::iequals(ext, ".xml"))
    return "application/xml";
  if (beast::iequals(ext, ".swf"))
    return "application/x-shockwave-flash";
  if (beast::iequals(ext, ".flv"))
    return "video/x-flv";
  if (beast::iequals(ext, ".png"))
    return "image/png";
  if (beast::iequals(ext, ".jpe"))
    return "image/jpeg";
  if (beast::iequals(ext, ".jpeg"))
    return "image/jpeg";
  if (beast::iequals(ext, ".jpg"))
    return "image/jpeg";
  if (beast::iequals(ext, ".gif"))
    return "image/gif";
  if (beast::iequals(ext, ".bmp"))
    return "image/bmp";
  if (beast::iequals(ext, ".ico"))
    return "image/vnd.microsoft.icon";
  if (beast::iequals(ext, ".tiff"))
    return "image/tiff";
  if (beast::iequals(ext, ".tif"))
    return "image/tiff";
  if (beast::iequals(ext, ".svg"))
    return "image/svg+xml";
  if (beast::iequals(ext, ".svgz"))
    return "image/svg+xml";
  return "application/text";
}

std::string pathCat(beast::string_view base, beast::string_view path)
{
  if (base.empty())
    return std::string(path);

  std::string result(base);

#ifdef BOOST_MSVC
  constexpr char pathSeparator = '\\';
  if (result.back() == pathSeparator)
    result.resize(result.size() - 1);

  result.append(path.data(), path.size());

  // Replace all / with the separator
  for (auto &c : result)
  {
    if (c == '/')
      c = pathSeparator;
  }
#else
  constexpr char pathSeparator = '/';
  if (result.back() == pathSeparator)
    result.resize(result.size() - 1);

  result.append(path.data(), path.size());
#endif

  return result;
}

template <class Body, class Allocator>
beast::http::message_generator handleRequest(
    beast::string_view docRoot, beast::http::request<Body, beast::http::basic_fields<Allocator>> &&req)
{
  const auto badRequest = [&req](beast::string_view why)
  {
    beast::http::response<beast::http::string_body> res{
        beast::http::status::bad_request,
        req.version()};

    res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);

    logResponse(req.target(), beast::http::status::bad_request);

    res.prepare_payload();
    return res;
  };

  const auto notFound = [&req](beast::string_view target)
  {
    beast::http::response<beast::http::string_body> res{
        beast::http::status::not_found,
        req.version()};

    res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";

    logResponse(req.target(), beast::http::status::not_found);

    res.prepare_payload();
    return res;
  };

  const auto serverError = [&req](beast::string_view what)
  {
    beast::http::response<beast::http::string_body> res{
        beast::http::status::internal_server_error,
        req.version()};

    res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(beast::http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error ocurred: '" + std::string(what) + "'";

    logResponse(req.target(), beast::http::status::internal_server_error);

    res.prepare_payload();
    return res;
  };

  // Check that we can handle the method
  if (req.method() != beast::http::verb::get && req.method() != beast::http::verb::head)
    return badRequest("Unknown HTTP method");

  // Filter out '..'
  if (req.target().empty() || req.target()[0] != '/' || req.target().find("..") != beast::string_view::npos)
    return badRequest("Illegal request target");

  // Build the path
  std::string path = pathCat(docRoot, req.target());
  if (req.target().back() == '/')
    path.append("index.html");

  // Open the file
  beast::error_code error;
  beast::http::file_body::value_type body;
  body.open(path.c_str(), beast::file_mode::scan, error);

  // Handle file not found
  if (error == beast::errc::no_such_file_or_directory)
    return notFound(req.target());

  // Unknown error
  if (error)
    return serverError(error.message());

  const auto size = body.size();

  // Handle HEAD
  if (req.method() == beast::http::verb::head)
  {
    beast::http::response<beast::http::empty_body> res{
        beast::http::status::ok, req.version()};
    res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(beast::http::field::content_type, mimeType(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());

    logResponse(req.target(), beast::http::status::ok);

    return res;
  }

  // Handle GET
  beast::http::response<beast::http::file_body> res{
      std::piecewise_construct,
      std::make_tuple(std::move(body)),
      std::make_tuple(beast::http::status::ok, req.version())};
  res.set(beast::http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(beast::http::field::content_type, mimeType(path));
  res.content_length(size);
  res.keep_alive(req.keep_alive());

  logResponse(req.target(), beast::http::status::ok);

  return res;
}

void handleConnection(asio::ip::tcp::socket &socket, std::shared_ptr<std::string const> const &docRoot)
{
  beast::error_code error;

  // Persist buffer between reads
  beast::flat_buffer buffer;
  while (true)
  {
    // Read a request
    beast::http::request<beast::http::string_body> req;
    beast::http::read(socket, buffer, req, error);

    if (error == beast::http::error::end_of_stream)
      break;
    if (error)
      return fail(error, "read");

    // Handle request
    beast::http::message_generator msg = handleRequest(*docRoot, std::move(req));

    bool keepAlive = msg.keep_alive();

    beast::write(socket, std::move(msg), error);

    if (error)
      return fail(error, "write");

    if (!keepAlive)
    {
      // Close the connection
      break;
    }
  }

  // Shutdown TCP
  socket.shutdown(asio::ip::tcp::socket::shutdown_send, error);
}

// Prefix docRoot with a '.'
void startServer(const char *addressRaw, unsigned short port, const char *docRootRaw)
{
  const auto address = asio::ip::make_address(addressRaw);
  const auto docRoot = std::make_shared<std::string>(docRootRaw);

  if (SERVER_LOGGING_ENABLED)
    std::cout << "Server starting on port " << port << " with root '" << *docRoot << "'\n";

  asio::io_context ctx{1};

  asio::ip::tcp::acceptor acceptor{
      ctx, {address, port}};

  if (SERVER_LOGGING_ENABLED)
    std::cout << "Server started\n";

  while (true)
  {
    asio::ip::tcp::socket socket{ctx};

    // Block until connection
    acceptor.accept(socket);

    // Handle on other thread
    std::thread{
        std::bind(&handleConnection, std::move(socket), docRoot)}
        .detach();
  }
}