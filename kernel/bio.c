// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 13
// #define HASH(blockno) (blockno % NBUCKET)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
  uint size;
  struct buf buckets[NBUCKET];
  struct spinlock locks[NBUCKET];
  struct spinlock hashlock;
} bcache;

void
binit(void)
{
  struct buf *b;

  bcache.size=0;
  initlock(&bcache.lock, "bcache");
  initlock(&bcache.hashlock, "bcache_hash");

  for(int i=0;i<NBUCKET;i++){
    initlock(&bcache.locks[i],"bcache_bucket");
  }
  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint idx=blockno%NBUCKET;
  uint mintimestamp;
  struct buf *targetB=0,*pre=0,*targetPre=0;
  acquire(&bcache.locks[idx]);

  // Is the block already cached?
  // bcache.bucket[i]是dummy
  for(b = bcache.buckets[idx].next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.locks[idx]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 分类讨论
  // 情况１）把没用过的buf放入对应bucket
  // 修改size，上锁
  // 不能unlock bucket[i],lock bcache,lock bucket[i],因为后面默认bucket[i]没有缓存b,（如果两个线程同时运行到此处）会导致buckets中对应多个block;
  acquire(&bcache.lock);
  if(bcache.size<NBUF){
    b=&bcache.buf[bcache.size++];
    release(&bcache.lock);
    
    //分配一个buf，等待返回bread从disk中读取内容
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;

    b->next=bcache.buckets[idx].next;
    bcache.buckets[idx].next=b;
    release(&bcache.locks[idx]);
    acquiresleep(&b->lock);
    return b;
  }
  release(&bcache.lock);
  //需要考虑同时插入
  release(&bcache.locks[idx]);

  //情况２)把buf从一个bucket放到另一个bucket
  acquire(&bcache.hashlock);
  for(int i=0;i<NBUCKET;i++){
    mintimestamp=-1;
    acquire(&bcache.locks[idx]);
    for(pre=&bcache.buckets[idx],b=pre->next;b!=0;pre=pre->next,b=b->next){
      if(b->dev == dev && b->blockno == blockno){
        b->refcnt++;
        release(&bcache.locks[idx]);
        release(&bcache.hashlock);
        acquiresleep(&b->lock);
        return b;
      }
      
      if(b->refcnt == 0 && mintimestamp>b->timestamp) {
        mintimestamp=b->timestamp;
        targetPre=pre;
        targetB=b;
      }
    }
    if(mintimestamp!=-1){
      targetB->dev = dev;
      targetB->blockno = blockno;
      targetB->valid = 0;
      targetB->refcnt = 1;

      targetPre->next=targetB->next;
      release(&bcache.locks[idx]);
      idx=blockno%NBUCKET;
      acquire(&bcache.locks[idx]);
      targetB->next=bcache.buckets[idx].next;
      bcache.buckets[idx].next=targetB;

      release(&bcache.locks[idx]);
      release(&bcache.hashlock);
      acquiresleep(&targetB->lock);
      return targetB;
    }
    release(&bcache.locks[idx]);

    if(++idx==NBUCKET){
      idx=0;
    }
  }
  // release(&bcache.hashlock);
  // release(&bcache.locks[idx]);

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int i=b->blockno%NBUCKET;
  acquire(&bcache.locks[i]);
  b->refcnt--;
  if (b->refcnt == 0) {
    b->timestamp=ticks;
  }
  
  release(&bcache.locks[i]);
}

void
bpin(struct buf *b) {
  uint i=b->blockno%NBUCKET;
  acquire(&bcache.locks[i]);
  b->refcnt++;
  release(&bcache.locks[i]);
}

void
bunpin(struct buf *b) {
  uint i=b->blockno%NBUCKET;
  acquire(&bcache.locks[i]);
  b->refcnt--;
  release(&bcache.locks[i]);
}


