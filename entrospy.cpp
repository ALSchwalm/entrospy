#include <iostream>
#include <ios>
#include <cmath>
#include <cstdio>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using counter_t = std::array<uint64_t, 256>;
constexpr auto DEFAULT_BLOCK_SIZE = 16 * 1024;

double shannon_score(const counter_t& counts, std::size_t total_size,
                     uint8_t low, uint8_t high) {
    // TODO: Support high/low
    double score = 0;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        auto count = counts[index];
        if (count == 0)
            continue;
        double p_i = count / static_cast<double>(total_size);
        score += p_i * log2(p_i);
    }
    return std::abs(score);
}

template <typename Iter>
void shannon_digest(Iter begin, Iter end, counter_t& counts) {
    for (auto iter = begin; iter != end; ++iter) {
        counts[*iter] += 1;
    }
}

void shannon_entire_file(const std::string& path,
                         std::pair<double, double> bounds) {
    using block_t = std::array<uint8_t, DEFAULT_BLOCK_SIZE>;
    auto block = std::unique_ptr<block_t>(new block_t{});
    counter_t count{};
    auto file = std::fopen(path.c_str(), "rb");

    while (!std::feof(file)) {
        std::size_t bytes_read =
            std::fread(block->begin(), 1, block->size(), file);
        shannon_digest(block->begin(), block->begin() + bytes_read, count);
    }

    auto score = shannon_score(count, std::ftell(file), 0, 255);
    if (score >= bounds.first && score <= bounds.second) {
        std::cout << path << ": " << score << std::endl;
    }
}

void shannon_file_blocks(const std::string& path, uint64_t block_size,
                         std::pair<double, double> bounds) {
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
            auto score = shannon_score(counter, block_size, 0, 255);
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
            auto score = shannon_score(counter, bytes_remaining, 0, 255);
            if (score >= bounds.first && score <= bounds.second) {
                std::cout << path << ": " << std::hex
                          << start_file_offset + bytes_read - bytes_remaining
                          << ": " << score << std::endl;
            }
        }
    }
}

void shannon_file(const std::string& path, uint64_t block_size,
                  bool print_blocks, std::pair<double, double> bounds) {
    if (!print_blocks) {
        shannon_entire_file(path, bounds);
    } else {
        shannon_file_blocks(path, block_size, bounds);
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

int main(int argc, char** argv) {
    po::options_description desc("Usage: entrospy [OPTION] PATH...", 100);

    desc.add_options()                    //
        ("help,h", "Print help messages") //
        ("paths", po::value<std::vector<std::string>>(),
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
        ("lower,l",
         "Do not show files with entropy lower than 'lower'") //
        ("upper,u",
         "Do not show files with entropy higher than 'upper'") //
        ("format,f", po::value<std::string>()->default_value("data"),
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
            auto default_lower =
                po::variable_value(std::numeric_limits<double>::lowest(), true);
            vm.insert(std::make_pair("lower", default_lower));
        }

        if (!vm.count("upper")) {
            auto default_upper =
                po::variable_value(std::numeric_limits<double>::max(), true);
            vm.insert(std::make_pair("upper", default_upper));
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
    auto bounds =
        std::make_pair(vm["lower"].as<double>(), vm["upper"].as<double>());
    auto paths = vm["paths"].as<std::vector<std::string>>();

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
                                     print_blocks, bounds);
                    }
                }
            }
        } else {
            shannon_file(path, block_size, print_blocks, bounds);
        }
    }
}
