#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/preprocessor/repetition/enum.hpp>

#include "shannon.hpp"
#include "output.hpp"

#define SHANNON_IDENT(z, n, value) value

template <uint8_t Base>
constexpr double log(double value) {
    return log2(value) / log2(Base);
}

namespace fs = boost::filesystem;

using counter_t = std::array<uint64_t, 256>;
using allowed_t = std::array<bool, 256>;
constexpr auto DEFAULT_BLOCK_SIZE = 16 * 1024;

constexpr allowed_t ALLOWED_DATA{BOOST_PP_ENUM(256, SHANNON_IDENT, true)};
constexpr allowed_t ALLOWED_TEXT{BOOST_PP_ENUM(128, SHANNON_IDENT, true)};
constexpr allowed_t ALLOWED_BASE64{
    BOOST_PP_ENUM(43, SHANNON_IDENT, false),
    true, // '+'
    BOOST_PP_ENUM(3, SHANNON_IDENT, false),
    BOOST_PP_ENUM(11, SHANNON_IDENT, true), // '/' and 0-9
    BOOST_PP_ENUM(7, SHANNON_IDENT, false),
    BOOST_PP_ENUM(26, SHANNON_IDENT, true), // A-Z
    BOOST_PP_ENUM(6, SHANNON_IDENT, false),
    BOOST_PP_ENUM(26, SHANNON_IDENT, true), // a-z
};

double shannon_score(const counter_t& counts, std::size_t total_size,
                     Format format) {
    const allowed_t* allowed;
    switch (format) {
    case Format::DATA:
        allowed = &ALLOWED_DATA;
        break;
    case Format::TEXT:
        allowed = &ALLOWED_TEXT;
        break;
    case Format::BASE64:
        allowed = &ALLOWED_BASE64;
        break;
    }

    double score = 0;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        auto count = counts[index];
        if (count == 0 || !(*allowed)[index])
            continue;
        double p_i = count / static_cast<double>(total_size);
        score += p_i * log2(p_i);
    }

    switch (format) {
    case Format::DATA:
        return std::abs(score) / 8.0;
    case Format::TEXT:
        return std::abs(score) / 7.0;
    case Format::BASE64:
        return std::abs(score) / 6.0;
    default:
        throw std::runtime_error("Unknown format");
    }
}

template <typename Iter>
void shannon_digest(Iter begin, Iter end, counter_t& counts) {
    for (auto iter = begin; iter != end; ++iter) {
        counts[*iter] += 1;
    }
}

void shannon_entire_file(const std::string& path,
                         std::pair<double, double> bounds, Format format) {
    using block_t = std::array<uint8_t, DEFAULT_BLOCK_SIZE>;
    auto block = std::unique_ptr<block_t>(new block_t{});
    counter_t count{};
    auto file = std::fopen(path.c_str(), "rb");

    while (!std::feof(file)) {
        std::size_t bytes_read =
            std::fread(block->begin(), 1, block->size(), file);
        shannon_digest(block->begin(), block->begin() + bytes_read, count);
    }

    auto score = shannon_score(count, std::ftell(file), format);
    if (score >= bounds.first && score <= bounds.second) {
        std::cout << path << ": score: " << score << std::endl;
    }
}

void shannon_file_blocks(const std::string& path, uint64_t block_size,
                         bool print_block, std::pair<double, double> bounds,
                         Format format) {
    uint64_t read_size;
    if (block_size < DEFAULT_BLOCK_SIZE) {
        read_size =
            std::ceil(DEFAULT_BLOCK_SIZE / static_cast<double>(block_size)) *
            block_size;
    } else {
        read_size = block_size;
    }

    std::vector<uint8_t> block(read_size);
    auto file = std::fopen(path.c_str(), "rb");
    auto filesize = fs::file_size(path);
    auto address_width = std::ceil(log<16>(filesize));

    while (!std::feof(file)) {
        auto start_file_offset = std::ftell(file);
        std::size_t bytes_read = std::fread(&block.front(), 1, read_size, file);
        for (std::size_t i = 0; i < bytes_read / block_size; ++i) {
            counter_t counter{};
            auto block_begin = block.begin() + block_size * i;
            auto block_end = block.begin() + block_size * (i + 1);
            shannon_digest(block_begin, block_end, counter);
            auto score = shannon_score(counter, block_size, format);

            if (score >= bounds.first && score <= bounds.second) {
                print_block_score(path, start_file_offset + i * block_size,
                                  address_width, score);
                if (print_block) {
                    print_block_bytes(block_begin, block_end,
                                      start_file_offset + i * block_size,
                                      address_width);
                }
            }
        }

        // At the end of the file, handle the part of the final block
        // TODO: skip this? It may have inflated entropy
        if (bytes_read < read_size && bytes_read % block_size != 0) {
            auto bytes_remaining = bytes_read % block_size;
            counter_t counter{};
            shannon_digest(block.begin() + bytes_read - bytes_remaining,
                           block.begin() + bytes_read, counter);
            auto score = shannon_score(counter, bytes_remaining, format);
            if (score >= bounds.first && score <= bounds.second) {
                print_block_score(path, start_file_offset + bytes_read -
                                            bytes_remaining,
                                  address_width, score);
            }
        }
    }
}

void shannon_file(const std::string& path, uint64_t block_size,
                  bool print_blocks, std::pair<double, double> bounds,
                  Format format) {
    if (!block_size) {
        shannon_entire_file(path, bounds, format);
    } else {
        shannon_file_blocks(path, block_size, print_blocks, bounds, format);
    }
}
