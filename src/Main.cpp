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

void dev_test() {

	int n = 0;
	std::string file_name = "anyone.torrent";
	std::uintmax_t filesize = std::filesystem::file_size("anyone.torrent");
	std::ifstream torrent_file(file_name, std::ios::binary);
	std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());

	auto dec = utils::bencode::parse(str);

	auto hash = torrent::info_hash(dec);

	auto metadata = torrent::initialize(dec);
	auto peers_response = torrent::discover_peers(metadata);
	json decoded_value;

	spdlog::debug("Info Hash: {0}", hash);
	//spdlog::debug("Piece Length:  {0}", info_struct.piece_length);
//	spdlog::debug("Piece Hashes: ");


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

	// ================================= File Writing ==========================


	// ================================= File Writing ==========================

	BTConnection bittorent_session(loop, tcpClient, decoded_value, metadata);
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

	std::string command = argv[1];





	if (command == "decode") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " decode <encoded_value>" << std::endl;
			return 1;
		}
		// You can use print statements as follows for debugging, they'll be visible when running tests.
		//std::cout << "Logs from your program will appear here!" << std::endl;

		//Uncomment this block to pass the first stage
		std::string encoded_value = argv[2];
		int n = 0;
		json decoded_value = decode_bencoded_value(encoded_value, n);
		std::cout << decoded_value.dump() << std::endl;
	}
	else if (command == "info") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " info <torrent_file>" << std::endl;
			return 1;
		}
		int n = 0;

		std::string file_name = argv[2];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);

		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto peers_response = torrent::discover_peers(metadata);

		std::cout << "Tracker URL: " << metadata.announce << std::endl;
		std::cout << "Length: "		 << metadata.info.length << std::endl;
		std::cout << "Info Hash: " << metadata.info_hash << std::endl;
		std::cout << "Piece Length: " << metadata.info.piece_length << std::endl;
		std::cout << "Piece Hashes: " << std::endl;

		for (std::size_t i = 0; i < metadata.info.pieces.size(); i += 20) {
			std::string piece = metadata.info.pieces.substr(i, 20);
			std::stringstream ss;
			for (unsigned char byte : piece) {
				ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
			}
			std::cout << ss.str() << std::endl;
		}

	}
	else if (command == "peers") {
		if (argc < 3) {
			std::cerr << "Usage: " << argv[0] << " peers <torrent_file>" << std::endl;
			return 1;
		}
		int n = 0;

		std::string file_name = argv[2];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);

		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto peers_response = torrent::discover_peers(metadata);

		std::cout << "Tracker URL: " << metadata.announce << std::endl;
		std::cout << "Length: " << metadata.info.length << std::endl;
		std::cout << "Info Hash: " << metadata.info_hash << std::endl;
		std::cout << "Piece Length: " << metadata.info.piece_length << std::endl;
		std::cout << "Piece Hashes: " << std::endl;

		for (std::size_t i = 0; i < metadata.info.pieces.size(); i += 20) {
			std::string piece = metadata.info.pieces.substr(i, 20);
			std::stringstream ss;
			for (unsigned char byte : piece) {
				ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
			}
			std::cout << ss.str() << std::endl;
		}

		auto peers = torrent::discover_peers(metadata);

	}
	else if (command == "handshake") {
		std::string file_name = argv[2];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);

		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto peers_response = torrent::discover_peers(metadata);

		std::cout << "Tracker URL: " << metadata.announce << std::endl;
		std::cout << "Length: " << metadata.info.length << std::endl;
		std::cout << "Info Hash: " << metadata.info_hash << std::endl;
		std::cout << "Piece Length: " << metadata.info.piece_length << std::endl;

		auto peers = torrent::discover_peers(metadata);
		auto loop = uvw::loop::get_default();

		auto tcpClient = loop->resource<uvw::tcp_handle>();
		auto ip_port = std::string(argv[3]);
		auto separator = ip_port.find(":");
		auto ip = ip_port.substr(0, separator);
		auto port = ip_port.substr(separator+1, ip_port.size());
		//std::cout << ip << "\n";
		//std::cout << port << "\n";
		tcpClient->connect(ip, atoi(port.c_str()));

		tcpClient->on<uvw::connect_event>([&tcpClient, hash](const uvw::connect_event& connect_event, uvw::tcp_handle& tcp_handle) {

			auto byte_repr = HexToBytes(hash);
			std::array<uint8_t, 20> infoHash;

			std::copy(std::begin(byte_repr), std::end(byte_repr), infoHash.begin());
			BitTorrentMessage msg(infoHash);
			// Send "hello" to the server
			auto data = msg.serialize();

			tcpClient->write(&data[0], data.size());
			tcpClient->read();
		});

		// Handle the write event
		tcpClient->on<uvw::write_event>([&tcpClient](const uvw::write_event& connect_event, uvw::tcp_handle& tcp_handle) {
			spdlog::debug("Message sent.");  
		});

		// Handle errors
		tcpClient->on<uvw::error_event>([](const uvw::error_event& err, uvw::tcp_handle&) {
			std::cerr << "Error: " << err.what() << std::endl;
		});
		// Handle the data event
		tcpClient->on<uvw::data_event>([](const uvw::data_event& event, uvw::tcp_handle& tcp_handle) {
			auto d = std::string(event.data.get(), event.length > 68 ? 68 : event.length);
			//std::cout << "Data received: " << std::string(event.data.get(), event.length) << " size: " << event.length << std::endl;
			std::vector<unsigned char> data;
			for (auto& c : d) {
				data.push_back(c);
			}
			std::copy(d.begin(), d.end(), data.begin());
			try {
				BitTorrentMessage msg = BitTorrentMessage::deserialize(data);
				std::cout << "Peer ID: ";
				for (int i = 0; i < msg.peerId.size(); i++)
				{
					printf("%02x", msg.peerId[i]);
				}
				std::cout << "\n";
				//std::cout << "Message deserialized successfully!\n";
				// Proceed to use `msg` as needed...
			}
			catch (const std::exception& e) {
				std::cerr << "Error deserializing message: " << e.what() << std::endl;
			}
			});

		loop->run();
	}
	else if (command == "download_piece") {
		int n = 0;
		std::string file_name = argv[4];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);

		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto peers_response = torrent::discover_peers(metadata);
		json decoded_value;
		//auto decoded_value = decode_bencoded_value(str, n);
		//spdlog::debug("Info Hash: {0}", hash);
		//spdlog::debug("Piece Length:  {0}", info_struct.piece_length);
		//spdlog::debug("Piece Hashes: ");

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

		// ================================= File Writing ==========================


		// ================================= File Writing ==========================

		BTConnection bittorent_session(loop, tcpClient, decoded_value, metadata);
		// Handle the data event
		tcpClient->on<uvw::data_event>([&bittorent_session](const uvw::data_event& event, uvw::tcp_handle& tcp_handle) {
			//std::cout << "Data received: " << std::string(event.data.get(), event.length) << " size: " << event.length << std::endl;
			std::vector<uint8_t> data(event.data.get(), event.data.get() + event.length);
			bittorent_session.onDataReceived(data);
			});

		bittorent_session.piece_index_to_download = atoi(argv[5]);
		bittorent_session.request_download_name = argv[3];
		// Create a timer handle
		auto timer = loop->resource<uvw::timer_handle>();


		loop->run();
	}
	else if (command == "download") {
		int n = 0;
		std::string file_name = argv[4];
		std::ifstream torrent_file(file_name);
		std::string str((std::istreambuf_iterator<char>(torrent_file)), std::istreambuf_iterator<char>());
		auto dec = utils::bencode::parse(str);

		auto hash = torrent::info_hash(dec);
		auto metadata = torrent::initialize(dec);
		auto peers_response = torrent::discover_peers(metadata);
		json decoded_value;
		//auto decoded_value = decode_bencoded_value(str, n);
		//spdlog::debug("Info Hash: {0}", hash);
		//spdlog::debug("Piece Length:  {0}", info_struct.piece_length);
		//spdlog::debug("Piece Hashes: ");

		auto loop = uvw::loop::get_default();

		auto tcpClient = loop->resource<uvw::tcp_handle>();
		//hash = "07ce596e5ab4a13053efdb039b3b038c26da3eac";
		//torrent::TrackerResponse peers_response;
		//peers_response.peers.push_back({ "127.0.0.1", 25428 ,"" });
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

		// ================================= File Writing ==========================


		// ================================= File Writing ==========================

		BTConnection bittorent_session(loop, tcpClient, decoded_value, metadata);
		// Handle the data event
		tcpClient->on<uvw::data_event>([&bittorent_session](const uvw::data_event& event, uvw::tcp_handle& tcp_handle) {
			//std::cout << "Data received: " << std::string(event.data.get(), event.length) << " size: " << event.length << std::endl;
			std::vector<uint8_t> data(event.data.get(), event.data.get() + event.length);
			bittorent_session.onDataReceived(data);
			});

		bittorent_session.piece_index_to_download = atoi("0");
		bittorent_session.request_download_name = argv[3];
		bittorent_session.downloadFullFile = true;
		// Create a timer handle
		auto timer = loop->resource<uvw::timer_handle>();


		loop->run();
	}
	else {
		std::cerr << "unknown command: " << command << std::endl;
		return 1;
	}

	return 0;
}
