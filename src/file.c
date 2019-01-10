/* (C) 2019 Harold Tay LGPLv3 */
/*
  Extends the f32 interface.  Only 2 operations are allowed:
  appending to the end of a file, and reading from the start of
  a file.  After a full cluster is read from the start of the
  file, that cluster is unlinked.  This is peculiar to FAT
  systems.
 */

#include <string.h>                   /* for memcpy */
#include "file.h"
#include "sd2.h"
#include "tx.h"
#include "fmt.h"

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define strncpy_P(a, b, c) strncpy(a, b, c)
#endif

#undef DBG_FILE

#ifdef DBG_FILE
#define PRINT(s) s
#define SD_BUFFER_CHECKOUT(addr) checkout(addr)
static void dump_sector(void)
{
  uint8_t i, j;
  for (i = 0; i < 2; i++) {
    for (j = 0; j < 16; j++) {
      tx_puts(fmt_x(sd_buffer[16*i + j]));
      tx_putc(' ');
    }
    tx_puts("\r\n");
  }
}
static int8_t checkout(uint32_t addr)
{
  extern uint32_t current_address; /* from sd2.c */
  int8_t res;
  tx_puts("Old sector ");
  tx_puts(fmt_32x(current_address));
  tx_puts(":\r\n");
  dump_sector();
  tx_puts("checkout new sector ");
  tx_puts(fmt_32x(addr));
  tx_puts(":\r\n");
  res = sd_buffer_checkout(addr);
  if (0 == res)
    dump_sector();
  return(res);
}
static void print_meta(struct file * f)
{
  tx_puts("size:");
    tx_puts(fmt_32x(f->f.file_size));
  tx_puts("\r\ncluster1:");
    tx_puts(fmt_32x(f->f.file_1st_cluster));
  tx_puts("\r\ndirent_sector:");
    tx_puts(fmt_32x(f->f.dirent_sector));
  tx_puts("\r\ndirent_offset:");
    tx_puts(fmt_i16d(f->f.dirent_offset));
  tx_puts("\r\nappend_cluster:");
    tx_puts(fmt_32x(f->append_cluster));
  tx_puts("\r\n");
}

#else
#define PRINT(s)
#define print_meta(a) /* nothing */
#define SD_BUFFER_CHECKOUT(addr) sd_buffer_checkout(addr)
#endif /* DBG_FILE */


int8_t file_openf(struct file * f, char * name)
{
  uint32_t bytes_left, i;
  int8_t res;
  uint16_t bytes_per_cluster;  /* XXX Theoretically not more than 32k */

  res = f32_findf(name, &(f->f));
  if (res) return(res);

  print_meta(f);

  /* Seek to EOF.  File could be empty. */

  bytes_left = f->f.file_size;
  f->append_cluster = f->f.file_1st_cluster;

  bytes_per_cluster = 512 * f32_nr_sectors_per_cluster();

  for(i = 0; ; i++) {
    uint32_t next;
    if (!f32_is_valid_cluster(f->append_cluster)) break;
    if (i > 125000) return(EFILE_LOOP);
    next = f32_next_cluster(f->append_cluster);
    if ((int32_t)next < 0) return((int8_t)next);
    if (!f32_is_valid_cluster(next)) break;
    bytes_left -= bytes_per_cluster;
    if (0 == bytes_left) {
      /* file ends exactly on a cluster boundary */
      break;
    }
    f->append_cluster = next;
  }

  if (bytes_left >= bytes_per_cluster)
    return(EFILE_CHAIN);

  f->seek_cluster = f->f.file_1st_cluster;
  f->seek_cluster_offset = 0;

  /*
    f->append_cluster is the last cluster in the file.
    This cluster could be null.
   */

  print_meta(f);
  return(0);
}
int8_t file_open(struct file * f, const char * name)
{
  char filename[12];
  strncpy_P(filename, name, sizeof(filename));
  return(file_openf(f, filename));
}

int8_t file_creatf(struct file * f, char * name)
{
  int8_t res;

  res = f32_creatf(name, &(f->f));
  if (res) return(res);

  f->f.file_1st_cluster = f->append_cluster = f->seek_cluster = 0;
  f->seek_cluster_offset = 0;
  f->sectors_read = 0;
  return(0);
}

int8_t file_seek(struct file * f, uint32_t offset)
{
  int8_t res;
  uint32_t cluster, sector_address;
  uint16_t bytes_per_cluster;

  PRINT(tx_puts("file_seek to:"));
  PRINT(tx_puts(fmt_32x(offset)));
  PRINT(tx_puts("\r\n"));

  if (offset > f->f.file_size) return(EFILE_SEEKERR);
  if (!f->f.file_size) return(EFILE_EMPTY);

  offset &= 0xfffffe00;  /* ignore byte offset into sector */

  /*
    Cdr down to find the right cluster.  Either we start at the
    beginning, or we start where we left off the last time.
   */

  bytes_per_cluster = 512 * f32_nr_sectors_per_cluster();

  if (!f->seek_cluster) {
    f->seek_cluster = f->f.file_1st_cluster;
    f->seek_cluster_offset = 0;
  }

  if (f->seek_cluster_offset > offset) {
    f->seek_cluster_offset = 0;
    f->seek_cluster = f->f.file_1st_cluster;
  }

  while (offset > bytes_per_cluster) {
    cluster = f32_next_cluster(f->seek_cluster);
    if ((int32_t)cluster < 0) return((int8_t)cluster);
    f->seek_cluster = cluster;
    f->seek_cluster_offset -= bytes_per_cluster;
    offset -= bytes_per_cluster;
  }

  PRINT(tx_puts("at cluster:"));
  PRINT(tx_puts(fmt_32x(f->seek_cluster)));
  PRINT(tx_puts(" offset:"));
  PRINT(tx_puts(fmt_32x(f->seek_cluster_offset)));
  PRINT(tx_puts("\r\n"));

  sector_address = f32_sector_address(f->seek_cluster) + offset/512;
  res = SD_BUFFER_CHECKOUT(sector_address);
  return(res);
}

int8_t file_head(struct file * f)
{
  int8_t res;

  if ((512 * f->sectors_read) > f->f.file_size)
    return(EFILE_EOF);
  res = SD_BUFFER_CHECKOUT(f->f.file_1st_cluster + 512*f->sectors_read);
  return(res);
}

int8_t file_advance(struct file * f)
{
  uint32_t second_cluster;
  uint16_t bytes_per_cluster;
  int8_t res;

  if ((512 * (f->sectors_read+1)) > f->f.file_size)
    return(EFILE_EOF);

  f->sectors_read++;
  if (f->sectors_read < f32_nr_sectors_per_cluster()) return(0);

  bytes_per_cluster = 512 * f32_nr_sectors_per_cluster();

  f->sectors_read = 0;
  second_cluster = f32_next_cluster(f->f.file_1st_cluster);
  if ((int32_t)second_cluster <= 0) second_cluster = 0;

  /* free the cluster */
  res = f32_fat_put(f->f.file_1st_cluster, 0);
  if (res) return(res);
  f->f.file_1st_cluster = second_cluster;
  f->f.file_size -= 512;
  if (f->seek_cluster_offset > 0)
    f->seek_cluster_offset -= bytes_per_cluster;
  else
    f->seek_cluster = f->f.file_1st_cluster;

  res = f32_update_dirent(&(f->f));
  return(res);
}

/*
  The current sector could be empty or partially full or
  completely full.
 */
int8_t file_append(struct file * f, char * s, uint8_t len)
{
  uint32_t sector;
  int8_t res;
  uint8_t sectors_written;
  uint16_t sector_offset, avail, count;

  while (len > 0) {

    sectors_written =
      (f->f.file_size / 512) % f32_nr_sectors_per_cluster();

    sector_offset = f->f.file_size % 512;

    if (0 == sectors_written && 0 == sector_offset) {
      uint32_t new_cluster;
      /* If it's an empty file, orphan any clusters */
      new_cluster = f32_fat_find_free(f->append_cluster);
      if ((int32_t)new_cluster < 0)
        return((int8_t)new_cluster);
      PRINT(tx_puts("old cluster "));
      PRINT(tx_puts(fmt_32x(f->append_cluster)));
      PRINT(tx_puts("->"));
      PRINT(tx_puts(fmt_32x(new_cluster)));
      PRINT(tx_puts("\r\n"));
      if (0 == f->f.file_size)
        f->f.file_1st_cluster = new_cluster;
      f->append_cluster = new_cluster;
    }

    sector = f32_sector_address(f->append_cluster) +
      sectors_written;
    PRINT(tx_puts("Sector "));
    PRINT(tx_puts(fmt_32x(sector)));
    PRINT(tx_puts("\r\n"));
    /* Don't read sector that will be overwritten */
    if (0 == sector_offset) {
      PRINT(tx_puts("Sector offset is 0, reusing buffer\r\n"));
      res = SD_BUFFER_CHECKOUT(SD_ADDRESS_NONE);
      sd_buffer_checkin(sector);
    } else
      res = SD_BUFFER_CHECKOUT(sector);
    if (res) return(res);

    avail = 512 - sector_offset;
    count = (len>avail?avail:len);
    memcpy(sd_buffer + sector_offset, s, count);
    s += count;
    len -= count;
    f->f.file_size += count;
    avail -= count;
    if (0 == avail) {
      print_meta(f);
      res = f32_update_dirent(&(f->f));
      if (res) return(res);
    }
  }
  return(0);
}
