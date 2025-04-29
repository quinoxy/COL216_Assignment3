#include "cache.hpp"
#include "dll.hpp"
#include <iostream>

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
    : setCount(1<<setBits),lineSizeBits(lineSizeBits), associativity(associativity), setBits(setBits), instructions(instructions), isHalted(false), PC(0) ,readInstrs(0), executionCycles(0), idleCycles(0), misses(0), evictions(0), writebacks(0), invalidations(0), byteTraffic(0), arrivedMemBuffer(0), memArrivedInCycle(false), sendMemBuffer(0) {
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
    if(!isHalted){
        auto instruction = instructions[PC];

        unsigned address = instruction.first.first;
        bool isWrite = instruction.first.second;
        bool alreadyProcessed = instruction.second;
        instructions[PC].second = true;

        unsigned tag = address >> lineSizeBits;
        unsigned setIndex = (address >> lineSizeBits) & (setCount - 1);
        cacheSet& currentSet = sets[setIndex];

        bool miss = false;
        auto [isMiss, linePtr] = currentSet.isMiss(tag, true);

        if (isMiss) {
            miss=true;
        }

        if(!alreadyProcessed){
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
                    isHalted = true;
                }
                else{
                    PC++;
                }

            }
            else if (miss && !isWrite){
                fromCacheToBus = {busTransactionType::Rd, address};
                isHalted = true;
            }
            else if (miss && isWrite){

                fromCacheToBus = {busTransactionType::RdX, address};
                isHalted = true;
            }
        }
        else {
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
                    std::cout<< "There is a major error... Exiting...." << std::endl;
                    exit(1);
                }
                else{
                    PC++;
                }

            }
            else if (miss){
                std::cout<< "There is a major error... Exiting...." << std::endl;
                exit(1);
                
            }
        }
    }
}

std::pair<bool, cacheLine> cache::processSnoop(busTransaction trs){
    unsigned tag = trs.value >> lineSizeBits;
    unsigned setIndex = (trs.value >> lineSizeBits) & (setCount - 1);
    cacheSet& currentSet = sets[setIndex];
    auto [isMiss, linePtr] = currentSet.isMiss(tag, false);
    if(isMiss){
        return {false, cacheLine(0)};
    }
    cacheLine ret = *linePtr;

    if (trs.type == busTransactionType::Rd) {

        linePtr->state = S;
    } else if (trs.type == busTransactionType::RdX) {
        linePtr -> state = I;

    } else if (trs.type == busTransactionType::WriteInvalidate) {
        linePtr -> state = I;
    }
    return {true, ret};
    
}


bus::bus(cache* cachePtr0, cache* cachePtr1,cache* cachePtr2, cache* cachePtr3) :
    from(-1), to(-1), cyclesBusy(0), totalTransactions(0), totalTraffic(0), currentOpIsEviction(false), currentProcessing({busTransactionType::None, 0}), busOwner(-1), typeOfNewLine(I) {
    
    cachePtrs[0] = cachePtr0;
    cachePtrs[1] = cachePtr1;
    cachePtrs[2] = cachePtr2;
    cachePtrs[3] = cachePtr3;
}


void bus::runForACycle(){

    if(cyclesBusy>1){
        cyclesBusy--;
        return;
    }else if(cyclesBusy==1){
        cyclesBusy = 0;
        if (currentProcessing.type == busTransactionType::Rd) {
            if (to!=4){
                if (from!=4){
                    cachePtrs[from] -> isHalted = false;
                }
                unsigned tag = currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits;
                unsigned setIndex = (currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits) & (cachePtrs[busOwner]->setCount - 1);
                cacheSet& currentSet = cachePtrs[busOwner]->sets[setIndex];
                cacheLine result = currentSet.addTag(currentProcessing.value, typeOfNewLine);
                if (result.state==M){
                    from = busOwner;
                    to = 4;
                    currentOpIsEviction = true;
                    cyclesBusy = 100;   
                } else{
                    transactionOver();
                }
            } else if(from == busOwner){
                transactionOver();
            }else{
                to = busOwner;
                cyclesBusy = 2 * (1 << cachePtrs[busOwner]->lineSizeBits);
                typeOfNewLine = S;
                currentOpIsEviction = false;

            }
        }else if (currentProcessing.type == busTransactionType::RdX) {
            if (from == 4){
                unsigned tag = currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits;
                unsigned setIndex = (currentProcessing.value >> cachePtrs[busOwner]->lineSizeBits) & (cachePtrs[busOwner]->setCount - 1);
                cacheSet& currentSet = cachePtrs[busOwner]->sets[setIndex];
                cacheLine result = currentSet.addTag(currentProcessing.value, typeOfNewLine);
                if (result.state == M) {
                    from = busOwner;
                    to = 4;
                    currentOpIsEviction = true;
                    cyclesBusy = 100;   
                } else {
                    transactionOver();
                }
            } else if (from == busOwner){
                transactionOver();
            } else {
                cachePtrs[from] -> isHalted = false;
                from = 4;
                to = busOwner;
                cyclesBusy = 100; 
            }

        }
    }



    if (cyclesBusy!=0){
        return;
    }
    //arbitration logic
    busTransaction transaction;
    unsigned owner = -1;
    for (unsigned cacheNumber =0; cacheNumber<4; cacheNumber++){
        if (cachePtrs[cacheNumber]->fromCacheToBus.type != busTransactionType::None) {
            busTransaction transaction = cachePtrs[cacheNumber]->fromCacheToBus;
            cachePtrs[cacheNumber]->fromCacheToBus = {busTransactionType::None, 0};//might want to remove this line later
            owner = cacheNumber;
        }
    }
    if (transaction.type == busTransactionType::None){
        return;
    }else if (transaction.type == busTransactionType::WriteInvalidate){
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++) {
            if (cacheNumber!=owner){
                cachePtrs[cacheNumber]->processSnoop(transaction);
            }
        }
        cachePtrs[owner]->isHalted = false;
        cachePtrs[owner]->PC++;
    } else if (transaction.type == busTransactionType::Rd){
        //collect the states of the cacheLines returned by snooping on the 3 caches other than the owner
        currentProcessing = transaction;
        busOwner = owner;
        cacheLineLabel labels[4];
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++) {
            if (cacheNumber != owner) {
                labels[cacheNumber] = cachePtrs[cacheNumber]->processSnoop(transaction).second.state;
            }
            else{
                labels[cacheNumber]=I;
            }
        }
        cacheLineLabel caseReadMiss = I;
        unsigned sender = -1;
        for(unsigned i= 0; i<4;i++){
            if (labels[i]!=I){
                caseReadMiss = labels[i];
                sender = i;
            }
        }
        
        if (caseReadMiss == I){
            from = 4 ; //(main memory)
            to = owner;
            cyclesBusy = 100;
            typeOfNewLine = E;
        }
        else if (caseReadMiss == M){
            cachePtrs[sender]->isHalted = true;
            from = sender;
            to = 4;
            cyclesBusy = 100;
            
            typeOfNewLine = S;
        }
        else{
            cachePtrs[sender]->isHalted = true;
            from = sender;
            to = owner;
            cyclesBusy = 2 * (1 << cachePtrs[sender]->lineSizeBits);
            typeOfNewLine = S;
        }
    }else if (transaction.type == busTransactionType::RdX){
        currentProcessing = transaction;
        busOwner = owner;
        cacheLineLabel labels[4];
        for (unsigned cacheNumber = 0; cacheNumber < 4; cacheNumber++) {
            if (cacheNumber != owner) {
                labels[cacheNumber] = cachePtrs[cacheNumber]->processSnoop(transaction).second.state;
            }
            else{
                labels[cacheNumber]=I;
            }
        }
        cacheLineLabel caseWriteMiss = I;
        unsigned sender = -1;
        for(unsigned i= 0; i<4;i++){
            if (labels[i]!=I){
                caseWriteMiss = labels[i];
                sender = i;
            }
        }

        if (caseWriteMiss != M) {
            from = 4; // (main memory)
            to = owner;
            cyclesBusy = 100;
            typeOfNewLine = E;
        } else {
            cachePtrs[sender]->isHalted = true;
            from = sender;
            to = 4; // (main memory)
            cyclesBusy = 100;
            typeOfNewLine = E;
        }

        
        
    }

}

void bus::transactionOver(){
    cachePtrs[busOwner] -> isHalted = false;
    busOwner = -1;
    from = -1;
    to = -1;
    currentOpIsEviction = false;
    typeOfNewLine = I;
    currentProcessing = {busTransactionType::None, 0};
}
