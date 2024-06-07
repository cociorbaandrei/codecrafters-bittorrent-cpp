#pragma once
#include "IHttpClient.h"
#include "spdlog/spdlog.h"
#include "utils.h"

#include <ITrackerService.h>
#include <stdio.h>

#include <memory>
#include <vector>
class TrackerService : public ITrackerService {
public:
    TrackerService(std::unique_ptr<IHttpClient> httpClient)
        : m_httpClient(std::move(httpClient))
    {
    }
    net::awaitable<std::vector<std::tuple<std::string, std::uint32_t>>> discoverPeers(
        const torrent::MetaData& torrent, bool compact = true)
    {
        std::string url_encoded_hash = utils::hex::urlencode(torrent.info_hash);
        char url_buffer[2048];
        snprintf(
            url_buffer,
            2048,
            "%s?info_hash=%s&peer_id=%s&port=%lld&uploaded=%lld&downloaded=%"
            "lld&left=%lld&compact=%d",
            torrent.announce.c_str(),
            url_encoded_hash.c_str(),
            torrent.peer_id.c_str(),
            torrent.port,
            torrent.uploaded,
            torrent.downloaded,
            torrent.left,
            compact ? 1 : 0);
        const auto&& [status, body] = co_await m_httpClient->Get(url_buffer);
        auto parsed_respone = utils::bencode::parse(body);
        utils::bencode::pretty_print(parsed_respone);
        auto peers = torrent::get_peers_2(parsed_respone, compact);
        spdlog::info("Peers IPs:");
        for (const auto& [ip, port] : peers) {
            spdlog::info("{0}:{1}", ip, port);
        }
        co_return peers;
    }

    ~TrackerService() = default;

private:
    std::unique_ptr<IHttpClient> m_httpClient;
};