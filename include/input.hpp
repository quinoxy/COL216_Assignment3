#ifndef INPUT_HPP
#define INPUT_HPP

#include <string>
#include <vector>
#include <utility>

enum OperationType
{
    READ,
    WRITE
};

struct MemoryOperation
{
    OperationType type;
    unsigned address;
};

class InputParser
{
public:
    InputParser(int argc, char *argv[]);
    void parseTraceFile(const std::string &traceFileName, std::vector<MemoryOperation> &operations);

    unsigned getAssociativity() const;
    unsigned getBlockBits() const;
    unsigned getSetBits() const;
    std::string getOutputFileName() const;
    std::vector<std::string> getTraceFiles() const;

private:
    unsigned associativity;
    unsigned blockBits;
    unsigned setBits;
    std::string outputFileName;
    std::vector<std::string> traceFiles;

    void printHelp() const;
};

#endif // INPUT_HPP