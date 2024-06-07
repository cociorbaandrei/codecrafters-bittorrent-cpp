#pragma once
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
// #include "lib/HTTPRequest.hpp"
#include <stdio.h>
#include <uvw.hpp>

#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <random>
#include <string_view>
#include <tuple>

class BitTorrentMessage {
public:
    static constexpr size_t ProtocolLength = 19;
    static constexpr std::string_view ProtocolString = "BitTorrent protocol";
    static constexpr std::array<uint8_t, 8> ReservedBytes = {0, 0, 0, 0, 0, 0, 0, 0};
    std::array<uint8_t, 20> infoHash; // This should be set to the actual SHA1 hash
    std::array<uint8_t, 20> peerId = {0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};

    // Constructor that initializes infoHash with a provided value
    BitTorrentMessage(const std::array<uint8_t, 20>& hash = {})
        : infoHash(hash)
    {
    }

    // Serialize the message to a byte array
    std::vector<char> serialize() const
    {
        // const char h[] = "\x13BitTorrent
        // protocol\x00\x00\x00\x00\x00\x00\x00\x00\x86\xd4\xc8\x00\x24\xa4\x69\xbe\x4c\x50\xbc\x5a\x10\x2c\xf7\x17\x80\x31\x00\x74-TR2940-k8hj0wgej6ch";
        //	std::vector<uint8_t> m = { 19, 66, 105, 116, 84, 111, 114, 114, 101, 110, 116, 32,
        // 112, 114, 111, 116, 111, 99, 111, 108, 0, 0, 0, 0, 0, 0, 0, 0, 95, 255, 14, 28, 138, 196,
        // 20, 134, 3, 16, 188, 193, 203, 118, 172, 40, 233, 96, 239, 190, 91, 118, 198, 4, 222,
        // 248, 170, 23, 224, 176, 48, 76, 249, 172, 156, 170, 181, 22, 198, 146 };
        std::vector<char> message;
        /*for (int i = 0; i < m.size(); i++)
                message.push_back(m[i]);*/
        message.reserve(
            1 + ProtocolLength + ReservedBytes.size() + infoHash.size() + peerId.size());

        message.push_back(static_cast<char>(ProtocolLength));
        message.insert(message.end(), ProtocolString.begin(), ProtocolString.end());
        message.insert(message.end(), ReservedBytes.begin(), ReservedBytes.end());
        message.insert(message.end(), infoHash.begin(), infoHash.end());
        message.insert(message.end(), peerId.begin(), peerId.end());

        return message;
    }
    // Deserialize a BitTorrentMessage from a byte vector
    static BitTorrentMessage deserialize(const std::vector<unsigned char>& data)
    {
        if (data.size() != 68) { // 1(protocol length) + 19(protocol string) + 8(reserved bytes) +
                                 // 20(infohash) + 20(peerId)
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
