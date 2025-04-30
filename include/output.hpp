#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "cache.hpp"
#include <string>
#include <vector>

class Output
{
public:
    static void printSimulationParameters(unsigned setBits, unsigned associativity, unsigned blockBits, const std::string &tracePrefix);
    static void printCoreStatistics(const std::vector<cache> &cores);
    static void printOverallBusSummary(const bus &b);
};

#endif // OUTPUT_HPP