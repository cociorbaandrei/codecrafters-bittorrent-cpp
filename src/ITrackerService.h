#pragma once

#include <string>
#include <utility>
#include "Boost.h"
namespace torrent{
    class MetaData;
}
class ITrackerService {
public:
    virtual ~ITrackerService() = default;
    virtual net::awaitable<std::vector<std::tuple<std::string, std::uint32_t, std::string>>> discoverPeers(const torrent::MetaData& torrent, bool compact = true) = 0;
};