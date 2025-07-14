#include "cache.hpp"
#include "dll.hpp"
#include <iostream>

const unsigned DEFAULT_CACHE_LINES_PER_SET = 4;
const unsigned DEFAULT_LINE_SIZE = 64;
const unsigned DEFAULT_LINE_SIZE_BITS = 6;
const unsigned DEFAULT_SETS_PER_CACHE = 64;
const unsigned DEFAULT_SET_BITS = 6;
const bool IF_EVICT_OR_TRANSFER_STALL = false;
const bool DEBUG = false;

cacheLine::cacheLine(unsigned s) : tag(0), state(I), size_bits(s), lastAccessedCycle(0){}

cacheSet::cacheSet(unsigned cacheLinesPerSet, unsigned lineSizeBits) : lineCount(cacheLinesPerSet), lineSizeBits(lineSizeBits), currentCapacity(0)
{
    if (DEBUG)
    {
        std::cout << "Initializing cacheSet with " << cacheLinesPerSet << " lines and line size bits " << lineSizeBits << std::endl;
    }
    for (unsigned i = 0; i < lineCount; ++i)
    {
        lines.push_back(cacheLine(lineSizeBits));
    }
}

std::pair<bool, cacheLine *> cacheSet::isMiss(unsigned tag, bool LRUUpdate, unsigned currentCycle) {
    for (auto& line : lines) {
        if (line.tag == tag && line.state != I) {
            if (LRUUpdate) {
                line.lastAccessedCycle = currentCycle;
            }
            return {false, &line}; // HIT
        }
    }
    return {true, nullptr}; // MISS
}


cacheLine cacheSet::addTag(unsigned tag, cacheLineLabel s, unsigned currentCycle) {
    cacheLine newLine(lineSizeBits);
    newLine.tag = tag;
    newLine.state = s;
    newLine.lastAccessedCycle = currentCycle;

    // First try to find an invalid line to reuse
    for (unsigned i = 0; i < lineCount; ++i) {
        if (lines[i].state == I) {
            cacheLine evicted = lines[i];
            lines[i] = newLine;
            return evicted;  // No eviction
        }
    }

    // All lines valid — find LRU (least recently used)
    unsigned lruIndex = 0;
    unsigned minCycle = lines[0].lastAccessedCycle;

    for (unsigned i = 1; i < lineCount; ++i) {
        if (lines[i].lastAccessedCycle < minCycle) {
            minCycle = lines[i].lastAccessedCycle;
            lruIndex = i;
        }
    }

    cacheLine evicted = lines[lruIndex];
    lines[lruIndex] = newLine;
    return evicted;
}


cache::cache(unsigned lineSizeBits, unsigned associativity, unsigned setBits, std::vector<std::pair<std::pair<unsigned int, bool>, bool>> instructions)
    : setCount(1 << setBits), lineSizeBits(lineSizeBits), associativity(associativity), setBits(setBits), instructions(instructions), isHalted(false), PC(0), readInstrs(0), executionCycles(0), idleCycles(0), misses(0), evictions(0), writebacks(0), invalidations(0), byteTraffic(0), arrivedMemBuffer(0), memArrivedInCycle(false), sendMemBuffer(0), ranForCycles(0)
{
    if (DEBUG)
    {
        std::cout << "Initializing cache with " << setCount << " sets, associativity " << associativity << ", and line size bits " << lineSizeBits << std::endl;
    }
    for (unsigned i = 0; i < setCount; ++i)
    {
        sets.emplace_back(associativity, lineSizeBits);
    }
    fromCacheToBus = {busTransactionType::None, 0};
    snoop = {busTransactionType::None, 0};
}

void cache::processInst()
{
    if (PC >= instructions.size())
    {
        if (DEBUG)
        {
            std::cout << "All instructions processed. Halting." << std::endl;
        }
        return;
    }
    ranForCycles ++;
    if (!isHalted)
    {
        auto instruction = instructions[PC];

        unsigned address = instruction.first.first;
        bool isWrite = instruction.first.second;
        bool alreadyProcessed = instruction.second;
        instructions[PC].second = true;

        unsigned tag = address >> lineSizeBits;
        unsigned setIndex = (address >> lineSizeBits) & (setCount - 1);
        cacheSet &currentSet = sets[setIndex];

        if (DEBUG)
        {
            std::cout << "Processing instruction at PC " << PC << ": address=" << address << ", isWrite=" << isWrite << std::endl;
        }

        bool miss = false;
        auto [isMiss, linePtr] = currentSet.isMiss(tag, true, ranForCycles);

        if (isMiss)
        {
            miss = true;
            if (DEBUG)
            {
                std::cout << "Cache miss for address " << address << std::endl;
            }
        }
        else
        {
            if (DEBUG)
            {
                std::cout << "Cache hit for address " << address << std::endl;
            }
        }

        if (!alreadyProcessed)
        {
            if (!miss && !isWrite)
            {
                readInstrs++;
                PC++;
                return;
            }
            else if (!miss && isWrite)
            {
                if (linePtr->state == E)
                {
                    linePtr->state = M;
                    PC++;
                }
                else if (linePtr->state == S)
                {
                    linePtr->state = M;
                    fromCacheToBus = {busTransactionType::WriteInvalidate, linePtr->tag};
                    isHalted = true;
                }
                else
                {
                    PC++;
                }
            }
            else if (miss && !isWrite)
            {
                readInstrs++;
                misses++;
                fromCacheToBus = {busTransactionType::Rd, address};
                isHalted = true;
            }
            else if (miss && isWrite)
            {
                misses++;
                fromCacheToBus = {busTransactionType::RdX, address};
                isHalted = true;
            }
        }
        else
        {
            if (!miss && !isWrite)
            {
                PC++;
                return;
            }
            else if (!miss && isWrite)
            {
                if (linePtr->state == E)
                {
                    linePtr->state = M;
                    PC++;
                }
                else if (linePtr->state == S)
                {
                    if (true)
                    {
                        std::cout << "There is a major error... Exiting...." << std::endl;
                    }
                    exit(1);
                }
                else
                {
                    PC++;
                }
            }
            else if (miss)
            {
                if (true)
                {
                    std::cout << "There is a major error... Exiting...." << std::endl;
                }
                exit(1);
            }
        }
    }
}

std::pair<bool, cacheLine> cache::processSnoop(busTransaction trs)
{
    unsigned tag = trs.value >> lineSizeBits;
    unsigned setIndex = (trs.value >> lineSizeBits) & (setCount - 1);
    cacheSet &currentSet = sets[setIndex];
    auto [isMiss, linePtr] = currentSet.isMiss(tag, false, ranForCycles);
    if (isMiss)
    {
        return {false, cacheLine(0)};
    }
    cacheLine ret = *linePtr;

    if (trs.type == busTransactionType::Rd)
    {
        linePtr->state = S;
    }
    else if (trs.type == busTransactionType::RdX)
    {
        linePtr->state = I;

        if (tag == fromCacheToBus.value>>lineSizeBits && fromCacheToBus.type == busTransactionType::WriteInvalidate){
            auto instruction = instructions[PC];
            bool isWrite = instruction.first.second;
            instructions[PC].second = true;
            if(!isWrite) readInstrs--;
            isHalted = false;
            fromCacheToBus = {busTransactionType::None, 0};

        }
    }
    else if (trs.type == busTransactionType::WriteInvalidate)
    {
        linePtr->state = I;

        if (tag == fromCacheToBus.value>>lineSizeBits && fromCacheToBus.type == busTransactionType::WriteInvalidate){
            auto instruction = instructions[PC];
            bool isWrite = instruction.first.second;
            instructions[PC].second = true;
            if(!isWrite) readInstrs--;
            isHalted = false;
            fromCacheToBus = {busTransactionType::None, 0};

        }
    }
    return {true, ret};
}

bus::bus(cache *cachePtr0, cache *cachePtr1, cache *cachePtr2, cache *cachePtr3) : from(5), to(5), cyclesBusy(0), totalTransactions(0), totalTraffic(0), currentOpIsEviction(false), currentProcessing({busTransactionType::None, 0}), busOwner(5), typeOfNewLine(I), ifEvictingOrTransferringSelfStall(IF_EVICT_OR_TRANSFER_STALL)
{
    if (DEBUG)
    {
        std::cout << "Initializing bus with 4 caches" << std::endl;
    }
    cachePtrs[0] = cachePtr0;
    cachePtrs[1] = cachePtr1;
    cachePtrs[2] = cachePtr2;
    cachePtrs[3] = cachePtr3;
}

void bus::runForACycle()
{
    if (DEBUG)
    {
        std::cout << "Running bus for a cycle. Cycles busy: " << cyclesBusy << std::endl;

    }

    if (cyclesBusy > 1)
    {
        if (DEBUG)
        {
            std::cout << "Bus is busy. Decrementing cyclesBusy to " << (cyclesBusy - 1) << std::endl;
        }
        cyclesBusy--;
    }
    else if (cyclesBusy == 1)
    {
        if (DEBUG)
        {
            std::cout << "Bus is completing its current operation." << std::endl;
        }
        cyclesBusy = 0;
        if (currentProcessing.type == busTransactionType::Rd)
        {
            if (DEBUG)
            {
                std::cout << "Processing Rd transaction. From: " << from << ", To: " << to << std::endl;
            }
            if (to != 4)
            {
                if (from != 4)
                {
                    if (DEBUG)
                    {
                        std::cout << "Unhalting cache " << from << std::endl;
                    }
                    if (ifEvictingOrTransferringSelfStall)
                    {
                        if (cachePtrs[from]->fromCacheToBus.type == busTransactionType::None)
                        {
                            cachePtrs[from]->isHalted = false;
                        }
                    }
                }
                unsigned tag = currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits;
                unsigned setIndex = (currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits) & (cachePtrs[busOwner]->setCount - 1);
                cacheSet &currentSet = cachePtrs[busOwner]->sets[setIndex];
                cacheLine result = currentSet.addTag(tag, typeOfNewLine, cachePtrs[busOwner]->ranForCycles);
                if (result.state == M)
                {
                    if (DEBUG)
                    {
                        std::cout << "Evicting modified line. From: " << busOwner << " to main memory." << std::endl;
                    }
                    from = busOwner;
                    to = 4;
                    currentOpIsEviction = true;
                    cyclesBusy = 100;
                    cachePtrs[busOwner]->writebacks++;
                    cachePtrs[busOwner]->evictions++;
                    totalTraffic += 1 << cachePtrs[busOwner]->lineSizeBits;
                    cachePtrs[busOwner]->byteTraffic += 1 << cachePtrs[busOwner]->lineSizeBits;
                    if (!ifEvictingOrTransferringSelfStall)
                    {
                        cachePtrs[busOwner]->isHalted = false;
                        cachePtrs[busOwner]->fromCacheToBus = {busTransactionType::None, 0};
                    }
                }
                else
                {
                    if (DEBUG)
                    {
                        std::cout << "Transaction completed without eviction." << std::endl;
                    }
                    if(result.state!=I){
                        cachePtrs[busOwner] -> evictions++;
                    }
                    transactionOver();
                }
            }
            else if (from == busOwner)
            {
                if (DEBUG)
                {
                    std::cout << "Transaction completed. Returning to owner." << std::endl;
                }
                transactionOver();
            }
            else
            {
                if (DEBUG)
                {
                    std::cout << "Fetching data from modified memory to owner." << std::endl;
                }
                to = busOwner;
                cyclesBusy = 2 * (1 << (cachePtrs[busOwner]->lineSizeBits-2));
                typeOfNewLine = S;
                currentOpIsEviction = false;
            }
        }
        else if (currentProcessing.type == busTransactionType::RdX)
        {
            if (DEBUG)
            {
                std::cout << "Processing RdX transaction. From: " << from << ", To: " << to << std::endl;
            }
            if (from == 4)
            {
                unsigned tag = currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits;
                unsigned setIndex = (currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits) & (cachePtrs[busOwner]->setCount - 1);
                cacheSet &currentSet = cachePtrs[busOwner]->sets[setIndex];
                cacheLine result = currentSet.addTag(tag, typeOfNewLine, cachePtrs[busOwner]->ranForCycles);
                if (result.state == M)
                {
                    if (DEBUG)
                    {
                        std::cout << "Evicting modified line. From: " << busOwner << " to main memory." << std::endl;
                    }
                    from = busOwner;
                    to = 4;
                    currentOpIsEviction = true;
                    cyclesBusy = 100;
                    cachePtrs[busOwner] ->writebacks++;
                    cachePtrs[busOwner] ->evictions++;
                    totalTraffic += 1 << cachePtrs[busOwner]->lineSizeBits;
                    cachePtrs[busOwner]->byteTraffic += 1 << cachePtrs[busOwner]->lineSizeBits;

                    if (!ifEvictingOrTransferringSelfStall)
                    {
                        cachePtrs[busOwner]->isHalted = false;
                        cachePtrs[busOwner]->fromCacheToBus = {busTransactionType::None, 0};
                    }
                }
                else
                {
                    if (DEBUG)
                    {
                        std::cout << "Transaction completed without eviction." << std::endl;
                    }
                    if(result.state!=I){
                        cachePtrs[busOwner]->evictions++;
                    }
                    transactionOver();
                }
            }
            else if (from == busOwner)
            {
                if (DEBUG)
                {
                    std::cout << "Transaction completed. Returning to owner." << std::endl;
                }
                transactionOver();
            }
            else
            {
                if (DEBUG)
                {
                    std::cout << "Fetching data from main memory to owner." << std::endl;
                }
                if (ifEvictingOrTransferringSelfStall)
                {
                    if (cachePtrs[from]->fromCacheToBus.type == busTransactionType::None)
                    {
                        cachePtrs[from]->isHalted = false;
                    }
                }
                from = 4;
                to = busOwner;
                cyclesBusy = 100;
            }
        }
    }

    for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
    {
        cachePtrs[cacheNumber]->processInst();
    }

    if (cyclesBusy != 0)
    {
        if (DEBUG)
        {
            std::cout << "Bus is still busy. Exiting cycle." << std::endl;
        }
        for(unsigned cache = 0; cache < 4; cache++)
        {
            if ((cachePtrs[cache] -> fromCacheToBus.type != busTransactionType::None && cache!=busOwner && cachePtrs[cache]->PC < cachePtrs[cache]->instructions.size())||(cachePtrs[cache]->fromCacheToBus.type == busTransactionType::None && cache!=busOwner && cachePtrs[cache]->PC < cachePtrs[cache]->instructions.size() && cachePtrs[cache]->isHalted))
            {
                cachePtrs[cache] -> idleCycles ++;
                if (DEBUG) {

                    std::cout << "Cache " << cache << " is idle. Idle cycles: " << cachePtrs[cache]->idleCycles << std::endl;
                }
                else if (cache!=busOwner){
                    if(DEBUG){
                        std::cout<< "Non- stalled cache " << cache << " has transaction type " << cachePtrs[cache]->fromCacheToBus.type <<" and PC = "<< cachePtrs[cache]->PC<<" and isHalted = "<< cachePtrs[cache]->isHalted << std::endl;
                    }
                }
            }
        }

        return;
    }

    if (DEBUG)
    {
        std::cout << "Arbitrating for new transaction." << std::endl;
    }
    busTransaction transaction;
    unsigned owner = 5;
    for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
    {
        if (cachePtrs[cacheNumber]->fromCacheToBus.type != busTransactionType::None)
        {
            transaction = cachePtrs[cacheNumber]->fromCacheToBus;
            cachePtrs[cacheNumber]->fromCacheToBus = {busTransactionType::None, 0};
            owner = cacheNumber;

            if (DEBUG)
            {
                std::cout << "Transaction found from cache " << cacheNumber << ": type=" << transaction.type << ", value=" << transaction.value << std::endl;
            }
            break;
        }
    }
    if (transaction.type!=busTransactionType::None){
        for(unsigned cache = 0; cache < 4; cache++)
        {
            if ((cachePtrs[cache] -> fromCacheToBus.type != busTransactionType::None && cache!=owner && cachePtrs[cache]->PC < cachePtrs[cache]->instructions.size())||(cachePtrs[cache]->fromCacheToBus.type == busTransactionType::None && cache!=owner && cachePtrs[cache]->PC < cachePtrs[cache]->instructions.size() && cachePtrs[cache]->isHalted))
            {
                cachePtrs[cache] -> idleCycles ++;
                if (DEBUG) {

                    std::cout << "Cache " << cache << " is idle. Idle cycles: " << cachePtrs[cache]->idleCycles << std::endl;
                }
            }
            else if (cache!=owner){
                if(DEBUG){
                    std::cout<< "Non- stalled cache " << cache << " has transaction type " << cachePtrs[cache]->fromCacheToBus.type <<" and PC = "<< cachePtrs[cache]->PC<<" and isHalted = "<< cachePtrs[cache]->isHalted << std::endl;
                }
            }
        }
    }
    if (transaction.type == busTransactionType::None)
    {
        if (DEBUG)
        {
            std::cout << "No transactions to process. Exiting cycle." << std::endl;
        }
        return;
    }
    else if (transaction.type == busTransactionType::WriteInvalidate)
    {   
        totalTransactions++;
        if (DEBUG)
        {
            std::cout << "Processing WriteInvalidate transaction." << std::endl;
        }
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
        {
            if (cacheNumber != owner)
            {
                cachePtrs[cacheNumber]->processSnoop(transaction);
            }
        }
        cachePtrs[owner] -> invalidations++;
        cachePtrs[owner]->isHalted = false;
        cachePtrs[owner]->fromCacheToBus = {busTransactionType::None, 0};
        cachePtrs[owner]->PC++;
    }
    else if (transaction.type == busTransactionType::Rd)
    {   
        totalTransactions++;
        if (DEBUG)
        {
            std::cout << "Processing Rd transaction." << std::endl;
        }
        currentProcessing = transaction;
        busOwner = owner;
        cacheLineLabel labels[4];
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
        {
            if (cacheNumber != owner)
            {
                labels[cacheNumber] = cachePtrs[cacheNumber]->processSnoop(transaction).second.state;
            }
            else
            {
                labels[cacheNumber] = I;
            }
        }
        cacheLineLabel caseReadMiss = I;
        unsigned sender = 5;
        for (unsigned i = 0; i < 4; i++)
        {
            if (labels[i] != I)
            {
                caseReadMiss = labels[i];
                sender = i;
            }
        }

        if (caseReadMiss == I)
        {
            if (DEBUG)
            {
                std::cout << "Read miss. Fetching from main memory." << std::endl;
            }
            from = 4;
            to = owner;
            cyclesBusy = 100;
            typeOfNewLine = E;
            totalTraffic += 1 << cachePtrs[owner]->lineSizeBits;
            cachePtrs[owner]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
        }
        else if (caseReadMiss == M)
        {
            if (DEBUG)
            {
                std::cout << "Read hit on modified line. Fetching from cache " << sender << " to main memory." << std::endl;
            }
            if (ifEvictingOrTransferringSelfStall)
                cachePtrs[sender]->isHalted = true;
            from = sender;
            to = 4;
            cyclesBusy = 100;
            typeOfNewLine = S;
            totalTraffic += 2 * (1 << cachePtrs[owner]->lineSizeBits);
            cachePtrs[owner]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
            cachePtrs[sender]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
        }
        else
        {
            if (DEBUG)
            {
                std::cout << "Read hit. Fetching from cache " << sender << " to owner." << std::endl;
            }
            if (ifEvictingOrTransferringSelfStall)
                cachePtrs[sender]->isHalted = true;
            from = sender;
            to = owner;
            cyclesBusy = 2 * (1 << (cachePtrs[sender]->lineSizeBits - 2));
            totalTraffic += 1 << cachePtrs[owner]->lineSizeBits;
            typeOfNewLine = S;
            cachePtrs[owner]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
            cachePtrs[sender]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
        }
    }
    else if (transaction.type == busTransactionType::RdX)
    {
        totalTransactions++;
        if (DEBUG)
        {
            std::cout << "Processing RdX transaction." << std::endl;
        }
        currentProcessing = transaction;
        busOwner = owner;
        cacheLineLabel labels[4];
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
        {
            if (cacheNumber != owner)
            {
                labels[cacheNumber] = cachePtrs[cacheNumber]->processSnoop(transaction).second.state;
            }
            else
            {
                labels[cacheNumber] = I;
            }
        }
        cacheLineLabel caseWriteMiss = I;
        unsigned sender = 5;
        for (unsigned i = 0; i < 4; i++)
        {
            if (labels[i] != I)
            {
                caseWriteMiss = labels[i];
                sender = i;
            }
        }

        if (caseWriteMiss != M)
        {
            if (DEBUG)
            {
                std::cout << "Write miss. Fetching from main memory." << std::endl;
            }
            from = 4;
            to = owner;
            cyclesBusy = 100;
            typeOfNewLine = E;
            totalTraffic += 1 << cachePtrs[owner]->lineSizeBits;
            cachePtrs[owner]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
        }
        else
        {
            if (DEBUG)
            {
                std::cout << "Write hit on modified line. Fetching from cache " << sender << " to main memory." << std::endl;
            }
            if (ifEvictingOrTransferringSelfStall)
                cachePtrs[sender]->isHalted = true;
            from = sender;
            to = 4;
            cyclesBusy = 100;
            typeOfNewLine = E;
            totalTraffic += 2 * (1 << cachePtrs[owner]->lineSizeBits);
            cachePtrs[owner]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
            cachePtrs[sender]->byteTraffic += 1 << cachePtrs[owner]->lineSizeBits;
        }
        cachePtrs[owner] -> invalidations++;
    }
}

void bus::transactionOver()
{
    if (DEBUG)
    {
        std::cout << "Transaction over. Resetting bus state." << std::endl;
    }
    if (ifEvictingOrTransferringSelfStall || currentOpIsEviction == false)
    {
        cachePtrs[busOwner]->isHalted = false;
        cachePtrs[busOwner]->fromCacheToBus = {busTransactionType::None, 0};
    }
    busOwner = 5;
    from = 5;
    to = 5;
    currentOpIsEviction = false;
    typeOfNewLine = I;
    currentProcessing = {busTransactionType::None, 0};
}
