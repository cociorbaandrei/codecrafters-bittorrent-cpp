#pragma once
#include <string>
#include <vector>
#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif // WIN32
#include "FileManager.h"
#include "lib/nlohmann/json.hpp"
#include "utils.h"

#include <memory>
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
class loop;
} // namespace uvw


class BTConnection {
public:
    bool handshakeReceived = false;
    std::vector<uint8_t> buffer;
    BTConnection(
        std::shared_ptr<uvw::loop> loop,
        std::shared_ptr<uvw::tcp_handle> tcp_handle,
        json decoded_json,
        torrent::MetaData metadata);
    void onDataReceived(const std::vector<uint8_t>& data);
    void requestDownload(size_t piece_index, size_t blockIndex);

    std::string request_download_name = "";
    int32_t piece_index_to_download = 0;
    bool downloadFullFile = false;

private:
    void handleHandshake();
    void handleOtherMessages();
    bool canParseMessage();
    BTMessage parseMessage();
    void dispatchMessage(const BTMessage& message);
    std::shared_ptr<uvw::tcp_handle> m_tcp_handle;
    std::shared_ptr<uvw::loop> m_loop;
    json m_decoded_json;
    std::vector<Piece> pieces;
    torrent::MetaData m_metadata;
    std::shared_ptr<FileManager> m_file_manager;
    void initializePieces(json metadata);
    void onBlockReceived(size_t pieceIndex, size_t blockIndex, const std::vector<uint8_t>& data);
    void writePieceToFile(size_t pieceIndex, const Piece& piece);
    bool isDownloadComplete()
    {
        return std::all_of(
            pieces.begin(), pieces.end(), [](const Piece& piece) { return piece.isComplete(); });
    }
};