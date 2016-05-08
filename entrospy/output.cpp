#include "output.hpp"

char byte_to_printable(uint8_t byte) {
    return (byte > 0x1f && byte < 0x7f) ? byte : '.';
}

void print_block_score(const std::string& path, uint64_t address,
                       uint64_t address_width, double score) {
    auto out = boost::format("%1%: %2%: score: %3%\n");
    out % path;
    out % boost::io::group(std::hex, std::setw(address_width),
                           std::setfill('0'), address);
    out % score;
    std::cout << out;
}
