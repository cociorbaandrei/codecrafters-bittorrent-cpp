#pragma once
#include "IHttpClient.h"
#include <regex>
#include <tuple>
using UrlComponents = std::tuple<std::string, std::string, std::string>;
UrlComponents parseUrl(const std::string &url);

class HttpClient : public IHttpClient {
public:
  HttpClient() {}
  net::awaitable<std::pair<std::size_t, std::string>> Get(const std::string &url) override;
};
