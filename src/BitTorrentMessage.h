#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
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

class BitTorrentMessage {
public:
	static constexpr size_t ProtocolLength = 19;
	static constexpr std::string_view ProtocolString = "BitTorrent protocol";
	static constexpr std::array<uint8_t, 8> ReservedBytes = { 0, 0, 0, 0, 0, 0, 0, 0 };
	std::array<uint8_t, 20> infoHash; // This should be set to the actual SHA1 hash
	std::array<uint8_t, 20> peerId = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99 };

	// Constructor that initializes infoHash with a provided value
	BitTorrentMessage(const std::array<uint8_t, 20>& hash = {}) : infoHash(hash) {}

	// Serialize the message to a byte array
	std::vector<char> serialize() const {
		std::vector<char> message;
		message.reserve(1 + ProtocolLength + ReservedBytes.size() + infoHash.size() + peerId.size());

		message.push_back(static_cast<char>(ProtocolLength));
		message.insert(message.end(), ProtocolString.begin(), ProtocolString.end());
		message.insert(message.end(), ReservedBytes.begin(), ReservedBytes.end());
		message.insert(message.end(), infoHash.begin(), infoHash.end());
		message.insert(message.end(), peerId.begin(), peerId.end());

		return message;
	}
	// Deserialize a BitTorrentMessage from a byte vector
	static BitTorrentMessage deserialize(const std::vector<unsigned char>& data) {
		if (data.size() != 68) { // 1(protocol length) + 19(protocol string) + 8(reserved bytes) + 20(infohash) + 20(peerId)
			throw std::runtime_error("Invalid data size for BitTorrentMessage.");
		}

		BitTorrentMessage msg;

		// Check protocol length
		char protocolLength = data[0];
		if (protocolLength != ProtocolLength) {
			throw std::runtime_error("Invalid protocol length.");
		}

		// Verify protocol string
		std::string protocolString(data.begin() + 1, data.begin() + 20);
		if (protocolString != ProtocolString) {
			throw std::runtime_error("Invalid protocol string.");
		}

		// No need to check reserved bytes as they are expected to be zeros

		// Extract infoHash
		std::copy_n(data.begin() + 28, 20, msg.infoHash.begin());

		// Extract peerId
		std::copy_n(data.begin() + 48, 20, msg.peerId.begin());
		return msg;
	}

};
