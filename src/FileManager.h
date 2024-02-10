// #pragma once
// #include <string>
// #include <vector>
// #include <algorithm>

// struct Block {
// 	std::vector<uint8_t> data;
// 	bool received = false;
// 	size_t expectedSize = 0;
// 	bool requested = false; // Add this to track request status
// };

// struct Piece {
// 	std::vector<Block> blocks;
// 	uint32_t currentBlock;
// 	bool isComplete() const {
// 		return std::all_of(std::begin(blocks), std::end(blocks), [](const Block& block) {return block.received; });
// 	}
// };
// class File;
// class FileManager{
// public:
//     FileManager() = default;
// private:

// }