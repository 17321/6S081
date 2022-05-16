#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct{
  uint8 ref;
  struct spinlock lock;
}cows[(PHYSTOP-KERNBASE)>>12];

void incPAref(uint64 pa){
    if(pa<KERNBASE){
        return;
    }
    uint64 cnt=(pa-KERNBASE)>>12;
    acquire(&cows[cnt].lock);
    cows[cnt].ref++;
    release(&cows[cnt].lock);
}

uint8 decPAref(uint64 pa){
    if(pa<KERNBASE){
        return 0;//?
    }
    uint64 cnt=(pa-KERNBASE)>>12;
    uint8 ret;
    acquire(&cows[cnt].lock);
    ret=--cows[cnt].ref;
    release(&cows[cnt].lock);
    return ret;
}
