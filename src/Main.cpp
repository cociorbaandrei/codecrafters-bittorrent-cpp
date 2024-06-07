
/*
#include "BTConnection.h"
#include "BitTorrentMessage.h"
#include "IntervalMap.h"
#include "lib/HTTPRequest.hpp"
#include "lib/nlohmann/json.hpp"
#include "lib/sha1.hpp"
#include "spdlog/spdlog.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <stdio.h>
#include <uvw.hpp>

#include <array>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>
*/

#include "BTConnection.h"
#include "BitTorrentMessage.h"
#include "Boost.h"
#include "FileReader.h"
#include "HttpClient.h"
#include "TrackerService.h"
#include "boost/asio/detached.hpp"
#include "boost/asio/read.hpp"
#include "spdlog/spdlog.h"
#include "utils.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// #pragma comment(lib, "ws2_32.lib")
void print_torrent_info(const auto& metadata)
{
    spdlog::info("Tracker URL: {0}", metadata.announce);
    spdlog::info("Length: {0}", metadata.info.length);
    spdlog::info("Info Hash: {0}", metadata.info_hash);
    spdlog::info("Piece Length: {0}", metadata.info.piece_length);
    spdlog::info("Piece Hashes:");

    for (std::size_t i = 0; i < metadata.info.pieces.size(); i += 20) {
        std::string piece = metadata.info.pieces.substr(i, 20);
        std::stringstream ss;
        for (unsigned char byte : piece) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        spdlog::info("{0}", ss.str());
    }
}

std::map<int, std::vector<std::pair<std::string, std::pair<int64_t, int64_t>>>> map_pieces_to_files(
    const auto& metadata)
{
    std::map<int, std::vector<std::pair<std::string, std::pair<int64_t, int64_t>>>> piece_map;
    int64_t piece_length = metadata.info.piece_length;
    int64_t total_length = 0;

    for (const auto& file : metadata.info.files) {
        int64_t file_length = file.length;
        int64_t file_offset = total_length;
        total_length += file_length;

        int start_piece = file_offset / piece_length;
        int end_piece = (file_offset + file_length - 1) / piece_length;

        for (int piece_index = start_piece; piece_index <= end_piece; ++piece_index) {
            int64_t piece_offset = piece_index * piece_length;
            int64_t file_piece_offset = std::max(int64_t(0), piece_offset - file_offset);
            int64_t file_piece_length = std::min(piece_length, file_length - file_piece_offset);
            auto used_len = 0;
            if (piece_map[piece_index].size() > 1) {
                for (const auto& [p, a] : piece_map[piece_index]) {
                    used_len += a.second;
                }
            }
            piece_map[piece_index].push_back(
                {file.path, {file_piece_offset, file_piece_length - used_len}});
        }
    }

    return piece_map;
}

void print_piece_map(
    const std::map<int, std::vector<std::pair<std::string, std::pair<int64_t, int64_t>>>>&
        piece_map)
{
    for (const auto& [piece_index, file_chunks] : piece_map) {
        std::cout << "Piece " << piece_index << " contains:\n";
        for (const auto& [file_path, offsets] : file_chunks) {
            std::cout << "  File: " << file_path << ", Offset: " << offsets.first
                      << ", Length: " << offsets.second << "\n";
        }
    }
}

// int main(int argc, char* argv[]) {
//     spdlog::set_level(spdlog::level::info);
// 	    // Create a console logger
//     auto console_sink =
//     std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
//     console_sink->set_pattern("[%^%l%$] %v");
//     console_sink->set_level(spdlog::level::debug);

// 	    // Create a logger with both sinks
//     auto logger = std::make_shared<spdlog::logger>("logger",
//     spdlog::sinks_init_list({console_sink}));
//     spdlog::register_logger(logger);

//     // Set the flush policy to flush on every log
//     logger->flush_on(spdlog::level::debug);
// 	// if (argc <= 2) {
// 	// 	dev_test();
// 	// 	return 0;
// 	// }

// 	// if (argc < 2) {
// 	// 	std::cerr << "Usage: " << argv[0] << " decode <encoded_value>"
// << std::endl;
// 	// 	return 1;
// 	// }

// 	std::string command("");
// 	command="download";
// 	// std::cout << argv[0] << "\n";
// 	// std::cout << argv[1] << "\n";
// 	// std::cout << argv[2] << "\n";
// 	// std::cout << argv[3] << "\n";
// 	// std::cout << argv[4] << "\n";

// 	std::cout << argc << "\n";
// 	if (command == "download") {
// 		int n = 0;
// 		std::string file_name = argv[3];
//         file_name
//         ="/Users/s1ncocio/codecrafters-bittorrent-cpp/sample.torrent";
// 		std::ifstream torrent_file(file_name);
// 		std::string str((std::istreambuf_iterator<char>(torrent_file)),
// std::istreambuf_iterator<char>()); 		auto dec =
// utils::bencode::parse(str); 		utils::bencode::pretty_print(dec);
// auto hash = torrent::info_hash(dec); 		auto metadata =
// torrent::initialize(dec); 		auto piece_map =
// map_pieces_to_files(metadata);
//     	print_piece_map(piece_map);
// 		auto peers_response = torrent::discover_peers(metadata, true);
// 		auto loop = uvw::loop::get_default();
// 		auto tcpClient = loop->resource<uvw::tcp_handle>();

// 		tcpClient->connect(std::get<0>(peers_response.peers[0]),
// std::get<1>(peers_response.peers[0]));

// 		tcpClient->on<uvw::connect_event>([&tcpClient, hash](const
// uvw::connect_event& connect_event, uvw::tcp_handle& tcp_handle) {
// 			spdlog::debug("Connected to server.");
// 			auto byte_repr = HexToBytes(hash);
// 			std::array<uint8_t, 20> infoHash;

// 			std::copy(std::begin(byte_repr), std::end(byte_repr),
// infoHash.begin()); 			BitTorrentMessage msg(infoHash);
// 			// Send "hello" to the server
// 			auto data = msg.serialize();

// 			tcpClient->write(&data[0], data.size());
// 			tcpClient->read();
// 		});

// 		// Set a close event callback
// 		tcpClient->on<uvw::close_event>([](const uvw::close_event&,
// uvw::tcp_handle&) { 			spdlog::debug("TCP client handle
// closed.");
// 		});

// 		// Handle the write event
// 		tcpClient->on<uvw::write_event>([&tcpClient](const
// uvw::write_event& connect_event, uvw::tcp_handle& tcp_handle) {
// 			//spdlog::debug("Message sent.");

// 		});

// 		// Handle errors
// 		tcpClient->on<uvw::error_event>([](const uvw::error_event& err,
// uvw::tcp_handle&) { 			spdlog::error("Error: {0}", err.what());
// 		});

// 		BTConnection bittorent_session(loop, tcpClient, {}, metadata);
// 		tcpClient->on<uvw::data_event>([&bittorent_session](const
// uvw::data_event& event, uvw::tcp_handle& tcp_handle) {
// 			//std::cout << "Data received: " <<
// std::string(event.data.get(), event.length) << " size: " << event.length <<
// std::endl; 			std::vector<uint8_t> data(event.data.get(),
// event.data.get() + event.length);
// bittorent_session.onDataReceived(data);
// 			});

// 		bittorent_session.piece_index_to_download = atoi("0");
// 		bittorent_session.request_download_name = argv[2];
// 		bittorent_session.request_download_name ="sample.txt";
// 		bittorent_session.downloadFullFile = true;
// 		auto timer = loop->resource<uvw::timer_handle>();

// 		loop->run();
// 	}
// 	else {
// 		std::cerr << "unknown command: " << command << std::endl;
// 		return 1;
// 	}

// 	return 0;
// }
bool canParseMessage(std::vector<uint8_t>& buffer)
{
    if (buffer.size() < 4)
        return false; // Not enough data for length prefix

    uint32_t length
        = ntohl(*reinterpret_cast<uint32_t*>(buffer.data())); // Assuming network byte order
    spdlog::info("Length {0}", length);
    if (length == 0) {
        // It's a keep-alive message
        // std::cout << "Keep-alive message received." << std::endl;
        spdlog::debug("Keep-alive message received.");
        return false;
    }
    return buffer.size() >= length + 4; // Check if we have the whole message
}
BTMessage parseMessageNoLen(std::vector<uint8_t>& buffer)
{
    // prettyPrintHex(buffer);
    BTMessageType type = static_cast<BTMessageType>(buffer[0]);
    std::vector<uint8_t> payload(buffer.begin() + 1, buffer.end());

    return {type, payload}; // Assuming BTMessage is a struct holding these two
}
awaitable<void> reader(tcp::socket socket)
{
    // let's do the handshake
    std::string buffer;
    std::size_t n = co_await boost::asio::async_read(
        socket, boost::asio::dynamic_buffer(buffer, 68), use_awaitable);
    if (buffer[0] == 19
        && std::string(buffer.begin() + 1, buffer.begin() + 20) == "BitTorrent protocol") {
        // Successfully received a valid handshake
        try {
            std::vector<uint8_t> payload(buffer.begin(), buffer.begin() + 68);
            BitTorrentMessage msg = BitTorrentMessage::deserialize(payload);
            // spdlog::debug("Peer ID: {:xsp}", spdlog::to_hex(msg.peerId));
            std::cout << "Peer ID: ";
            for (int i = 0; i < msg.peerId.size(); i++) {
                printf("%02x", msg.peerId[i]);
            }
            std::cout << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Error deserializing message: " << e.what() << std::endl;
        }
    }

    spdlog::info(
        "Handshake with {0}:{1} succesfull {2}",
        socket.remote_endpoint().address().to_string(),
        socket.remote_endpoint().port(),
        buffer.size());
    try {
        while (true) {
            std::vector<uint8_t> length_msg(4);
            std::size_t n = co_await boost::asio::async_read(
                socket, boost::asio::buffer(length_msg), use_awaitable);
            uint32_t length = ntohl(
                *reinterpret_cast<uint32_t*>(length_msg.data())); // Assuming network byte orde
            spdlog::info(
                "Received from {0}:{1} {2}",
                socket.remote_endpoint().address().to_string(),
                socket.remote_endpoint().port(),
                n);

            if (length == 0) {
                spdlog::debug("Keep-alive message received.");
                continue;
            }
            std::vector<uint8_t> read_msg(length);
            n = co_await boost::asio::async_read(
                socket, boost::asio::buffer(read_msg), use_awaitable);

            auto message = parseMessageNoLen(read_msg);
            switch (message.type) {
            case BTMessageType::Choke:
                spdlog::info("Received BTMessageType::Choke");
                // handleChoke(message.payload);
                break;
            case BTMessageType::Interested: {
                spdlog::info("Received BTMessageType::ChInterestedoke");
                char data[] = "\x00\x00\x00\x01\x01";
                break;
            }
            case BTMessageType::Have: {
                spdlog::info("Received BTMessageType::Have");
                prettyPrintHex(message.payload);
                auto pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(message.payload.data()));
                spdlog::info("Have {0}", pieceIndex);
                break;
            }
            case BTMessageType::Piece: {
                spdlog::debug("Received BTMessageType::Piece");
                // Piece index (network byte order)

                auto pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(message.payload.data()));
                auto begin = ntohl(*reinterpret_cast<const uint32_t*>(message.payload.data() + 4));
                std::vector<uint8_t> piece_data(message.payload.begin() + 8, message.payload.end());

                const int blockSize = 16 * 1024; // 16 KiB

                spdlog::info(
                    "Piece Index: {:0}, Begin: {:1}, Size: {:2}",
                    pieceIndex,
                    begin,
                    piece_data.size());

                break;
            }
            case BTMessageType::Unchoke: {
                // std::cout << "Received BTMessageType::Unchoke\n";
                spdlog::info("Received BTMessageType::Unchoke");
                break;
            }
            case BTMessageType::Bitfield: {
                spdlog::info("Received BTMessageType::Bitfield");
                prettyPrintHex(message.payload);

                // utils::hex::parseBitfield(utils::hex::bytesToHexString(message.payload));
                char data[] = "\x00\x00\x00\x01\x02";

                break;
            }
            default:
                spdlog::error(
                    "Unknown message type received: {:0}", static_cast<int>(message.type));
                prettyPrintHex(message.payload);

                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Read error: " << e.what() << std::endl;
    }
}

awaitable<void> connect_to_server(
    const std::string& host, const std::string& service, std::string hash)
{
    auto executor = co_await boost::asio::this_coro::executor;

    tcp::resolver resolver(executor);
    auto endpoints = co_await resolver.async_resolve(host, service, use_awaitable);

    tcp::socket socket(executor);

    // Connect to IPv4 and IPv6
    co_await boost::asio::async_connect(socket, endpoints, use_awaitable);

    spdlog::debug(
        "Connected to {0}:{1}",
        socket.remote_endpoint().address().to_string(),
        socket.remote_endpoint().port());

    auto byte_repr = utils::hex::HexToBytes(hash);
    std::array<uint8_t, 20> infoHash;
    std::copy(std::begin(byte_repr), std::end(byte_repr), infoHash.begin());
    BitTorrentMessage msg(infoHash);
    auto data = msg.serialize();
    co_await socket.async_write_some(boost::asio::buffer(data), use_awaitable);

    // Let's do the handshake

    net::co_spawn(executor, reader(std::move(socket)), net::detached);
}

net::awaitable<void> co_main(int argc, char** argv)
{
    spdlog::set_level(spdlog::level::info);
    try {
        auto executor = co_await net::this_coro::executor;
        std::string fileName(argv[1]);
        spdlog::info("Loading torrent: {0}", fileName);

        auto fileReader = std::make_shared<FileReader>();
        auto data = co_await fileReader->readFile(fileName);

        auto dec = utils::bencode::parse(data);
        utils::bencode::pretty_print(dec);
        auto hash = torrent::info_hash(dec);
        auto metadata = torrent::initialize(dec);
        print_piece_map(map_pieces_to_files(metadata));
        auto trackerService = std::make_unique<TrackerService>(std::make_unique<HttpClient>());
        auto peers = co_await trackerService->discoverPeers(metadata, false);

        for (const auto& [ip, port] : peers) {
            co_await connect_to_server(ip, std::to_string(port), hash);
            break;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv)
{
    try {
        net::io_context ioc;
        // Run the asynchronous operation
        net::co_spawn(ioc, co_main(argc, argv), net::detached);

        // Run the I/O service
        ioc.run();
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
