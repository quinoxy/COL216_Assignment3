#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <map>
#include <utility>
#include "dll.hpp"
extern const unsigned DEFAULT_CACHE_LINES_PER_SET;
extern const unsigned DEFAULT_LINE_SIZE;
extern const unsigned DEFAULT_LINE_SIZE_BITS;
extern const unsigned DEFAULT_SETS_PER_CACHE;
extern const unsigned DEFAULT_SET_BITS;
extern const bool IF_EVICT_OR_TRANSFER_STALL;
extern const bool DEBUG;
enum cacheLineLabel
{
    M,
    E,
    S,
    I
};
enum busTransactionType
{
    Rd,
    RdX,
    WriteInvalidate,
    None
};

struct busTransaction
{
    busTransactionType type;
    unsigned value;
};

class cacheLine
{
private:
    unsigned size_bits;

public:
    unsigned tag;
    cacheLineLabel state;

    cacheLine(unsigned s);
};

class cacheSet
{

    unsigned lineCount;
    std::vector<cacheLine> lines;
    unsigned lineSizeBits;
    unsigned currentCapacity;
    doublyLinkedList LRUMem;
    std::map<unsigned, doublyLinkedList::Node *> mapForLRU;

public:
    cacheSet(unsigned cacheLinesPerSet = DEFAULT_CACHE_LINES_PER_SET, unsigned lineSizeBits = DEFAULT_LINE_SIZE_BITS);
    std::pair<bool, cacheLine *> isMiss(unsigned tag, bool LRUUpdate);
    cacheLine addTag(unsigned tag, cacheLineLabel s);
};

class cache
{
public:
    unsigned setCount;
    std::vector<cacheSet> sets;
    unsigned lineSizeBits;
    unsigned associativity;
    unsigned setBits;
    std::vector<std::pair<std::pair<unsigned int, bool>, bool>> instructions;
    bool isHalted;
    unsigned readInstrs;
    unsigned PC;
    unsigned executionCycles;
    unsigned idleCycles;
    unsigned misses;
    unsigned evictions;
    unsigned writebacks;
    unsigned invalidations;
    unsigned byteTraffic;
    busTransaction fromCacheToBus;
    unsigned arrivedMemBuffer;
    bool memArrivedInCycle;
    unsigned sendMemBuffer;
    busTransaction snoop;

public:
    cache(unsigned lineSizeBits = DEFAULT_LINE_SIZE_BITS, unsigned associativity = DEFAULT_CACHE_LINES_PER_SET, unsigned setBits = DEFAULT_SET_BITS, std::vector<std::pair<std::pair<unsigned int, bool>, bool>> instructions = {});
    void processCycle();
    void processInst();
    std::pair<bool, cacheLine> processSnoop(busTransaction trs);
};

class bus
{
public:
    unsigned from;       // where to where write is occuring
    unsigned to;         // where to where data transfer is occuring
    cache *cachePtrs[4]; // pointers to all the caches
    unsigned cyclesBusy; // number of cycles the bus is busy
    unsigned totalTransactions;
    unsigned totalTraffic;
    bool currentOpIsEviction;
    busTransaction currentProcessing;
    int busOwner;
    cacheLineLabel typeOfNewLine;
    bool ifEvictingOrTransferringSelfStall;

    bus(cache *cachePtr0, cache *cachePtr1, cache *cachePtr2, cache *cachePtr3);
    void runForACycle();
    void transactionOver();
};
#endif