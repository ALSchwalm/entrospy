
#include <ios>
#include <cmath>
#include <cstdio>
#include <boost/filesystem.hpp>

#include "shannon.hpp"
#include "output.hpp"
#include "graph.hpp"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

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

    PrintingPolicy policy;
    DataFormat format;
    std::vector<std::string> paths;

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
         "respectively")                              //
        ("print,p", "Print a hex view of each block") //
        ("addr", po::value<AddressFormat>(&policy.addr_format)
                     ->default_value(AddressFormat::HEX, "hex"),
         "Format to use for printed addresses: 'hex' or 'dec'") //
        ("lower,l", po::value<double>(&policy.bounds.first),
         "Do not show files with entropy lower than 'lower'") //
        ("upper,u", po::value<double>(&policy.bounds.second),
         "Do not show files with entropy higher than 'upper'") //
        ("format,f", po::value<DataFormat>(&format)
                         ->default_value(DataFormat::DATA, "data"),
         "Input format: 'data','text' or 'base64'") //
        ("graph,g",
         "Output a gnuplot script to standard out") //
        ("recursive,r",
         "Run directories in PATH recursively"); //

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

    uint64_t block_size = 0;
    if (vm.count("block")) {
        block_size = parse_block_size(vm["block"].as<std::string>());
    }

    if (vm.count("print")) {
        if (!vm.count("block")) {
            std::cerr << "entrospy: cannot specify 'print' without 'block'"
                      << std::endl;
            return EXIT_FAILURE;
        } else {
            policy.print_blocks = true;
        }
    }

    if (vm.count("graph")) {
        policy.print_graph = true;
    }

    auto title = boost::format("Entropy for %1% (bs=%2%)") %
                 boost::algorithm::join(paths, ", ") % block_size;

    EntropyGraph graph{boost::str(title), "output", block_size, policy};
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
                        shannon_file(iter->path().string(), block_size, policy,
                                     format, graph);
                    }
                }
            }
        } else {
            shannon_file(path, block_size, policy, format, graph);
        }
    }

    if (policy.print_graph) {
        std::cout << graph;
    }
}
