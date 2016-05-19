#ifndef ENTROSPY_GRAPH
#define ENTROSPY_GRAPH

#include <map>

#include "output.hpp"

class EntropyGraph {
    std::string m_title;
    uint64_t m_block_size;
    PrintingPolicy m_policy;

    using scores_t = std::vector<std::pair<std::streampos, double>>;
    std::map<std::string, scores_t> m_scores;

public:
    EntropyGraph(const std::string& title, uint64_t block_size,
                 const PrintingPolicy& policy);
    void insert(const std::string& filename, std::streampos position,
                double scores);

    friend std::ostream& operator<<(std::ostream&, const EntropyGraph&);
};

std::ostream& operator<<(std::ostream&, const EntropyGraph&);

#endif
