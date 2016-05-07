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

template <typename Iter>
void shannon_digest(Iter begin, Iter end, counter_t& counts) {
    for (auto iter = begin; iter != end; ++iter) {
        counts[*iter] += 1;
    }
}

void shannon_file(const std::string& path, double lower_bound,
                  double upper_bound) {
    uint64_t total_size = 0;
    auto block = std::unique_ptr<block_t>(new block_t{});
    counter_t count{};
    auto file = std::fopen(path.c_str(), "rb");

    while (!std::feof(file)) {
        std::size_t bytes_read =
            std::fread(block->begin(), 1, block->size(), file);
        total_size += bytes_read;
        shannon_digest(block->begin(), block->begin() + bytes_read, count);
    }

    auto score = shannon_score(count, total_size, 0, 255);
    if (score >= lower_bound && score <= upper_bound) {
        std::cout << "entropy: " << path << ": "
                  << shannon_score(count, total_size, 0, 255) << std::endl;
    }
}

bool is_hidden(const fs::path& path) {
    const auto& name = path.filename().string();
    if (name != ".." && name != "." && name[0] == '.') {
        return true;
    }
    return false;
}

int main(int argc, char** argv) {
    po::options_description desc("Usage: entrospy [OPTION] PATH...");

    desc.add_options()                       //
        ("help,h", "Print help messages")    //
        ("debug", "Enable debugging output") //
        ("all,a",
         "Do not ignore hidden files and directories") //
        ("paths", po::value<std::vector<std::string>>(),
         "Paths to search") //
        ("lower,l", po::value<double>()->default_value(
                        std::numeric_limits<double>::lowest()),
         "Do not show files with entropy lower than 'lower'") //
        ("upper,u",
         po::value<double>()->default_value(std::numeric_limits<double>::max()),
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

        po::notify(vm);
    } catch (po::error& e) {
        std::cerr << "entrospy: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto lower = vm["lower"].as<double>();
    auto upper = vm["upper"].as<double>();
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
                        shannon_file(iter->path().string(), lower, upper);
                    }
                }
            }
        } else {
            shannon_file(path, lower, upper);
        }
    }
}
