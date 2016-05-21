
#include "category.hpp"
#include "output.hpp"

std::string categorize(double score, const PrintingPolicy& policy) {
    if (score > 0.93) {
        return "compressed/random/encrypted";
    } else if (score > 0.8) {
        return "uncompressed binary format";
    } else if (score > 0.65) {
        return "uncompressed text format";
    } else if (score > 0.55) {
        return "text";
    } else if (score > 0.35) {
        return "executable instructions";
    } else {
        return "low entropy format";
    }
}
