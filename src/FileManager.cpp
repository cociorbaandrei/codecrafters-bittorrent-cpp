#include "FileManager.h"
#include "spdlog/spdlog.h"
#include <uvw.hpp>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"

FileManager::FileManager(const torrent::MetaData& metadata, std::shared_ptr<uvw::tcp_handle> tcp_handle, std::shared_ptr<uvw::loop> loop)
    : m_filename(metadata.info.name)
    , m_metadata(metadata)
    , m_tcp_handle(tcp_handle)
    , m_loop(loop)
{
    m_update_timer = loop->resource<uvw::timer_handle>();

    m_update_timer->on<uvw::timer_event>([this](const uvw::timer_event&, uvw::timer_handle& handle) {
        update();
    });
    using namespace indicators;
    m_progress_bar = std::make_shared<indicators::ProgressBar>(
        option::BarWidth{50},
        option::Start{"["},
        option::Fill{"="},
        option::Lead{">"},
        option::Remainder{" "},
        option::End{"]"},
        option::PostfixText{"Downloading: " + m_filename},
        option::ForegroundColor{Color::yellow},
        option::ShowElapsedTime{true},
        option::ShowRemainingTime{true},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
    );
}

void FileManager::initializePieces(const std::vector<uint8_t>& bitfield)
{
    auto bit_string = utils::hex::parseBitfield(utils::hex::bytesToHexString(bitfield));
    for(int bit = 0; bit < bit_string.length(); bit++){
        m_available_pieces[bit] = bit_string[bit] == '1' ? true : false;
        if(bit_string[bit] == '1')
            m_pieces_to_download.emplace(bit);
    }

    std::string pieces_hex = m_metadata.info.pieces;
	std::int32_t piece_length = m_metadata.info.piece_length;

	uint32_t no_pieces = pieces_hex.length() / 20;

	const int blockSize = 16 * 1024; // 16 KiB
	const int pieceLength = piece_length;
	const int fullBlocks = pieceLength / blockSize; // Number of full blocks


	//auto totalLength = metadata["info"]["length"].template get<std::uint32_t>();
	auto totalLength = m_metadata.info.length;
	const int lastBlockSize = pieceLength % blockSize; // Size of the last block, if any
	const int totalBlocks = fullBlocks + (lastBlockSize > 0 ? 1 : 0); // Total blocks including the last partial block, if any
	size_t totalPieces = totalLength / piece_length + (totalLength % piece_length > 0 ? 1 : 0);

	size_t lastPieceSize = totalLength % pieceLength;
	if (lastPieceSize == 0) { // If the total size is a perfect multiple of the piece size
		lastPieceSize = pieceLength; // The last piece is a full piece
	}
	size_t numberOfBlocksInLastPiece = lastPieceSize / blockSize;
	if (lastPieceSize % blockSize != 0) { // If there's a remainder
		numberOfBlocksInLastPiece += 1; // There's an additional, partially-filled block
	}
	size_t sizeOfLastBlockInLastPiece = lastPieceSize % blockSize;
	if (sizeOfLastBlockInLastPiece == 0 && lastPieceSize != 0) {
		sizeOfLastBlockInLastPiece = blockSize; // The last block is a full block if no remainder
	}

	spdlog::debug("Number of blocks: {:0}", totalBlocks);
	spdlog::debug("Number of blocks in last piece: {:0}", numberOfBlocksInLastPiece);
	spdlog::debug("Last piece size: {:0}", lastPieceSize);
	spdlog::debug("Size of the last block in the last piece: {:0} bytes", sizeOfLastBlockInLastPiece);

	for (size_t i = 0; i < totalPieces; ++i) {
		Piece piece;
        piece.pieceLength = m_metadata.info.piece_length;
		size_t blocksInPiece = i == totalPieces - 1 ? numberOfBlocksInLastPiece : totalBlocks;
		for (size_t j = 0; j < blocksInPiece; ++j) {
			piece.blocks.push_back(Block{
				.expectedSize = blocksInPiece - 1 == j ? sizeOfLastBlockInLastPiece : blockSize
			});
		}
		m_pieces.push_back(std::move(piece));
	}
}

void FileManager::onBlockReceived(size_t pieceIndex, size_t begin, size_t blockIndex, const std::vector<uint8_t>& data)
{
	Piece& piece = m_pieces[pieceIndex];
	if (blockIndex >= piece.blocks.size())
		return;
    
	Block& block = piece.blocks[blockIndex];
    if(block.received == true)
        return;
	block.data = data;
	block.received = true;
    
    if (piece.isComplete()) {
        m_pieces_to_download.erase(pieceIndex);
        m_pieces[pieceIndex].endTime = std::chrono::high_resolution_clock::now();;
		//char data[] = "\x00\x00\x00\x01\x02";
		//m_tcp_handle->write(data, 5);
        auto mapValue = [](double x, double a, double b, double c, double d) {
            return c + (x - a) * (d - c) / (b - a);
        };
        m_progress_bar->set_progress(mapValue(pieceIndex, 0, m_pieces.size(), 0 , 100));
      
        if(pieceIndex % 50 == 0){
		    spdlog::info("Piece {0} block {1} downloaded to {2} {3}", pieceIndex, blockIndex, m_metadata.info.name, m_pieces.size());
              m_progress_bar->set_option(indicators::option::PostfixText{std::to_string( m_pieces[pieceIndex].downloadSpeedMBps())});
        }
		//writePieceToFile(pieceIndex, piece);
		if(pieceIndex == m_pieces.size() - 1){
			m_tcp_handle->stop();
			m_tcp_handle->close();
            m_update_timer->close();
			return;
		}
		//m_tcp_handle->close();
		// if(downloadFullFile){
		// 	piece_index_to_download += 1;
		// 	requestDownload(piece_index_to_download, 0);
		// }else{
		// 	m_tcp_handle->close();
		// }
	}
}

void FileManager::onHave(size_t pieceIndex)
{
    m_available_pieces[pieceIndex] = true;
    if(m_pieces_to_download.find(pieceIndex) != m_pieces_to_download.end())
        m_pieces_to_download.emplace(pieceIndex);
}

void FileManager::update()
{
    if(m_pieces_to_download.empty())
        return;
    int piece_index = *m_pieces_to_download.begin();
    if(m_pieces[piece_index].isComplete())
        return;

	if (piece_index >= m_pieces.size())
		return;
    if(m_pieces[piece_index].isComplete())
        return;
    m_pieces[piece_index].startTime = std::chrono::high_resolution_clock::now();;
	std::string pieces = m_metadata.info.pieces;
	std::int32_t piece_length = m_metadata.info.piece_length;

	auto totalLength = m_metadata.info.length;

	const int blockSize = 16 * 1024; // 16 KiB
	const int pieceLength = piece_length;
	const int fullBlocks = pieceLength / blockSize; // Number of full blocks
	const int lastBlockSize = pieceLength % blockSize; // Size of the last block, if any
	const int totalBlocks = fullBlocks + (lastBlockSize > 0 ? 1 : 0); // Total blocks including the last partial block, if any
	size_t totalPieces = totalLength / piece_length + (totalLength % piece_length > 0 ? 1 : 0);

	size_t lastPieceSize = totalLength % pieceLength;
	if (lastPieceSize == 0) { // If the total size is a perfect multiple of the piece size
		lastPieceSize = pieceLength; // The last piece is a full piece
	}
	size_t numberOfBlocksInLastPiece = lastPieceSize / blockSize;
	if (lastPieceSize % blockSize != 0) { // If there's a remainder
		numberOfBlocksInLastPiece += 1; // There's an additional, partially-filled block
	}
	size_t sizeOfLastBlockInLastPiece = lastPieceSize % blockSize;
	if (sizeOfLastBlockInLastPiece == 0 && lastPieceSize != 0) {
		sizeOfLastBlockInLastPiece = blockSize; // The last block is a full block if no remainder
	}

//	spdlog::debug("Number of blocks: {:0}", totalBlocks);
//	spdlog::debug("Last piece size: {:0}", lastPieceSize);
//	spdlog::debug("Size of the last block in the last piece: {:0} bytes", sizeOfLastBlockInLastPiece);

	for (int block = 0; block < m_pieces[piece_index].blocks.size(); ++block) {

	//	int block = blockIndex;
		int begin = block * blockSize;
		int length = m_pieces.size() - 1 == piece_index && m_pieces[piece_index].blocks.size() - 1 == block ? sizeOfLastBlockInLastPiece : blockSize;
      //  spdlog::info("Requesting Piece {:0} block {:1}", piece_index, block);
		// Construct the message
		//std::array<unsigned char, 17> requestMessage; // 4 bytes for length prefix, 1 for message ID, 12 for payload
		std::unique_ptr< char[]> requestMessage{ new  char[17] {0} }; // Example data
		// Length prefix (13, since payload is 12 bytes long)
		requestMessage[0] = 0;
		requestMessage[1] = 0;
		requestMessage[2] = 0;
		requestMessage[3] = 13;
		// Message ID (6)
		requestMessage[4] = 6;
		// Piece index (network byte order)
		*reinterpret_cast<uint32_t*>(requestMessage.get() + 5) = htonl(piece_index);
		// Begin (network byte order)
		*reinterpret_cast<uint32_t*>(requestMessage.get() + 9) = htonl(begin);
		// Length (network byte order)
		*reinterpret_cast<uint32_t*>(requestMessage.get() + 13) = htonl(length);

		// Send the message
		m_tcp_handle->write(std::move(requestMessage), 17);
        m_pieces[piece_index].blocks[block].requested = true;
	}


}

void FileManager::startDownload()
{
    m_update_timer->start(uvw::timer_handle::time{1}, uvw::timer_handle::time{5});
}