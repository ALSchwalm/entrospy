#ifndef ENTROSPY_SHANNON
#define ENTROSPY_SHANNON

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

// boost does not support enum classes with program_options, so use enum
enum class Format {
    DATA = 1 << 8,  // All bytes are allowed, max entropy = 8
    TEXT = 1 << 7,  // Only ASCII allowed, max entropy = 7
    BASE64 = 1 << 6 // Only base64 characters allowed, max entropy = 6
};

inline std::istream& operator>>(std::istream& in, Format& format) {
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
        throw boost::program_options::validation_error(
            boost::program_options::validation_error::invalid_option,
            "Unknown format");
    }
    return in;
}

void shannon_file(const std::string& path, uint64_t block_size,
                  bool print_blocks, std::pair<double, double> bounds,
                  Format format);

#endif
