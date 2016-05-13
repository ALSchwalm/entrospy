#ifndef ENTROSPY_OUTPUT
#define ENTROSPY_OUTPUT

#include <iostream>
#include <iomanip>
#include <boost/format.hpp>

enum class AddressFormat { DECIMAL, HEX };

struct PrintingPolicy {
    bool print_blocks = false;
    std::pair<double, double> bounds = {std::numeric_limits<double>::lowest(),
                                        std::numeric_limits<double>::max()};
    AddressFormat addr_format = AddressFormat::DECIMAL;
};

char byte_to_printable(uint8_t);

template <typename Iter>
void print_block_bytes(Iter begin, Iter end, uint64_t offset,
                       uint64_t address_width) {
    const auto BYTES_PER_LINE = 16;
    auto iter = begin;
    for (auto i = 0; i < std::distance(begin, end) / BYTES_PER_LINE; ++i) {
        auto address = offset + BYTES_PER_LINE * i;
        std::cout << boost::format("%1%  ") %
                         boost::io::group(std::hex, std::setw(address_width),
                                          std::setfill('0'), address);

        auto byte_iter = iter;
        auto line_end = iter + BYTES_PER_LINE;
        for (; byte_iter < line_end; ++byte_iter) {
            std::cout << boost::format("%02x ") %
                             static_cast<unsigned>(*byte_iter);
        }

        line_end -= 1;
        std::cout << "  |";
        for (; iter < line_end; ++iter) {
            std::cout << byte_to_printable(*iter);
        }
        std::cout << byte_to_printable(*iter++) << "|\n";
    }

    if (iter != end) {
        auto byte_iter = iter;
        auto printed = end - begin - (end - iter);
        std::cout << boost::format("%1%  ") %
                         boost::io::group(std::hex, std::setw(address_width),
                                          std::setfill('0'), offset + printed);
        for (; byte_iter != end; ++byte_iter) {
            std::cout << boost::format("%02x ") %
                             static_cast<unsigned>(*byte_iter);
        }

        auto spacing = BYTES_PER_LINE - ((end - begin) % BYTES_PER_LINE);
        std::cout << std::string(spacing * 3, ' ') << "  |";
        for (; iter < end; ++iter) {
            std::cout << byte_to_printable(*iter);
        }
        std::cout << std::string(spacing, ' ') << "|\n";
    }
}

void print_score(const std::string& path, uint64_t address,
                 uint64_t address_width, double score,
                 const PrintingPolicy& policy);

void print_score(const std::string& path, double score,
                 const PrintingPolicy& policy);

#endif
