#include "output.hpp"
#include <iostream>
#include <cmath>
#include <iomanip>

void Output::printSimulationParameters(const std::string &tracePrefix, unsigned setBits, unsigned associativity, unsigned blockBits)
{
    unsigned blockSize = 1 << blockBits;
    unsigned numSets = 1 << setBits;
    unsigned cacheSizeKB = (numSets * associativity * blockSize) / 1024;

    std::cout << "Simulation Parameters:\n";
    std::cout << "Trace Prefix: " << tracePrefix << "\n";
    std::cout << "Set Index Bits: " << setBits << "\n";
    std::cout << "Associativity: " << associativity << "\n";
    std::cout << "Block Bits: " << blockBits << "\n";
    std::cout << "Block Size (Bytes): " << blockSize << "\n";
    std::cout << "Number of Sets: " << numSets << "\n";
    std::cout << "Cache Size (KB per core): " << cacheSizeKB << "\n";
    std::cout << "MESI Protocol: Enabled\n";
    std::cout << "Write Policy: Write-back, Write-allocate\n";
    std::cout << "Replacement Policy: LRU\n";
    std::cout << "Bus: Central snooping bus\n\n";
}

void Output::printCoreStatistics(const std::vector<cache> &cores)
{
    for (size_t i = 0; i < cores.size(); ++i)
    {
        const cache &core = cores[i];
        std::cout << "Core " << i << " Statistics:\n";
        std::cout << "Total Instructions: " << core.instructions.size() << "\n";
        std::cout << "Total Reads: " << core.readInstrs << "\n";
        std::cout << "Total Writes: " << core.instructions.size() - core.readInstrs << "\n";
        std::cout << "Total Execution Cycles: " << core.executionCycles << "\n";
        std::cout << "Idle Cycles: " << core.idleCycles << "\n";
        std::cout << "Cache Misses: " << core.misses << "\n";
        std::cout << "Cache Miss Rate: " << std::fixed << std::setprecision(2) << static_cast<double>(core.misses) * 100.0/core.instructions.size() << "%\n";
        std::cout << "Cache Evictions: " << core.evictions << "\n";
        std::cout << "Writebacks: " << core.writebacks << "\n";
        std::cout << "Bus Invalidations: " << core.invalidations << "\n";
        std::cout << "Data Traffic (Bytes): " << core.byteTraffic << "\n\n";
    }
}

void Output::printOverallBusSummary(const bus &inputBus)
{

    std::cout << "Overall Bus Summary:\n";
    std::cout << "Total Bus Transactions: " << inputBus.totalTransactions << "\n";
    std::cout << "Total Bus Traffic (Bytes): " << inputBus.totalTraffic << "\n";
}