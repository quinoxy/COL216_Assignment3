#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <map>
#include <utility>
#include "dll.hpp"
unsigned DEFAULT_CACHE_LINES_PER_SET = 2;
unsigned DEFAULT_LINE_SIZE = 32;
unsigned DEFAULT_LINE_SIZE_BITS = 5;
unsigned DEFAULT_SETS_PER_CACHE = 64;
unsigned DEFAULT_SET_BITS = 6;

enum cacheLineLabel {M,E,S,I};
enum busTransactionType{
    Rd,
    RdX,
    WriteInvalidate,
    None
};

struct busTransaction {
    busTransactionType type;
    unsigned value;
};

class cacheLine{
private:
    unsigned size_bits;

public:
    unsigned tag;
    cacheLineLabel state;
    bool valid;

    cacheLine(unsigned s);
};


class cacheSet{

    unsigned lineCount;
    std::vector<cacheLine> lines;
    unsigned lineSizeBits;
    unsigned currentCapacity;
    doublyLinkedList LRUMem;
    std::map<unsigned,doublyLinkedList::Node*> mapForLRU;

public:
    cacheSet(unsigned cacheLinesPerSet = DEFAULT_CACHE_LINES_PER_SET, unsigned lineSizeBits = DEFAULT_LINE_SIZE_BITS);
    std::pair<bool, cacheLine*> isMiss(unsigned tag, bool LRUUpdate);
    cacheLine addTag(unsigned tag, cacheLineLabel s);

};

class cache{
public:
    unsigned setCount;
    std::vector<cacheSet> sets;
    unsigned lineSizeBits;
    unsigned associativity;
    unsigned setBits;
    std::vector<std::pair<std::pair<unsigned int, bool>,bool>> instructions;
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
    cache(unsigned lineSizeBits=DEFAULT_LINE_SIZE_BITS, unsigned associativity=DEFAULT_CACHE_LINES_PER_SET, unsigned setBits=DEFAULT_SET_BITS, std::vector<std::pair<std::pair<unsigned int, bool>,bool>> instructions);
    void processCycle();
    void processInst();
    void processSnoop();

};

class Bus{
    unsigned state;
    unsigned cyclesBusy;


};
#endif