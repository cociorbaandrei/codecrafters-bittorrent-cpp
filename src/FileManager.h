#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <set>
#include "utils.h"
#include <indicators/cursor_control.hpp>
#include <indicators/progress_bar.hpp>
#include <iomanip>
#include <chrono>
struct Block {
	std::vector<uint8_t> data;
	bool received = false;
	size_t expectedSize = 0;
	bool requested = false; // Add this to track request status
};

struct Piece {
	std::vector<Block> blocks;
	uint32_t currentBlock;
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime, endTime;
    uint32_t pieceLength;
	bool isComplete() const {
		return std::all_of(std::begin(blocks), std::end(blocks), [](const Block& block) {return block.received; });
	}
    double downloadSpeedMBps(){
        std::chrono::duration<double> timeTaken = endTime - startTime;
        // Calculate download speed in bytes per second
        double speedBps = blocks.size() *  16 * 1024 / timeTaken.count();

        // Convert speed to megabytes per second (MBps)
        double speedMBps = speedBps / (1024 * 1024);
        return speedMBps;
    }
};

namespace uvw{
    class tcp_handle;
    class loop;
    class timer_handle;
    class file_req;
}
class File;

class FileManager{
public:
    FileManager(const torrent::MetaData& metadata, std::shared_ptr<uvw::tcp_handle> tcp_handle, std::shared_ptr<uvw::loop> loop);
    void initializePieces(const std::vector<uint8_t>& bitfield);
    void writePieceToFile(size_t pieceIndex, const Piece& piece);
    void onBlockReceived(size_t pieceIndex, size_t begin, size_t blockIndex, const std::vector<uint8_t>& data);
    void onHave(size_t pieceIndex);
    void update();
    void startDownload();
private:
    std::string m_filename;
    std::vector<Piece> m_pieces;
    std::unordered_map<size_t, bool> m_available_pieces;
    torrent::MetaData m_metadata;
    std::shared_ptr<uvw::tcp_handle> m_tcp_handle;
    std::shared_ptr<uvw::loop> m_loop;
    std::shared_ptr<uvw::timer_handle> m_update_timer;
    std::set<int> m_pieces_to_download;
    std::shared_ptr<indicators::ProgressBar> m_progress_bar;
    std::shared_ptr<uvw::file_req> m_file;
};