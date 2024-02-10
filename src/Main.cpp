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
void dev_test() {

	int n = 0;
	std::string file_name = "anyone.torrent";
	std::uintmax_t filesize = std::filesystem::file_size("anyone.torrent");
	std::ifstream torrent_file(file_name, std::ios::binary);
	std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());

	auto dec = utils::bencode::parse(str);
	auto hash = torrent::info_hash(dec);
	auto metadata = torrent::initialize(dec);
	print_torrent_info(metadata);
	auto peers_response = torrent::discover_peers(metadata);

	auto loop = uvw::loop::get_default();

	auto tcpClient = loop->resource<uvw::tcp_handle>();

//	torrent::TrackerResponse peers_response;
//	peers_response.peers.push_back({ "127.0.0.1", 25428 ,""});
	tcpClient->connect(std::get<0>(peers_response.peers[0]), std::get<1>(peers_response.peers[0]));

	tcpClient->on<uvw::connect_event>([&tcpClient, hash](const uvw::connect_event& connect_event, uvw::tcp_handle& tcp_handle) {
		spdlog::debug("Connected to server.");  
		auto byte_repr = HexToBytes(hash);
		std::array<uint8_t, 20> infoHash;

		std::copy(std::begin(byte_repr), std::end(byte_repr), infoHash.begin());
		BitTorrentMessage msg(infoHash);

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
	// Handle the data event
	tcpClient->on<uvw::data_event>([&bittorent_session](const uvw::data_event& event, uvw::tcp_handle& tcp_handle) {
		//std::cout << "Data received: " << std::string(event.data.get(), event.length) << " size: " << event.length << std::endl;
		std::vector<uint8_t> data(event.data.get(), event.data.get() + event.length);
		bittorent_session.onDataReceived(data);
	});

	bittorent_session.piece_index_to_download = atoi("0");
	bittorent_session.request_download_name = "piece.gif";
	bittorent_session.downloadFullFile = true;
	// Create a timer handle
	auto timer = loop->resource<uvw::timer_handle>();

	loop->run();
}


int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
	
	if (argc <= 2) {
		dev_test();
		return 0;
	}

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
		return 1;
	}
	std::string command(argv[2]);
	if (command == "download") {
		int n = 0;
		std::string file_name = argv[4];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);

		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto peers_response = torrent::discover_peers(metadata);
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
		bittorent_session.request_download_name = argv[3];
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
