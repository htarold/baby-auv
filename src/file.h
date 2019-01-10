/* (C) 2019 Harold Tay LGPLv3 */
#ifndef FILE_H
#define FILE_H

/*
  There is no way to close a file.  File operations are
  sector based; writes are buffered so may not be synced
  immediately.  This means the last sector of a file could
  be lost.
 */
#include "f32.h"
struct file {
  struct f32_file f;
  uint32_t append_cluster;            /* could be <= 0 (empty file) */
  uint32_t seek_cluster;
  uint32_t seek_cluster_offset;
  uint8_t sectors_read;
};

/*
  In addition to these errors, functions may also return errors
  from sd2.c.  0 is returned on success.
 */
#define EFILE_LOOP    -20
#define EFILE_CHAIN   -21
#define EFILE_EOF     -22
#define EFILE_EMPTY   -23
#define EFILE_SEEKERR -24

/*
  Open a file, which must be in the root directory.
  Name is 11 chars long, first 8 chars are the base name, next 3
  are the extension, the dot separator is implied.  Both name
  and extension are right padded with spaces.
 */
extern int8_t file_open(struct file * f, const char * name);
extern int8_t file_openf(struct file * f, char * name);

extern int8_t file_creatf(struct file * f, char * name);

extern int8_t file_seek(struct file * f, uint32_t offset);

/*
  Read the first sector on the file.
 */
extern int8_t file_head(struct file * f);

/*
  Behead the file by 1 sector.  It's only permanent after the
  first sector is unlinked from the chain.
 */
extern int8_t file_advance(struct file * f);

/*
  Append to the file.
 */
extern int8_t file_append(struct file * f, char * s, uint8_t len);

#endif /* FILE_H */
