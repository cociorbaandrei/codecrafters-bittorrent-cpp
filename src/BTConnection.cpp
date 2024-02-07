#include "BTConnection.h"
#include "BitTorrentMessage.h"
#include "lib/nlohmann/json.hpp"
#include <iostream>

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

void prettyPrintHex(const std::vector<uint8_t>& data, size_t bytesPerLine) {
	std::stringstream hexStream;
	size_t byteCount = 0;
	std::cout << "======================Hexdump===========================\n";
	for (auto byte : data) {
		// Print byte in hex format
		hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
		++byteCount;

		// Insert a newline after every `bytesPerLine` bytes to maintain the requested formatting
		if (byteCount % bytesPerLine == 0) {
			hexStream << std::endl;
		}
	}

	// Handle case where the last line is not full
	if (byteCount % bytesPerLine != 0) {
		hexStream << std::endl;
	}

	std::cout << hexStream.str();
	std::cout << "======================Hexdump===========================\n";
}

BTConnection::BTConnection(std::shared_ptr<uvw::tcp_handle> tcp_handle, json decoded_json)
	: m_tcp_handle(tcp_handle)
	, m_decoded_json(decoded_json)
{
}

void BTConnection::onDataReceived(const std::vector<uint8_t>& data)
{
	buffer.insert(buffer.end(), data.begin(), data.end());

	if (!handshakeReceived) {
		// Assuming we've received enough bytes for the handshake (68 bytes)
		if (buffer.size() >= 68) {
			handleHandshake();
		}
	}
	else {
		handleOtherMessages();
	}
}

void BTConnection::handleHandshake()
{
	if (buffer[0] == 19 && std::string(buffer.begin() + 1, buffer.begin() + 20) == "BitTorrent protocol") {
		// Successfully received a valid handshake
		try {
			std::vector<uint8_t> payload(buffer.begin() , buffer.begin() + 68);
			BitTorrentMessage msg = BitTorrentMessage::deserialize(payload);
			std::cout << "Peer ID: ";
			for (int i = 0; i < msg.peerId.size(); i++)
			{
				printf("%02x", msg.peerId[i]);
			}
			std::cout << "\n";
			handshakeReceived = true;
		}
		catch (const std::exception& e) {
			std::cerr << "Error deserializing message: " << e.what() << std::endl;
		}
		// Remove the handshake from the buffer
		buffer.erase(buffer.begin(), buffer.begin() + 68);

		// Check if there's more data in the buffer to process
		handleOtherMessages();
	}
	else {
		// Invalid handshake; consider dropping the connection
	}
}

void BTConnection::handleOtherMessages()
{
	// Try to parse messages from the buffer
	while (canParseMessage()) {
		auto message = parseMessage();

		dispatchMessage(message);
	}
}

bool BTConnection::canParseMessage()
{
	if (buffer.size() < 4) return false; // Not enough data for length prefix

	uint32_t length = ntohl(*reinterpret_cast<uint32_t*>(buffer.data())); // Assuming network byte order
	if (length == 0) {
		// It's a keep-alive message
		std::cout << "Keep-alive message received." << std::endl;
		return false;
	}
	return buffer.size() >= length + 4; // Check if we have the whole message
}

BTMessage BTConnection::parseMessage()
{
	uint32_t length = ntohl(*reinterpret_cast<uint32_t*>(buffer.data()));
	BTMessageType type = static_cast<BTMessageType>(buffer[4]);
	std::vector<uint8_t> payload(buffer.begin() + 5, buffer.begin() + 4 + length);

	// Remove parsed message from buffer
	buffer.erase(buffer.begin(), buffer.begin() + 4 + length);

	return { type, payload }; // Assuming BTMessage is a struct holding these two
}

void BTConnection::dispatchMessage(const BTMessage& message) {
	//prettyPrintHex(message.payload);
	switch (message.type) {
	case BTMessageType::Choke:
		//handleChoke(message.payload);
		break;
	case BTMessageType::Piece:
	{
		std::cout << "Received BTMessageType::Piece\n";
		// Piece index (network byte order)

		auto pieceIndex = ntohl (*reinterpret_cast<const uint32_t*>(message.payload.data()));
		auto begin = ntohl(*reinterpret_cast<const uint32_t*>(message.payload.data() + 4));
		std::vector<uint8_t> piece_data(message.payload.begin() + 8, message.payload.end());
		
		std::int32_t piece_length;
		m_decoded_json["info"].at("piece length").get_to(piece_length);
		const int blockSize = 16 * 1024; // 16 KiB
		const int pieceLength = piece_length;
		const int fullBlocks = pieceLength / blockSize; // Number of full blocks
		
		std::cout << "Piece Index: " << pieceIndex << " Begin: " << begin << "Size: " << piece_data.size() << "\n";
		onBlockReceived(pieceIndex, begin / blockSize, piece_data);
		break;
	}
	case BTMessageType::Unchoke:
	{
		std::cout << "Received BTMessageType::Unchoke\n";

		std::string pieces;

		if (request_download_name != "")
			requestDownload(piece_index_to_download);

		break;
	}
	case BTMessageType::Bitfield:
	{
		std::cout << "Received BTMessageType::Bitfield\n";
		char data[] = "\x00\x00\x00\x01\x02";
		m_tcp_handle->write(data, 5);
		break;
	}
	default:
		std::cerr << "Unknown message type received: " << static_cast<int>(message.type) << std::endl;
		break;
	}
}

void BTConnection::requestDownload(size_t piece_index)
{

	initializePieces(m_decoded_json);
	std::string pieces;
	std::int32_t piece_length;
	m_decoded_json["info"].at("pieces").get_to(pieces);
	m_decoded_json["info"].at("piece length").get_to(piece_length);
	uint32_t no_pieces = pieces.length() / 20;

	const int blockSize = 16 * 1024; // 16 KiB
	const int pieceIndex = piece_index;
	const int pieceLength = piece_length;
	const int fullBlocks = pieceLength / blockSize; // Number of full blocks
	const int lastBlockSize = pieceLength % blockSize; // Size of the last block, if any
	const int totalBlocks = fullBlocks + (lastBlockSize > 0 ? 1 : 0); // Total blocks including the last partial block, if any
	for (int block = 0; block < totalBlocks; ++block) {
		int begin = block * blockSize;
		int length = (block < fullBlocks) ? blockSize : lastBlockSize;

		// Construct the message
		std::array<unsigned char, 17> requestMessage; // 4 bytes for length prefix, 1 for message ID, 12 for payload
		// Length prefix (13, since payload is 12 bytes long)
		requestMessage[0] = 0;
		requestMessage[1] = 0;
		requestMessage[2] = 0;
		requestMessage[3] = 13;
		// Message ID (6)
		requestMessage[4] = 6;
		// Piece index (network byte order)
		*reinterpret_cast<uint32_t*>(requestMessage.data() + 5) = htonl(pieceIndex);
		// Begin (network byte order)
		*reinterpret_cast<uint32_t*>(requestMessage.data() + 9) = htonl(begin);
		// Length (network byte order)
		*reinterpret_cast<uint32_t*>(requestMessage.data() + 13) = htonl(length);

		// Send the message
		m_tcp_handle->write(reinterpret_cast<char*>(requestMessage.data()), requestMessage.size());
	}
}

void BTConnection::initializePieces(json metadata)
{
	std::int32_t piece_length;
	std::string pieces_hex;

	m_decoded_json["info"].at("piece length").get_to(piece_length);
	m_decoded_json["info"].at("pieces").get_to(pieces_hex);
	uint32_t no_pieces = pieces_hex.length() / 20;

	const int blockSize = 16 * 1024; // 16 KiB
	const int pieceLength = piece_length;
	const int fullBlocks = pieceLength / blockSize; // Number of full blocks
	const int lastBlockSize = pieceLength % blockSize; // Size of the last block, if any
	const int totalBlocks = fullBlocks + (lastBlockSize > 0 ? 1 : 0); // Total blocks including the last partial block, if any

	auto totalLength = metadata["info"]["length"].template get<std::uint32_t>();

	size_t totalPieces = totalLength / piece_length + (totalLength % piece_length > 0 ? 1 : 0);
	for (size_t i = 0; i < totalPieces; ++i) {
		Piece piece;
		size_t blocksInPiece = totalBlocks;
		for (size_t j = 0; j < blocksInPiece; ++j) {
			piece.blocks.push_back(Block{});
		}
		pieces.push_back(std::move(piece));
	}
}

void BTConnection::onBlockReceived(size_t pieceIndex, size_t blockIndex, const std::vector<uint8_t>& data)
{
	Piece& piece = pieces[pieceIndex];
	Block& block = piece.blocks[blockIndex];
	block.data = data;
	block.received = true;

	// Check if the piece is complete
	if (piece.isComplete()) {
		writePieceToFile(pieceIndex, piece);
		//requestDownload(pieceIndex + 1);
	}
}

void BTConnection::writePieceToFile(size_t pieceIndex, const Piece& piece)
{
	std::int32_t piece_length;

	m_decoded_json["info"].at("piece length").get_to(piece_length);

	std::ofstream file(request_download_name, std::ios::binary | std::ios::app |  std::ios::out);
	if (!file.is_open()) {
		// Handle error
		return;
	}

	file.seekp(pieceIndex * piece_length);
	for (const auto& block : piece.blocks) {
		file.write(reinterpret_cast<const char*>(block.data.data()), block.data.size());
	}
}
