#include "L2Cache.h"

uint8_t L1Cache[L1_SIZE]; /* [0] + [64] + [128] + ... + [16320] */
uint8_t L2Cache[L2_SIZE]; /* [0] + [64] + [128] + ... + [32704]*/
uint8_t DRAM[DRAM_SIZE];
uint32_t time;
Cache CacheInfoL1;
Cache CacheInfoL2;

void accessL2(uint32_t indexL1, uint32_t offset, uint32_t AddressMemory, uint32_t address, uint8_t *data, uint32_t mode);

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
    CacheInfoL1.init = 0;
    CacheInfoL2.init = 0;
  }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t index, Tag, AddressMemory, offset;

  /* init cache */
  if (CacheInfoL1.init == 0) {
    for (int i = 0; i < L2_LINES - 1; i++)
    {
        if (i < L1_LINES - 1)
        {
            CacheInfoL1.line[i].Valid = 0;
        }
        CacheInfoL2.line[i].Valid = 0;
    }
    CacheInfoL1.init = 1;
  }

  // tirar os bits do offset
  AddressMemory = address >> 6; // Log2(BLOCK_SIZE) = 6 

  offset = address & (BLOCK_SIZE - 1); // mask is 0011 1111
  index = AddressMemory & ((L1_SIZE / BLOCK_SIZE) - 1); // mask is the number of lines (256) (1111 1111)
  Tag = address >> 14; // log2(L1_SIZE) = 14 OR 8 + 6 = 14

  CacheLine *Line = &CacheInfoL1.line[index];

  /* access Cache*/

  if (!Line->Valid || Line->Tag != Tag) {         // if block not present - miss
    
    if ((Line->Valid) && (Line->Dirty))
    {                                                                                   // line has dirty block
      AddressMemory = Line->Tag << 6;                                                   // get address of the block in memory
      accessDRAM(AddressMemory, &(L1Cache[index * BLOCK_SIZE + offset]), MODE_WRITE); // then write back old block
    }

    
    accessL2(index, offset, AddressMemory, address, data, mode);

    Line->Valid = 1;
    Line->Tag = Tag;
    Line->Index = index;
    Line->Dirty = 0;
  } // if miss, then replaced with the correct block

  if (mode == MODE_READ) {    // read data from cache line
    memcpy(data, &(L1Cache[index*BLOCK_SIZE + offset]), WORD_SIZE);
    time += L1_READ_TIME;
  }

  if (mode == MODE_WRITE) { // write data from cache line
    memcpy(&(L1Cache[index*BLOCK_SIZE + offset]), data, WORD_SIZE);
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

// ------------------------------------------------------------------------------------------------------------------------------- //

void accessL2(uint32_t indexL1, uint32_t offset, uint32_t AddressMemory, uint32_t address, uint8_t *data, uint32_t mode) {
    uint32_t indexL2, TagL2;
    uint8_t TempBlock[BLOCK_SIZE];

    indexL2 = AddressMemory & ((L2_SIZE / BLOCK_SIZE) - 1); // mask is the number of lines (512) (1 1111 1111)
    TagL2 = address >> 15; // log2(L2_SIZE) = 15 OR 9 + 6 = 15

    CacheLine *Line = &CacheInfoL2.line[indexL2];


    if (!Line->Valid || Line->Tag != TagL2)
    { // if block not present - miss

        accessDRAM(AddressMemory, TempBlock, MODE_READ); // get new block from DRAM
        
        if ((Line->Valid) && (Line->Dirty))
        {                                                                                   // line has dirty block
            AddressMemory = Line->Tag << 6;                                                 // get address of the block in memory
            accessDRAM(AddressMemory, &(L2Cache[indexL2 * BLOCK_SIZE + offset]), MODE_WRITE); // then write back old block
        }

        memcpy(&(L2Cache[indexL2 * BLOCK_SIZE + offset]), TempBlock,
               BLOCK_SIZE); // copy new block to cache line
        memcpy(&(L1Cache[indexL1 * BLOCK_SIZE + offset]), TempBlock,
               BLOCK_SIZE); // copy new block to cache line

        time += L2_WRITE_TIME;
        Line->Valid = 1;
        Line->Tag = TagL2;
        Line->Index = indexL2;
        Line->Dirty = 0;
    } // if miss, then replaced with the correct block


    if (mode == MODE_READ)
    { // read data from cache line
        memcpy(data, &(L2Cache[indexL2 * BLOCK_SIZE + offset]), WORD_SIZE);
        time += L2_READ_TIME;
    }

    if (mode == MODE_WRITE)
    { // write data from cache line
        memcpy(&(L2Cache[indexL2 * BLOCK_SIZE + offset]), data, WORD_SIZE);
        time += L2_WRITE_TIME;
        Line->Dirty = 1;
    }
}


void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}
