#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "output.hpp"

std::istream& operator>>(std::istream& in, AddressFormat& format) {
    std::string token;
    in >> token;
    boost::algorithm::to_lower(token);
    if (token == "dec") {
        format = AddressFormat::DECIMAL;
    } else if (token == "hex") {
        format = AddressFormat::HEX;
    } else {
        throw boost::program_options::validation_error(
            boost::program_options::validation_error::invalid_option,
            "Unknown address format");
    }
    return in;
}

uint8_t address_width(AddressFormat format, uint64_t address) {
    switch (format) {
    case AddressFormat::DECIMAL:
        return std::ceil(std::log10(address));
    case AddressFormat::HEX:
        return std::ceil(std::log2(address) / std::log2(16));
    }
    assert(false && "Unknown address format");
}

void print_score(const std::string& path, uint64_t address,
                 uint64_t address_width, double score,
                 const PrintingPolicy& policy) {
    auto out = boost::format("%1%: %2%: score: %3%\n");
    out % path;
    out % boost::io::group(address_format_modifier(policy.addr_format),
                           std::setw(address_width), std::setfill('0'),
                           address);
    out % score;
    std::cout << out;
}

void print_score(const std::string& path, double score, const PrintingPolicy&) {
    std::cout << path << ": score: " << score << "\n";
}

void print_graph_header(const std::string& path, uint64_t block_size,
                        uint64_t file_size, const PrintingPolicy& policy) {

    auto input_file = boost::filesystem::path(path).stem().string();
    auto output_file = input_file + "_entropy.png";
    auto title = input_file + " Entropy";
    boost::replace_all(title, "_", "\\_");

    std::cout << boost::format("set term pngcairo size 1024, 768\n"
                               "set output '%1%'\n"
                               "set title '%2% (block size=%3%)'\n"
                               "set grid\n"
                               "set xlabel 'address'\n"
                               "set ylabel 'entropy'\n"
                               "set xtics rotate by -30\n"
                               "set format x '%%.0f'\n"
                               "set style line 1 lc rgb '#A0A0A0' lt 0 lw 2\n"
                               "set grid back ls 1\n"
                               "plot [0:%4%] [0:1] '-' title ''"
                               "  with points lc rgb '#0000BB' pt 6 ps 0.8\n") %
                     output_file % title % block_size % file_size;
}

void print_graph_score(std::streampos position, double score,
                       const PrintingPolicy&) {
    std::cout << position << " " << score << std::endl;
}

void print_graph_finalize(const PrintingPolicy& policy) {
    std::cout << "quit" << std::endl;
}
