#include "cache.hpp"
#include "dll.hpp"

cacheLine::cacheLine(unsigned s): tag(0), valid(false), state(I), size_bits(s){}

cacheSet::cacheSet(unsigned cacheLinesPerSet, unsigned lineSizeBits): lineCount(cacheLinesPerSet), lineSizeBits(lineSizeBits), currentCapacity(0), LRUMem() {

    for (unsigned i = 0; i < lineCount; ++i) {
        lines.push_back(cacheLine(lineSizeBits));
    }
}

bool cacheSet::isMiss(unsigned tag){
    
    for (const auto& line : lines) {
        if (line.valid && line.tag == tag) {
            doublyLinkedList::Node * newNode = mapForLRU[tag];
            LRUMem.deleteNode(newNode);
            LRUMem.insertAtHead(newNode);
            return false;  
        }
    }
    return true;
}

void cacheSet::addTag(unsigned tag, cacheLineLabel s){
    //check if there are any invalid lines in the set, using indexes and place tag there
    
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!lines[i].valid || lines[i].state == I) {

            lines[i].tag = tag;
            lines[i].valid = true;
            lines[i].state = s;

            mapForLRU[tag] = new doublyLinkedList::Node(i);
            LRUMem.insertAtHead(mapForLRU[tag]);
            return;

        }else{

            doublyLinkedList::Node* toBeDeleted = LRUMem.tail->prev;

            mapForLRU.erase(lines[toBeDeleted->data].tag);
            LRUMem.deleteNode(toBeDeleted);

            unsigned i = toBeDeleted->data;
            lines[i].tag = tag;
            lines[i].state = s;
            lines[i].valid = true;

            mapForLRU[tag] = new doublyLinkedList::Node(i);
            LRUMem.insertAtHead(mapForLRU[tag]);

            return;

        }
    }
}

cache::cache(unsigned lineSizeBits, unsigned associativity, unsigned setBits) 
    : setCount(1<<setBits),lineSizeBits(lineSizeBits), associativity(associativity), setBits(setBits) {
    for (unsigned i = 0; i < setCount; ++i) {
        sets.emplace_back(associativity, lineSizeBits);
    }        
}
