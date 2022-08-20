/****************************************************************************
 * apps/fsutils/mkfatfs/fat32.h
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

#ifndef __APPS_FSUTILS_MKFATFS_FAT32_H
#define __APPS_FSUTILS_MKFATFS_FAT32_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <time.h>

#include <nuttx/kmalloc.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * These offsets describes the master boot record (MBR).
 *
 * The following fields are common to FAT12/16/32 (but all value descriptions
 * refer to the interpretation under FAT32).
 */

#define MBR_JUMP             0 /*  3@0:  Jump instruction to boot code (ignored) */
#define MBR_OEMNAME          3 /*  8@3:  Usually "MSWIN4.1" */
#define MBR_BYTESPERSEC     11 /*  2@11: Bytes per sector: 512, 1024, 2048, 4096  */
#define MBR_SECPERCLUS      13 /*  1@13: Sectors per allocation unit: 2**n, n=0..7 */
#define MBR_RESVDSECCOUNT   14 /*  2@14: Reserved sector count: Usually 32 */
#define MBR_NUMFATS         16 /*  1@16: Number of FAT data structures: always 2 */
#define MBR_ROOTENTCNT      17 /*  2@17: FAT12/16: Must be 0 for FAT32 */
#define MBR_TOTSEC16        19 /*  2@19: FAT12/16: Must be 0, see MBR_TOTSEC32 */
#define MBR_MEDIA           21 /*  1@21: Media code: f0, f8, f9-fa, fc-ff */
#define MBR_FATSZ16         22 /*  2@22: FAT12/16: Must be 0, see MBR_FATSZ32 */
#define MBR_SECPERTRK       24 /*  2@24: Sectors per track geometry value */
#define MBR_NUMHEADS        26 /*  2@26: Number of heads geometry value */
#define MBR_HIDSEC          28 /*  4@28: Count of hidden sectors preceding FAT */
#define MBR_TOTSEC32        32 /*  4@32: Total count of sectors on the volume */

/* The following fields are only valid for FAT12/16 */

#define MBR16_DRVNUM        36 /*  1@36: Drive number for MSDOS bootstrap */
                               /*  1@37: Reserved (zero) */
#define MBR16_BOOTSIG       38 /*  1@38: Extended boot signature: 0x29 if following valid */
#define MBR16_VOLID         39 /*  4@39: Volume serial number */
#define MBR16_VOLLAB        43 /* 11@43: Volume label */
#define MBR16_FILESYSTYPE   54 /*  8@54: "FAT12  ", "FAT16  ", or "FAT    " */

#define MBR16_BOOTCODE      62 /* Boot code may be placed in the remainder of the sector */
#define MBR16_BOOTCODESIZE 448

/* The following fields are only valid for FAT32 */

#define MBR32_FATSZ32       36 /*  4@36: Count of sectors occupied by one FAT */
#define MBR32_EXTFLAGS      40 /*  2@40: 0-3:Active FAT, 7=0 both FATS, 7=1 one FAT */
#define MBR32_FSVER         42 /*  2@42: MSB:Major LSB:Minor revision number (0.0) */
#define MBR32_ROOTCLUS      44 /*  4@44: Cluster no. of 1st cluster of root dir */
#define MBR32_FSINFO        48 /*  2@48: Sector number of fsinfo structure. Usually 1. */
#define MBR32_BKBOOTSEC     50 /*  2@50: Sector number of boot record. Usually 6  */
                               /* 12@52: Reserved (zero) */
#define MBR32_DRVNUM        64 /*  1@64: Drive number for MSDOS bootstrap */
                               /*  1@65: Reserved (zero) */
#define MBR32_BOOTSIG       66 /*  1@66: Extended boot signature: 0x29 if following valid */
#define MBR32_VOLID         67 /*  4@67: Volume serial number */
#define MBR32_VOLLAB        71 /* 11@71: Volume label */
#define MBR32_FILESYSTYPE   82 /*  8@82: "FAT12  ", "FAT16  ", or "FAT    " */

#define MBR32_BOOTCODE      90 /* Boot code may be placed in the remainder of the sector */
#define MBR32_BOOTCODESIZE 420

/* If the sector is not an MBR, then it could have a partition table at
 * this offset.
 */

#define MBR_TABLE          446

/* The magic bytes at the end of the MBR are common to FAT12/16/32 */

#define MBR_SIGNATURE      510 /*  2@510: Valid MBRs have 0x55aa here */

#define BOOT_SIGNATURE16   0xaa55
#define BOOT_SIGNATURE32   0xaa550000

/* The extended boot signature (BS16/32_BOOTSIG) */

#define EXTBOOT_SIGNATURE  0x29

/* These offsets describes the partition table.
 * 446@0: Generally unused and zero; but may include IDM Boot Manager menu
 * entry at 8@394
 */

/* n = 0,1,2,3 */

#define PART_ENTRY(n)     (446+((n) << 4))
#define PART_ENTRY1        446 /* 16@446: Partition table, first entry */
#define PART_ENTRY2        462 /* 16@462: Partition table, second entry */
#define PART_ENTRY3        478 /* 16@478: Partition table, third entry */
#define PART_ENTRY4        494 /* 16@494: Partition table, fourth entry */
#define PART_SIGNATURE     510 /* 2@510: Valid partitions have 0x55aa here */

/****************************************************************************
 * These offsets describes one partition table entry.  NOTE that ent entries
 * are aligned to 16-bit offsets so that the STARTSECTOR and SIZE values are
 * not properly aligned.
 */

#define PART_BOOTINDICATOR   0  /* 1@0:  Boot indicator (0x80: active;0x00:otherwise) */
#define PART_STARTCHS        1  /* 3@1:  Starting Cylinder/Head/Sector values */
#define PART_TYPE            4  /* 1@4:  Partition type description */
#define PART_ENDCHS          5  /* 3@5:  Ending Cylinder/Head/Sector values */
#define PART_STARTSECTOR     8  /* 4@8:  Starting sector */
#define PART_SIZE           12  /* 4@12: Partition size (in sectors) */

/****************************************************************************
 * Partition table types.
 */

#define PART_TYPE_NONE       0  /* No partition */
#define PART_TYPE_FAT12      1  /* FAT12 */
#define PART_TYPE_FAT16A     4  /* FAT16 (Partition smaller than 32MB) */
#define PART_TYPE_EXT        5  /* Extended MS-DOS Partition */
#define PART_TYPE_FAT16B     6  /* FAT16 (Partition larger than 32MB) */
#define PART_TYPE_FAT32     11  /* FAT32 (Partition up to 2048Gb) */
#define PART_TYPE_FAT32X    12  /* Same as 11, but uses LBA1 0x13 extensions */
#define PART_TYPE_FAT16X    14  /* Same as 6, but uses LBA1 0x13 extensions */
#define PART_TYPE_EXTX      15  /* Same as 5, but uses LBA1 0x13 extensions */

/****************************************************************************
 * Fat Boot Record (FBR).
 *
 * The FBR is always the very first sector in the partition. Validity check
 * is performed by comparing the 16 bit word at offset 1FE to AA55.  The
 * structure of the FBR is shown below.
 *
 * NOTE that the fields of the FBR are equivalent to the MBR.
 */

#define FBR_OEMNAME          3 /*  8@3:  Usually "MSWIN4.1" */
#define FBR_BYTESPERSEC     11 /*  2@11: Bytes per sector: 512, 1024, 2048, 4096  */
#define FBR_SECPERCLUS      13 /*  1@13: Sectors per allocation unit: 2**n, n=0..7 */
#define FBR_RESVDSECCOUNT   14 /*  2@14: Reserved sector count: Usually 32 */
#define FBR_NUMFATS         16 /*  1@16: Number of FAT data structures: always 2 */
#define FBR_ROOTENTCNT      17 /*  2@17: FAT12/16: Must be 0 for FAT32 */
#define FBR_TOTSEC16        19 /*  2@19: FAT12/16: Must be 0, see FBR_TOTSEC32 */
#define FBR_MEDIA           21 /*  1@21: Media code: f0, f8, f9-fa, fc-ff */
#define FBR_FATSZ16         22 /*  2@22: FAT12/16: Must be 0, see FBR_FATSZ32 */
#define FBR_SECPERTRK       24 /*  2@24: Sectors per track geometry value */
#define FBR_NUMHEADS        26 /*  2@26: Number of heads geometry value */
#define FBR_HIDSEC          28 /*  4@28: Count of hidden sectors preceding FAT */
#define FBR_TOTSEC32        32 /*  4@32: Total count of sectors on the volume */
#define FBR_FATSZ32         36 /*  4@36: Count of sectors occupied by one FAT (FAT32) */
#define FBR_EXTFLAGS        40 /*  2@40: Ext Flags */
#define FBR_FSVER           42 /*  2@42: FS Version */
#define FBR_ROOTCLUS        44 /*  4@44: Cluster no. of 1st cluster of root dir */
#define FBR_FSINFO          48 /*  2@48: FS Info Sector */
#define FBR_BKBOOTSEC       50 /*  2@50: Sector number of boot record. Usually 6  */
                               /*  52-301 Reserved */

/* The following fields are only valid for FAT12/16 */

#define FBR16_DRVNUM        36 /*  1@36: Drive number for MSDOS bootstrap */
                               /*  1@37: Reserved (zero) */
#define FBR16_BOOTSIG       38 /*  1@38: Extended boot signature: 0x29 if following valid */
#define FBR16_VOLID         39 /*  4@39: Volume serial number */
#define FBR16_VOLLAB        43 /* 11@43: Volume label */
#define FBR16_FILESYSTYPE   54 /*  8@54: "FAT12  ", "FAT16  ", or "FAT    " */

/* The following fields are only valid for FAT32 */

#define FBR32_DRVNUM        64 /*  1@64: Drive number for MSDOS bootstrap */
#define FBR32_BOOTSIG       65 /*  1@65: Extended boot signature: 0x29 if following valid */
#define FBR32_VOLID         66 /*  4@66: Volume serial number */
#define FBR32_VOLLAB        71 /* 11@71: Volume label */
#define FBR32_FILESYSTYPE   82 /*  8@62: "FAT12  ", "FAT16  ", or "FAT    " */

/* The magic bytes at the end of the MBR are common to FAT12/16/32 */

#define FBR_SIGNATURE      510 /*  2@510: Valid MBRs have 0x55aa here */

/****************************************************************************
 * Each FAT "short" 8.3 file name directory entry is 32-bytes long.
 *
 * Sizes and limits
 */

/****************************************************************************
 * Each FAT "short" 8.3 file name directory entry is 32-bytes long.
 *
 * Sizes and limits
 */

#define DIR_MAXFNAME        11  /* Max short name size is 8+3 = 11 */

/* The following define offsets relative to the beginning of a directory
 * entry.
 */

#define DIR_NAME             0 /* 11@ 0: NAME: 8 bytes + 3 byte extension */
#define DIR_ATTRIBUTES      11 /*  1@11: File attributes (see below) */
#define DIR_NTRES           12 /*  1@12: Reserved for use by NT */
#define DIR_CRTTIMETENTH    13 /*  1@13: Tenth sec creation timestamp */
#define DIR_CRTIME          14 /*  2@14: Time file created */
#define DIR_CRDATE          16 /*  2@16: Date file created */
#define DIR_LASTACCDATE     18 /*  2@19: Last access date */
#define DIR_FSTCLUSTHI      20 /*  2@20: MS first cluster number */
#define DIR_WRTTIME         22 /*  2@22: Time of last write */
#define DIR_WRTDATE         24 /*  2@24: Date of last write */
#define DIR_FSTCLUSTLO      26 /*  2@26: LS first cluster number */
#define DIR_FILESIZE        28 /*  4@28: File size in bytes */
#define DIR_SIZE            32 /* The size of one directory entry */
#define DIR_SHIFT            5 /* log2 of DIR_SIZE */

/* First byte of the directory name has special meanings: */

#define DIR0_EMPTY         0xe5 /* The directory entry is empty */
#define DIR0_ALLEMPTY      0x00 /* This entry and all following are empty */
#define DIR0_E5            0x05 /* The actual value is 0xe5 */

/* NTRES flags in the FAT directory */

#define FATNTRES_LCNAME    0x08 /* Lower case in name */
#define FATNTRES_LCEXT     0x10 /* Lower case in extension */

/* Directory indexing helper.  Each directory entry is 32-bytes in length.
 * The number of directory entries in a sector then varies with the size
 * of the sector supported in hardware.
 */

#define DIRSEC_NDXMASK(f)   (((f)->fs_hwsectorsize - 1) >> 5)
#define DIRSEC_NDIRS(f)     (((f)->fs_hwsectorsize) >> 5)
#define DIRSEC_BYTENDX(f,i) (((i) & DIRSEC_NDXMASK(fs)) << 5)

#define SEC_NDXMASK(f)      ((f)->fs_hwsectorsize - 1)
#define SEC_NSECTORS(f,n)   ((n) / (f)->fs_hwsectorsize)

#define CLUS_NDXMASK(f)     ((f)->fs_fatsecperclus - 1)

/* The FAT "long" file name (LFN) directory entry */

#ifdef CONFIG_FAT_LFN

/* Sizes and limits */

# ifndef CONFIG_FAT_MAXFNAME   /* The maximum support filename can be limited */
#   define LDIR_MAXFNAME   255 /* Max unicode characters in file name */
# elif CONFIG_FAT_MAXFNAME <= 255
#   define LDIR_MAXFNAME  CONFIG_FAT_MAXFNAME
# else
#   error "Illegal value for CONFIG_FAT_MAXFNAME"
# endif

# define LDIR_MAXLFNCHARS   13  /* Max unicode characters in one LFN entry */
# define LDIR_MAXLFNS       20  /* Max number of LFN entries */

/* LFN directory entry offsets */

# define LDIR_SEQ            0  /*  1@ 0: Sequence number */
# define LDIR_WCHAR1_5       1  /* 10@ 1: File name characters 1-5 (5 Unicode characters) */
# define LDIR_ATTRIBUTES    11  /*  1@11: File attributes (always 0x0f) */
# define LDIR_NTRES         12  /*  1@12: Reserved for use by NT  (always 0x00) */
# define LDIR_CHECKSUM      13  /*  1@13: Checksum of the DOS filename */
# define LDIR_WCHAR6_11     14  /* 12@14: File name characters 6-11 (6 Unicode characters) */
# define LDIR_FSTCLUSTLO    26  /*  2@26: First cluster (always 0x0000) */
# define LDIR_WCHAR12_13    28  /*  4@28: File name characters 12-13 (2 Unicode characters) */

/* LFN sequence number and allocation status */

# define LDIR0_EMPTY       DIR0_EMPTY    /* The directory entry is empty */
# define LDIR0_ALLEMPTY    DIR0_ALLEMPTY /* This entry and all following are empty */
# define LDIR0_E5          DIR0_E5       /* The actual value is 0xe5 */
# define LDIR0_LAST        0x40          /* Last LFN in file name (appears first) */
# define LDIR0_SEQ_MASK    0x1f          /* Mask for sequence number (1-20) */

/* The LFN entry attribute */

# define LDDIR_LFNATTR     0x0f
#endif

/* File system types */

#define FSTYPE_FAT12         0
#define FSTYPE_FAT16         1
#define FSTYPE_FAT32         2

/* File buffer flags (ff_bflags) */

#define FFBUFF_VALID         1
#define FFBUFF_DIRTY         2
#define FFBUFF_MODIFIED      4

/* Mount status flags (ff_bflags) */

#define UMOUNT_FORCED        8

/****************************************************************************
 * These offset describe the FSINFO sector
 */

#define FSI_LEADSIG          0 /*   4@0:   0x41615252  = "RRaA" */
                               /* 480@4:   Reserved (zero) */
#define FSI_STRUCTSIG      484 /*   4@484: 0x61417272 = "rrAa" */
#define FSI_FREECOUNT      488 /*   4@488: Last free cluster count on volume */
#define FSI_NXTFREE        492 /*   4@492: Cluster number of 1st free cluster */
                               /*  12@496: Reserved (zero) */
#define FSI_TRAILSIG       508 /*   4@508: 0xaa550000 */

/****************************************************************************
 * FAT values
 */

#define FAT_EOF            0x0ffffff8
#define FAT_BAD            0x0ffffff7

/****************************************************************************
 * Maximum cluster by FAT type.  This is the key value used to distinguish
 * between FAT12, 16, and 32.
 */

/* FAT12: For M$, the calculation is ((1 << 12) - 19).  But we will follow
 * the Linux tradition of allowing slightly more clusters for FAT12.
 */

#define FAT_MAXCLUST12 ((1 << 12) - 16)

/* FAT16: For M$, the calculation is ((1 << 16) - 19). (The uint32_t cast is
 * needed for architectures where int is only 16 bits).
 */

#define FAT_MINCLUST16 (FAT_MAXCLUST12 + 1)
#define FAT_MAXCLUST16 (((uint32_t)1 << 16) - 16)

/* FAT32: M$ reserves the MS 4 bits of a FAT32 FAT entry so only 18 bits are
 * available.  For M$, the calculation is ((1 << 28) - 19). (The uint32_t
 * cast is needed for architectures where int is only 16 bits).  M$ also
 * claims that the minimum size is 65,527.
 */

#define FAT_MINCLUST32  65524
/* #define FAT_MINCLUST32  (FAT_MAXCLUST16 + 1) */
#define FAT_MAXCLUST32  (((uint32_t)1 << 28) - 16)

/* Endian-ness helpers */

#ifdef CONFIG_ENDIAN_BIG
#  define FAT_PUTUINT16(p,v) \
    do \
      { \
        (p)[0] = ((v) >> 8); \
        (p)[1] = ((v) & 0xff); \
    } \
    while (0)
#else
#  define FAT_PUTUINT16(p,v) \
    do \
      { \
        (p)[0] = ((v) & 0xff); \
        (p)[1] = ((v) >> 8); \
    } \
    while (0)
#endif

#ifdef CONFIG_ENDIAN_BIG
#  define FAT_PUTUINT32(p,v) \
    do \
      { \
        (p)[0] =  ((v) >> 24); \
        (p)[1] = (((v) >> 16) & 0xff); \
        (p)[2] = (((v) >>  8) & 0xff); \
        (p)[3] =  ((v)        & 0xff); \
    } \
    while (0)
#else
#  define FAT_PUTUINT32(p,v) \
    do \
      { \
        (p)[0] =  ((v)        & 0xff); \
        (p)[1] = (((v) >>  8) & 0xff); \
        (p)[2] = (((v) >> 16) & 0xff); \
        (p)[3] =  ((v) >> 24); \
    } \
    while (0)
#endif

/* Access to data in raw sector data */

#define UBYTE_VAL(p,o)            (((uint8_t*)(p))[o])
#define UBYTE_PTR(p,o)            &UBYTE_VAL(p,o)
#define UBYTE_PUT(p,o,v)          (UBYTE_VAL(p,o)=(uint8_t)(v))

#define UINT16_PTR(p,o)           ((uint16_t*)UBYTE_PTR(p,o))
#define UINT16_VAL(p,o)           (*UINT16_PTR(p,o))
#define UINT16_PUT(p,o,v)         (UINT16_VAL(p,o)=(uint16_t)(v))

#define UINT32_PTR(p,o)           ((uint32_t*)UBYTE_PTR(p,o))
#define UINT32_VAL(p,o)           (*UINT32_PTR(p,o))
#define UINT32_PUT(p,o,v)         (UINT32_VAL(p,o)=(uint32_t)(v))

/* Regardless of the endian-ness of the target or alignment of the data, no
 * special operations are required for byte, string or byte array accesses.
 * The FAT data stream is little endian so multiple byte values must be
 * accessed byte-by-byte for big-endian targets.
 */

#define MBR_PUTSECPERCLUS(p,v)    UBYTE_PUT(p,MBR_SECPERCLUS,v)
#define MBR_PUTNUMFATS(p,v)       UBYTE_PUT(p,MBR_NUMFATS,v)
#define MBR_PUTMEDIA(p,v)         UBYTE_PUT(p,MBR_MEDIA,v)
#define MBR_PUTDRVNUM16(p,v)      UBYTE_PUT(p,MBR16_DRVNUM,v)
#define MBR_PUTDRVNUM32(p,v)      UBYTE_PUT(p,MBR32_DRVNUM,v)
#define MBR_PUTBOOTSIG16(p,v)     UBYTE_PUT(p,MBR16_BOOTSIG,v)
#define MBR_PUTBOOTSIG32(p,v)     UBYTE_PUT(p,MBR32_BOOTSIG,v)

#define FBR_PUTSECPERCLUS(p,v)    UBYTE_PUT(p,FBR_SECPERCLUS,v)
#define FBR_PUTNUMFATS(p,v)       UBYTE_PUT(p,FBR_NUMFATS,v)
#define FBR_PUTMEDIA(p,v)         UBYTE_PUT(p,FBR_MEDIA,v)
#define FBR_PUTDRVNUM16(p,v)      UBYTE_PUT(p,FBR16_DRVNUM,v)
#define FBR_PUTDRVNUM32(p,v)      UBYTE_PUT(p,FBR32_DRVNUM,v)
#define FBR_PUTBOOTSIG16(p,v)     UBYTE_PUT(p,FBR16_BOOTSIG,v)
#define FBR_PUTBOOTSIG32(p,v)     UBYTE_PUT(p,FBR32_BOOTSIG,v)

#define PART_PUTTYPE(n,p,v)       UBYTE_PUT(p,PART_ENTRY(n)+PART_TYPE,v)
#define PART1_PUTTYPE(p,v)        UBYTE_PUT(p,PART_ENTRY1+PART_TYPE,v)
#define PART2_PUTTYPE(p,v)        UBYTE_PUT(p,PART_ENTRY2+PART_TYPE,v)
#define PART3_PUTTYPE(p,v)        UBYTE_PUT(p,PART_ENTRY3+PART_TYPE,v)
#define PART4_PUTTYPE(p,v)        UBYTE_PUT(p,PART_ENTRY4+PART_TYPE,v)

#define DIR_PUTATTRIBUTES(p,v)    UBYTE_PUT(p,DIR_ATTRIBUTES,v)
#define DIR_PUTNTRES(p,v)         UBYTE_PUT(p,DIR_NTRES,v)
#define DIR_PUTCRTTIMETENTH(p,v)  UBYTE_PUT(p,DIR_CRTTIMETENTH,v)

#ifdef CONFIG_FAT_LFN
# define LDIR_PUTSEQ(p,v)         UBYTE_PUT(p,LDIR_SEQ,v)
# define LDIR_PUTATTRIBUTES(p,v)  UBYTE_PUT(p,LDIR_ATTRIBUTES,v)
# define LDIR_PUTNTRES(p,v)       UBYTE_PUT(p,LDIR_NTRES,v)
# define LDIR_PUTCHECKSUM(p,v)    UBYTE_PUT(p,LDIR_CHECKSUM,v)
#endif

/* For the all targets, unaligned values need to be accessed byte-by-byte.
 * Some architectures may handle unaligned accesses with special interrupt
 * handlers.  But even in that case, it is more efficient to avoid the traps.
 */

/* Unaligned multi-byte access macros */

#define MBR_PUTBYTESPERSEC(p,v)    FAT_PUTUINT16(UBYTE_PTR(p,MBR_BYTESPERSEC),v)
#define MBR_PUTROOTENTCNT(p,v)     FAT_PUTUINT16(UBYTE_PTR(p,MBR_ROOTENTCNT),v)
#define MBR_PUTTOTSEC16(p,v)       FAT_PUTUINT16(UBYTE_PTR(p,MBR_TOTSEC16),v)
#define MBR_PUTVOLID16(p,v)        FAT_PUTUINT32(UBYTE_PTR(p,MBR16_VOLID),v)
#define MBR_PUTVOLID32(p,v)        FAT_PUTUINT32(UBYTE_PTR(p,MBR32_VOLID),v)

#define FBR_PUTBYTESPERSEC(p,v)    fat_putuint16(UBYTE_PTR(p,FBR_BYTESPERSEC),v)
#define FBR_PUTROOTENTCNT(p,v)     fat_putuint16(UBYTE_PTR(p,FBR_ROOTENTCNT),v)
#define FBR_PUTTOTSEC16(p,v)       fat_putuint16(UBYTE_PTR(p,FBR_TOTSEC16),v)
#define FBR_PUTVOLID16(p,v)        fat_putuint32(UBYTE_PTR(p,FBR16_VOLID),v)
#define FBR_PUTVOLID32(p,v)        fat_putuint32(UBYTE_PTR(p,FBR32_VOLID),v)

#define PART_PUTSTARTSECTOR(n,p,v) FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY(n)+PART_STARTSECTOR),v)
#define PART_PUTSIZE(n,p,v)        FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY(n)+PART_SIZE),v)
#define PART1_PUTSTARTSECTOR(p,v)  FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY1+PART_STARTSECTOR),v)
#define PART1_PUTSIZE(p,v)         FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY1+PART_SIZE),v)
#define PART2_PUTSTARTSECTOR(p,v)  FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY2+PART_STARTSECTOR),v)
#define PART2_PUTSIZE(p,v)         FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY2+PART_SIZE),v)
#define PART3_PUTSTARTSECTOR(p,v)  FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY3+PART_STARTSECTOR),v)
#define PART3_PUTSIZE(p,v)         FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY3+PART_SIZE),v)
#define PART4_PUTSTARTSECTOR(p,v)  FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY4+PART_STARTSECTOR),v)
#define PART4_PUTSIZE(p,v)         FAT_PUTUINT32(UBYTE_PTR(p,PART_ENTRY4+PART_SIZE),v)

#ifdef CONFIG_FAT_LFN
# define LDIR_PTRWCHAR1_5(p)       UBYTE_PTR(p,LDIR_WCHAR1_5)
# define LDIR_PTRWCHAR6_11(p)      UBYTE_PTR(p,LDIR_WCHAR6_11)
# define LDIR_PTRWCHAR12_13(p)     UBYTE_PTR(p,LDIR_WCHAR12_13)
#endif

/* But for multi-byte values, the endian-ness of the target vs. the little
 * endian order of the byte stream or alignment of the data within the byte
 * stream can force special, byte-by-byte accesses.
 */

#ifdef CONFIG_ENDIAN_BIG

/* If the target is big-endian, then even aligned multi-byte values must be
 * accessed byte-by-byte.
 */

# define MBR_PUTRESVDSECCOUNT(p,v) FAT_PUTUINT16(UBYTE_PTR(p,MBR_RESVDSECCOUNT),v)
# define MBR_PUTFATSZ16(p,v)       FAT_PUTUINT16(UBYTE_PTR(p,MBR_FATSZ16),v)
# define MBR_PUTSECPERTRK(p,v)     FAT_PUTUINT16(UBYTE_PTR(p,MBR_SECPERTRK),v)
# define MBR_PUTNUMHEADS(p,v)      FAT_PUTUINT16(UBYTE_PTR(p,MBR_NUMHEADS),v)
# define MBR_PUTHIDSEC(p,v)        FAT_PUTUINT32(UBYTE_PTR(p,MBR_HIDSEC),v)
# define MBR_PUTTOTSEC32(p,v)      FAT_PUTUINT32(UBYTE_PTR(p,MBR_TOTSEC32),v)
# define MBR_PUTFATSZ32(p,v)       FAT_PUTUINT32(UBYTE_PTR(p,MBR32_FATSZ32),v)
# define MBR_PUTEXTFLAGS(p,v)      FAT_PUTUINT16(UBYTE_PTR(p,MBR32_EXTFLAGS),v)
# define MBR_PUTFSVER(p,v)         FAT_PUTUINT16(UBYTE_PTR(p,MBR32_FSVER),v)
# define MBR_PUTROOTCLUS(p,v)      FAT_PUTUINT32(UBYTE_PTR(p,MBR32_ROOTCLUS),v)
# define MBR_PUTFSINFO(p,v)        FAT_PUTUINT16(UBYTE_PTR(p,MBR32_FSINFO),v)
# define MBR_PUTBKBOOTSEC(p,v)     FAT_PUTUINT16(UBYTE_PTR(p,MBR32_BKBOOTSEC),v)
# define MBR_PUTSIGNATURE(p,v)     FAT_PUTUINT16(UBYTE_PTR(p,MBR_SIGNATURE),v)

# define FBR_PUTRESVDSECCOUNT(p,v) fat_putuint16(UBYTE_PTR(p,FBR_RESVDSECCOUNT),v)
# define FBR_PUTFATSZ16(p,v)       fat_putuint16(UBYTE_PTR(p,FBR_FATSZ16),v)
# define FBR_PUTSECPERTRK(p,v)     fat_putuint16(UBYTE_PTR(p,FBR_SECPERTRK),v)
# define FBR_PUTNUMHEADS(p,v)      fat_putuint16(UBYTE_PTR(p,FBR_NUMHEADS),v)
# define FBR_PUTHIDSEC(p,v)        fat_putuint32(UBYTE_PTR(p,FBR_HIDSEC),v)
# define FBR_PUTTOTSEC32(p,v)      fat_putuint32(UBYTE_PTR(p,FBR_TOTSEC32),v)
# define FBR_PUTFATSZ32(p,v)       fat_putuint32(UBYTE_PTR(p,FBR_FATSZ32),v)
# define FBR_PUTEXTFLAGS(p,v)      fat_putuint16(UBYTE_PTR(p,FBR_EXTFLAGS),v)
# define FBR_PUTFSVER(p,v)         fat_putuint16(UBYTE_PTR(p,FBR_FSVER),v)
# define FBR_PUTROOTCLUS(p,v)      fat_putuint32(UBYTE_PTR(p,FBR_ROOTCLUS),v)
# define FBR_PUTFSINFO(p,v)        fat_putuint16(UBYTE_PTR(p,FBR_FSINFO),v)
# define FBR_PUTBKBOOTSEC(p,v)     fat_putuint16(UBYTE_PTR(p,FBR_BKBOOTSEC),v)
# define FBR_PUTSIGNATURE(p,v)     fat_putuint16(UBYTE_PTR(p,FBR_SIGNATURE),v)

# define FSI_PUTLEADSIG(p,v)       FAT_PUTUINT32(UBYTE_PTR(p,FSI_LEADSIG),v)
# define FSI_PUTSTRUCTSIG(p,v)     FAT_PUTUINT32(UBYTE_PTR(p,FSI_STRUCTSIG),v)
# define FSI_PUTFREECOUNT(p,v)     FAT_PUTUINT32(UBYTE_PTR(p,FSI_FREECOUNT),v)
# define FSI_PUTNXTFREE(p,v)       FAT_PUTUINT32(UBYTE_PTR(p,FSI_NXTFREE),v)
# define FSI_PUTTRAILSIG(p,v)      FAT_PUTUINT32(UBYTE_PTR(p,FSI_TRAILSIG),v)

# define DIR_PUTCRTIME(p,v)        FAT_PUTUINT16(UBYTE_PTR(p,DIR_CRTIME),v)
# define DIR_PUTCRDATE(p,v)        FAT_PUTUINT16(UBYTE_PTR(p,DIR_CRDATE),v)
# define DIR_PUTLASTACCDATE(p,v)   FAT_PUTUINT16(UBYTE_PTR(p,DIR_LASTACCDATE),v)
# define DIR_PUTFSTCLUSTHI(p,v)    FAT_PUTUINT16(UBYTE_PTR(p,DIR_FSTCLUSTHI),v)
# define DIR_PUTWRTTIME(p,v)       FAT_PUTUINT16(UBYTE_PTR(p,DIR_WRTTIME),v)
# define DIR_PUTWRTDATE(p,v)       FAT_PUTUINT16(UBYTE_PTR(p,DIR_WRTDATE),v)
# define DIR_PUTFSTCLUSTLO(p,v)    FAT_PUTUINT16(UBYTE_PTR(p,DIR_FSTCLUSTLO),v)
# define DIR_PUTFILESIZE(p,v)      FAT_PUTUINT32(UBYTE_PTR(p,DIR_FILESIZE),v)

# ifdef CONFIG_FAT_LFN
#  define LDIR_PUTWCHAR1(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR1_5),v)
#  define LDIR_PUTWCHAR2(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR1_5+2),v)
#  define LDIR_PUTWCHAR3(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR1_5+4),v)
#  define LDIR_PUTWCHAR4(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR1_5+6),v)
#  define LDIR_PUTWCHAR5(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR1_5+8),v)
#  define LDIR_PUTWCHAR6(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR6_11),v)
#  define LDIR_PUTWCHAR7(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR6_11+2),v)
#  define LDIR_PUTWCHAR8(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR6_11+4),v)
#  define LDIR_PUTWCHAR9(p)        FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR6_11+6),v)
#  define LDIR_PUTWCHAR10(p)       FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR6_11+8),v)
#  define LDIR_PUTWCHAR11(p)       FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR6_11+10),v)
#  define LDIR_PUTWCHAR12(p)       FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR12_13),v)
#  define LDIR_PUTWCHAR13(p)       FAT_PUTUINT16(UBYTE_PTR(p,LDIR_WCHAR12_13+2),v)
# endif

# define FSI_PUTLEADSIG(p,v)       FAT_PUTUINT32(UBYTE_PTR(p,FSI_LEADSIG),v)
# define FSI_PUTSTRUCTSIG(p,v)     FAT_PUTUINT32(UBYTE_PTR(p,FSI_STRUCTSIG),v)
# define FSI_PUTFREECOUNT(p,v)     FAT_PUTUINT32(UBYTE_PTR(p,FSI_FREECOUNT),v)
# define FSI_PUTNXTFREE(p,v)       FAT_PUTUINT32(UBYTE_PTR(p,FSI_NXTFREE),v)
# define FSI_PUTTRAILSIG(p,v)      FAT_PUTUINT32(UBYTE_PTR(p,FSI_TRAILSIG),v)

# define FAT_PUTFAT16(p,i,v)       FAT_PUTUINT16(UBYTE_PTR(p,i),v)
# define FAT_PUTFAT32(p,i,v)       FAT_PUTUINT32(UBYTE_PTR(p,i),v)

#else

/* But nothing special has to be done for the little endian-case for access
 * to aligned multibyte values.
 */

# define MBR_PUTRESVDSECCOUNT(p,v) UINT16_PUT(p,MBR_RESVDSECCOUNT,v)
# define MBR_PUTFATSZ16(p,v)       UINT16_PUT(p,MBR_FATSZ16,v)
# define MBR_PUTSECPERTRK(p,v)     UINT16_PUT(p,MBR_SECPERTRK,v)
# define MBR_PUTNUMHEADS(p,v)      UINT16_PUT(p,MBR_NUMHEADS,v)
# define MBR_PUTHIDSEC(p,v)        UINT32_PUT(p,MBR_HIDSEC,v)
# define MBR_PUTTOTSEC32(p,v)      UINT32_PUT(p,MBR_TOTSEC32,v)
# define MBR_PUTFATSZ32(p,v)       UINT32_PUT(p,MBR32_FATSZ32,v)
# define MBR_PUTEXTFLAGS(p,v)      UINT16_PUT(p,MBR32_EXTFLAGS,v)
# define MBR_PUTFSVER(p,v)         UINT16_PUT(p,MBR32_FSVER,v)
# define MBR_PUTROOTCLUS(p,v)      UINT32_PUT(p,MBR32_ROOTCLUS,v)
# define MBR_PUTFSINFO(p,v)        UINT16_PUT(p,MBR32_FSINFO,v)
# define MBR_PUTBKBOOTSEC(p,v)     UINT16_PUT(p,MBR32_BKBOOTSEC,v)
# define MBR_PUTSIGNATURE(p,v)     UINT16_PUT(p,MBR_SIGNATURE,v)

# define FBR_PUTRESVDSECCOUNT(p,v) UINT16_PUT(p,FBR_RESVDSECCOUNT,v)
# define FBR_PUTFATSZ16(p,v)       UINT16_PUT(p,FBR_FATSZ16,v)
# define FBR_PUTSECPERTRK(p,v)     UINT16_PUT(p,FBR_SECPERTRK,v)
# define FBR_PUTNUMHEADS(p,v)      UINT16_PUT(p,FBR_NUMHEADS,v)
# define FBR_PUTHIDSEC(p,v)        UINT32_PUT(p,FBR_HIDSEC,v)
# define FBR_PUTTOTSEC32(p,v)      UINT32_PUT(p,FBR_TOTSEC32,v)
# define FBR_PUTFATSZ32(p,v)       UINT32_PUT(p,FBR_FATSZ32,v)
# define FBR_PUTEXTFLAGS(p,v)      UINT16_PUT(p,FBR_EXTFLAGS,v)
# define FBR_PUTFSVER(p,v)         UINT16_PUT(p,FBR_FSVER,v)
# define FBR_PUTROOTCLUS(p,v)      UINT32_PUT(p,FBR_ROOTCLUS,v)
# define FBR_PUTFSINFO(p,v)        UINT16_PUT(p,FBR_FSINFO,v)
# define FBR_PUTBKBOOTSEC(p,v)     UINT16_PUT(p,FBR_BKBOOTSEC,v)
# define FBR_PUTSIGNATURE(p,v)     UINT16_PUT(p,FBR_SIGNATURE,v)

# define FSI_PUTLEADSIG(p,v)       UINT32_PUT(p,FSI_LEADSIG,v)
# define FSI_PUTSTRUCTSIG(p,v)     UINT32_PUT(p,FSI_STRUCTSIG,v)
# define FSI_PUTFREECOUNT(p,v)     UINT32_PUT(p,FSI_FREECOUNT,v)
# define FSI_PUTNXTFREE(p,v)       UINT32_PUT(p,FSI_NXTFREE,v)
# define FSI_PUTTRAILSIG(p,v)      UINT32_PUT(p,FSI_TRAILSIG,v)

# define DIR_PUTCRTIME(p,v)        UINT16_PUT(p,DIR_CRTIME,v)
# define DIR_PUTCRDATE(p,v)        UINT16_PUT(p,DIR_CRDATE,v)
# define DIR_PUTLASTACCDATE(p,v)   UINT16_PUT(p,DIR_LASTACCDATE,v)
# define DIR_PUTFSTCLUSTHI(p,v)    UINT16_PUT(p,DIR_FSTCLUSTHI,v)
# define DIR_PUTWRTTIME(p,v)       UINT16_PUT(p,DIR_WRTTIME,v)
# define DIR_PUTWRTDATE(p,v)       UINT16_PUT(p,DIR_WRTDATE,v)
# define DIR_PUTFSTCLUSTLO(p,v)    UINT16_PUT(p,DIR_FSTCLUSTLO,v)
# define DIR_PUTFILESIZE(p,v)      UINT32_PUT(p,DIR_FILESIZE,v)

# ifdef CONFIG_FAT_LFN
#  define LDIR_PUTWCHAR1(p,v)      UINT16_PUT(p,LDIR_WCHAR1_5,v)
#  define LDIR_PUTWCHAR2(p,v)      UINT16_PUT(p,LDIR_WCHAR1_5+2,v)
#  define LDIR_PUTWCHAR3(p,v)      UINT16_PUT(p,LDIR_WCHAR1_5+4,v)
#  define LDIR_PUTWCHAR4(p,v)      UINT16_PUT(p,LDIR_WCHAR1_5+6,v)
#  define LDIR_PUTWCHAR5(p,v)      UINT16_PUT(p,LDIR_WCHAR1_5+8,v)
#  define LDIR_PUTWCHAR6(p,v)      UINT16_PUT(p,LDIR_WCHAR6_11,v)
#  define LDIR_PUTWCHAR7(p,v)      UINT16_PUT(p,LDIR_WCHAR6_11+2,v)
#  define LDIR_PUTWCHAR8(p,v)      UINT16_PUT(p,LDIR_WCHAR6_11+4,v)
#  define LDIR_PUTWCHAR9(p,v)      UINT16_PUT(p,LDIR_WCHAR6_11+6,v)
#  define LDIR_PUTWCHAR10(p,v)     UINT16_PUT(p,LDIR_WCHAR6_11+8,v)
#  define LDIR_PUTWCHAR11(p,v)     UINT16_PUT(p,LDIR_WCHAR6_11+10,v)
#  define LDIR_PUTWCHAR12(p,v)     UINT16_PUT(p,LDIR_WCHAR12_13,v)
#  define LDIR_PUTWCHAR13(p,v)     UINT16_PUT(p,LDIR_WCHAR12_13+2,v)
# endif

# define FSI_PUTLEADSIG(p,v)       UINT32_PUT(p,FSI_LEADSIG,v)
# define FSI_PUTSTRUCTSIG(p,v)     UINT32_PUT(p,FSI_STRUCTSIG,v)
# define FSI_PUTFREECOUNT(p,v)     UINT32_PUT(p,FSI_FREECOUNT,v)
# define FSI_PUTNXTFREE(p,v)       UINT32_PUT(p,FSI_NXTFREE,v)
# define FSI_PUTTRAILSIG(p,v)      UINT32_PUT(p,FSI_TRAILSIG,v)

# define FAT_PUTFAT16(p,i,v)       UINT16_PUT(p,i,v)
# define FAT_PUTFAT32(p,i,v)       UINT32_PUT(p,i,v)

#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_FSUTILS_MKFATFS_FAT32_H */
