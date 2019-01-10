/* (C) 2019 Harold Tay LGPLv3 */
#ifndef F32_H
#define F32_H
#include <stdint.h>
extern int8_t f32_init(void);
extern uint32_t f32_next_cluster(uint32_t cluster);
extern int8_t f32_fat_put(uint32_t cluster, uint32_t val);
extern uint32_t f32_fat_find_free(uint32_t old_cluster);
extern uint32_t f32_cluster_number(uint32_t sector);
extern uint32_t f32_sector_address(uint32_t cluster);

#define F32_ENOENT     1
#define F32_ENOSPC     2
#define F32_ENOMEDIUM  3
#define F32_EEXIST     4
#define F32_EISDIR     5
#define F32_EBADNAME   6

struct f32_file {
  uint32_t file_size;
  uint32_t file_1st_cluster;
  uint32_t dirent_sector;
  uint16_t dirent_offset;
#define F32_ATTR_FILE 0x00
#define F32_ATTR_DIR  0x10
  uint8_t attributes;
  /* XXX Date can be provided in a global variable? */
};
/*
  Searches root level only.  File names are in raw form, i.e.
  must be 11 chars long, basename being 8 chars, extension is 3
  chars (implied dot separator is not used), both fields right
  padded with spaces).  Long form names not used.

  _find() and _creat() take fixed strings in flash;
  _findf() and creatf() takes strings in RAM.
*/
extern int8_t f32_find(const char * filename, struct f32_file * lp);
extern int8_t f32_findf(char filename[11], struct f32_file * lp);
extern int8_t f32_creat(const char * filename, struct f32_file * lp);
extern int8_t f32_creatf(char filename[11], struct f32_file * lp);
#define f32_filesize_dirent_offset() 0x1c
extern uint8_t f32_nr_sectors_per_cluster(void);
extern int8_t f32_is_valid_cluster(uint32_t cluster);
extern int8_t f32_chdir(char filename[11]);
extern int8_t f32_mkdir(char filename[11]);
extern int8_t f32_update_dirent(struct f32_file * lp);

/*
  This may not be very useful.  Raw fat32 dirent.
 */
struct f32_dir {
  uint8_t name[11];
  uint8_t attr;
  uint8_t resvd;
  uint8_t time_10;
  uint8_t time[2];
  uint8_t cdate[2];
  uint8_t adate[2];
  uint8_t clust1hi[2];
  uint8_t wtime[2];
  uint8_t wdate[2];
  uint8_t clust1lo[2];
  uint8_t size[4];
};

#endif /* F32_H */
