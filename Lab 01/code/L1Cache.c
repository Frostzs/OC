#include "L1Cache.h"

uint8_t L1Cache[L1_SIZE]; /* [0] + [64] + [128] */
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache CacheInfo;

/**************** Time Manipulation ***************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}

/*********************** L1 cache *************************/

void initCache() { 
    CacheInfo.init = 0;
  }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, Tag, AddressMemory, offset;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (CacheInfo.init == 0) {
    
    for (int i = 0; i < L1_LINES - 1; i++)
    {
      CacheInfo.line[i].Valid = 0;
    }

    CacheInfo.init = 1;
  }
  
  // tirar os bits do offset
  AddressMemory = address >> 6; // Log2(BLOCK_SIZE) = 6 

  offset = address & (BLOCK_SIZE - 1); // mask is 0011 1111
  index = AddressMemory & ((L1_SIZE / BLOCK_SIZE) - 1); // mask is the number of lines (256) (1111 1111)
  Tag = address >> 14; // log2(L1_SIZE) = 14 OR 8 + 6 = 14

  CacheLine *Line = &CacheInfo.line[index];

  /* access Cache*/

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    accessDRAM(AddressMemory, TempBlock, MODE_READ); // get new block from DRAM

    if ((Line->Valid) && (Line->Dirty)) { // line has dirty block
      AddressMemory = Line->Tag << 6;        // get address of the block in memory
      accessDRAM(AddressMemory, &(L1Cache[0]), MODE_WRITE); // then write back old block
    }

    memcpy(&(L1Cache[0]), TempBlock,
           BLOCK_SIZE); // copy new block to cache line
    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Index = index;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    if (0 == (address % 8)) { // even word on block
      memcpy(data, &(L1Cache[0]), WORD_SIZE);
    } else { // odd word on block
      memcpy(data, &(L1Cache[WORD_SIZE]), WORD_SIZE);
    }
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    if (!(address % 8)) {   // even word on block
      memcpy(&(L1Cache[0]), data, WORD_SIZE);
    } else { // odd word on block
      memcpy(&(L1Cache[WORD_SIZE]), data, WORD_SIZE);
    }
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
