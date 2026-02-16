#ifndef _file_h_INCLUDED
#define _file_h_INCLUDED

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "attribute.h"
#include "keatures.h"

bool kissat_file_exists (const char *path);
bool kissat_file_readable (const char *path);
bool kissat_file_writable (const char *path);
size_t kissat_file_size (const char *path);
bool kissat_find_executable (const char *name);

typedef struct file file;

struct file {
  FILE *file;
  bool close;
  bool reading;
  bool compressed;
  const char *path;
  uint64_t bytes;
};

void kissat_read_already_open_file (file *, FILE *, const char *path);
void kissat_write_already_open_file (file *, FILE *, const char *path);

bool kissat_open_to_read_file (file *, const char *path);
bool kissat_open_to_write_file (file *, const char *path);

void kissat_close_file (file *);

#ifndef KISSAT_HAS_COMPRESSION

bool kissat_looks_like_a_compressed_file (const char *path);

#endif

// clang-format off

static inline size_t
kissat_read (file *, void *, size_t) ATTRIBUTE_ALWAYS_INLINE;

static inline size_t
kissat_write (file *, void *, size_t) ATTRIBUTE_ALWAYS_INLINE;

static inline int kissat_getc (file *) ATTRIBUTE_ALWAYS_INLINE;

static inline int kissat_putc (file *, int) ATTRIBUTE_ALWAYS_INLINE;

static inline void kissat_flush (file *) ATTRIBUTE_ALWAYS_INLINE;

// clang-format on

static inline size_t kissat_read (file *file_ptr, void *ptr, size_t bytes) {
  assert (file_ptr);
  assert (file_ptr->file);
  assert (file_ptr->reading);
#ifdef KISSAT_HAS_UNLOCKEDIO
  size_t res = fread_unlocked (ptr, 1, bytes, file_ptr->file);
#else
  size_t res = fread (ptr, 1, bytes, file_ptr->file);
#endif
  file_ptr->bytes += res;
  return res;
}

static inline size_t kissat_write (file *file_ptr, void *ptr,
                                   size_t bytes) {
  assert (file_ptr);
  assert (file_ptr->file);
  assert (!file_ptr->reading);
#ifdef KISSAT_HAS_UNLOCKEDIO
  size_t res = fwrite_unlocked (ptr, 1, bytes, file_ptr->file);
#else
  size_t res = fwrite (ptr, 1, bytes, file_ptr->file);
#endif
  file_ptr->bytes += res;
  return res;
}

static inline int kissat_getc (file *file_ptr) {
  assert (file_ptr);
  assert (file_ptr->file);
  assert (file_ptr->reading);
#ifdef KISSAT_HAS_UNLOCKEDIO
  int res = getc_unlocked (file_ptr->file);
#else
  int res = getc (file_ptr->file);
#endif
  if (res != EOF)
    file_ptr->bytes++;
  return res;
}

static inline int kissat_putc (file *file_ptr, int ch) {
  assert (file_ptr);
  assert (file_ptr->file);
  assert (!file_ptr->reading);
#ifdef KISSAT_HAS_UNLOCKEDIO
  int res = putc_unlocked (ch, file_ptr->file);
#else
  int res = putc (ch, file_ptr->file);
#endif
  if (res != EOF)
    file_ptr->bytes++;
  return ch;
}

static inline void kissat_flush (file *file_ptr) {
  assert (file_ptr);
  assert (file_ptr->file);
  assert (!file_ptr->reading);
#ifdef KISSAT_HAS_UNLOCKEDIO
  fflush_unlocked (file_ptr->file);
#else
  fflush (file_ptr->file);
#endif
}

#endif
