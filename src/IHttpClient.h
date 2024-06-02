#pragma once
#include <string>
#include <utility>
#include "Boost.h"

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual net::awaitable<std::pair<std::size_t, std::string>> Get(const std::string& url) = 0;
};