/*
  (C) 2019 Harold Tay LGPLv3
  Fat32.  Only the lowest level functionality is provided.
  Assumes little endian platform (which AVR is).
 */

#include <stdint.h>
#include <string.h>
#include "f32.h"
#include "sd2.h"
#include "tx.h"

static uint8_t  f32_sectors_per_cluster;
static uint8_t  f32_log2_sectors_per_cluster;
static uint16_t f32_nr_reserved_sectors;
static uint8_t  f32_nr_fats;
       uint32_t f32_fat_start;        /* Don't abuse! */
       uint32_t f32_sectors_per_fat;  /* likewise */
/* cluster number, usually 2: */
static uint32_t f32_root_dir_1st_cluster;
static uint32_t f32_cwd_1st_cluster;
static uint32_t f32_clusters_start;
static uint32_t dev_start, dev_nr_sectors;

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define pgm_read_byte_near(x) *((x))
#define memcpy_P(a, b, c)     memcpy(a, b, c)
#define strcpy_P(a, b)        strcpy(a, b)
#define PROGMEM /* nothing */
#endif

#undef DBG_F32
#ifdef DBG_F32
#define dbg_msg(a, b) tx_msg(a, b)
#define DBG(x) x
static void dbg_dump_sector(void)
{
  uint16_t i;
  for (i = 0; i < 512; i++) {
    if (0 == i%16) tx_puts("\r\n");
    else tx_putc(' ');
    tx_puthex(sd_buffer[i]);
  }
  tx_puts("\r\n");
}
static void dbg_msg32(char * msg, uint32_t val)
{
  tx_puts(msg);
  tx_puthex32(val);
  tx_puts("\r\n");
}
#else
#define dbg_msg32(a,b)    /* nothing */
#define dbg_msg(a, b)     /* nothing */
#define DBG(x)            /* nothing */
#define dbg_dump_sector() /* nothing */
#endif /* DBG_F32 */

#define COPY4(u, byte_addr) memcpy((void *)(&u), (void *)(byte_addr), 4)
#define COPY2(u, byte_addr) memcpy((void *)(&u), (void *)(byte_addr), 2)

int8_t f32_init(void)
{
  uint16_t us;
  int8_t res;

  tx_strlit("f32_init calling sd_init\r\n");
  res = sd_init();
  if (res) return(res);

  tx_strlit("f32_init calling sd_bread\r\n");
  res = sd_bread(0);
  if (res) return(res);

  if (sd_buffer[510]!= 0x55 || sd_buffer[511] != 0xaa) {
    tx_strlit("f32_init:F32_ENOMEDIUM\r\n");
    dbg_dump_sector();
    return(F32_ENOMEDIUM);
  }

  COPY4(dev_start, sd_buffer + 446 + 8);
  COPY4(dev_nr_sectors, sd_buffer + 446 + 12);

  dbg_msg("f32_init calling sd_bread ", dev_start);
  res = sd_bread(dev_start);
  if (res) goto fail;

  res--;
  COPY2(us, sd_buffer + 0x0b);        /* bytes per sector */
  if (us != 512) goto fail;

  f32_sectors_per_cluster = sd_buffer[0x0d];
  dbg_msg("f32_init: sectors_per_cluster = ", f32_sectors_per_cluster);
  f32_log2_sectors_per_cluster = 0;
  for(us = 1; us <= 128; us <<= 1, f32_log2_sectors_per_cluster++)
    if (us == f32_sectors_per_cluster) break;
  res--;
  if (us > 64) goto fail;

  COPY2(f32_nr_reserved_sectors, sd_buffer + 0x0e);

  f32_nr_fats = sd_buffer[0x10];      /* must have 2 fats */
  if (f32_nr_fats != 2) { res = 3; goto fail; }
  COPY4(f32_sectors_per_fat, sd_buffer + 0x24);
  COPY4(f32_root_dir_1st_cluster, sd_buffer + 0x2c);
  f32_cwd_1st_cluster = f32_root_dir_1st_cluster;

  res--;
  if (sd_buffer[510] != 0x55 || sd_buffer[511] != 0xaa) 
    goto fail;

  f32_fat_start = dev_start + f32_nr_reserved_sectors;
  f32_clusters_start = dev_start + f32_nr_reserved_sectors +
    (f32_nr_fats * f32_sectors_per_fat);

#define SECTOR_ADDRESS(cluster) \
  (f32_clusters_start + (cluster-2)*f32_sectors_per_cluster)

#define CLUSTER_NUMBER(sector) \
  (((sector - f32_clusters_start)>>f32_log2_sectors_per_cluster) + 2)

  return(0);
fail:
  dbg_dump_sector();
  dbg_msg("f32_init: ", res);
  return(res);
}

#define F32_CLUSTER_MASK 0x0fffffff

#define sd_dw_buffer ((uint32_t *)sd_buffer)

uint32_t f32_next_cluster(uint32_t cluster)
{
  uint32_t sector;
  int8_t res;

  cluster &= F32_CLUSTER_MASK;
  sector = f32_fat_start + cluster/128;
  res = sd_buffer_checkout(sector);
  if (res) return(res);
  return(sd_dw_buffer[cluster%128] & F32_CLUSTER_MASK);
}

int8_t f32_fat_put(uint32_t cluster, uint32_t val)
{
  int8_t res;
#ifdef DBG_F32
  tx_puts("\r\n## f32_fat_put(");
  tx_puthex32(cluster);
  tx_puts(", ");
  tx_puthex32(val);
  tx_puts(")\r\n");
  tx_puts("## Loading sector ");
  tx_puthex32(f32_fat_start + cluster/128);
  tx_puts("\r\n## At index ");
  tx_puthex32(cluster%128);
  tx_puts("\r\n## At byte offset ");
  tx_puthex32(f32_fat_start*512 + cluster*4);
  tx_puts("\r\n");
#endif
  cluster &= F32_CLUSTER_MASK;
  val &= F32_CLUSTER_MASK;
  res = sd_buffer_checkout(f32_fat_start + cluster/128);
  if (res) return(res);
  sd_dw_buffer[cluster%128] = val;
  res = sd_buffer_sync();
  sd_buffer_checkin(SD_ADDRESS_NONE);
  return(res);
}

uint32_t f32_fat_find_free(uint32_t old_cluster)
{
  static uint32_t cluster;
  int8_t res;

  if (!cluster) cluster = 2;
  while (cluster < f32_sectors_per_fat*128) {
    res = sd_buffer_checkout(f32_fat_start + cluster/128);
    if (res) return(res);
    do {
      if (!sd_dw_buffer[cluster%128]) {
        /* f32_fat_put(cluster, -1); */
        res = f32_fat_put(cluster, 0xfffffffe); /* XXX */
        if (res) return(res);
        if (old_cluster > 0) {
          res = f32_fat_put(old_cluster, cluster);
          if (res) return(res);
        }
        return(cluster);
      }
      cluster++;
    } while (cluster%128);
  }
  return((uint32_t)-F32_ENOSPC);
}

/* Names are left justified, right padded with spaces */
static int8_t part_is_legal(char * s, int8_t len)
{
  int8_t i, j;
  static const char illegal[] PROGMEM = {
    0x22, 0x2a, 0x2b, 0x2c, 0x2e, 0x2f, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f, 0x5b, 0x5c, 0x5d, 0x7c };
  for (i = 0; i < len; i++) {
    if (s[i] == 0x20) break;
    if (s[i] < 0x20) {
      dbg_msg("legal:char<0x20:", s[i]);
      return(0);
    }
    for (j = 0; j < sizeof(illegal); j++)
      if (s[i] == pgm_read_byte_near(illegal+j)) {
        dbg_msg("legal:bad char:", s[i]);
        return(0);
      }
  }
  if (8 == len && 0 == i) {           /* empty extension is ok */
    dbg_msg("legal:part too short", i);
    return(0);
  }
  for ( ; i < len; i++)
    if (s[i] != 0x20) {
      dbg_msg("legal:embedded space:", s[i]);
      return(0);
    }
  return(1);
}

static int8_t name_is_legal(char filename[11])
{
  DBG(tx_puts("filename:"));
  DBG(tx_puts(filename?filename:""));
  DBG(tx_puts("\r\n"));
  if (!part_is_legal(filename, 8)) return(0);
  DBG(tx_puts("name ok, checking extension...\r\n"));
  if (!part_is_legal(filename+8, 3)) return(0);
  DBG(tx_puts("extension ok\r\n"));
  return(1);
}
/* Find a dirent in the current directory (default is /) */
int8_t f32_findf(char filename[11], struct f32_file * lp)
{
  uint32_t cluster;
  uint16_t us;
  int8_t res;
  uint8_t * dirent;

  DBG(tx_puts("f32_findf:filename="));
  DBG(tx_puts(filename?filename:""));
  DBG(tx_puts("\r\n"));

  /*
    Name is either "" or valid.
   */
  if (filename)
    if (!name_is_legal(filename))
      return(F32_EBADNAME);

  for(cluster = f32_cwd_1st_cluster; ; ) {
    uint8_t i, j;
    lp->dirent_sector =
      f32_clusters_start + (cluster-2)*f32_sectors_per_cluster;

    for(i = 0; i < f32_sectors_per_cluster; i++) {
      res = sd_buffer_checkout(lp->dirent_sector + i);
      if (res) return(res);
      dbg_msg("f32_findf trying sector ", i);
      for(j = 0; j < 16; j++) {
        lp->dirent_offset = 32*j;
        dirent = sd_buffer + lp->dirent_offset;
        if (!filename) {              /* Looking for empty dirent */
          dbg_msg("  f32_findf trying entry ", j);
          if (!*dirent || *dirent == 0xe5)
            goto found_empty;
        } else {
          if (!*dirent) continue;
          res = strncmp((char *)dirent, filename, 11);
          if (0 == res) goto found;
        }
      }
    }
    cluster = f32_next_cluster(cluster);
    if (-1 == cluster) return(-1);    /* XXX ? */
    if (cluster >= (0xfffffff8 & F32_CLUSTER_MASK)) break;
  }
  /* not_found: */
  return(F32_ENOENT);

found_empty:
  memset(dirent + 0x1c, 0, 4);        /* file size */
  memset(dirent + 0x14, 0, 2);        /* 1st cluster */
  memset(dirent + 0x1a, 0, 2);        /* 1st cluster */
  /* Fall Through */
found:
  COPY4(lp->file_size, dirent + 0x1c);
  COPY2(us, dirent + 0x14);
  lp->file_1st_cluster = ((uint32_t)us << 16);
  COPY2(us, dirent + 0x1a);
  lp->file_1st_cluster |= us;

  if (*filename)
    return((dirent[0xb] & 0x10)?F32_EISDIR:0);
  return(0);
}

int8_t f32_find(const char * filename, struct f32_file * lp)
{
  char * p;
  char buf[12];
  p = 0;
  if (filename) strcpy_P(p = buf, filename);
  return(f32_findf(p, lp));
}

int8_t f32_creatf(char filename[11], struct f32_file * lp)
{
  int8_t res;

  res = f32_findf(filename, lp);
  if (!res) return(F32_EEXIST);
  if (res != F32_ENOENT) return(res);

  res = f32_findf(0, lp);             /* find empty slot */
  if (res) return(res);
  DBG(tx_msg("f32_creatf:dirent_sector=",lp->dirent_sector));
  DBG(tx_msg("f32_creatf:dirent_offset=",lp->dirent_offset));

  /* sd_buffer now contains the dirent */
  memcpy(sd_buffer + lp->dirent_offset, filename, 11);
  lp->file_1st_cluster = 0;
  lp->attributes = F32_ATTR_FILE;
  lp->file_size = 0;
  res = f32_update_dirent(lp);
  if (res) return(res);
  return(sd_buffer_sync());
}
int8_t f32_creat(const char * filename, struct f32_file * lp)
{
  char buf[11];
  char * p;
  p = 0;
  if (filename) memcpy_P(p = buf, filename, 11);
  return(f32_creatf(buf, lp));
}

/* one path component at a time */
int8_t f32_chdir(char filename[11])
{
  struct f32_file f;
  int8_t res;

  if (!filename) {
    f32_cwd_1st_cluster = f32_root_dir_1st_cluster;
    return(0);
  }

  res = f32_findf(filename, &f);
  if (res != F32_EISDIR) return(res);
  f32_cwd_1st_cluster = f.file_1st_cluster;
  return(0);
}

/*
  XXX mkdir() functionality
 */

static void copy_cluster_addr(uint8_t * dirent, uint32_t cluster)
{
  uint16_t us;
  us = cluster >> 16;
  memcpy(dirent + 0x14, &us, 2);
  us = cluster & 0xffff;
  memcpy(dirent + 0x1a, &us, 2);
}
int8_t f32_update_dirent(struct f32_file * lp)
{
  int8_t res;
  uint32_t * sizep;
  uint8_t * dirent;

  res = sd_buffer_checkout(lp->dirent_sector);
  if (res) return(res);

  dirent = sd_buffer + lp->dirent_offset;
  sizep = (uint32_t *)(dirent + 0x1c);
  if (F32_ATTR_DIR == lp->attributes)
    *sizep = 0UL;
  else
    *sizep = lp->file_size;
  copy_cluster_addr(dirent, lp->file_1st_cluster);
  /* Could be updated from a global clock XXX */
  *((uint16_t *)(dirent + 14)) = 0;   /* ctime */
  *((uint16_t *)(dirent + 16)) = 1;   /* cdate */
  *((uint16_t *)(dirent + 18)) = 1;   /* adate */
  *((uint16_t *)(dirent + 22)) = 0;   /* wtime */
  *((uint16_t *)(dirent + 24)) = 1;   /* wdate */

  return(0);
}

int8_t f32_mkdir(char filename[11])
{
  int8_t res;
  uint8_t i;
  struct f32_file f;
  uint32_t sector;

  res = f32_creatf(filename, &f);
  DBG(tx_msg("f32_mkdir:f32_creatf=", res));
  if (res) return(res);
  /* is created as a regular file, with no clusters allocated */
  res = sd_buffer_checkout(f.dirent_sector);
  if (res) return(res);
  f.file_1st_cluster = f32_fat_find_free(0);
  dbg_msg32("f32_mkdir:f.file_1st_cluster=", f.file_1st_cluster);
  if ((int32_t)f.file_1st_cluster < 0) return(F32_ENOSPC);
  res = f32_update_dirent(&f);
  if (res) return(res);
  /* XXX use existing sd_buffer to set dir attr flag */
  sd_buffer[f.dirent_offset + 0xb] = 0x10;

  /* Zero out entire cluster (contents of new dir). */
  sector = f32_sector_address(f.file_1st_cluster);
  for (i = 0; i < f32_sectors_per_cluster; i++) {
    res = sd_buffer_checkout(SD_ADDRESS_NONE);
    if (res) return(res);
    memset(sd_buffer, 0, 512);
    sd_buffer_checkin(sector + i);
  }

  /* Add . and .. entries to first sector */
  sector = f32_sector_address(f.file_1st_cluster);
  res = sd_buffer_checkout(sector);
  if (res) return(res);
  sd_buffer[0] = '.';
  for (i = 1; i < 11; i++) sd_buffer[i] = ' ';
  dbg_msg32("dot 1st cluster =", f.file_1st_cluster);
  copy_cluster_addr(sd_buffer, f.file_1st_cluster);
  sd_buffer[0xb] = 0x10; 

  sd_buffer[32+0] = '.';
  sd_buffer[32+1] = '.';
  for (i = 2; i < 11; i++) sd_buffer[32+i] = ' ';
  dbg_msg32("dotdot 1st cluster =", f32_cwd_1st_cluster);
  copy_cluster_addr(sd_buffer + 32, f32_cwd_1st_cluster);
  sd_buffer[32+0xb] = 0x10;
  return(sd_buffer_sync());
}

/*
  Convenience functions
 */
uint32_t f32_sector_address(uint32_t cluster)
{
  return(f32_clusters_start + (cluster-2)*f32_sectors_per_cluster);
}
uint32_t f32_cluster_number(uint32_t sector)
{
  return
    (((sector - f32_clusters_start)>>f32_log2_sectors_per_cluster) + 2);
}
uint8_t f32_nr_sectors_per_cluster(void)
{
  return(f32_sectors_per_cluster);
}
int8_t f32_is_valid_cluster(uint32_t cluster)
{
  if (cluster < 2) return(0);
  return(cluster < (0xfffffff8 & F32_CLUSTER_MASK));
}
