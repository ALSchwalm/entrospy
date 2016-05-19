#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "graph.hpp"

EntropyGraph::EntropyGraph(const std::string& title, uint64_t block_size,
                           const PrintingPolicy& policy)
    : m_title{title}, m_block_size{block_size}, m_policy{policy}, m_scores{} {}

void EntropyGraph::insert(const std::string& path, std::streampos position,
                          double score) {
    m_scores[path].emplace_back(position, score);
}

std::ostream& operator<<(std::ostream& out, const EntropyGraph& graph) {
    out << boost::format("set term qt noenhanced size 1024, 768\n"
                         "set title '%1%'\n"
                         "set grid\n"
                         "set xlabel 'address'\n"
                         "set ylabel 'entropy'\n"
                         "set xtics rotate by -30\n"
                         "set format x '%%.0f'\n"
                         "set style line 1 lc rgb '#A0A0A0' lt 0 lw 2\n"
                         "set grid back ls 1\n") %
               graph.m_title;

    std::vector<std::string> points_cfg;
    uint64_t max_file_size = 0;
    for (const auto& pv : graph.m_scores) {
        const auto& path = pv.first;
        auto file_size = boost::filesystem::file_size(path);
        if (file_size > max_file_size) {
            max_file_size = file_size;
        }

        std::string p =
            boost::str(boost::format("'-' title '%1%' with points") % path);
        points_cfg.emplace_back(p);
    }

    out << boost::format("plot [0:%1%] [0:1] %2%") % max_file_size %
               boost::algorithm::join(points_cfg, ", ");
    out << "\n";

    for (const auto& pv : graph.m_scores) {
        const auto& scores = pv.second;
        for (const auto& score : scores) {
            auto position = score.first;
            double value = score.second;

            out << position << " " << value << "\n";
        }
        out << "e\n";
    }
    out << "quit\n";
    return out;
}
