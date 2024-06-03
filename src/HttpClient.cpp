#include "HttpClient.h"
#include "spdlog/spdlog.h"

UrlComponents parseUrl(const std::string &url) {
  std::regex urlRegex(R"(^(https?)://([^/]+)(/.*)?$)");
  std::smatch urlMatchResult;

  UrlComponents components;

  if (std::regex_match(url, urlMatchResult, urlRegex)) {
    return {urlMatchResult[1].str(), urlMatchResult[2].str(),
            urlMatchResult[3].str() == "" ? "/" : urlMatchResult[3].str()};
  } else {
    throw std::invalid_argument("Invalid URL format");
  }

  return components;
}

net::awaitable<std::pair<std::size_t, std::string>> HttpClient::Get(const std::string &url) {
    try {
      auto executor = co_await net::this_coro::executor;
      using executor_with_default =
          net::use_awaitable_t<>::executor_with_default<net::any_io_executor>;
      using tcp_stream = typename beast::tcp_stream::rebind_executor<
          executor_with_default>::other;
      const auto &&[protocol, host, query] = parseUrl(url);
      spdlog::info("protocol=\"{0}\" host=\"{1}\", querry=\"{2}\"", protocol, host, query);
      beast::flat_buffer buffer;
      http::response<http::dynamic_body> res;
      http::request<http::string_body> req{http::verb::get, query, 11};
      req.set(http::field::host, host);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      auto resolver = net::use_awaitable.as_default_on(tcp::resolver(executor));
      // Look up the domain name
      auto const results =
          co_await resolver.async_resolve(host, protocol, net::use_awaitable);
      if (protocol == "https") {
        ssl::context ctx(ssl::context::sslv23_client);
        ctx.set_default_verify_paths();

        beast::ssl_stream<beast::tcp_stream> stream{
            net::use_awaitable.as_default_on(beast::tcp_stream(executor)), ctx};

        co_await beast::get_lowest_layer(stream).async_connect(
            results, net::use_awaitable);

        co_await stream.async_handshake(ssl::stream_base::client,
                                        net::use_awaitable);

        co_await http::async_write(stream, req, net::use_awaitable);
        co_await http::async_read(stream, buffer, res, net::use_awaitable);
     //   co_await stream.async_shutdown(net::use_awaitable);
      } else {
        beast::tcp_stream stream(
            net::use_awaitable.as_default_on(beast::tcp_stream(executor)));
        co_await beast::get_lowest_layer(stream).async_connect(
            results, net::use_awaitable);
        co_await http::async_write(stream, req, net::use_awaitable);
        co_await http::async_read(stream, buffer, res, net::use_awaitable);
      }
     
      co_return std::make_pair(res.result_int(), beast::buffers_to_string(res.body().data()));
    } catch (std::exception &e) {
      co_return std::make_pair(0u, "Error: " + std::string(e.what()));
    }
  }