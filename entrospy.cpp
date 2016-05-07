#include <iostream>
#include <ios>
#include <cmath>
#include <cstdio>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/algorithm/string.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using counter_t = std::array<uint64_t, 256>;
constexpr auto DEFAULT_BLOCK_SIZE = 16 * 1024;

// boost does not support enum classes with program_options, so use enum
enum Format {
    DATA = 1 << 8,  // All bytes are allowed, max entropy = 8
    TEXT = 1 << 7,  // Only ASCII allowed, max entropy = 7
    BASE64 = 1 << 6 // Only base64 characters allowed, max entropy = 6
};

double shannon_score(const counter_t& counts, std::size_t total_size,
                     Format format) {
    std::array<bool, 256> allowed{};
    switch (format) {
    case Format::DATA:
        for (auto& value : allowed) {
            value = true;
        }
        break;
    case Format::TEXT:
        for (uint8_t i = 0; i < 128; ++i) {
            allowed[i] = true;
        }
        break;
    case Format::BASE64:
        for (uint8_t i = 'a'; i <= 'z'; ++i) {
            allowed[i] = true;
        }
        for (uint8_t i = 'A'; i <= 'Z'; ++i) {
            allowed[i] = true;
        }
        for (uint8_t i = '0'; i <= '9'; ++i) {
            allowed[i] = true;
        }
        allowed['/'] = true;
        allowed['+'] = true;
        break;
    }

    double score = 0;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        auto count = counts[index];
        if (count == 0 || !allowed[index])
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
        std::cout << path << ": " << score << std::endl;
    }
}

void shannon_file_blocks(const std::string& path, uint64_t block_size,
                         std::pair<double, double> bounds, Format format) {
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

    while (!std::feof(file)) {
        auto start_file_offset = std::ftell(file);
        std::size_t bytes_read = std::fread(&block.front(), 1, read_size, file);
        for (std::size_t i = 0; i < bytes_read / block_size; ++i) {
            counter_t counter{};
            shannon_digest(block.begin() + block_size * i,
                           block.begin() + block_size * (i + 1), counter);
            auto score = shannon_score(counter, block_size, format);
            if (score >= bounds.first && score <= bounds.second) {
                std::cout << path << ": " << std::hex
                          << start_file_offset + i * block_size << ": " << score
                          << std::endl;
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
                std::cout << path << ": " << std::hex
                          << start_file_offset + bytes_read - bytes_remaining
                          << ": " << score << std::endl;
            }
        }
    }
}

void shannon_file(const std::string& path, uint64_t block_size,
                  bool print_blocks, std::pair<double, double> bounds,
                  Format format) {
    if (!print_blocks) {
        shannon_entire_file(path, bounds, format);
    } else {
        shannon_file_blocks(path, block_size, bounds, format);
    }
}

bool is_hidden(const fs::path& path) {
    const auto& name = path.filename().string();
    if (name != ".." && name != "." && name[0] == '.') {
        return true;
    }
    return false;
}

uint64_t parse_block_size(const std::string& bs) {
    auto num = std::stol(bs);
    auto suffix = std::tolower(bs.back());
    if (!std::isdigit(suffix)) {
        switch (suffix) {
        case 'g':
            num *= 1024 * 1024 * 2014;
            break;
        case 'm':
            num *= 1024 * 1024;
            break;
        case 'k':
            num *= 1024;
            break;
        case 'b':
            break;
        default:
            std::cerr << "Unknown suffix: " << bs.back() << " assuming bytes"
                      << std::endl;
        }
    }
    return num;
}

std::istream& operator>>(std::istream& in, Format& format) {
    std::string token;
    in >> token;
    boost::algorithm::to_lower(token);
    if (token == "data") {
        format = Format::DATA;
    } else if (token == "text") {
        format = Format::TEXT;
    } else if (token == "base64") {
        format = Format::BASE64;
    } else {
        throw po::validation_error(po::validation_error::invalid_option,
                                   "Unknown format");
    }
    return in;
}

int main(int argc, char** argv) {
    po::options_description desc("Usage: entrospy [OPTION] PATH...", 100);

    Format format;
    std::vector<std::string> paths;
    std::pair<double, double> bounds;

    desc.add_options()                    //
        ("help,h", "Print help messages") //
        ("paths", po::value<std::vector<std::string>>(&paths),
         "Paths to search")                  //
        ("debug", "Enable debugging output") //
        ("all,a",
         "Do not ignore hidden files and directories") //
        ("block,b", po::value<std::string>(),
         "Process each file 'block' at a time. This will show the"
         " entropy for each block rather than the total file. The provided"
         " argument will be interpreted as a byte count unless suffixed"
         " with 'K', 'M', or 'G' for kilo, mega, and giga-bytes "
         "respectively") //
        ("lower,l", po::value<double>(&bounds.first),
         "Do not show files with entropy lower than 'lower'") //
        ("upper,u", po::value<double>(&bounds.second),
         "Do not show files with entropy higher than 'upper'") //
        ("format,f", po::value<Format>(&format)->default_value(Format::DATA),
         "Input format: 'data','text' or 'base64'")             //
        ("recursive,r", "Run directories in PATH recursively"); //

    po::positional_options_description p;
    p.add("paths", -1);

    po::variables_map vm;

    try {
        po::store(po::command_line_parser(argc, argv)
                      .options(desc)
                      .positional(p)
                      .run(),
                  vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        // Do not use program_options default arguments because they
        // take up too much space in the help menu (and aren't really
        // meaningful defaults)
        if (!vm.count("lower")) {
            bounds.first = std::numeric_limits<double>::lowest();
        }

        if (!vm.count("upper")) {
            bounds.second = std::numeric_limits<double>::max();
        }

        po::notify(vm);
    } catch (po::error& e) {
        std::cerr << "entrospy: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    bool print_blocks = false;
    uint64_t block_size = 0;
    if (vm.count("block")) {
        print_blocks = true;
        block_size = parse_block_size(vm["block"].as<std::string>());
    }

    for (const auto& path : paths) {
        if (fs::is_directory(path)) {
            if (!vm.count("recursive")) {
                std::cerr << "entrospy: " << path << ": Is a directory"
                          << std::endl;
                continue;
            } else {
                for (fs::recursive_directory_iterator iter(path), end;
                     iter != end; ++iter) {
                    if (fs::is_directory(iter->path())) {
                        if (is_hidden(iter->path()) && !vm.count("all")) {
                            iter.no_push();
                        }
                        continue;
                    }
                    if (!is_hidden(iter->path()) || vm.count("all")) {
                        shannon_file(iter->path().string(), block_size,
                                     print_blocks, bounds, format);
                    }
                }
            }
        } else {
            shannon_file(path, block_size, print_blocks, bounds, format);
        }
    }
}
