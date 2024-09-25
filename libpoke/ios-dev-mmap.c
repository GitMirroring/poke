/* ios-dev-map.c - Memory mapped devices.  */

/* Copyright (C) 2024 Andreas Klinger */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file implements an IO device that can be used in order to edit
   the memory mapped from device drivers via dev_map syscall.  */

#include <config.h>
#include <stdlib.h>
#include <unistd.h>

/* We want 64-bit file offsets in all systems.  */
#define _FILE_OFFSET_BITS 64

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

#include "ios.h"
#include "ios-dev.h"

/* State associated with a file device.  */
struct ios_dev_mmap
{
  char *filename;
  int fd;
  int reg_file;
  uint64_t flags;
  int open_flags;
  int prot;
  uint64_t base;
  uint64_t size;
  void *addr;
};

static const char *
ios_dev_mmap_get_if_name () {
  return "MMAP";
}

static char *
ios_dev_mmap_handler_normalize (const char *handler, uint64_t flags, int* error)
{
  char *new_handler = NULL;

  if (strlen (handler) > 6
      && handler[0] == 'm'
      && handler[1] == 'm'
      && handler[2] == 'a'
      && handler[3] == 'p'
      && handler[4] == ':'
      && handler[5] == '/'
      && handler[6] == '/')
    {
      new_handler = strdup (handler);
      if (new_handler == NULL && error)
        *error = IOD_ENOMEM;
    }

  if (error)
    *error = IOD_OK;
  return new_handler;
}

/* Returns 0 when the flags are inconsistent. */
static inline int
ios_dev_mmap_convert_flags_open (int mode_flags)
{
  int flags_for_open = 0;

  if ((mode_flags & IOS_F_READ)
      && (mode_flags & IOS_F_WRITE))
    {
      flags_for_open |= O_RDWR;
    }
  else if (mode_flags & IOS_F_READ)
    {
      flags_for_open |= O_RDONLY;
    }
  else if (mode_flags & IOS_F_WRITE)
    {
      flags_for_open |= O_WRONLY;
    }
  else
    /* Cannot open a file neither to write nor to read.  */
    return -1;

  return flags_for_open;
}

/* Returns 0 when the flags are inconsistent. */
static inline int
ios_dev_mmap_convert_mmap_prot (int open_flags)
{
  int mmap_prot = 0;

  if (open_flags & O_RDWR)
    {
      mmap_prot |= PROT_READ | PROT_WRITE;
    }
  else if (open_flags & O_RDONLY)
    {
      mmap_prot |= PROT_READ;
    }
  else if (open_flags & O_WRONLY)
    {
      mmap_prot |= PROT_WRITE;
    }
  else
    /* Cannot dev_map neither to write nor to read.  */
    return -1;

  return mmap_prot;
}

static void *
ios_dev_mmap_open (const char *handler, uint64_t flags, int *error,
                   void *data __attribute__ ((unused)))
{
  struct ios_dev_mmap *dev_map = NULL;
  int internal_error = IOD_ERROR;
  uint8_t mode_flags = flags & IOS_FLAGS_MODE;
  int open_flags = 0;
  int fd = -1;
  const char *p;
  char *end;
  struct stat st;
  int ret;

  dev_map = malloc (sizeof (struct ios_dev_mmap));
  if (!dev_map)
    goto err;

  memset (dev_map, 0, sizeof (struct ios_dev_mmap));

  /* Format of handler:
     mmap://BASE/SIZE/FILE-NAME  */

  /* Skip the mmap:// */
  p = handler + 7;

  /* parse the base address of memory mapped area. This is an uint64.  */
  dev_map->base = strtoull (p, &end, 0);
  if (*p != '\0' && *end == '/')
    /* Valid integer found.  */;
  else
    goto err;
  p = end + 1;

  /* parse the size of the memory mapped area. This is an uint64.  */
  dev_map->size = strtoull (p, &end, 0);
  if (*p != '\0' && *end == '/')
    /* Valid integer found.  */;
  else
    goto err;
  p = end + 1;

  /* The rest of the string is the name, which may be empty.  */
  dev_map->filename = strdup (p);
  if (!dev_map->filename)
    goto err;

  /* Ok now do some validation.  */
  /* base needs to be a multiple of PAGE_SIZE */
  if (dev_map->base % getpagesize ())
    {
      internal_error = IOD_EFLAGS;
      goto err;
    }

  if (mode_flags)
    {
      /* Decide what mode to use to open the file.  */
      open_flags = ios_dev_mmap_convert_flags_open (mode_flags);
      if (open_flags == -1)
        {
          internal_error = IOD_EFLAGS;
          goto err;
        }
      fd = open (dev_map->filename, open_flags);
      if (fd == -1)
        {
          internal_error = IOD_ENOENT;
          goto err;
        }
      flags = mode_flags;
    }
  else
    {
      /* Try read-write initially.
         If that fails, then try read-only.
         If that fails, then try write-only.  */
      open_flags = O_RDWR;
      fd = open (dev_map->filename, open_flags);
      flags |= (IOS_F_READ | IOS_F_WRITE);
      if (fd == -1)
        {
          open_flags = O_RDONLY;
          fd = open (dev_map->filename, open_flags);
          if (fd != -1)
            flags &= ~IOS_F_WRITE;
        }
      if (fd == -1)
        {
          open_flags = O_WRONLY;
          fd = open (dev_map->filename, open_flags);
          if (fd != -1)
            flags &= ~IOS_F_READ;
        }
      if (fd == -1)
        {
          internal_error = IOD_ENOENT;
          goto err;
        }
    }

  /* limit the size of the mapping for regular files for avoiding
     SIGBUS when accessing memory outside of the file */
  ret = fstat (fd, &st);
  if (ret == -1)
    {
      internal_error = IOD_ENOENT;
      goto err;
    }
  if ((st.st_mode & S_IFMT) == S_IFREG)
    dev_map->reg_file = 1;
  else
    dev_map->reg_file = 0;

  if (dev_map->reg_file && (st.st_size < dev_map->size))
    dev_map->size = st.st_size;

  dev_map->fd = fd;
  dev_map->flags = flags;
  dev_map->open_flags = open_flags;
  dev_map->prot = ios_dev_mmap_convert_mmap_prot (open_flags);

  dev_map->addr = mmap (0, dev_map->size, dev_map->prot, MAP_SHARED,
                        fd, dev_map->base);
  if (dev_map->addr == MAP_FAILED)
    {
      internal_error = IOD_EMMAP;
      goto err;
    }

  /* should never be the case that returned address is not page aligned as mmap
     fails if dev_map->base is not aligned.
     But we double check because pread and pwrite rely on this alignment */
  if ((unsigned long)dev_map->addr & ((unsigned long)getpagesize () - 1))
    {
      internal_error = IOD_EMMAP;
      goto err;
    }

  if (error)
    *error = IOD_OK;

  return dev_map;

err:
  if (fd != -1)
    close (fd);
  if (dev_map)
    free (dev_map->filename);
  free (dev_map);

  if (error)
    *error = internal_error;
  return NULL;
}

static int
ios_dev_mmap_close (void *iod)
{
  struct ios_dev_mmap *dev_map = iod;

  munmap (dev_map->addr, dev_map->size);
  close (dev_map->fd);

  free (dev_map->filename);
  free (dev_map);
  return IOD_OK;
}

static uint64_t
ios_dev_mmap_get_flags (void *iod)
{
  struct ios_dev_mmap *dev_map = iod;

  return dev_map->flags;
}

static int
ios_dev_mmap_pread (void *iod, void *buf, size_t count, ios_dev_off offset)
{
  struct ios_dev_mmap *dev_map = iod;
  int align = sizeof (void*);
  uint8_t *m = buf;

  if (offset > dev_map->size || count > dev_map->size - offset)
    return IOD_EOF;

  /* copy unaligned bytes
     dev_mmap->addr is always page aligned because of mmap */
  while (count && offset % align)
    {
      *m = *(const volatile uint8_t*)(dev_map->addr + offset);
      count--;
      offset++;
      m++;
    }

  /* copy with the address bus size */
  while (count >= align)
    {
      if (align == 4)
        {
          *(uint32_t*)m = *(const volatile uint32_t*)(dev_map->addr + offset);
        }
      else if (align == 8)
        {
          *(uint64_t*)m = *(const volatile uint64_t*)(dev_map->addr + offset);
        }
      else
        break;
      count -= align;
      offset += align;
      m += align;
    }

  /* copy remaining unaligned bytes */
  while (count)
    {
      *m = *(const volatile uint8_t*)(dev_map->addr + offset);
      count--;
      offset++;
      m++;
    }

  return IOD_OK;
}

static int
ios_dev_mmap_pwrite (void *iod, const void *buf, size_t count,
                     ios_dev_off offset)
{
  struct ios_dev_mmap *dev_map = iod;
  int align = sizeof (void*);
  const uint8_t *m = buf;

  if (offset > dev_map->size || count > dev_map->size - offset)
    return IOD_EOF;

  /* copy unaligned bytes
     dev_mmap->addr is always page aligned because of mmap */
  while (count && offset % align)
    {
      *(volatile uint8_t*)(dev_map->addr + offset) = *(const uint8_t*)m;
      count--;
      offset++;
      m++;
    }

  /* copy with the address bus size */
  while (count >= align)
    {
      if (align == 4)
        {
          *(volatile uint32_t*)(dev_map->addr + offset) = *(const uint32_t*)m;
        }
      else if (align == 8)
        {
          *(volatile uint64_t*)(dev_map->addr + offset) = *(const uint64_t*)m;
        }
      else
        break;
      count -= align;
      offset += align;
      m += align;
    }

  /* copy remaining unaligned bytes */
  while (count)
    {
      *(volatile uint8_t*)(dev_map->addr + offset) = *(const uint8_t*)m;
      count--;
      offset++;
      m++;
    }

  return IOD_OK;
}

static ios_dev_off
ios_dev_mmap_size (void *iod)
{
  struct ios_dev_mmap *dev_map = iod;

  return dev_map->size;
}

static int
ios_dev_mmap_flush (void *iod, ios_dev_off offset)
{
  struct ios_dev_mmap *dev_map = iod;
  int ret;

  if (dev_map->reg_file)
    {
      ret = msync (dev_map->addr, dev_map->size, MS_SYNC);
      if (ret == -1)
        return IOD_EMMAP;
    }

  return IOD_OK;
}

static int
ios_dev_mmap_volatile_by_default (void *iod, const char *handler)
{
  return 1;
}

struct ios_dev_if ios_dev_mmap =
  {
    .get_if_name = ios_dev_mmap_get_if_name,
    .handler_normalize = ios_dev_mmap_handler_normalize,
    .open = ios_dev_mmap_open,
    .close = ios_dev_mmap_close,
    .pread = ios_dev_mmap_pread,
    .pwrite = ios_dev_mmap_pwrite,
    .get_flags = ios_dev_mmap_get_flags,
    .size = ios_dev_mmap_size,
    .flush = ios_dev_mmap_flush,
    .volatile_by_default = ios_dev_mmap_volatile_by_default,
  };
