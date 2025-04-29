#include "cache.hpp"
#include "dll.hpp"

cacheLine::cacheLine(unsigned s): tag(0), valid(false), state(I), size_bits(s){}

cacheSet::cacheSet(unsigned cacheLinesPerSet, unsigned lineSizeBits): lineCount(cacheLinesPerSet), lineSizeBits(lineSizeBits), currentCapacity(0), LRUMem() {

    for (unsigned i = 0; i < lineCount; ++i) {
        lines.push_back(cacheLine(lineSizeBits));
    }
}

std::pair<bool, cacheLine*> cacheSet::isMiss(unsigned tag, bool LRUUpdate){
    
    for (auto& line : lines) {
        if (line.valid && line.tag == tag && line.state!=I) {
            if (LRUUpdate) {
                doublyLinkedList::Node * newNode = mapForLRU[tag];
                LRUMem.deleteNode(newNode);
                LRUMem.insertAtHead(newNode);
            }
            return {false, &line};  
        }
    }
    return {true,nullptr};
}

cacheLine cacheSet::addTag(unsigned tag, cacheLineLabel s){
    //check if there are any invalid lines in the set, using indexes and place tag there
    
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!lines[i].valid || lines[i].state == I) {
            cacheLine initialLine = lines[i];
            lines[i].tag = tag;
            lines[i].valid = true;
            lines[i].state = s;

            mapForLRU[tag] = new doublyLinkedList::Node(i);
            LRUMem.insertAtHead(mapForLRU[tag]);
            return initialLine;

        }else{

            doublyLinkedList::Node* toBeDeleted = LRUMem.tail->prev;
            cacheLine toBeDeletedLine = lines[toBeDeleted->data];
            mapForLRU.erase(lines[toBeDeleted->data].tag);
            LRUMem.deleteNode(toBeDeleted);

            unsigned i = toBeDeleted->data;
            lines[i].tag = tag;
            lines[i].state = s;
            lines[i].valid = true;

            mapForLRU[tag] = new doublyLinkedList::Node(i);
            LRUMem.insertAtHead(mapForLRU[tag]);

            return toBeDeletedLine;

        }
    }
}

cache::cache(unsigned lineSizeBits, unsigned associativity, unsigned setBits, std::vector<std::pair<std::pair<unsigned int, bool>,bool>> instructions)
    : setCount(1<<setBits),lineSizeBits(lineSizeBits), associativity(associativity), setBits(setBits), instructions(instructions), PC(0) ,readInstrs(0), executionCycles(0), idleCycles(0), misses(0), evictions(0), writebacks(0), invalidations(0), byteTraffic(0), arrivedMemBuffer(0), memArrivedInCycle(false), sendMemBuffer(0) {
    for (unsigned i = 0; i < setCount; ++i) {
        sets.emplace_back(associativity, lineSizeBits);
    }    
    fromCacheToBus = {busTransactionType::None, 0}; 
    snoop = {busTransactionType::None, 0};
    

}

void cache::processInst(){
    if (PC >= instructions.size()) {
        return;
    }

    auto instruction = instructions[PC];
    unsigned address = instruction.first.first;
    bool isWrite = instruction.first.second;
    bool alreadyProcessed = instruction.second;
    if(!alreadyProcessed){
        instructions[PC].second = true;
        unsigned tag = address >> lineSizeBits;
        unsigned setIndex = (address >> lineSizeBits) & (setCount - 1);
        cacheSet& currentSet = sets[setIndex];
        bool miss = false;
        auto [isMiss, linePtr] = currentSet.isMiss(tag, true);
        if (isMiss) {
            miss=true;
        }
        if(!miss && !isWrite){
            PC++;
            return;
        }
        else if(!miss && isWrite){
            if (linePtr->state = E){
                linePtr->state=M;
                PC++;
            }
            else if(linePtr->state = S){
                linePtr->state = M;
                fromCacheToBus = {busTransactionType::WriteInvalidate, linePtr->tag};
            }
            else{
                PC++;
            }

        }
        else if (miss && !isWrite){
            fromCacheToBus = {busTransactionType::Rd, address};
        }
        else if (miss && isWrite){

            fromCacheToBus = {busTransactionType::RdX, address};
        }


    }else{
        
    }
}


