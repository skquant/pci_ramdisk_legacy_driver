/***********************************************************************************
모듈명 : Ez.h
Author : Seong Kee. Kim (김성기)
설  명 : SRB_FUNCTION_EXECUTE_SCSI
***********************************************************************************/
//Windows
#include "scsi.h"
//Ansi
#include "stdio.h"
#include "string.h"

// Device Extension Device Flags
#define DFLAGS_DEVICE_PRESENT        0x0001    // Indicates that some device is present.
#define DFLAGS_ATAPI_DEVICE          0x0002    // Indicates whether Atapi commands can be used.
#define DFLAGS_TAPE_DEVICE           0x0004    // Indicates whether this is a tape device.
#define DFLAGS_INT_DRQ               0x0008    // Indicates whether device interrupts as DRQ is set after
                                               // receiving Atapi Packet Command
#define DFLAGS_REMOVABLE_DRIVE       0x0010    // Indicates that the drive has the 'removable' bit set in
                                               // identify data (offset 128)
#define DFLAGS_MEDIA_STATUS_ENABLED  0x0020    // Media status notification enabled
#define DFLAGS_ATAPI_CHANGER         0x0040    // Indicates atapi 2.5 changer present.
#define DFLAGS_SANYO_ATAPI_CHANGER   0x0080    // Indicates multi-platter device, not conforming to the 2.5 spec.
#define DFLAGS_CHANGER_INITED        0x0100    // Indicates that the init path for changers has already been done.

#define	DRIVER_REGPATH            L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\Ez"             
#define	SIZE_SECTOR		(0x200) //512
#define	MEM_BLOCK_SIZE	(0x10000)

#define	DEFAULT_DISK_SIZE				0x80000000 //(1024*1024*1024+512*4)     // 1G
#define	DEFAULT_ROOT_DIR_ENTRIES		512
#define	DEFAULT_SECTORS_PER_CLUSTER		2
#define DIR_ENTRIES_PER_SECTOR			16

#define HWSEGMENT	(0x10000)

static	ULONG	g_AdapterCount = 0;
extern	ULONG	g_dwDelayTime;

//PCIINFO 구조체 선언 
typedef struct _PCI_INFO{ 
	ULONG	m_dwBaseAddress;
	ULONG   m_Access1;
	ULONG	m_dwLength;
	ULONG	m_dwMemType;
} PCI_INFO;
//HW_DEVICE_EXTENSION 구조체 선언 
typedef struct _HW_DEVICE_EXTENSION {
    PSCSI_REQUEST_BLOCK CurrentSrb;   // Current request on controller.
	PCI_INFO			m_PciInfo[2];
	
	ULONG				m_dwInterruptVectorNumber;

    ULONG				NumberChannels;

    ULONG				InterruptMode;

    BOOLEAN				PrimaryAddress;

	//Disk Information...............................................
	PUCHAR				DiskImage;	
	ULONG				DiskLength;
    ULONG				NumberOfCylinders;
    ULONG				TracksPerCylinder;
    ULONG				SectorsPerTrack;
    ULONG				BytesPerSector;

	UNICODE_STRING  Win32NameString;

	// Driver is being used by the crash dump utility or ntldr.    
    BOOLEAN				DriverMustPoll;
	ULONG				DeviceFlags[4];
	USHORT				MaximumBlockXfer[4];
	ULONG				Confgig_PortNumber;
	PUCHAR				Data_LinearAddress;
	ULONG				m_TotalHwseg;

} HW_DEVICE_EXTENSION, *PHW_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////////////////
//HW_LU_EXTENSION 구조체 선언 
typedef struct _HW_LU_EXTENSION {
  ULONG Reserved;
} HW_LU_EXTENSION, *PHW_LU_EXTENSION;

/////////////////////////////////////////////////////////////////////////////////////////

typedef struct _PARTI{		// partition record
	UCHAR indicator;		// 00 boot indicator (80 = active partition)
	
	UCHAR starthead;		// 01 start head
	UCHAR startsec;			// 02 bits 0-5: start sector, bits 6-7: bits 8-9 of start track
	UCHAR starttrack;		// 03 bits 0-7 of start track
	
	UCHAR parttype;			// 04 partition type
	
	UCHAR endhead;			// 05 end head
	UCHAR endsec;			// 06 end sector
	UCHAR endtrack;			// 07 end track
	
	ULONG bias;				// 08 sector bias to start of partition
	ULONG partsize;			// 0C partition size in sectors//매체의 크기
} PARTI, *PPARTI;
/*
Part.indicator = 0x80;
Part.starthead = 1;
Part.startsec = 1;
Part.starttrack = 0;
Part.endhead = 0xFE;
Part.endsec = 0x3F;
Part.endtrack = 1;
Part.parttype = 4;
Part.bias = 0x3F;
Part.partsize = 0x7D43;
*/

////////////////////////////////////////////////////////////////////////////////////////
//DriverEntry.c Module //ULONG DriverEntry(IN PVOID DriverObject,IN PVOID Arg);
//EzController.c Module
ULONG EzController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    );
//EzHwInitialize.c Module 
BOOLEAN		EzHwInitialize(IN PVOID HwDeviceExtension);
NTSTATUS	GetRegDWORDInfo( PWCHAR pEntryKey, PUNICODE_STRING pRegKey, PULONG pKeyValue );
//EzInterrupt.c Module
//EzResetController.c Module
BOOLEAN		EzResetController(IN PVOID HwDeviceExtension,IN ULONG PathId);
//EzStartIo.c
BOOLEAN		EzStartIo(IN PVOID HwDeviceExtension,IN PSCSI_REQUEST_BLOCK Srb);
ULONG		DiskSendCommand(IN PVOID HwDeviceExtension,IN PSCSI_REQUEST_BLOCK Srb);
ULONG		DiskReadWrite(IN PVOID HwDeviceExtension,IN PSCSI_REQUEST_BLOCK Srb);
LONG		EzStringCmp (PCHAR FirstStr,PCHAR SecondStr,ULONG Count);   
//EzDeviceAccess.c
VOID		Delay(ULONG fact);
VOID		ReadWriteMemSlot(PULONG dst, PULONG src, ULONG count);
VOID		WriteMemSlotNum( PVOID HwDeviceExtension, ULONG SlotNum );
ULONG		WriteToSector( PVOID HwDeviceExtension, ULONG StartingMemAddress, ULONG TotalMemBytes,PUCHAR WriteBuffer);
ULONG		ReadFromSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer);
ULONG		ReadSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer);
ULONG		WriteSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer);
VOID		MemoryReadWrite(PULONG dst, PULONG src, ULONG count);
/////////////////////////////////////////////////////////////////////////////////////////