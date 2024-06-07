#pragma once

#include "Boost.h"

#include <string>
#include <utility>
namespace torrent {
class MetaData;
}
class ITrackerService {
public:
    virtual ~ITrackerService() = default;
    virtual net::awaitable<std::vector<std::tuple<std::string, std::uint32_t>>> discoverPeers(
        const torrent::MetaData& torrent, bool compact = true)
        = 0;
};