#ifndef ENTROSPY_OUTPUT
#define ENTROSPY_OUTPUT

#include <iostream>
#include <iomanip>
#include <boost/format.hpp>

enum class AddressFormat { DECIMAL, HEX };
std::istream& operator>>(std::istream& in, AddressFormat& format);

struct PrintingPolicy {
    bool print_blocks = false;
    bool print_graph = false;
    std::pair<double, double> bounds = {std::numeric_limits<double>::lowest(),
                                        std::numeric_limits<double>::max()};
    AddressFormat addr_format = AddressFormat::DECIMAL;
};

uint8_t address_width(AddressFormat format, uint64_t address);

inline auto address_format_modifier(AddressFormat format)
    -> decltype(&std::hex) {
    switch (format) {
    case AddressFormat::DECIMAL:
        return std::dec;
    case AddressFormat::HEX:
        return std::hex;
    }
    assert(false && "Unknown address format");
}

inline char byte_to_printable(uint8_t byte) {
    return (byte > 0x1f && byte < 0x7f) ? byte : '.';
}

template <typename Iter>
void print_block_bytes(Iter begin, Iter end, uint64_t offset,
                       uint64_t address_width, const PrintingPolicy& policy) {
    const auto BYTES_PER_LINE = 16;
    auto iter = begin;
    for (auto i = 0; i < std::distance(begin, end) / BYTES_PER_LINE; ++i) {
        auto address = offset + BYTES_PER_LINE * i;
        std::cout << boost::format("%1%  ") %
                         boost::io::group(address_format_modifier(
                                              policy.addr_format),
                                          std::setw(address_width),
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
                         boost::io::group(address_format_modifier(
                                              policy.addr_format),
                                          std::setw(address_width),
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
