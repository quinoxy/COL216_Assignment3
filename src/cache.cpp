#include "cache.hpp"
#include "dll.hpp"
#include <iostream>

const unsigned DEFAULT_CACHE_LINES_PER_SET = 4;
const unsigned DEFAULT_LINE_SIZE = 64;
const unsigned DEFAULT_LINE_SIZE_BITS = 6;
const unsigned DEFAULT_SETS_PER_CACHE = 64;
const unsigned DEFAULT_SET_BITS = 6;
const unsigned IF_EVICT_OR_TRANSFER_STALL = true;

cacheLine::cacheLine(unsigned s) : tag(0), state(I), size_bits(s) {}

cacheSet::cacheSet(unsigned cacheLinesPerSet, unsigned lineSizeBits) : lineCount(cacheLinesPerSet), lineSizeBits(lineSizeBits), currentCapacity(0), LRUMem()
{
    std::cout << "Initializing cacheSet with " << cacheLinesPerSet << " lines and line size bits " << lineSizeBits << std::endl;
    for (unsigned i = 0; i < lineCount; ++i)
    {
        lines.push_back(cacheLine(lineSizeBits));
    }
}

std::pair<bool, cacheLine *> cacheSet::isMiss(unsigned tag, bool LRUUpdate)
{
    std::cout << "Checking for tag " << tag << " in cacheSet" << std::endl;
    for (auto &line : lines)
    {
        if (line.tag == tag && line.state != I)
        {
            std::cout << "Cache hit for tag " << tag << std::endl;
            if (LRUUpdate)
            {
                doublyLinkedList::Node *newNode = mapForLRU[tag];
                LRUMem.deleteNode(newNode);
                LRUMem.insertAtHead(newNode);
            }
            return {false, &line};
        }
    }
    std::cout << "Cache miss for tag " << tag << std::endl;
    return {true, nullptr};
}

cacheLine cacheSet::addTag(unsigned tag, cacheLineLabel s)
{
    std::cout << "Adding tag " << tag << " with state " << s << " to cacheSet" << std::endl;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].state == I)
        {
            cacheLine initialLine = lines[i];
            lines[i].tag = tag;
            lines[i].state = s;

            mapForLRU[tag] = new doublyLinkedList::Node(i);
            LRUMem.insertAtHead(mapForLRU[tag]);
            return initialLine;
        }
    }

    doublyLinkedList::Node *toBeDeleted = LRUMem.tail->prev;
    cacheLine toBeDeletedLine = lines[toBeDeleted->data];
    mapForLRU.erase(lines[toBeDeleted->data].tag);
    LRUMem.deleteNode(toBeDeleted);

    unsigned i = toBeDeleted->data;
    lines[i].tag = tag;
    lines[i].state = s;

    mapForLRU[tag] = new doublyLinkedList::Node(i);
    LRUMem.insertAtHead(mapForLRU[tag]);

    std::cout << "Evicted line with tag " << toBeDeletedLine.tag << " to add new tag " << tag << std::endl;
    return toBeDeletedLine;
}

cache::cache(unsigned lineSizeBits, unsigned associativity, unsigned setBits, std::vector<std::pair<std::pair<unsigned int, bool>, bool>> instructions)
    : setCount(1 << setBits), lineSizeBits(lineSizeBits), associativity(associativity), setBits(setBits), instructions(instructions), isHalted(false), PC(0), readInstrs(0), executionCycles(0), idleCycles(0), misses(0), evictions(0), writebacks(0), invalidations(0), byteTraffic(0), arrivedMemBuffer(0), memArrivedInCycle(false), sendMemBuffer(0)
{
    std::cout << "Initializing cache with " << setCount << " sets, associativity " << associativity << ", and line size bits " << lineSizeBits << std::endl;
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
        std::cout << "All instructions processed. Halting." << std::endl;
        return;
    }
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

        std::cout << "Processing instruction at PC " << PC << ": address=" << address << ", isWrite=" << isWrite << std::endl;

        bool miss = false;
        auto [isMiss, linePtr] = currentSet.isMiss(tag, true);

        if (isMiss)
        {
            miss = true;
            std::cout << "Cache miss for address " << address << std::endl;
        }
        else
        {
            std::cout << "Cache hit for address " << address << std::endl;
        }

        if (!alreadyProcessed)
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
                fromCacheToBus = {busTransactionType::Rd, address};
                isHalted = true;
            }
            else if (miss && isWrite)
            {
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
                    std::cout << "There is a major error... Exiting...." << std::endl;
                    exit(1);
                }
                else
                {
                    PC++;
                }
            }
            else if (miss)
            {
                std::cout << "There is a major error... Exiting...." << std::endl;
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
    auto [isMiss, linePtr] = currentSet.isMiss(tag, false);
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
    }
    else if (trs.type == busTransactionType::WriteInvalidate)
    {
        linePtr->state = I;
    }
    return {true, ret};
}

bus::bus(cache *cachePtr0, cache *cachePtr1, cache *cachePtr2, cache *cachePtr3) : from(-1), to(-1), cyclesBusy(0), totalTransactions(0), totalTraffic(0), currentOpIsEviction(false), currentProcessing({busTransactionType::None, 0}), busOwner(-1), typeOfNewLine(I), ifEvictingOrTransferringSelfStall(IF_EVICT_OR_TRANSFER_STALL)
{
    std::cout << "Initializing bus with 4 caches" << std::endl;
    cachePtrs[0] = cachePtr0;
    cachePtrs[1] = cachePtr1;
    cachePtrs[2] = cachePtr2;
    cachePtrs[3] = cachePtr3;
}

void bus::runForACycle()
{
    std::cout << "Running bus for a cycle. Cycles busy: " << cyclesBusy << std::endl;

    if (cyclesBusy > 1)
    {
        std::cout << "Bus is busy. Decrementing cyclesBusy to " << (cyclesBusy - 1) << std::endl;
        cyclesBusy--;
        
    }
    else if (cyclesBusy == 1)
    {
        std::cout << "Bus is completing its current operation." << std::endl;
        cyclesBusy = 0;
        if (currentProcessing.type == busTransactionType::Rd)
        {
            std::cout << "Processing Rd transaction. From: " << from << ", To: " << to << std::endl;
            if (to != 4)
            {
                if (from != 4)
                {
                    std::cout << "Unhalting cache " << from << std::endl;
                    if(ifEvictingOrTransferringSelfStall){if(cachePtrs[from]->fromCacheToBus.type == busTransactionType::None) {cachePtrs[from]->isHalted = false;}}
                }
                unsigned tag = currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits;
                unsigned setIndex = (currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits) & (cachePtrs[busOwner]->setCount - 1);
                cacheSet &currentSet = cachePtrs[busOwner]->sets[setIndex];
                cacheLine result = currentSet.addTag(tag, typeOfNewLine);
                if (result.state == M)
                {
                    std::cout << "Evicting modified line. From: " << busOwner << " to main memory." << std::endl;
                    from = busOwner;
                    to = 4;
                    currentOpIsEviction = true;
                    cyclesBusy = 100;
                    if(!ifEvictingOrTransferringSelfStall) {cachePtrs[busOwner]->isHalted = false;
                        cachePtrs[busOwner] -> fromCacheToBus = {busTransactionType::None, 0};
                    }
                }
                else
                {
                    std::cout << "Transaction completed without eviction." << std::endl;
                    transactionOver();
                }
            }
            else if (from == busOwner)
            {
                std::cout << "Transaction completed. Returning to owner." << std::endl;
                transactionOver();
            }
            else
            {
                std::cout << "Fetching data from main memory to owner." << std::endl;
                to = busOwner;
                cyclesBusy = 2 * (1 << cachePtrs[busOwner]->lineSizeBits);
                typeOfNewLine = S;
                currentOpIsEviction = false;
            }
        }
        else if (currentProcessing.type == busTransactionType::RdX)
        {
            std::cout << "Processing RdX transaction. From: " << from << ", To: " << to << std::endl;
            if (from == 4)
            {
                unsigned tag = currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits;
                unsigned setIndex = (currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits) & (cachePtrs[busOwner]->setCount - 1);
                cacheSet &currentSet = cachePtrs[busOwner]->sets[setIndex];
                cacheLine result = currentSet.addTag(tag, typeOfNewLine);
                if (result.state == M)
                {
                    std::cout << "Evicting modified line. From: " << busOwner << " to main memory." << std::endl;
                    from = busOwner;
                    to = 4;
                    currentOpIsEviction = true;
                    cyclesBusy = 100;
                    if(!ifEvictingOrTransferringSelfStall) {cachePtrs[busOwner]->isHalted = false;
                        cachePtrs[busOwner] -> fromCacheToBus = {busTransactionType::None, 0};
                    }
                }
                else
                {
                    std::cout << "Transaction completed without eviction." << std::endl;
                    transactionOver();
                }
            }
            else if (from == busOwner)
            {
                std::cout << "Transaction completed. Returning to owner." << std::endl;
                transactionOver();
            }
            else
            {
                std::cout << "Fetching data from main memory to owner." << std::endl;
                if(ifEvictingOrTransferringSelfStall){if (cachePtrs[from]->fromCacheToBus.type == busTransactionType::None) {cachePtrs[from]->isHalted = false;}}
                from = 4;
                to = busOwner;
                cyclesBusy = 100;
            }
        }
    }

    for (unsigned cacheNumber = 0; cacheNumber<4; cacheNumber ++){
        cachePtrs[cacheNumber]->processInst();
    }

    if (cyclesBusy != 0)
    {
        std::cout << "Bus is still busy. Exiting cycle." << std::endl;
        return;
    }

    std::cout << "Arbitrating for new transaction." << std::endl;
    busTransaction transaction;
    unsigned owner = -1;
    for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
    {
        if (cachePtrs[cacheNumber]->fromCacheToBus.type != busTransactionType::None)
        {
            transaction = cachePtrs[cacheNumber]->fromCacheToBus;
            cachePtrs[cacheNumber]->fromCacheToBus = {busTransactionType::None, 0};
            owner = cacheNumber;

            std::cout << "Transaction found from cache " << cacheNumber << ": type=" << transaction.type << ", value=" << transaction.value << std::endl;
            break;
        }
    }
    if (transaction.type == busTransactionType::None)
    {
        std::cout << "No transactions to process. Exiting cycle." << std::endl;
        return;
    }
    else if (transaction.type == busTransactionType::WriteInvalidate)
    {
        std::cout << "Processing WriteInvalidate transaction." << std::endl;
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++)
        {
            if (cacheNumber != owner)
            {
                cachePtrs[cacheNumber]->processSnoop(transaction);
            }
        }
        cachePtrs[owner]->isHalted = false;
        cachePtrs[owner]->fromCacheToBus = {busTransactionType::None, 0};
        cachePtrs[owner]->PC++;
    }
    else if (transaction.type == busTransactionType::Rd)
    {
        std::cout << "Processing Rd transaction." << std::endl;
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
        unsigned sender = -1;
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
            std::cout << "Read miss. Fetching from main memory." << std::endl;
            from = 4;
            to = owner;
            cyclesBusy = 100;
            typeOfNewLine = E;
        }
        else if (caseReadMiss == M)
        {
            std::cout << "Read hit on modified line. Fetching from cache " << sender << " to main memory." << std::endl;
            if(ifEvictingOrTransferringSelfStall)cachePtrs[sender]->isHalted = true;
            from = sender;
            to = 4;
            cyclesBusy = 100;
            typeOfNewLine = S;
        }
        else
        {
            std::cout << "Read hit. Fetching from cache " << sender << " to owner." << std::endl;
            if(ifEvictingOrTransferringSelfStall)cachePtrs[sender]->isHalted = true;
            from = sender;
            to = owner;
            cyclesBusy = 2 * (1 << cachePtrs[sender]->lineSizeBits);
            typeOfNewLine = S;
        }
    }
    else if (transaction.type == busTransactionType::RdX)
    {
        std::cout << "Processing RdX transaction." << std::endl;
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
        unsigned sender = -1;
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
            std::cout << "Write miss. Fetching from main memory." << std::endl;
            from = 4;
            to = owner;
            cyclesBusy = 100;
            typeOfNewLine = E;
        }
        else
        {
            std::cout << "Write hit on modified line. Fetching from cache " << sender << " to main memory." << std::endl;
            if(ifEvictingOrTransferringSelfStall)cachePtrs[sender]->isHalted = true;
            from = sender;
            to = 4;
            cyclesBusy = 100;
            typeOfNewLine = E;
        }
    }
}

void bus::transactionOver()
{
    std::cout << "Transaction over. Resetting bus state." << std::endl;
    if(ifEvictingOrTransferringSelfStall || currentOpIsEviction==false){
        cachePtrs[busOwner]->isHalted = false;
        cachePtrs[busOwner]->fromCacheToBus = {busTransactionType::None, 0};
    }
    busOwner = -1;
    from = -1;
    to = -1;
    currentOpIsEviction = false;
    typeOfNewLine = I;
    currentProcessing = {busTransactionType::None, 0};
}
