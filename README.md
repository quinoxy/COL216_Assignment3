make:
Makes L1 simulate which can be run with respective parameters

make report:
Runs the latex file to generate the report that is present

make clean:
cleans

We have implemented 2 versions of this, to switch between them, go to cache.cpp, where at the top where globals have been declared you will find an IF_EVICT_OR_TRANSFER_STALL. Toggle that to toggle between versions, its purpose is explained in the report.