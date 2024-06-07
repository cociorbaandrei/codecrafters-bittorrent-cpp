#pragma once
#include "Boost.h"

#include <string>
#include <utility>

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual net::awaitable<std::pair<std::size_t, std::string>> Get(const std::string& url) = 0;
};