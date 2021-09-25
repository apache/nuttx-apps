/****************************************************************************
 * apps/fsutils/mkgpt/mkgpt.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <crc32.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef FAR
#define FAR
#endif

#define EFI_VERSION       0x00010000
#define EFI_MAGIC         "EFI PART"
#define EFI_ENTRIES       128
#define EFI_NAMELEN       36

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct efi_header_s
{
  uint8_t  magic[8];
  uint32_t version;
  uint32_t header_sz;
  uint32_t crc32;
  uint32_t reserved;
  uint64_t header_lba;
  uint64_t backup_lba;
  uint64_t first_lba;
  uint64_t last_lba;
  uint8_t  volume_uuid[16];
  uint64_t entries_lba;
  uint32_t entries_count;
  uint32_t entries_size;
  uint32_t entries_crc32;
}
__attribute__ ((packed));

struct efi_entry_s
{
  uint8_t  type_uuid[16];
  uint8_t  uniq_uuid[16];
  uint64_t first_lba;
  uint64_t last_lba;
  uint64_t attr;
  uint16_t name[EFI_NAMELEN];
};

struct efi_ptable_s
{
  uint8_t mbr[512];
  union
  {
    struct efi_header_s header;
    uint8_t block[512];
  };

  struct efi_entry_s entry[EFI_ENTRIES];
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_partition_type_uuid[16] =
{
  0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
  0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};

static const uint8_t g_partition_type_efi[16] =
{
  0x28, 0x73, 0x2a, 0xc1, 0x1f, 0xf8, 0xd2, 0x11,
  0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b,
};

static const uint8_t g_partition_type_linux[16] =
{
  0xa2, 0xa0, 0xd0, 0xeb, 0xe5, 0xb9, 0x33, 0x44,
  0x87, 0xc0, 0x68, 0xb6, 0xb7, 0x26, 0x99, 0xc7,
};

static const uint8_t g_partition_type_swap[16] =
{
  0x6d, 0xfd, 0x57, 0x06, 0xab, 0xa4, 0xc4, 0x43,
  0x84, 0xe5, 0x09, 0x33, 0xc8, 0x4b, 0x4f, 0x4f,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void get_uuid(FAR uint8_t *uuid)
{
  int fd;
  fd = open("/dev/urandom", O_RDONLY);
  if (fd > 0)
    {
      read(fd, uuid, 16);
      close(fd);
    }
}

static void init_mbr(FAR uint8_t *mbr, uint32_t blocks)
{
  /* nonbootable */

  mbr[0x1be] = 0x00;

  /* bogus CHS */

  mbr[0x1bf] = 0x00;
  mbr[0x1c0] = 0x01;
  mbr[0x1c1] = 0x00;

  /* GPT partition */

  mbr[0x1c2] = 0xee;

  /* bogus CHS */

  mbr[0x1c3] = 0xfe;
  mbr[0x1c4] = 0xff;
  mbr[0x1c5] = 0xff;

  /* start */

  mbr[0x1c6] = 0x01;
  mbr[0x1c7] = 0x00;
  mbr[0x1c8] = 0x00;
  mbr[0x1c9] = 0x00;

  memcpy(mbr + 0x1ca, &blocks, sizeof(uint32_t));

  /* signature */

  mbr[0x1fe] = 0x55;
  mbr[0x1ff] = 0xaa;
}

int add_ptn(FAR struct efi_ptable_s *ptbl, uint64_t first, uint64_t last,
            FAR const char *name, FAR const uint8_t *type)
{
  FAR struct efi_header_s *hdr = &ptbl->header;
  FAR struct efi_entry_s *entry = ptbl->entry;
  int n;

  if (first < 34)
    {
      fprintf(stderr, "partition '%s' overlaps partition table\n", name);
      return -1;
    }

  if (last > hdr->last_lba)
    {
      fprintf(stderr, "partition '%s' does not fit on disk\n", name);
      return -1;
    }

  if (hdr->entries_count > EFI_ENTRIES)
    {
      fprintf(stderr, "out of partition table entries\n");
      return -1;
    }

  memcpy(entry[hdr->entries_count].type_uuid, type, 16);
  get_uuid(entry[hdr->entries_count].uniq_uuid);
  entry[hdr->entries_count].first_lba = first;
  entry[hdr->entries_count].last_lba = last;
  entry[hdr->entries_count].attr = 1ull << 63;
  for (n = 0; n < EFI_NAMELEN && *name; n++)
    {
      entry[hdr->entries_count].name[n] = *name++;
    }

  hdr->entries_count++;
  return 0;
}

static int usage(void)
{
  fprintf(stderr,
    "usage: mkgpt write <disk> [ <partition> ]*\n"
    "       mkgpt read <disk>\n"
    "\n"
    "partition:  [<name>]:<size>[kmg][=linux|=swap|=efi]\n"
    "       or:  @<file-of-partitions>\n"
    );
  return 0;
}

static void show(FAR struct efi_ptable_s *ptbl)
{
  FAR struct efi_entry_s *entry = ptbl->entry;
  char name[EFI_NAMELEN + 1];
  unsigned n;
  unsigned m;

  fprintf(stderr, "ptn  start block   end block     name\n");
  fprintf(stderr, "---- ------------- ------------- --------------------\n");

  for (n = 0; n < ptbl->header.entries_count; n++)
    {
      for (m = 0; m < EFI_NAMELEN; m++)
        {
          name[m] = entry[n].name[m] & 127;
        }

      name[m] = 0;
      fprintf(stderr, "#%03d %13" PRIu64 " %13" PRIu64" %s\n",
              n + 1, entry[n].first_lba, entry[n].last_lba, name);
    }
}

static uint64_t parse_size(FAR const char *sz)
{
  size_t l = strlen(sz);
  uint64_t n = strtoull(sz, 0, 0);

  if (l)
    {
      switch (sz[l - 1])
        {
          case 'k':
          case 'K':
            n *= 1024;
            break;
          case 'm':
          case 'M':
            n *= 1024 * 1024;
            break;
          case 'g':
          case 'G':
            n *= 1024 * 1024 * 1024;
            break;
        }
    }

  return n;
}

static int parse_ptn(FAR struct efi_ptable_s *ptbl, FAR char *x,
                     FAR uint64_t *lba)
{
  FAR const uint8_t *type = g_partition_type_uuid;
  FAR char *y = strchr(x, ':');
  FAR char *z;
  uint64_t sz;

  if (!y)
    {
      fprintf(stderr, "invalid partition entry: %s\n", x);
      return -1;
    }

  *y++ = 0;
  z = strchr(y, '=');
  if (z)
    {
      *z++ = 0;
      if (!strcmp(z, "efi"))
        {
          type = g_partition_type_efi;
        }
      else if (!strcmp(z, "linux"))
        {
          type = g_partition_type_linux;
        }
      else if (!strcmp(z, "swap"))
        {
          type = g_partition_type_swap;
        }
    }

  if (*y == '0')
    {
      sz = ptbl->header.last_lba - *lba;
    }
  else
    {
      sz = parse_size(y);
      if (sz & 511)
        {
          fprintf(stderr, "partition size must be multiple of 512\n");
          return -1;
        }

      sz /= 512;
    }

  if (sz == 0)
    {
      fprintf(stderr, "zero size partitions not allowed\n");
      return -1;
    }

  if (x[0] && add_ptn(ptbl, *lba, *lba + sz - 1, x, type))
    {
      return -1;
    }

  *lba += sz;
  return 0;
}

static void update_crc32(FAR struct efi_ptable_s *ptbl)
{
  uint32_t n;

  n = crc32part((FAR const uint8_t *)ptbl->entry,
                sizeof(ptbl->entry[0]) *
                ptbl->header.entries_count, ~0l) ^ ~0l;
  ptbl->header.entries_crc32 = n;

  ptbl->header.crc32 = 0;
  n = crc32part((FAR const uint8_t *)&ptbl->header,
                sizeof(ptbl->header), ~0l) ^ ~0l;
  ptbl->header.crc32 = n;
}

static int verify_gpt_pratition(FAR struct efi_ptable_s *ptbl)
{
  FAR struct efi_header_s *hdr = &ptbl->header;
  uint32_t orgcrc;

  if ((ptbl->mbr[0x1c2] != 0xee && ptbl->mbr[0x1c2] != 0x00) ||
      ptbl->mbr[0x1fe] != 0x55 || ptbl->mbr[0x1ff] != 0xaa)
    {
      fprintf(stderr, "Invalid Protective or Hybrid MBR(%x)\n",
              ptbl->mbr[0x1c2]);
      return -1;
    }

  if (memcmp(hdr->magic, EFI_MAGIC, sizeof(hdr->magic)))
    {
      fprintf(stderr, "GPT: Header signature is wrong\n");
      return -1;
    }

  orgcrc = hdr->crc32;
  hdr->crc32 = 0;
  if (orgcrc != (crc32part((FAR const uint8_t *)hdr,
      sizeof(*hdr), ~0l) ^ ~0l))
    {
      fprintf(stderr, "GPT: Header CRC is wrong\n");
      return -1;
    }

  hdr->crc32 = orgcrc;
  if (hdr->header_lba != 1)
    {
      fprintf(stderr, "GPT: Header starting lba incorrect\n");
      return -1;
    }

  if (hdr->entries_crc32 != (crc32part((FAR const uint8_t *)ptbl->entry,
      sizeof(ptbl->entry[0]) * ptbl->header.entries_count, ~0l) ^ ~0l))
    {
      fprintf(stderr, "GPT: Header entry signature is wrong\n");
      return -1;
    }

  show(ptbl);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char **argv)
{
  FAR struct efi_header_s *hdr;
  FAR struct efi_ptable_s *ptbl;
  FAR const char *device;
  struct stat s;
  uint64_t sz;
  uint64_t lba;
  uint64_t off;
  int ret = -1;
  int fd;
  bool verify = false;

  if (argc < 2)
    {
      return usage();
    }

  device = argv[2];
  if (!strcmp(argv[1], "write") && argc >= 3)
    {
      argc -= 2;
      argv += 2;
    }
  else if (!strcmp(argv[1], "read"))
    {
      verify = true;
    }
  else
    {
      return usage();
    }

  fd = open(device, O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "error: cannot open '%s'\n", device);
      return ret;
    }

  if (fstat(fd, &s))
    {
      fprintf(stderr, "error: cannot stat '%s'\n", device);
      goto err;
    }

  sz = s.st_size;
  if (sz & 511)
    {
      fprintf(stderr, "error: file size not multiple of 512\n");
      goto err;
    }

  sz /= 512;
  ptbl = calloc(1, sizeof(*ptbl));
  if (!ptbl)
    {
      goto err;
    }

  if (verify)
    {
      if (read(fd, ptbl, sizeof(*ptbl)) != sizeof(*ptbl))
        {
          fprintf(stderr, "error read primary partition table\n");
          goto out;
        }

      ret = verify_gpt_pratition(ptbl);
      goto out;
    }

  init_mbr(ptbl->mbr, sz - 1);
  hdr = &ptbl->header;
  memcpy(hdr->magic, EFI_MAGIC, sizeof(hdr->magic));
  hdr->version = EFI_VERSION;
  hdr->header_sz = sizeof(struct efi_header_s);
  hdr->header_lba = 1;
  hdr->backup_lba = sz - 1;
  hdr->first_lba = 34;
  hdr->last_lba = sz - 33;
  get_uuid(hdr->volume_uuid);
  hdr->entries_lba = 2;
  hdr->entries_count = 0;
  hdr->entries_size = sizeof(struct efi_entry_s);
  lba = hdr->first_lba;

  while (argc > 1)
    {
      if (argv[1][0] == '@')
        {
          FAR char *p;
          FAR FILE *f;
          char line[256];

          f = fopen(argv[1] + 1, "r");
          if (!f)
            {
              fprintf(stderr, "cannot read partitions from '%s\n", argv[1]);
              goto out;
            }

          while (fgets(line, sizeof(line), f))
            {
              p = line + strlen(line);
              while (p > line)
                {
                  if (*--p > ' ')
                    {
                      break;
                    }

                  *p = 0;
                }

              p = line;
              while (*p && (*p <= ' '))
                {
                  p++;
                }

              if (*p == '#')
                {
                  continue;
                }

              if (*p == 0)
                {
                  continue;
                }

              if (parse_ptn(ptbl, p, &lba))
                {
                  fclose(f);
                  goto out;
                }
            }

          fclose(f);
        }
      else
        {
          if (parse_ptn(ptbl, argv[1], &lba))
            {
              goto out;
            }
        }

      argc--;
      argv++;
    }

  update_crc32(ptbl);
  show(ptbl);
  off = (sz - 33) * 512;
  if (write(fd, ptbl, sizeof(*ptbl)) != sizeof(*ptbl))
    {
      fprintf(stderr, "error writing primary partition table\n");
      goto out;
    }

  if (lseek(fd, off, SEEK_SET) != off)
    {
      fprintf(stderr, "error seeking to secondary partition table\n");
      goto out;
    }

  /* reverse these for the secondary copy */

  hdr->header_lba = sz -1;
  hdr->backup_lba = 1;
  update_crc32(ptbl);

  if (write(fd, &ptbl->entry, sizeof(ptbl->entry)) !=
      sizeof(ptbl->entry))
    {
      fprintf(stderr, "error writing secondary partition table\n");
      goto out;
    }

  if (write(fd, &ptbl->header, 512) != 512)
    {
      fprintf(stderr, "error writing secondary partition header\n");
      goto out;
    }

  fsync(fd);

out:
  free(ptbl);
err:
  close(fd);
  return ret;
}
