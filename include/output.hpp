#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "cache.hpp"
#include <string>
#include <vector>

class Output
{
public:
    static void printSimulationParameters(const std::string &tracePrefix, unsigned setBits, unsigned associativity, unsigned blockBits);
    static void printCoreStatistics(const std::vector<cache> &cores);
    static void printOverallBusSummary(const bus &Bus);
};

#endif // OUTPUT_HPP