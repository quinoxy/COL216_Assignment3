#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "cache.hpp"
#include <string>
#include <vector>

class Output
{
public:
    static void printSimulationParameters(std::ostream &out, const std::string &tracePrefix, unsigned setBits, unsigned associativity, unsigned blockBits);
    static void printCoreStatistics(std::ostream &out, const std::vector<cache> &cores);
    static void printOverallBusSummary(std::ostream &out, const bus &Bus);
};

#endif // OUTPUT_HPP