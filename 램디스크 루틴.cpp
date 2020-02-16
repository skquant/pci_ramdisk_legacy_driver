#pragma pack(1)

typedef struct  _BOOT_SECTOR
{
    UCHAR       bsJump[3];          // x86 jmp instruction, checked by FS
    CCHAR       bsOemName[8];       // OEM name of formatter
    USHORT      bsBytesPerSec;      // Bytes per Sector
    UCHAR       bsSecPerClus;       // Sectors per Cluster
    USHORT      bsResSectors;       // Reserved Sectors
    UCHAR       bsFATs;             // Number of FATs - we always use 1
    USHORT      bsRootDirEnts;      // Number of Root Dir Entries
    USHORT      bsSectors;          // Number of Sectors
    UCHAR       bsMedia;            // Media type - we use RAMDISK_MEDIA_TYPE == 0xF8
    USHORT      bsFATsecs;          // Number of FAT sectors
    USHORT      bsSecPerTrack;      // Sectors per Track - we use 32
    USHORT      bsHeads;            // Number of Heads - we use 2
    ULONG       bsHiddenSecs;       // Hidden Sectors - we set to 0
    ULONG       bsHugeSectors;      // Number of Sectors if > 32 MB size
    UCHAR       bsDriveNumber;      // Drive Number - not used
    UCHAR       bsReserved1;        // Reserved
    UCHAR       bsBootSignature;    // New Format Boot Signature - 0x29
    ULONG       bsVolumeID;         // VolumeID - set to 0x12345678
    CCHAR       bsLabel[11];        // Label - set to RamDisk
    CCHAR       bsFileSystemType[8];// File System Type - FAT12 or FAT16
    CCHAR       bsReserved2[448];   // Reserved
    UCHAR       bsSig2[2];          // Originial Boot Signature - 0x55, 0xAA
}   BOOT_SECTOR, *PBOOT_SECTOR;

typedef struct  _DIR_ENTRY
{
    UCHAR       deName[8];          // File Name
    UCHAR       deExtension[3];     // File Extension
    UCHAR       deAttributes;       // File Attributes
    UCHAR       deReserved;         // Reserved
    USHORT      deTime;             // File Time
    USHORT      deDate;             // File Date
    USHORT      deStartCluster;     // First Cluster of file
    ULONG       deFileSize;         // File Length
}   DIR_ENTRY, *PDIR_ENTRY;

#pragma pack()
/////////////////////////////////////////////////////////////////////////////
    ULONG           defaultRootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
    ULONG           rootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
    
    ULONG           defaultSectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;
    ULONG           sectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;
    
    
    PBOOT_SECTOR    bootSector = (PBOOT_SECTOR) DiskExtension->DiskImage;
    PUCHAR          firstFatSector;
    
    int             fatType;        // Type of FAT - 12 or 16
    USHORT          fatEntries;     // Number of cluster entries in FAT
    USHORT          fatSectorCnt;   // Number of sectors for FAT

    PDIR_ENTRY      rootDir;        // Pointer to first entry in root dir    


        rootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
        sectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;

    //
    // Round Root Directory entries up if necessary
    //
    if (rootDirEntries & (DIR_ENTRIES_PER_SECTOR - 1))
    {
        
        rootDirEntries =
            (rootDirEntries + ( DIR_ENTRIES_PER_SECTOR - 1 )) &
                ~ ( DIR_ENTRIES_PER_SECTOR - 1 );
    }

    //
    // We need to have the 0xeb and 0x90 since this is one of the
    // checks the file system recognizer uses
    //
    bootSector->bsJump[0] = 0xeb;
    bootSector->bsJump[1] = 0x3c;
    bootSector->bsJump[2] = 0x90;

    strncpy(bootSector->bsOemName, "RobertN ", 8);
    bootSector->bsBytesPerSec = (SHORT)DiskExtension->BytesPerSector;
    bootSector->bsResSectors = 1;
    bootSector->bsFATs = 1;
    bootSector->bsRootDirEnts = (USHORT)rootDirEntries;

    bootSector->bsSectors =
        (USHORT)( DiskExtension->DiskLength / DiskExtension->BytesPerSector );
    bootSector->bsMedia = 0xF8;

    bootSector->bsSecPerClus = (UCHAR)sectorsPerCluster;

    //
    // Calculate number of sectors required for FAT
    //
    fatEntries =
        (bootSector->bsSectors - bootSector->bsResSectors -
            bootSector->bsRootDirEnts / DIR_ENTRIES_PER_SECTOR) /
                bootSector->bsSecPerClus + 2;

    //
    // Choose between 12 and 16 bit FAT based on number of clusters we
    // need to map
    //
    if (fatEntries > 4087)
    {
        fatType =  16;

        fatSectorCnt = (fatEntries * 2 + 511) / 512;

        fatEntries -= fatSectorCnt;

        fatSectorCnt = (fatEntries * 2 + 511) / 512;
    }
    else
    {
        fatType =  12;

        fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
        
        fatEntries -= fatSectorCnt;
        
        fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
    }

    bootSector->bsFATsecs = fatSectorCnt;
    bootSector->bsSecPerTrack = (USHORT)DiskExtension->SectorsPerTrack;
    bootSector->bsHeads = (USHORT)DiskExtension->TracksPerCylinder;
    bootSector->bsBootSignature = 0x29;
    bootSector->bsVolumeID = 0x12345678;
    strncpy(bootSector->bsLabel, "Disk    ", 11);
    strncpy(bootSector->bsFileSystemType, "FAT1?   ", 8);
    bootSector->bsFileSystemType[4] = ( fatType == 16 ) ? '6' : '2';
    
    bootSector->bsSig2[0] = 0x55;
    bootSector->bsSig2[1] = 0xAA;
    
    //
    // The FAT is located immediately following the boot sector.
    //
    firstFatSector = (PUCHAR)(bootSector + 1);
    firstFatSector[0] = RAMDISK_MEDIA_TYPE;
    firstFatSector[1] = 0xFF;
    firstFatSector[2] = 0xFF;

    if (fatType == 16)
        firstFatSector[3] = 0xFF;

    //
    // The Root Directory follows the FAT
    //
    rootDir = (PDIR_ENTRY)(bootSector + 1 + fatSectorCnt);
    strcpy(rootDir->deName, "Ez");
    strcpy(rootDir->deExtension, "IVE");
    rootDir->deAttributes = DIR_ATTR_VOLUME;
}
