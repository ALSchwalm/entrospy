#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

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
