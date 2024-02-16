#include "cachelab.h"
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// struct cache from line
typedef struct {
    bool valid;
    unsigned long long tag;
    int timeStamp;
} cacheLine;

typedef struct {
    cacheLine* lines;
} cacheSet;

typedef struct {
    cacheSet* sets;
} cache;

// define global variables
int s, E, b;
    /*
        s: number of sets in the cache ;
        e: number of lines in each set ;
        b: number of bytes in each line;
    */

bool h, v;  // help, verbose

char* t;    // trace file

int miss, hit, eviction;

int curTimeStamp;

// statement of functions
void printUsage();

void getParam(int argc, char* argv[]);

cache* initCache(int s, int E);

void releaseMem(cache* c, char* t);

void processFile(char* path, cache* c);

void accessCache(char op, unsigned long long addr, cache* c);

void accessCacheSet(unsigned long long matchTag, char op, cacheSet* set);

void lruUpdateSet(unsigned long long matchTag, cacheSet* set, int times);

void printUsage() {
    printf(
        "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
        "Options:\n"
        "  -h         Print this help message.\n"
        "  -v         Optional verbose flag.\n"
        "  -s <num>   Number of set index bits.\n"
        "  -E <num>   Number of lines per set.\n"
        "  -b <num>   Number of block offset bits.\n"
        "  -t <file>  Trace file.\n\n"
        "Examples:\n"
        "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
        "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void getParam(int argc, char* argv[]) {
    // parse the command line
    int opt;
    while (-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))) {
        switch (opt) {
            case 'h':
                h = true;
                break;
            case 'v':
                v = true;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                // allocate memory for name of trace file first
                t = (char*)calloc(1000, sizeof(char)); 
                strcpy(t, optarg);
                break;
            default:
                printUsage();
                break;
        }
    }
    if (h) {
        printUsage();
        exit(0);
    }
    if (s <= 0 || E <= 0 || b <= 0 || t == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage();
        exit(-1);
    }
}

cache* initCache(int s, int E) {
    // remember s is the exponential of sets
    int S = 1 << s;

    cache* Cache = (cache*)calloc(1, sizeof(cache));
    cacheSet* setArr = (cacheSet*)calloc(S, sizeof(cacheSet));
    Cache->sets = setArr;
    for (int i = 0; i < S; i++) {
        (setArr + i)->lines = (cacheLine*)calloc(E, sizeof(cacheLine));
    }
    return Cache;
}

void releaseMem(cache* Cache, char* t) {
    int S = 1 << s;
    for (int i = 0; i < S; i++) {
        cacheSet* set = Cache->sets + i;
        free(set->lines);
    }
    free(Cache->sets);
    free(Cache);
    free(t);
}

void processFile(char* path, cache* Cache) {
    FILE* fp = fopen(path, "r");

    if (fp == NULL) {
        printf("%s: No such file or directory\n", path);
        exit(-1);
    }

    char op;
    unsigned long long addr;
    int size;
    char strLine[2024]; // temporary string

    while (fgets(strLine, sizeof(strLine), fp)) {
        if (strLine[0] == 'I') continue;
        if (sscanf(strLine, " %c %llX,%d", &op, &addr, &size) < 3) {
            printf("%s: Invalid Argument %s", path, strLine);
            exit(-1);
        }
        if (v) printf("%c %llX,%d ", op, addr, size);
        accessCache(op, addr, Cache);
    }
    fclose(fp);
}

void accessCache(char op, unsigned long long addr, cache* Cache) {
    // get the tag, set, and offset
    int matchTag = addr >> (b + s);
    int matchSetAddr = (addr >> b) & ((1 << s) - 1);
    cacheSet* set = Cache->sets + matchSetAddr;
    accessCacheSet(matchTag, op, set);
}

void accessCacheSet(unsigned long long matchTag, char op, cacheSet* set) {
    switch (op) {
        case 'L':
            lruUpdateSet(matchTag, set, 1);
            break;
        case 'M':
            // two times: load and store
            lruUpdateSet(matchTag, set, 2);
            break;
        case 'S':
            lruUpdateSet(matchTag, set, 1);
            break;
        default:
            printf("Invalid op %c\n", op);
            exit(-1);
    }
}

void lruUpdateSet(unsigned long long matchTag, cacheSet* set, int times) {
    /*
        we don't use 2-dimentional array to store the cache,
        just allocate the space for each line and use the address to access it
    */
    int isMiss = 0, isHit = 0, isEvict = 0;
    
    // 
    for (int k = 0; k < times; k++) {
        // check if hit and update the time stamp
        for (int i = 0; i < E; i++) {
            cacheLine* line = set->lines + i;
            if (line->valid && matchTag == line->tag) {
                isHit++;
                if (v) printf("%s ", "hit ");
                line->timeStamp = curTimeStamp++;
                break;
            }
        }
        if (isHit) continue;

        // if not hit, check if evict
        isMiss++;
        if (v) printf("%s", "miss ");

        // check if full
        bool isFilled = false;
        for (int i = 0; i < E; i++) {
            cacheLine* line = set->lines + i;
            if (!line->valid) {
                line->tag = matchTag;
                line->valid = true;
                line->timeStamp = curTimeStamp++;
                isFilled = true;
                break;
            }
        }
        if (isFilled) continue;

        isEvict++;
        if (v) printf("%s", "eviction ");

        // if not full, then evict the oldest
        cacheLine* oldestLine;
        int minStamp = 0x7fffffff;
        for (int i = 0; i < E; i++) {
            cacheLine* line = set->lines + i;
            if (minStamp > line->timeStamp) {
                minStamp = line->timeStamp;
                oldestLine = line;
            }
        }
        oldestLine->tag = matchTag;
        oldestLine->timeStamp = curTimeStamp++;
    }
    if (v) printf("\n");
    hit += isHit;
    miss += isMiss;
    eviction += isEvict;
}

int main(int argc, char* argv[]) {
    getParam(argc, argv); 

    // no need to care for the block size
    cache* c = initCache(s, E);

    processFile(t, c);

    releaseMem(c, t);

    printSummary(hit, miss, eviction);

    return 0;
}
