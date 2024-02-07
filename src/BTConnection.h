#pragma once
#include <vector>
#include <string>
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#else
	#include <arpa/inet.h>
#endif // WIN32
#include <memory>
#include "lib/nlohmann/json.hpp"
using json = nlohmann::json;
void prettyPrintHex(const std::vector<uint8_t>& data, size_t bytesPerLine = 16);
enum class BTMessageType : uint8_t {
	Choke = 0,
	Unchoke = 1,
	Interested = 2,
	NotInterested = 3,
	Have = 4,
	Bitfield = 5,
	Request = 6,
	Piece = 7,
	Cancel = 8,
	// Add other message types as needed
};

struct BTMessage {
	BTMessageType type;
	std::vector<uint8_t> payload;
};
namespace uvw {
	class tcp_handle;
}

struct Block {
	std::vector<uint8_t> data;
	bool received = false;
};

struct Piece {
	std::vector<Block> blocks;
	uint32_t currentBlock;
	bool isComplete() const {
		return std::all_of(std::begin(blocks), std::end(blocks), [](const Block& block) {return block.received; });
	}
};

class BTConnection {
public:
	bool handshakeReceived = false;
	std::vector<uint8_t> buffer;
	BTConnection(std::shared_ptr<uvw::tcp_handle> tcp_handle, json decoded_json);
	void onDataReceived(const std::vector<uint8_t>& data);
	void requestDownload(size_t piece_index);

	std::string request_download_name =  ""; 
	int32_t piece_index_to_download = 0;
private:
	void handleHandshake();
	void handleOtherMessages();
	bool canParseMessage();
	BTMessage parseMessage();
	void dispatchMessage(const BTMessage& message);
	std::shared_ptr<uvw::tcp_handle> m_tcp_handle;
	json m_decoded_json;

	std::vector<Piece> pieces;

	void initializePieces(json metadata);
	void onBlockReceived(size_t pieceIndex, size_t blockIndex, const std::vector<uint8_t>& data);
	void writePieceToFile(size_t pieceIndex, const Piece& piece);
	bool isDownloadComplete() {
		return std::all_of(pieces.begin(), pieces.end(), [](const Piece& piece) { return piece.isComplete(); });
	}


};