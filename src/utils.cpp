#include "utils.h"
#include <algorithm>
#include <random>
#include <string>
#include <ranges>
#include "spdlog/spdlog.h"
#include "lib/sha1.hpp"
#include "lib/HTTPRequest.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/HTTPRequest.hpp"
#include <random>
#include <stdio.h>
#include <tuple>
#include <uvw.hpp>
#include <iostream>
#include <memory>
#include <array>
#include <cstring>
#include <iostream>
#include <string_view>
#include <chrono>
#include <thread>
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace utils::bencode {
	BencodeInt decode_int(const std::string& encoded_value, size_t& index) {
		size_t end_index = encoded_value.find('e', index);
		if (end_index == std::string::npos) {
			throw std::runtime_error("Invalid bencoded integer (no ending 'e').");
		}
		BencodeInt number = std::stoll(encoded_value.substr(index + 1, end_index - index - 1));
		index = end_index + 1; // Move past the 'e'
		return number;
	}

	BencodeStr decode_str(const std::string& encoded_value, size_t& index) {
		size_t colon_index = encoded_value.find(':', index);
		if (colon_index == std::string::npos) {
			throw std::runtime_error("Invalid bencoded string (no colon found).");
		}
		int length = std::stoi(encoded_value.substr(index, colon_index - index));
		index = colon_index + 1 + length; // Move past the colon and the string
		return encoded_value.substr(colon_index + 1, length);
	}

	BencodeListPtr decode_list(const std::string& encoded_value, size_t& index) {
		auto list = std::make_shared<BencodeList>();
		index++; // Move past the 'l'
		while (encoded_value[index] != 'e') {
			list->push_back(decode_bencoded_v(encoded_value, index));
		}
		index++; // Move past the 'e'
		return list;
	}

	BencodeDictPtr decode_dict(const std::string& encoded_value, size_t& index) {
		auto dict = std::make_shared<BencodeDict>();
		index++; // Move past the 'd'
		while (index < encoded_value.size() && encoded_value[index] != 'e') {
			BencodeStr key = decode_str(encoded_value, index);
			(*dict)[key] = decode_bencoded_v(encoded_value, index);
		}
		index++; // Move past the 'e'
		return dict;
	}

	BencodeValue decode_bencoded_v(const std::string& encoded_value, size_t& index) {
		if (encoded_value[index] == 'i') {
			return decode_int(encoded_value, index);
		}
		else if (std::isdigit(encoded_value[index])) {
			return decode_str(encoded_value, index);
		}
		else if (encoded_value[index] == 'l') {
			return decode_list(encoded_value, index);
		}
		else if (encoded_value[index] == 'd') {
			return decode_dict(encoded_value, index);
		}
		else {
			throw std::runtime_error("Invalid bencoded value.");
		}
	}

	BencodeValue parse(const std::string& encoded_value) {
		size_t index = 0;
		return decode_bencoded_v(encoded_value, index);
	}

	std::string serialize_int(BencodeInt value) {
		return "i" + std::to_string(value) + "e";
	}

	std::string serialize_str(const BencodeStr& value) {
		return std::to_string(value.size()) + ":" + value;
	}

	std::string serialize_list(const BencodeListPtr& list) {
		std::string result = "l";
		for (const auto& item : *list) {
			result += serialize(item);
		}
		result += "e";
		return result;
	}

	std::string serialize_dict(const BencodeDictPtr& dict) {
		std::string result = "d";
		// Ensure keys are serialized in lexicographical order
		std::vector<std::string> keys;
		for (const auto& pair : *dict) {
			keys.push_back(pair.first);
		}
		std::sort(keys.begin(), keys.end());

		for (const auto& key : keys) {
			result += serialize_str(key) + serialize((*dict)[key]);
		}
		result += "e";
		return result;
	}

	std::string serialize(const BencodeValue& value) {
		return std::visit(overloaded{
			[](BencodeInt arg) { return serialize_int(arg); },
			[](const BencodeStr& arg) { return serialize_str(arg); },
			[](const BencodeListPtr& arg) { return serialize_list(arg); },
			[](const BencodeDictPtr& arg) { return serialize_dict(arg); }
			}, value);
	}


}


namespace torrent {
	MetaData initialize(utils::bencode::BencodeValue data)
	{
		auto dictionary = *std::get<utils::bencode::BencodeDictPtr>(data);
		auto announce = std::get<utils::bencode::BencodeStr>(dictionary["announce"]);
		auto info = *std::get<utils::bencode::BencodeDictPtr>(dictionary["info"]);

		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<std::mt19937::result_type> dist6(0, 9); // distribution in range [1, 6]
		std::string peer_id;
		for (int i = 0; i < 20; i++)
		{
			peer_id += std::to_string(dist6(dev));
		}
		//std::cout << "Peer Id: " << peer_id << std::endl;
		//peer_id = "AAAAAAAAAAAAAAAAAAAAA";
		MetaData metadata = {
			.info_hash = info_hash(data),
			.announce = announce,
			.info = {
				.length = std::get<int64_t>(info["length"]),
				.name = std::get<std::string>(info["name"]),
				.piece_length = std::get<int64_t>(info["piece length"]),
				.pieces = std::get<std::string>(info["pieces"]),
			 },
			.peer_id = peer_id,
			.port = 25428,
			.uploaded = 0,
			.downloaded = 0,
			.left = std::get<int64_t>(info["length"]),
			.compact = 1
		};
		return metadata;
	}

	std::string info_hash(utils::bencode::BencodeValue data)
	{
		auto dictionary = *std::get<utils::bencode::BencodeDictPtr>(data);
		auto info_ser = utils::bencode::serialize(dictionary["info"]);
		SHA1 checksum;
		checksum.update(info_ser);
		const std::string hash = checksum.final();

		return hash;
	}
	TrackerResponse discover_peers(const MetaData& torrent) {
		char url_buffer[2048];
		//strncpy(url_buffer, torrent->announce.c_str(), torrent->announce.length());
		//std::cout << urlencode(torrent->info_hash) << std::endl;
		std::string url_encoded_hash = utils::hex::urlencode(torrent.info_hash);
		sprintf(
			url_buffer,
			"%s?info_hash=%s&peer_id=%s&port=%ld&uploaded=%ld&downloaded=%ld&left=%ld&compact=1",
			torrent.announce.c_str(),
			url_encoded_hash.c_str(),
			torrent.peer_id.c_str(),
			torrent.port,
			torrent.uploaded,
			torrent.downloaded,
			torrent.left);
		//std::cout << "Request Url: " << url_buffer << "\n";
		try
		{
			int n = 0;
			http::Request request{ url_buffer };
			const auto response = request.send("GET");
			std::string str_response = std::string{ response.body.begin(), response.body.end() };

			auto parsed_respone = utils::bencode::parse(str_response);

			auto peers = get_peers(parsed_respone, true);
			spdlog::debug("Peers IPs:");
			for (const auto& [ip, port, peer_id] : peers)
			{
				spdlog::debug("{0}:{1}", ip, port);
				std::cout << ip << ":" << port << "\n";
			}
			return { 10, peers };
		}
		catch (const std::exception& e)
		{
			spdlog::error("Request failed, error: {0}", e.what());
			//std::cerr << "Request failed, error: " << e.what() << '\n';
		}
		return {  };
	}
	std::vector<std::tuple<std::string, std::uint32_t, std::string>> get_peers(utils::bencode::BencodeValue object, bool compact) 
	{
		if (compact) {
			auto dictionary = *std::get<utils::bencode::BencodeDictPtr>(object);
			auto peers = std::get<utils::bencode::BencodeStr>(dictionary["peers"]);
			std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;

			for (int i = 0; i < peers.length(); i += 6) {
				std::string peer_ip = peers.substr(i, 4);
				std::string peer_port = peers.substr(i + 4, 2);

				std::uint8_t ip[4] = { 0 };
				std::uint8_t port[2] = { 0 };

				for (int j = 0; j < 4; j++)
					ip[j] = static_cast<uint8_t>(peer_ip[j]);

				for (int j = 0; j < 2; j++)
					port[j] = static_cast<uint8_t>(peer_port[j]);

				std::string ip_str = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]);
				std::uint32_t port_value = (port[0] << 8) | port[1];
				result.emplace_back(ip_str, port_value, "");
			}

			return result;
		}

		std::vector<std::tuple<std::string, std::uint32_t, std::string>> result;
		//for (const auto& value : object) {
		//	const auto ip = value["ip"].get <std::string>();
		//	const auto port = value["port"].get <std::uint32_t>();
		//	const auto peer_id = string_to_hex(value["peer id"].get <std::string>());
		//	result.emplace_back(ip, port, peer_id);
		//}
		return result;
	}
}

namespace utils::hex {
	std::string bytesToHexString(const std::vector<uint8_t>& bytes) {
		std::stringstream ss;
		ss << std::hex << std::setfill('0');
		for (uint8_t byte : bytes) {
			ss << std::setw(2) << static_cast<int>(byte);
		}
		return ss.str();
	}

	std::vector<unsigned char> HexToBytes(const std::string& hex) {
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
		for (int i = 0, ix = s.size(); i < ix; i++)
		{
			const char& c = s[i];
			if ((48 <= c && c <= 57) ||//0-9
				(65 <= c && c <= 90) ||//abc...xyz
				(97 <= c && c <= 122) || //ABC...XYZ
				(c == '-' || c == '_' || c == '.' || c == '~')
				)
			{
				e << c;
			}
			else
			{
				e << '%';
				e << lookup[(c & 0xF0) >> 4];
				e << lookup[(c & 0x0F)];
			}
		}
		return e.str();
	}
	// Convert a hex character to its binary representation
	std::string hexCharToBinary(char hex) {
		switch (toupper(hex)) {
			case '0': return "0000";
			case '1': return "0001";
			case '2': return "0010";
			case '3': return "0011";
			case '4': return "0100";
			case '5': return "0101";
			case '6': return "0110";
			case '7': return "0111";
			case '8': return "1000";
			case '9': return "1001";
			case 'A': return "1010";
			case 'B': return "1011";
			case 'C': return "1100";
			case 'D': return "1101";
			case 'E': return "1110";
			case 'F': return "1111";
			default: return "0000"; // Should not happen
		}
	}

	// Parse a hex string to a binary string
	std::string hexToBinary(const std::string& hex) {
		std::string binary;
		for (char c : hex) {
			binary += hexCharToBinary(c);
		}
		return binary;
	}

	// Parse the bitfield message and print which pieces are available
	std::string parseBitfield(const std::string& hexBitfield) {
		std::string binaryBitfield = hexToBinary(hexBitfield);
		
		int pieceIndex = 0;
		for (char bit : binaryBitfield.substr(0, binaryBitfield.length()- 2)) {
			std::cout << "Piece " << pieceIndex << ": " << (bit == '1' ? "Available" : "Missing") << std::endl;
			++pieceIndex;
		}
		return binaryBitfield;
	}
}

static std::string base64_encode(const std::string& in) {

	std::string out;

	int val = 0, valb = -6;
	for (unsigned char c : in) {
		val = (val << 8) + c;
		valb += 8;
		while (valb >= 0) {
			out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
			valb -= 6;
		}
	}
	if (valb > -6) out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
	while (out.size() % 4) out.push_back('=');
	return out;
}
