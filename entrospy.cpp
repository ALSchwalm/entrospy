#include <iostream>
#include <cmath>
#include <cstdio>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using counter_t = std::array<uint64_t, 256>;
using block_t = std::array<uint8_t, 16 * 1024>;

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
    return -score;
}

void shannon_digest(const uint8_t* data, std::size_t size, counter_t& counts,
                    uint8_t low, uint8_t high) {
    // TODO: Support high/low
    for (std::size_t i = 0; i < size; ++i, ++data) {
        counts[*data] += 1;
    }
}

void shannon_file(const std::string& path) {
    uint64_t total_size = 0;
    auto block = std::unique_ptr<block_t>(new block_t{});
    counter_t count{};
    auto file = std::fopen(path.c_str(), "rb");

    while (!std::feof(file)) {
        std::size_t bytes_read =
            std::fread(block->data(), 1, block->size(), file);
        total_size += bytes_read;
        shannon_digest(block->data(), bytes_read, count, 0, 255);
    }

    std::cout << "entropy:" << path << ": "
              << shannon_score(count, total_size, 0, 255) << std::endl;
}

int main(int argc, char** argv) {
    po::options_description desc("Usage: entrospy [OPTION] PATH...");

    desc.add_options()                                                      //
        ("help,h", "Print help messages")                                   //
        ("debug", "Enable debugging output")                                //
        ("paths", po::value<std::vector<std::string>>(), "Paths to search") //
        ("format,f",
         "Input format: 'data','text' or 'base64'. Defaults to 'data'") //
        ("recursive,r", "Run directories in PATH recursively");         //

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

        po::notify(vm);
    } catch (po::error& e) {
        std::cerr << "entrospy: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
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
                        continue;
                    }
                    shannon_file(iter->path().string());
                }
            }
        } else {
            shannon_file(path);
        }
    }
}
