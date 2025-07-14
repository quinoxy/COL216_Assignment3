#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "cache.hpp"
#include <string>
#include <vector>
#include <iostream>

class Output
{
public:
    static void printSimulationParameters(const std::string &tracePrefix, unsigned setBits, unsigned associativity, unsigned blockBits, std::ostream &out = std::cout);
    static void printCoreStatistics(const std::vector<cache> &cores, std::ostream &out = std::cout);
    static void printOverallBusSummary(const bus &Bus, std::ostream &out = std::cout);
};

#endif // OUTPUT_HPP