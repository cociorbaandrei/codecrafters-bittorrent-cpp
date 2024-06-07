#pragma once
#include "Boost.h"

#include <string>

class IFileReader {
public:
    virtual ~IFileReader() = default;
    virtual net::awaitable<std::string> readFile(const std::string& filename) = 0;
    virtual net::awaitable<void> readFileInChunks(
        const std::string& filename,
        std::size_t chunk_size,
        std::size_t offset,
        std::size_t num_bytes = 0)
        = 0;
};