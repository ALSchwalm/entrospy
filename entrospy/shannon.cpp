#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/preprocessor/repetition/enum.hpp>

#include "shannon.hpp"
#include "output.hpp"

#define SHANNON_IDENT(z, n, value) value

template <uint8_t Base>
constexpr double log(double value) {
    return log2(value) / log2(Base);
}

namespace fs = boost::filesystem;

using counter_t = std::array<uint64_t, 256>;
using allowed_t = std::array<bool, 256>;
constexpr auto DEFAULT_BLOCK_SIZE = 16 * 1024;

constexpr allowed_t ALLOWED_DATA{BOOST_PP_ENUM(256, SHANNON_IDENT, true)};
constexpr allowed_t ALLOWED_TEXT{BOOST_PP_ENUM(128, SHANNON_IDENT, true)};
constexpr allowed_t ALLOWED_BASE64{
    BOOST_PP_ENUM(43, SHANNON_IDENT, false),
    true, // '+'
    BOOST_PP_ENUM(3, SHANNON_IDENT, false),
    BOOST_PP_ENUM(11, SHANNON_IDENT, true), // '/' and 0-9
    BOOST_PP_ENUM(7, SHANNON_IDENT, false),
    BOOST_PP_ENUM(26, SHANNON_IDENT, true), // A-Z
    BOOST_PP_ENUM(6, SHANNON_IDENT, false),
    BOOST_PP_ENUM(26, SHANNON_IDENT, true), // a-z
};

double shannon_score(const counter_t& counts, std::size_t total_size,
                     DataFormat format) {
    const allowed_t* allowed = nullptr;
    switch (format) {
    case DataFormat::DATA:
        allowed = &ALLOWED_DATA;
        break;
    case DataFormat::TEXT:
        allowed = &ALLOWED_TEXT;
        break;
    case DataFormat::BASE64:
        allowed = &ALLOWED_BASE64;
        break;
    }

    double score = 0;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        auto count = counts[index];
        if (count == 0 || !(*allowed)[index])
            continue;
        double p_i = count / static_cast<double>(total_size);
        score += p_i * log2(p_i);
    }

    switch (format) {
    case DataFormat::DATA:
        return std::abs(score) / 8.0;
    case DataFormat::TEXT:
        return std::abs(score) / 7.0;
    case DataFormat::BASE64:
        return std::abs(score) / 6.0;
    }
    return 0.0;
}

template <typename Iter>
void shannon_digest(Iter begin, Iter end, counter_t& counts) {
    for (auto iter = begin; iter != end; ++iter) {
        counts[*iter] += 1;
    }
}

class shannon_iterator
    : public boost::iterator_facade<shannon_iterator, double,
                                    boost::forward_traversal_tag, double> {
private:
    FILE* m_file = nullptr;
    DataFormat m_format;
    uint64_t m_block_size;
    std::vector<uint8_t> m_block;
    bool m_clear_stats;
    counter_t m_counter;

public:
    shannon_iterator() = default;
    shannon_iterator(const std::string& path, uint64_t block_size,
                     DataFormat format, bool clear_stats = true)
        : m_file{std::fopen(path.c_str(), "rb")},
          m_format{format},
          m_block_size{block_size},
          m_block(block_size),
          m_clear_stats{clear_stats},
          m_counter{} {
        this->increment(); // Get meaningful data in the buffer
    }
    void increment() {
        assert(m_block.size() >= m_block_size);
        std::size_t bytes_read =
            std::fread(&m_block.front(), 1, m_block_size, m_file);
        m_block.resize(bytes_read);

        if (m_clear_stats) {
            m_counter.fill(0);
        }
        shannon_digest(m_block.begin(), m_block.end(), m_counter);
    }

    bool equal(shannon_iterator const& other) const {
        return (m_file && std::feof(m_file) && other.m_file == nullptr) ||
               (std::feof(m_file) && other.m_file && std::feof(other.m_file)) ||
               (m_file == nullptr && other.m_file == nullptr);
    }

    double dereference() const {
        if (m_clear_stats) {
            return shannon_score(m_counter, m_block_size, m_format);
        } else {
            return shannon_score(m_counter, std::ftell(m_file), m_format);
        }
    }

    const std::vector<uint8_t>& block() const { return m_block; }
    int64_t position() { return std::ftell(m_file) - m_block_size; }
};

void shannon_file(const std::string& path, uint64_t block_size,
                  const PrintingPolicy& policy, DataFormat format) {
    if (!block_size) {
        shannon_iterator iter{path, DEFAULT_BLOCK_SIZE, format, false};
        shannon_iterator end{};
        for (; iter != end; ++iter) {
        }
        auto score = *iter;

        if (score < policy.bounds.first || score > policy.bounds.second) {
            return;
        }

        print_score(path, score, policy);
    } else {
        shannon_iterator iter{path, block_size, format};
        shannon_iterator end{};
        auto addr_width =
            address_width(policy.addr_format, fs::file_size(path));
        for (; iter != end; ++iter) {
            auto score = *iter;

            if (score < policy.bounds.first || score > policy.bounds.second) {
                continue;
            }

            print_score(path, iter.position(), addr_width, score, policy);
            if (policy.print_blocks) {
                print_block_bytes(iter.block().begin(), iter.block().end(),
                                  iter.position(), addr_width, policy);
            }
        }
    }
}
