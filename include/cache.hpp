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


    cacheSet(unsigned cacheLinesPerSet = DEFAULT_CACHE_LINES_PER_SET, unsigned lineSizeBits = DEFAULT_LINE_SIZE_BITS);
    bool isMiss(unsigned tag);
    void addTag(unsigned tag, cacheLineLabel s);

};

class cache{
    unsigned setCount;
    std::vector<cacheSet> sets;
    unsigned lineSizeBits;
    unsigned associativity;
    unsigned setBits;
public:
    cache(unsigned lineSizeBits=DEFAULT_LINE_SIZE_BITS, unsigned associativity=DEFAULT_CACHE_LINES_PER_SET, unsigned setBits=DEFAULT_SET_BITS);

};
#endif