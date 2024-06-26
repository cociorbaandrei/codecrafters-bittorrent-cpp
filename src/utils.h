#pragma once

#include <memory> // Include for std::shared_ptr
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


// Helper to facilitate std::visit with multiple lambdas
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace utils::bencode {

// Forward declaration of the structures to allow recursive types
struct BencodeDict;
struct BencodeList;

// Define types for bencode values using std::shared_ptr for recursive structures
using BencodeInt = int64_t;
using BencodeStr = std::string;
using BencodeListPtr = std::shared_ptr<BencodeList>; // Use shared_ptr for recursive definition
using BencodeDictPtr = std::shared_ptr<BencodeDict>; // Use shared_ptr for recursive definition
using BencodeValue = std::variant<BencodeInt, BencodeStr, BencodeListPtr, BencodeDictPtr>;

struct BencodeList : public std::vector<BencodeValue> {
}; // Now BencodeList can contain BencodeValue including itself indirectly
struct BencodeDict : public std::unordered_map<std::string, BencodeValue> {
}; // Same for BencodeDict

BencodeValue decode_bencoded_v(const std::string& encoded_value, size_t& index);
BencodeInt decode_int(const std::string& encoded_value, size_t& index);
BencodeStr decode_str(const std::string& encoded_value, size_t& index);
BencodeListPtr decode_list(const std::string& encoded_value, size_t& index);
BencodeDictPtr decode_dict(const std::string& encoded_value, size_t& index);
BencodeValue parse(const std::string& encoded_value);

std::string serialize_int(BencodeInt value);
std::string serialize_str(const BencodeStr& value);
std::string serialize_list(const BencodeListPtr& list);
std::string serialize_dict(const BencodeDictPtr& dict);
std::string serialize(const BencodeValue& value);
void pretty_print(const BencodeValue& value, int indent = 0);
} // namespace utils::bencode

namespace torrent {
struct ParsedURL {
    std::string scheme;
    std::string host;
    std::string path;
    std::string port;
};
struct FileEntry {
    std::string path;
    int64_t length;
};
struct Info {
    std::int64_t length;
    std::string name;
    std::int64_t piece_length;
    std::string pieces;
    std::vector<FileEntry> files; // For multi-file torrents
};

struct MetaData {
    std::string info_hash;
    std::string announce;
    Info info;
    std::string peer_id;
    std::uint64_t port;
    std::uint64_t uploaded;
    std::uint64_t downloaded;
    std::int64_t left;
    bool compact;
};

struct TrackerResponse {
    std::int32_t interval;
    std::vector<std::tuple<std::string, std::uint32_t, std::string>> peers;
};
ParsedURL parse_url(const std::string& url);
MetaData initialize(utils::bencode::BencodeValue data);
std::string info_hash(utils::bencode::BencodeValue data);
TrackerResponse discover_peers(const MetaData& torrent, bool compact = true);
std::vector<std::tuple<std::string, std::uint32_t, std::string>> get_peers(
    utils::bencode::BencodeValue object, bool compact = false);
std::vector<std::tuple<std::string, std::uint32_t>> get_peers_2(
    utils::bencode::BencodeValue object, bool compact = false);
} // namespace torrent

namespace utils::hex {

std::vector<unsigned char> HexToBytes(const std::string& hex);
std::string urlencode(const std::string& p);

// Convert a hex character to its binary representation
std::string hexCharToBinary(char hex);
// Parse a hex string to a binary string
std::string hexToBinary(const std::string& hex);
std::string parseBitfield(const std::string& hexBitfield);
std::string bytesToHexString(const std::vector<uint8_t>& bytes);
} // namespace utils::hex