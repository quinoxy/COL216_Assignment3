#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <map>
#include <utility>
#include "dll.hpp"
int DEFAULT_CACHE_LINES_PER_SET = 2;
int DEFAULT_LINE_SIZE = 32;
int DEFAULT_LINE_SIZE_BITS = 5;
int DEFAULT_SETS_PER_CACHE = 64;
int DEFAULT_SET_BITS = 6;

enum cacheLineLabel {M,E,S,I};

class cacheLine{
private:
    int size_bits;

public:
    int tag;
    cacheLineLabel state;
    bool valid;

    cacheLine(int s);
};


class cacheSet{

    int lineCount;
    std::vector<cacheLine> lines;
    int lineSizeBits;
    int currentCapacity;
    doublyLinkedList LRUMem;
    std::map<int,doublyLinkedList::Node*> mapForLRU;


    cacheSet(int cacheLinesPerSet = DEFAULT_CACHE_LINES_PER_SET, int lineSizeBits = DEFAULT_LINE_SIZE_BITS);
    bool isMiss(int tag);
    void addTag(int tag, cacheLineLabel s);

};

class cache{
    int setCount;
    std::vector<cacheSet> sets;
    int lineSizeBits;
    int associativity;
    int setBits;
public:
    cache(int lineSizeBits=DEFAULT_LINE_SIZE_BITS, int associativity=DEFAULT_CACHE_LINES_PER_SET, int setBits=DEFAULT_SET_BITS);

};
#endif