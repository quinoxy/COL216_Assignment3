#include "output.hpp"
#include <iostream>
#include <cmath>
#include <iomanip>

void Output::printSimulationParameters(std::ostream &out, const std::string &tracePrefix, unsigned setBits, unsigned associativity, unsigned blockBits)
{
    unsigned blockSize = 1 << blockBits;
    unsigned numSets = 1 << setBits;
    unsigned cacheSizeKB = (numSets * associativity * blockSize) / 1024;

    out << "Simulation Parameters:\n";
    out << "Trace Prefix: " << tracePrefix << "\n";
    out << "Set Index Bits: " << setBits << "\n";
    out << "Associativity: " << associativity << "\n";
    out << "Block Bits: " << blockBits << "\n";
    out << "Block Size (Bytes): " << blockSize << "\n";
    out << "Number of Sets: " << numSets << "\n";
    out << "Cache Size (KB per core): " << cacheSizeKB << "\n";
    out << "MESI Protocol: Enabled\n";
    out << "Write Policy: Write-back, Write-allocate\n";
    out << "Replacement Policy: LRU\n";
    out << "Bus: Central snooping bus\n\n";
}

void Output::printCoreStatistics(std::ostream &out, const std::vector<cache> &cores)
{
    for (size_t i = 0; i < cores.size(); ++i)
    {
        const cache &core = cores[i];
        out << "Core " << i << " Statistics:\n";
        out << "Total Instructions: " << core.instructions.size() << "\n";
        out << "Total Reads: " << core.readInstrs << "\n";
        out << "Total Writes: " << core.instructions.size() - core.readInstrs << "\n";
        out << "Total Execution Cycles: " << core.ranForCycles - core.idleCycles << "\n";
        out << "Idle Cycles: " << core.idleCycles << "\n";
        out << "Cache Misses: " << core.misses << "\n";
        out << "Cache Miss Rate: " << std::fixed << std::setprecision(2) << static_cast<double>(core.misses) * 100.0/core.instructions.size() << "%\n";
        out << "Cache Evictions: " << core.evictions << "\n";
        out << "Writebacks: " << core.writebacks << "\n";
        out << "Bus Invalidations: " << core.invalidations << "\n";
        out << "Data Traffic (Bytes): " << core.byteTraffic << "\n\n";
    }
}

void Output::printOverallBusSummary(std::ostream &out, const bus &inputBus)
{

    out << "Overall Bus Summary:\n";
    out << "Total Bus Transactions: " << inputBus.totalTransactions << "\n";
    out << "Total Bus Traffic (Bytes): " << inputBus.totalTraffic << "\n";
}