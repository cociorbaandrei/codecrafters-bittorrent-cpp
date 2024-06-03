#pragma once
#include "IFileReader.h"
#include "Boost.h"
#include <iostream>
#include <vector>
#include <string>
#include <fcntl.h>
#include <unistd.h>


namespace net = boost::asio;

class FileReader : public IFileReader {
public:
    FileReader() = default;

    net::awaitable<std::string> readFile(const std::string& filename) override {
        auto executor = co_await net::this_coro::executor;
        auto& io_context = static_cast<net::io_context&>(executor.context());

        int file_descriptor = open(filename.c_str(), O_RDONLY);
        if (file_descriptor == -1) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            co_return "";
        }

        net::posix::stream_descriptor file(io_context, file_descriptor);

        std::vector<char> buffer(1024);
        std::string content;

        while (true) {
            boost::system::error_code ec;
            std::size_t n = co_await file.async_read_some(net::buffer(buffer), net::redirect_error(net::use_awaitable, ec));
            if (ec == net::error::eof) {
                break;
            } else if (ec) {
                std::cerr << "Error reading file: " << ec.message() << std::endl;
                co_return content;
            }
            content.append(buffer.data(), n);
        }
        co_return content;
    }

    net::awaitable<void> readFileInChunks(const std::string& filename, std::size_t chunk_size, std::size_t offset, std::size_t num_bytes = 0) override {
        auto executor = co_await net::this_coro::executor;
        auto& io_context = static_cast<net::io_context&>(executor.context());

        int file_descriptor = open(filename.c_str(), O_RDONLY);
        if (file_descriptor == -1) {
            std::cerr << "Failed to open file: " << filename << std::endl;
            co_return;
        }

        net::posix::stream_descriptor file(io_context, file_descriptor);

        if (lseek(file_descriptor, offset, SEEK_SET) == -1) {
            std::cerr << "Failed to seek to offset: " << offset << std::endl;
            co_return;
        }

        std::vector<char> buffer(chunk_size);
        std::size_t total_bytes_read = 0;

        while (true) {
            boost::system::error_code ec;
            std::size_t n = co_await file.async_read_some(net::buffer(buffer), net::redirect_error(net::use_awaitable, ec));
            if (ec == net::error::eof || (num_bytes > 0 && total_bytes_read + n >= num_bytes)) {
                std::cout << "Read " << (num_bytes > 0 && total_bytes_read + n >= num_bytes ? num_bytes - total_bytes_read : n) 
                          << " bytes: " << std::string(buffer.data(), (num_bytes > 0 && total_bytes_read + n >= num_bytes) ? num_bytes - total_bytes_read : n) << std::endl;
                break;
            } else if (ec) {
                std::cerr << "Error reading file: " << ec.message() << std::endl;
                co_return;
            }
            std::cout << "Read " << n << " bytes: " << std::string(buffer.data(), n) << std::endl;
            total_bytes_read += n;
        }
        co_return;
    }
};