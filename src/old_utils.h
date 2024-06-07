#pragma once

#include "lib/HTTPRequest.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "spdlog/spdlog.h"
#include "utils.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdio.h>
#include <uvw.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <iostream>
#include <list>
#include <memory>
#include <random>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <variant>

using json = nlohmann::json;

std::vector<unsigned char> HexToBytes(const std::string& hex);
std::string urlencode(const std::string& p);
bool isASCII(const std::string& s);
std::string string_to_hex(const std::string& in);
json decode_bencoded_value(const std::string& encoded_value, int& chars_processed);

namespace ns {
struct Info {
    std::int64_t length;
    std::string name;
    std::int64_t piece_length;
    std::string pieces;
};
void from_json(const json& j, Info& p);
std::vector<std::tuple<std::string, std::uint32_t, std::string>> get_peers(
    json object, bool compact = false);
std::string to_bencode(const json& j);

struct Torrent {
    std::string info_hash;
    std::string announce;
    ns::Info info;
    std::string peer_id;
    std::uint64_t port;
    std::uint64_t uploaded;
    std::uint64_t downloaded;
    std::uint64_t left;
    bool compact;
};

struct TrackerResponse {
    std::int32_t interval;
    std::vector<std::tuple<std::string, std::uint32_t, std::string>> peers;
};

TrackerResponse discover_peers(Torrent* torrent);
} // namespace ns

std::vector<unsigned char> convertToVector(const std::unique_ptr<char[]>& data, size_t size);
static std::string base64_encode(const std::string& in);