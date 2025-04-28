#include "input.hpp"
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    try {
        // Initialize the InputParser with command-line arguments
        InputParser parser(argc, argv);

        // Print parsed values
        std::cout << "Associativity: " << parser.getAssociativity() << "\n";
        std::cout << "Block Bits: " << parser.getLineSizeBits() << "\n";
        std::cout << "Set Bits: " << parser.getSetBits() << "\n";
        std::cout << "Output File Name: " << parser.getOutputFileName() << "\n";

        std::vector<std::string> traceFiles = parser.getTraceFiles();
        std::cout << "Trace Files:\n";
        for (const auto& file : traceFiles) {
            std::cout << "  " << file << "\n";
        }
        
        // Parse and print instructions from the first trace file (if provided)
        if (!traceFiles.empty()) {
            
            for(int i = 0; i< 4;i++){
                
            
            std::vector<std::pair<std::pair<unsigned int, bool>, bool>> instructions;
            parser.parseTraceFile(traceFiles[i], instructions);

            std::cout << "Instructions from " << traceFiles[i] << ":\n";
            for (const auto& [pr1, pr2] : instructions) {
                std::cout << (pr1.second ? "Write" : "Read") << " " << pr1.first << " "<<pr2 << "\n";
            }
        }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}