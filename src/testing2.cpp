#include "input.hpp"
#include "cache.hpp"
#include "output.hpp"
#include <iostream>
#include <vector>
#include <fstream>

int main(int argc, char *argv[])
{
    try
    {
        // Parse input using InputParser
        InputParser parser(argc, argv);

        // Get simulation parameters
        unsigned associativity = parser.getAssociativity();
        unsigned blockBits = parser.getLineSizeBits();
        unsigned setBits = parser.getSetBits();
        std::vector<std::string> traceFiles = parser.getTraceFiles();

        // Parse trace files for each core
        std::vector<std::vector<std::pair<std::pair<unsigned int, bool>, bool>>> instructionsPerCore(4);
        for (int i = 0; i < 4; ++i)
        {
            parser.parseTraceFile(traceFiles[i], instructionsPerCore[i]);
        }

        // Initialize caches for each core
        std::vector<cache> caches;
        for (int i = 0; i < 4; ++i)
        {
            caches.emplace_back(blockBits, associativity, setBits, instructionsPerCore[i]);
        }
        // Initialize bus
        bus busObj(&caches[0], &caches[1], &caches[2], &caches[3]);

        // Run simulation cycles
        bool allCachesCompleted = false;
        unsigned cycleCount = 0;
        int cnt = 0;
        while (!allCachesCompleted)
        {
            std::cout << "";
            if (true)
            {
                std::cout << "Cycle " << cycleCount << ":\n";
            }
            allCachesCompleted = true;
            cnt++;
            busObj.runForACycle();
            for (int i = 0; i < 4; ++i)
            {
                if (caches[i].PC < caches[i].instructions.size())
                {
                    allCachesCompleted = false;
                }
            }
            // Run bus for a cycle
            if(busObj.cyclesBusy!=0){
                allCachesCompleted = false;
            }

            if (allCachesCompleted)
            {
                if (DEBUG)
                {
                    std::cout << "All caches have completed their instructions." << std::endl;
                }
            }

            cycleCount++;
            //if(cnt==300)break;
        }
        std::ofstream outputFile;
        std::string outputFileName = parser.getOutputFileName();
        if(outputFileName.empty()){
            outputFileName = "/dev/stdout";
        }
        outputFile.open(outputFileName);
        
        if (!outputFile.is_open()){
            std::cerr << "Error: Unable to open output file: " << outputFileName << "\n";
            return 1;
        }

        

        Output output;

        output.printSimulationParameters(outputFile,parser.applicationName, setBits, associativity, blockBits);
        output.printCoreStatistics(outputFile,caches);
        output.printOverallBusSummary(outputFile, busObj);

        if (true)
        {
            std::cout << "Simulation completed in " << cycleCount << " cycles.\n";
        }
        outputFile.close();
    }
    catch (const std::exception &e)
    {
        if (true)
        {
            std::cerr << "Error: " << e.what() << "\n";
        }
        return 1;
    }

    return 0;
}