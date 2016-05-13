#ifndef ENTROSPY_SHANNON
#define ENTROSPY_SHANNON

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

class PrintingPolicy;

// boost does not support enum classes with program_options, so use enum
enum class DataFormat {
    DATA = 1 << 8,  // All bytes are allowed, max entropy = 8
    TEXT = 1 << 7,  // Only ASCII allowed, max entropy = 7
    BASE64 = 1 << 6 // Only base64 characters allowed, max entropy = 6
};

inline std::istream& operator>>(std::istream& in, DataFormat& format) {
    std::string token;
    in >> token;
    boost::algorithm::to_lower(token);
    if (token == "data") {
        format = DataFormat::DATA;
    } else if (token == "text") {
        format = DataFormat::TEXT;
    } else if (token == "base64") {
        format = DataFormat::BASE64;
    } else {
        throw boost::program_options::validation_error(
            boost::program_options::validation_error::invalid_option,
            "Unknown format");
    }
    return in;
}

void shannon_file(const std::string& path, uint64_t block_size,
                  const PrintingPolicy& policy, DataFormat format);
#endif
