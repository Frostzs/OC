#include "L2Cache.h"

int main() {

  

    int clock1;


    resetTime();
    initCache();

    int addr = 0;
    int addr2 = addr + (1<<14); // mesmo indice, diferente tag

    read(addr, (unsigned char *)(&addr)); // da miss porque a cache esta vazia, coloca o na cache
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", addr, addr, clock1);

    read(addr2, (unsigned char *)(&addr2)); // da miss e da replace na cache
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", addr2, addr2, clock1);
    

    read(addr, (unsigned char *)(&addr));   // da miss e da replace na cache
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", addr, addr, clock1);

  
  return 0;
}