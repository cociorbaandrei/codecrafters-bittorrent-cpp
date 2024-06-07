#include "old_utils.h"

using json = nlohmann::json;

std::vector<unsigned char> HexToBytes(const std::string& hex)
{
    std::vector<unsigned char> bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }

    return bytes;
}

std::string urlencode(const std::string& p)
{
    std::vector<unsigned char> s(HexToBytes(p));
    static const char lookup[] = "0123456789abcdef";
    std::stringstream e;
    for (int i = 0, ix = s.size(); i < ix; i++) {
        const char& c = s[i];
        if ((48 <= c && c <= 57) || // 0-9
            (65 <= c && c <= 90) || // abc...xyz
            (97 <= c && c <= 122) || // ABC...XYZ
            (c == '-' || c == '_' || c == '.' || c == '~')) {
            e << c;
        } else {
            e << '%';
            e << lookup[(c & 0xF0) >> 4];
            e << lookup[(c & 0x0F)];
        }
    }
    return e.str();
}
bool isASCII(const std::string& s)
{
    return !std::any_of(
        s.begin(), s.end(), [](char c) { return static_cast<unsigned char>(c) > 127; });
}

std::string string_to_hex(const std::string& in)
{
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    for (size_t i = 0; in.length() > i; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(in[i]));
    }

    return ss.str();
}

json decode_bencoded_value(const std::string& encoded_value, int& chars_processed)
{
    if (encoded_value.empty())
        return json::array();
    if (std::isdigit(encoded_value[0])) {
        // Example: "5:hello" -> "hello"
        size_t colon_index = encoded_value.find(':');
        if (colon_index != std::string::npos) {
            std::string number_string = encoded_value.substr(0, colon_index);
            int64_t number = std::atoll(number_string.c_str());
            std::string str = encoded_value.substr(colon_index + 1, number);
            chars_processed += 1 + number_string.length() + str.length();
            return json(str);
        } else {
            throw std::runtime_error("Invalid encoded value: " + encoded_value);
        }
    } else if (encoded_value[0] == 'i') {
        size_t end_index = encoded_value.find('e');
        std::string data = encoded_value.substr(1, end_index - 1);
        chars_processed += end_index;
        int64_t number = std::atoll(data.c_str());
        return json(number);
    }
    if (encoded_value[0] == 'l') {
        std::string content = encoded_value.substr(1, encoded_value.length());
        chars_processed += 1;
        auto parsed_content = json::array();
        while (content.length() != 0 && content[0] != 'e') {
            if (std::isdigit(content[0])) {
                size_t colon_index = content.find(':');
                int n = 0;
                auto decoded_value = decode_bencoded_value(content, n);
                parsed_content.emplace_back(decoded_value);
                auto decoded_string = decoded_value.template get<std::string>();
                chars_processed += n;
                content = content.substr(n, content.length());
            } else if (content[0] == 'i') {
                size_t end_index = content.find('e');
                std::string data = content.substr(1, end_index - 1);
                int64_t number = std::atoll(data.c_str());
                content = content.substr(2 + data.length(), content.length());
                chars_processed += 2 + data.length();
                parsed_content.emplace_back(number);
            } else if (content[0] == 'l') {
                int n = 0;
                auto decoded_list = decode_bencoded_value(content, n);
                parsed_content.emplace_back(decoded_list);
                content = content.substr(n, content.length());
                chars_processed += n;
                // std::cout << decoded_list.dump() << std::endl;
            } else if (content[0] == 'd') {
                int n = 0;
                auto decoded_list = decode_bencoded_value(content, n);
                parsed_content.emplace_back(decoded_list);
                content = content.substr(n, content.length());
                chars_processed += n;
                // std::cout << decoded_list.dump() << std::endl;
            }
        }
        content = content.substr(1, content.length());
        chars_processed += 1;
        return parsed_content;
    }
    if (encoded_value[0] == 'd') {
        std::string content = encoded_value.substr(1, encoded_value.length());
        chars_processed += 1;
        auto parsed_content = json::object();

        while (content.length() != 0 && content[0] != 'e') {
            std::string decoded_key = "";

            if (std::isdigit(content[0])) {
                size_t colon_index = content.find(':');
                int n = 0;
                auto decoded_value = decode_bencoded_value(content, n);
                auto decoded_string = decoded_value.template get<std::string>();
                decoded_key = decoded_value;
                chars_processed += n;
                content = content.substr(n, content.length());
            } else {
                throw std::runtime_error("Expected key as string: " + encoded_value);
            }

            auto decoded_dict_value = json::object();
            if (std::isdigit(content[0])) {
                size_t colon_index = content.find(':');
                int n = 0;
                auto decoded_value = decode_bencoded_value(content, n);
                decoded_dict_value = decoded_value;
                auto decoded_string = decoded_value.template get<std::string>();
                chars_processed += n;
                content = content.substr(n, content.length());
            } else if (content[0] == 'i') {
                size_t end_index = content.find('e');
                std::string data = content.substr(1, end_index - 1);
                int64_t number = std::atoll(data.c_str());
                content = content.substr(2 + data.length(), content.length());
                chars_processed += 2 + data.length();
                decoded_dict_value = json(number);
            } else if (content[0] == 'l') {
                int n = 0;
                auto decoded_list = decode_bencoded_value(content, n);
                content = content.substr(n, content.length());
                chars_processed += n;
                decoded_dict_value = decoded_list;
                // std::cout << decoded_list.dump() << std::endl;
            } else if (content[0] == 'd') {
                int n = 0;
                auto decoded_dict = decode_bencoded_value(content, n);
                decoded_dict_value = decoded_dict;
                content = content.substr(n, content.length());
                chars_processed += n;
            }

            parsed_content[decoded_key] = decoded_dict_value;
        }

        if (content.size() > 1)
            content = content.substr(1, content.length());
        chars_processed += 1;
        return parsed_content;
    } else {
        throw std::runtime_error("Unhandled encoded value: " + encoded_value);
    }
}

namespace ns {

void from_json(const json& j, Info& p)
{
    j.at("name").get_to(p.name);
    j.at("length").get_to(p.length);
    j.at("piece length").get_to(p.piece_length);
    j.at("pieces").get_to(p.pieces);
}
std::vector<std::tuple<std::string, std::uint32_t, std::string>> get_peers(
    json object, bool compact)
{

    if (compact) {
        std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
        std::string peers = object.template get<std::string>();
        for (int i = 0; i < peers.length(); i += 6) {
            std::string peer_ip = peers.substr(i, 4);
            std::string peer_port = peers.substr(i + 4, 2);

            std::uint8_t ip[4] = {0};
            std::uint8_t port[2] = {0};

            for (int j = 0; j < 4; j++)
                ip[j] = static_cast<uint8_t>(peer_ip[j]);

            for (int j = 0; j < 2; j++)
                port[j] = static_cast<uint8_t>(peer_port[j]);

            std::string ip_str = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "."
                + std::to_string(ip[2]) + "." + std::to_string(ip[3]);
            std::uint32_t port_value = (port[0] << 8) | port[1];
            result.emplace_back(ip_str, port_value, "");
        }

        return result;
    }

    std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
    for (const auto& value : object) {
        const auto ip = value["ip"].get<std::string>();
        const auto port = value["port"].get<std::uint32_t>();
        const auto peer_id = string_to_hex(value["peer id"].get<std::string>());
        result.emplace_back(ip, port, peer_id);
    }
    return result;
}

std::string to_bencode(const json& j)
{
    if (j.is_number()) {
        return "i" + std::to_string(j.template get<std::int64_t>()) + "e";
    }
    if (j.is_string()) {
        auto s = j.template get<std::string>();
        return std::to_string(s.length()) + ":" + s;
    }
    if (j.is_array()) {
        std::string s = "l";
        for (auto& el : j.items())
            s += to_bencode(el.value());
        return s + "e";
    }

    std::string result = "d";
    for (auto& el : j.items()) {
        result += to_bencode(el.key());
        result += to_bencode(j[el.key()]);
    }
    return result + "e";
}

TrackerResponse discover_peers(Torrent* torrent)
{
    char url_buffer[2048];
    // strncpy(url_buffer, torrent->announce.c_str(), torrent->announce.length());
    // std::cout << urlencode(torrent->info_hash) << std::endl;
    std::string url_encoded_hash = urlencode(torrent->info_hash);
    snprintf(
        url_buffer,
        2048,
        "%s?info_hash=%s&peer_id=%s&port=%lld&uploaded=%lld&downloaded=%lld&left=%lld&compact=1",
        torrent->announce.c_str(),
        url_encoded_hash.c_str(),
        torrent->peer_id.c_str(),
        torrent->port,
        torrent->uploaded,
        torrent->downloaded,
        torrent->left);
    // std::cout << "Request Url: " << url_buffer << "\n";
    try {
        int n = 0;
        http::Request request{url_buffer};
        const auto response = request.send("GET");
        std::string str_response = std::string{response.body.begin(), response.body.end()};
        json r = decode_bencoded_value(str_response, n);
        auto peers = get_peers(r["peers"], true);
        spdlog::debug("Peers IPs:");
        for (const auto& [ip, port, peer_id] : peers) {
            spdlog::debug("{0}:{1}", ip, port);
            std::cout << ip << ":" << port << "\n";
        }
        return {10, peers};
    } catch (const std::exception& e) {
        spdlog::error("Request failed, error: {0}", e.what());
        // std::cerr << "Request failed, error: " << e.what() << '\n';
    }
    return {};
}
} // namespace ns

std::vector<unsigned char> convertToVector(const std::unique_ptr<char[]>& data, size_t size)
{
    // Initialize a vector with the size of the data
    std::vector<unsigned char> result(size);

    // Copy the data from the unique_ptr to the vector
    // reinterpret_cast is used to convert char* to unsigned char*
    std::copy_n(reinterpret_cast<unsigned char*>(data.get()), size, result.begin());

    return result;
}