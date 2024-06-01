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
#include "BitTorrentMessage.h"
#include "BTConnection.h"
#include <chrono>
#include <thread>
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <variant>
#include <list>
#include <unordered_map>
#include "utils.h"
#include "old_utils.h"
#include "IntervalMap.h"

#pragma comment(lib, "ws2_32.lib")

void print_torrent_info(const auto& metadata)
{
	spdlog::info("Tracker URL: {0}", metadata.announce);
	spdlog::info("Length: {0}", metadata.info.length);
	spdlog::info("Info Hash: {0}", metadata.info_hash);
	spdlog::info("Piece Length: {0}",  metadata.info.piece_length);
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

#include <map>
#include <vector>
#include <string>
#include <iostream>

std::map<int, std::vector<std::pair<std::string, std::pair<int64_t, int64_t>>>> map_pieces_to_files(const auto& metadata) {
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
			if(piece_map[piece_index].size() > 1){
				for(const auto&[p, a] :  piece_map[piece_index]){
					used_len += a.second;
				}
			}
            piece_map[piece_index].push_back({file.path, {file_piece_offset, file_piece_length - used_len}});
        }
    }

    return piece_map;
}

void print_piece_map(const std::map<int, std::vector<std::pair<std::string, std::pair<int64_t, int64_t>>>>& piece_map) {
    for (const auto& [piece_index, file_chunks] : piece_map) {
        std::cout << "Piece " << piece_index << " contains:\n";
        for (const auto& [file_path, offsets] : file_chunks) {
            std::cout << "  File: " << file_path << ", Offset: " << offsets.first << ", Length: " << offsets.second << "\n";
        }
    }
}
int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
	    // Create a console logger
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%^%l%$] %v");
    console_sink->set_level(spdlog::level::debug);

	    // Create a logger with both sinks
    auto logger = std::make_shared<spdlog::logger>("logger", spdlog::sinks_init_list({console_sink}));
    spdlog::register_logger(logger);

    // Set the flush policy to flush on every log
    logger->flush_on(spdlog::level::debug);
	// if (argc <= 2) {
	// 	dev_test();
	// 	return 0;
	// }

	// if (argc < 2) {
	// 	std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
	// 	return 1;
	// }

	std::string command("");
	command="download";
	// std::cout << argv[0] << "\n";
	// std::cout << argv[1] << "\n";
	// std::cout << argv[2] << "\n";
	// std::cout << argv[3] << "\n";
	// std::cout << argv[4] << "\n";

	std::cout << argc << "\n";
	if (command == "download") {
		int n = 0;
		std::string file_name = argv[3];
        file_name ="/Users/s1ncocio/codecrafters-bittorrent-cpp/sample.torrent";
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);
		utils::bencode::pretty_print(dec);
		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto piece_map = map_pieces_to_files(metadata);
    	print_piece_map(piece_map);
		auto peers_response = torrent::discover_peers(metadata, true);
		auto loop = uvw::loop::get_default();
		auto tcpClient = loop->resource<uvw::tcp_handle>();
			
		tcpClient->connect(std::get<0>(peers_response.peers[0]), std::get<1>(peers_response.peers[0]));

		tcpClient->on<uvw::connect_event>([&tcpClient, hash](const uvw::connect_event& connect_event, uvw::tcp_handle& tcp_handle) {
			spdlog::debug("Connected to server.");
			auto byte_repr = HexToBytes(hash);
			std::array<uint8_t, 20> infoHash;

			std::copy(std::begin(byte_repr), std::end(byte_repr), infoHash.begin());
			BitTorrentMessage msg(infoHash);
			// Send "hello" to the server
			auto data = msg.serialize();

			tcpClient->write(&data[0], data.size());
			tcpClient->read();
		});

		// Set a close event callback
		tcpClient->on<uvw::close_event>([](const uvw::close_event&, uvw::tcp_handle&) {
			spdlog::debug("TCP client handle closed.");
		});

		// Handle the write event
		tcpClient->on<uvw::write_event>([&tcpClient](const uvw::write_event& connect_event, uvw::tcp_handle& tcp_handle) {
			//spdlog::debug("Message sent."); 

		});

		// Handle errors
		tcpClient->on<uvw::error_event>([](const uvw::error_event& err, uvw::tcp_handle&) {
			spdlog::error("Error: {0}", err.what());
		});


		BTConnection bittorent_session(loop, tcpClient, {}, metadata);
		tcpClient->on<uvw::data_event>([&bittorent_session](const uvw::data_event& event, uvw::tcp_handle& tcp_handle) {
			//std::cout << "Data received: " << std::string(event.data.get(), event.length) << " size: " << event.length << std::endl;
			std::vector<uint8_t> data(event.data.get(), event.data.get() + event.length);
			bittorent_session.onDataReceived(data);
			});

		bittorent_session.piece_index_to_download = atoi("0");
		bittorent_session.request_download_name = argv[2];
		bittorent_session.request_download_name ="sample.txt";
		bittorent_session.downloadFullFile = true;
		auto timer = loop->resource<uvw::timer_handle>();

		loop->run();
	}
	else {
		std::cerr << "unknown command: " << command << std::endl;
		return 1;
	}

	return 0;
}
