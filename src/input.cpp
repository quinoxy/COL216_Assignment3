#include "input.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

InputParser::InputParser(int argc, char *argv[])
    : associativity(2), blockBits(5), setBits(6), outputFileName("output.txt")
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-h")
        {
            printHelp();
            exit(0);
        }
        else if (arg == "-E" && i + 1 < argc)
        {
            associativity = std::stoi(argv[++i]);
        }
        else if (arg == "-b" && i + 1 < argc)
        {
            blockBits = std::stoi(argv[++i]);
        }
        else if (arg == "-s" && i + 1 < argc)
        {
            setBits = std::stoi(argv[++i]);
        }
        else if (arg == "-o" && i + 1 < argc)
        {
            outputFileName = argv[++i];
        }
        else if (arg == "-t" && i + 1 < argc)
        {
            traceFiles.push_back(argv[++i]);
        }
        else
        {
            throw std::invalid_argument("Invalid argument: " + arg);
        }
    }

    if (traceFiles.empty())
    {
        throw std::invalid_argument("No trace files provided. Use -t <tracefile>.");
    }
}

void InputParser::parseTraceFile(const std::string &traceFileName, std::vector<MemoryOperation> &operations)
{
    std::ifstream file(traceFileName);
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open trace file: " + traceFileName);
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        char op;
        unsigned address;
        if (iss >> op >> std::hex >> address)
        {
            OperationType type = (op == 'R') ? READ : WRITE;
            operations.push_back({type, address});
        }
    }
}

unsigned InputParser::getAssociativity() const
{
    return associativity;
}

unsigned InputParser::getBlockBits() const
{
    return blockBits;
}

unsigned InputParser::getSetBits() const
{
    return setBits;
}

std::string InputParser::getOutputFileName() const
{
    return outputFileName;
}

std::vector<std::string> InputParser::getTraceFiles() const
{
    return traceFiles;
}

void InputParser::printHelp() const
{
    std::cout << "Usage: ./L1simulate [options]\n"
              << "-t <tracefile>: name of parallel application trace file\n"
              << "-s <s>: number of set index bits (number of sets = 2^s)\n"
              << "-b <b>: number of block bits (block size = 2^b)\n"
              << "-E <E>: associativity (number of cache lines per set)\n"
              << "-o <outfilename>: logs output in file for plotting\n";
}