//61940047 oyama yasuhiro

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "buf.h"
#include "extern.h"

char stat_char[] = "LVDKWO";

void insert_head(struct buf_header *h, struct buf_header *p){
  if(h->hash_fp != NULL){
    p->hash_bp = h;
    p->hash_fp = h->hash_fp;
    h->hash_fp->hash_bp = p;
    h->hash_fp = p;
  }else{
    p->hash_bp = h;
    p->hash_fp = h;
    h->hash_fp = p;
    h->hash_bp = p;
  }
}

void insert_tail(struct buf_header *h, struct buf_header *p){
  if(h->hash_bp != NULL){
    p->hash_fp = h;
    p->hash_bp = h->hash_bp;
    h->hash_bp->hash_fp = p;
    h->hash_bp = p;
  }else{
    p->hash_fp = h;
    p->hash_bp = h;
    h->hash_bp = p;
    h->hash_fp = p;
  }
}

void remove_from_hash(struct buf_header *p){
  if(p->hash_bp != p->hash_fp){	
    p->hash_bp->hash_fp = p->hash_fp;
    p->hash_fp->hash_bp = p->hash_bp;
    p->hash_fp = p->hash_bp = NULL;
  }else{
    p->hash_bp->hash_fp = p->hash_bp->hash_bp = NULL;
    p->hash_fp = p->hash_bp = NULL;
  }
}

void insert_free_head(struct buf_header *p){
  if(free_head.free_fp != NULL){
    p->free_bp = &free_head;
    p->free_fp = free_head.free_fp;
    free_head.free_fp->free_bp = p;
    free_head.free_fp = p;
  }else{
    p->free_bp = &free_head;
    p->free_fp = &free_head;
    free_head.free_fp = p;
    free_head.free_bp = p;
  }
}

void insert_free_tail(struct buf_header *p){
  if(free_head.free_bp != NULL){
    p->free_bp = free_head.free_bp;
    p->free_fp = &free_head;
    free_head.free_bp->free_fp = p;
    free_head.free_bp = p;
  }else{
    p->free_fp = &free_head;
    p->free_bp = &free_head;
    free_head.free_bp = p;
    free_head.free_fp = p;
  }
}

void remove_from_free(struct buf_header *p){
  if(p->free_bp != p->free_fp){
    p->free_bp->free_fp = p->free_fp;
    p->free_fp->free_bp = p->free_bp;
    p->free_fp = p->free_bp = NULL;
  }else{
    p->free_bp->free_fp = p->free_bp->free_bp = NULL;
    p->free_fp = p->free_bp = NULL;
  }
}

struct buf_header * search_hash(int blkno){
  int h;
  struct buf_header *p;

  h = blkno % NHASH;
  if(hash_head[h].hash_fp != NULL){
    for(p = hash_head[h].hash_fp; p != &hash_head[h]; p = p->hash_fp){
      if(p->blkno == blkno){
        return p;
      }
    }
  }

  return NULL;
}

void help(void){
  printf("list of commands:\n");
  printf("help: print this help.\n");
  printf("init: initialize buffers, hash lists and free list.\n");
  printf("buf [n]: you can pass a buf number. if an buf number is passed, print the corresponding buffer's status. if not, print all buffers' status.\n");
  printf("hash [n]: you can pass a hash number. if a hash number is passed, print the corresponding hash list. if not, print all hash lists.\n");
  printf("free: print free list.\n");
  printf("getblk n: pass a block number which you want to get.\n");
  printf("brelse n: pass a block number which you want to release.\n");
  printf("set n stat [stat ...]: pass a block number and one or more states which you want to set the block to.\n");
  printf("reset n stat [stat ...]: pass a block number and one or more states which you want to remove from the block.\n");
  printf("quit: close this program.\n");
}

void init(void){
  int blkno[12] = { 28, 4, 64,
                    17, 5, 97,
                    98, 50, 10,
                    3, 35, 99 };
  int bufno[6] = { 9, 4, 1, 0, 5, 8 };

  for(int i = 0; i < 4; i++){
    hash_head[i].hash_fp = hash_head[i].hash_bp = NULL;
  }

  free_head.free_fp = free_head.free_bp = NULL;

  for(int i = 0; i < 12; i++){
    buf[i].blkno = blkno[i];
    buf[i].bufno = i;
    buf[i].stat = STAT_VALID|STAT_LOCKED;
    buf[i].hash_fp = buf[i].hash_bp = NULL;
    buf[i].free_fp = buf[i].free_bp = NULL;
    insert_tail(&hash_head[i/3], &buf[i]);
  }

  for(int i = 0; i < 6; i++){
    buf[bufno[i]].stat &= ~STAT_LOCKED;
    insert_free_tail(&buf[bufno[i]]);
  }
}

void print_buf(int ac, char *av[]){
  if(ac == 1){
    for(int i = 0; i < 12; i++){
      char str[] = "------";
      for(int j = 0; j < 6; j++){
        if(buf[i].stat & stat_bit[j]){
	  str[5-j] = stat_char[j];
	}
      }
      printf("[%2d:%3d%7s]\n", i, buf[i].blkno, str);
    }
  }else{
    for(int i = 1; i < ac; i++){
      char *endp;
      int bufno = (int)strtol(av[i], &endp, 10);
      if(*endp != 0){
        fprintf(stderr, "av[%d]: give a number\n", i);
      }else{
        if(bufno < 0 || bufno > 12){
          fprintf(stderr, "av[%d]: give a number between 0 and 11\n", i);
        }else{
          char str[] = "------";
          for(int j = 0; j < 6; j++){
            if(buf[bufno].stat & stat_bit[j]){
	      str[5-j] = stat_char[j];
            }
          }
          printf("[%2d:%3d%7s]\n", bufno, buf[bufno].blkno, str);
        }
      }
    }
  }
}

void print_hash(int ac, char *av[]){
  if(ac == 1){
    for(int i = 0; i < 4; i++){
      printf("%d: ", i);
      struct buf_header *p;
      if(hash_head[i].hash_fp != NULL){
        for(p = hash_head[i].hash_fp; p != &hash_head[i]; p = p->hash_fp){
	  char str[] = "------";
          for(int j = 0; j < 6; j++){
            if(p->stat & stat_bit[j]){
              str[5-j] = stat_char[j];
            }
          }
          printf("[%2d:%3d%7s] ", p->bufno, p->blkno, str);
        }
      }
      printf("\n");
    }
  }else{
    for(int i = 1; i < ac; i++){
      char *endp;
      int hash = (int)strtol(av[i], &endp, 10);
      if(*endp != 0){
        fprintf(stderr, "av[%d]: give a number\n", i);
      }else{
        if(hash < 0 || hash > 3){
          fprintf(stderr, "av[%d]: give a number between 0 and 3\n", i);
        }else{
          printf("%d: ", hash);
          struct buf_header *p;
	  if(hash_head[hash].hash_fp != NULL){
            for(p = hash_head[hash].hash_fp; p != &hash_head[hash]; p = p->hash_fp){
              char str[] = "------";
              for(int j = 0; j < 6; j++){
                if(p->stat & stat_bit[j]){
                  str[5-j] = stat_char[j];
                }
              }
              printf("[%2d:%3d%7s] ", p->bufno, p->blkno, str);	
            }
	  }
          printf("\n");
        }	
      }
    }
  }
}

void print_free(){
  struct buf_header *p;
  if(free_head.free_fp != NULL){
    for(p = free_head.free_fp; p != &free_head; p = p->free_fp){
      char str[] = "------";
      for(int j = 0; j < 6; j++){
        if(p->stat & stat_bit[j]){
          str[5-j] = stat_char[j];
        }
      }
      printf("[%2d:%3d%7s] ", p->bufno, p->blkno, str);
    }
  }
  printf("\n");
}

struct buf_header * getblk(int blkno){
  struct buf_header *p = NULL;
  while(p == NULL){
    if((p = search_hash(blkno)) != NULL){
      if(p->stat & STAT_LOCKED){
        printf("scenario 5\n");
	p->stat |= STAT_WAITED;
        //sleep();
        printf("Process goes to sleep\n");
        return NULL;
      }

      printf("scenario 1\n");
      p->stat |= STAT_LOCKED;
      remove_from_free(p);
      return p;
    }else{
      if(free_head.free_fp == NULL){
        printf("scenario 4\n");
        //sleep();
        printf("Process goes to sleep\n");
        return NULL;
      }
      p = free_head.free_fp;
      remove_from_free(p);
      if(p->stat & STAT_DWR){
        printf("scenario 3\n");
	p->stat &= ~STAT_DWR;
	p->stat |= STAT_LOCKED|STAT_KRDWR|STAT_OLD;
	p = NULL;
        continue;
      }

      printf("scenario 2\n");
      p->stat |= STAT_LOCKED;
      p->stat &= ~STAT_VALID;
      remove_from_hash(p);
      insert_tail(&hash_head[blkno % NHASH], p);
      p->blkno = blkno;
      return p;
    }
  }
}

void brelse(struct buf_header *buffer){
  //wakeup();
  printf("Wakeup processes waiting for any buffer\n");
  //wakeup();
  if(buffer->stat & STAT_WAITED){
    buffer->stat &= ~STAT_WAITED;
    printf("Wakeup processes waiting for buffer of blkno %d\n", buffer->blkno);
  }
  //raise_cpu_level();
  unsigned int is_valid = buffer->stat & STAT_VALID;
  unsigned int is_old = buffer->stat & STAT_OLD;
  if(is_valid && !is_old){
    insert_free_tail(buffer);
    printf("buffer(blkno %d) was inserted at end of free list\n", buffer->blkno);
  }else{
    buffer->stat &= ~STAT_OLD;
    insert_free_head(buffer);
    printf("buffer(blkno %d) was inserted at beginning of free list\n", buffer->blkno);
  }
  //lower_cpu_level();
  buffer->stat &= ~STAT_LOCKED;
}

void set_stat(int blkno, unsigned int stat){
  struct buf_header *p = search_hash(blkno);
  if(p != NULL){
    p->stat |= stat;
  }
}

void reset_stat(int blkno, unsigned int stat){
  struct buf_header *p = search_hash(blkno);
  if(p != NULL){
    p->stat &= ~stat;
  }
}

void quit(void){
  exit(0);
}
