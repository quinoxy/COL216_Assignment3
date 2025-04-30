#include "input.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

InputParser::InputParser(int argc, char *argv[])
    : associativity(2), lineSizeBits(5), setBits(6), outputFileName("output.txt")
    // default values are s: 6, b: 5, E: 2
{
    bool traceFileProvided = false;

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
            lineSizeBits = std::stoi(argv[++i]);
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
            if (traceFileProvided)
            {
                throw std::invalid_argument("Error: -t option can only be specified once.");
            }
            traceFileProvided = true;

            // Generate the 4 trace file names based on the application name
            std::string appName = argv[++i];
            for (int core = 0; core < 4; ++core)
            {
                traceFiles.push_back(appName + "_proc" + std::to_string(core) + ".trace");
            }
            applicationName = appName;
        }
        else
        {
            throw std::invalid_argument("Invalid argument: " + arg);
        }
    }

    if (!traceFileProvided)
    {
        throw std::invalid_argument("No application name provided. Use -t <appName>.");
    }

    if (traceFiles.size() != 4)
    {
        throw std::invalid_argument("Error: Exactly 4 trace files are required for the application.");
    }
}

void InputParser::parseTraceFile(const std::string &traceFileName, std::vector<std::pair<std::pair<unsigned int, bool>,bool>> &instructions)
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
        std::string addressStr;

        // Ensure there are exactly two arguments in the line
        if (!(iss >> op >> addressStr) || !(iss >> std::ws).eof())
        {
            throw std::invalid_argument("Invalid line format in trace file: " + line);
        }

        if (op != 'R' && op != 'W')
        {
            throw std::invalid_argument("Invalid operation in trace file: " + line);
        }

        // Check if the address is a valid hexadecimal number
        if (addressStr.size() > 10 || addressStr.substr(0, 2) != "0x" || 
            addressStr.find_first_not_of("0123456789abcdefABCDEF", 2) != std::string::npos)
        {
            throw std::invalid_argument("Invalid hexadecimal address in trace file: " + line);
        }

        // Remove the "0x" prefix and pad with leading zeros if necessary
        addressStr = addressStr.substr(2);
        while (addressStr.size() < 8)
        {
            addressStr = "0" + addressStr;
        }

        unsigned int address = std::stoul(addressStr, nullptr, 16);
        bool isWrite = (op == 'W');
        instructions.push_back({{address, isWrite}, false});
    }
}

unsigned InputParser::getAssociativity() const
{
    return associativity;
}

unsigned InputParser::getLineSizeBits() const
{
    return lineSizeBits;
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