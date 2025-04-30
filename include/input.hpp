#ifndef INPUT_HPP
#define INPUT_HPP

#include <string>
#include <vector>
#include <utility>


class InputParser
{
public:
    InputParser(int argc, char *argv[]);
    void parseTraceFile(const std::string &traceFileName, std::vector<std::pair<std::pair<unsigned int, bool>, bool>> &instructions);

    unsigned getAssociativity() const;
    unsigned getLineSizeBits() const;
    unsigned getSetBits() const;
    std::string getPrefix() const;
    std::string getOutputFileName() const;
    std::vector<std::string> getTraceFiles() const;

private:
    unsigned associativity;
    unsigned lineSizeBits;
    unsigned setBits;
    std::string outputFileName;
    std::vector<std::string> traceFiles;

    void printHelp() const;
};

#endif // INPUT_HPP