#ifndef ENTROSPY_CATEGORY
#define ENTROSPY_CATEGORY

#include <string>

class PrintingPolicy;

std::string categorize(double score, const PrintingPolicy& policy);

#endif
