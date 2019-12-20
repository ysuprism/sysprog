//61940047 oyama yasuhiro

#define NHASH 4
#define STAT_LOCKED 0x00000001
#define STAT_VALID  0x00000002
#define STAT_DWR    0x00000004
#define STAT_KRDWR  0x00000008
#define STAT_WAITED 0x00000010
#define STAT_OLD 0x00000020

struct buf_header{
  int blkno;
  int bufno;
  struct buf_header *hash_fp;
  struct buf_header *hash_bp;
  unsigned int stat;
  struct buf_header *free_fp;
  struct buf_header *free_bp;
  char *cache_data;
};

void insert_head(struct buf_header *, struct buf_header *);

void insert_tail(struct buf_header *, struct buf_header *);

void remove_from_hash(struct buf_header *);

void insert_free_tail(struct buf_header *);

void insert_free_head(struct buf_header *);

void remove_from_free(struct buf_header *);

struct buf_header * getblk(int);

struct buf_header * search_hash(int);

void help(void);

void init(void);

void print_buf(int, char *[]);

void print_hash(int, char *[]);

void print_free(void);

struct buf_header * getblk(int);

void brelse(struct buf_header *);

void set_stat(int , unsigned int);

void reset_stat(int, unsigned int);

void quit(void);
