#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#pragma optimize("", off)

ULONG			g_dwDelayTime;

/***********************************************************************************
함수명 : Delay
인  자 :
			ULONG fact
리턴값 : VOID
설  명 : 
***********************************************************************************/
VOID Delay(ULONG fact)
{
	ULONG i;

	for (i = 0; i < fact; i++) {
		_asm	in	al, 61h
	} //end for
}
/***********************************************************************************
함수명 : ReadWriteMemSlot
인  자 :
			PULONG dst
			PULONG src
			ULONG count
리턴값 : VOID
설  명 : 
***********************************************************************************/
VOID ReadWriteMemSlot(PULONG dst, PULONG src, ULONG count)
{
	ULONG i;
	//VOID WriteMemSlotNum( PVOID HwDeviceExtension, ULONG SlotNum );

	for (i = 0; i < count / 4; i++) {
		Delay(g_dwDelayTime);
		dst[i] = src[i];
	} //end for
}
/***********************************************************************************
함수명 : WriteMemSlotNum
인  자 :
			PVOID HwDeviceExtension
			ULONG SlotNum
리턴값 : VOID
설  명 : 
***********************************************************************************/
VOID WriteMemSlotNum( PVOID HwDeviceExtension, ULONG SlotNum )
{
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

	Delay(g_dwDelayTime);
	WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, SlotNum ); // block으로 이동..
}
/***********************************************************************************
함수명 : ReadFromSector
인  자 :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
리턴값 : ULONG
설  명 : 섹터에 씁니다.
***********************************************************************************/
ULONG WriteToSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer)
{
//	ULONG MemSlotSeg;
//	ULONG MemSlotOff;
	ULONG	i;
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
	ULONG MemoryMappedAddress = (ULONG)DeviceExtension->Data_LinearAddress;

	for (i = 0; i < SectorCount; i++) {

//		MemSlotSeg = ((StartingSector + i) * SIZE_SECTOR) / MEM_BLOCK_SIZE;
//		MemSlotOff = ((StartingSector + i) * SIZE_SECTOR) % MEM_BLOCK_SIZE;

//		WriteMemSlotNum( HwDeviceExtension, MemSlotSeg);
//		ReadWriteMemSlot( MemoryMappedAddress + MemSlotOff , Buffer + (i * SIZE_SECTOR), SIZE_SECTOR);
//		ReadWriteMemSlot( MemoryMappedAddress , Buffer + (i * SIZE_SECTOR), SIZE_SECTOR);
	}

	return 1;
}
/***********************************************************************************
함수명 : ReadFromSector
인  자 :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
리턴값 : ULONG
설  명 : 섹터에서 읽어 들입니다.
***********************************************************************************/
ULONG ReadFromSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer)
{
//	ULONG MemSlotSeg;
//	ULONG MemSlotOff;
	ULONG	i;
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
	ULONG MemoryMappedAddress = (ULONG)DeviceExtension->Data_LinearAddress;

	for (i = 0; i < SectorCount; i++) {

//		MemSlotSeg = ((StartingSector + i) * SIZE_SECTOR) / MEM_BLOCK_SIZE;
//		MemSlotOff = ((StartingSector + i) * SIZE_SECTOR) % MEM_BLOCK_SIZE;

//		WriteMemSlotNum( HwDeviceExtension, MemSlotSeg);
//		ReadWriteMemSlot(Buffer + (i * SIZE_SECTOR), MemoryMappedAddress + MemSlotOff, SIZE_SECTOR);
	} //end for

	return 1;
}
/***********************************************************************************
함수명 : ReadSector
인  자 :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
리턴값 : ULONG
설  명 : ReadFromSector(DeviceExtension, 0, 1, tempBuffer);
***********************************************************************************/
ULONG ReadSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer)
{
	ULONG MemSlotSeg;
	ULONG MemSlotOff;
	ULONG Disk ;
	ULONG	i;//, j;
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
	ULONG MemoryMappedAddress = (ULONG)DeviceExtension->Data_LinearAddress;

	//DbgPrint((" - ReadSector\n"));
	for (i = 0; i < SectorCount; i++) {

		MemSlotSeg = ((StartingSector + i) * SIZE_SECTOR) / MEM_BLOCK_SIZE; //MEM_BLOCK_SIZE 0x10000
		MemSlotOff = ((StartingSector + i) * SIZE_SECTOR) % MEM_BLOCK_SIZE;

		Disk = (MemSlotSeg << 4) + MemSlotOff;

		//WriteMemSlotNum( HwDeviceExtension, MemSlotSeg);
		
		Delay(g_dwDelayTime);		
		//WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, SlotNum ); // block으로 이동..
		WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, 0x00000000 ); // block으로 이동..
		//위
		//아래
		//ReadWriteMemSlot(Buffer + (i * SIZE_SECTOR), MemoryMappedAddress + MemSlotOff, SIZE_SECTOR);		
/*		
		for (j = 0; j < SIZE_SECTOR / 4; j++) {
			Delay(g_dwDelayTime);
			Buffer[j] = MemoryMappedAddress[j];	
			
		} //end for
*/		
		MemoryReadWrite(Buffer + (i * SIZE_SECTOR) , MemoryMappedAddress + Disk, SIZE_SECTOR);
		//Buffer[i] = READ_PORT_UCHAR(MemoryMappedAddress);
		/*
		for (j = 0; j < 0x200; j++) {
			Buffer[j] = READ_PORT_UCHAR(&MemoryMappedAddress[j]);
		}
		*/
	} //end for
	//DbgPrint(("Buffer=0x%x, MemoryMappedAddress=0x%x\n", Buffer, MemoryMappedAddress));
	return 1L;
}
/***********************************************************************************
함수명 : WriteSector
인  자 :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
리턴값 : ULONG
설  명 : WriteToSector(DeviceExtension, 0, 1, PartBuffer);//DeviceExtension->DiskImage
***********************************************************************************/
ULONG WriteSector(PVOID HwDeviceExtension, ULONG StartingSector, ULONG SectorCount, PUCHAR Buffer)
{
	ULONG MemSlotSeg;
	ULONG MemSlotOff;
	ULONG Disk;

	ULONG	i;//, j;
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
	ULONG MemoryMappedAddress = (ULONG)DeviceExtension->Data_LinearAddress;

	//DbgPrint((" - WriteSector\n"));

	for (i = 0; i < SectorCount; i++) {

		MemSlotSeg = ((StartingSector + i) * SIZE_SECTOR) / MEM_BLOCK_SIZE;
		MemSlotOff = ((StartingSector + i) * SIZE_SECTOR) % MEM_BLOCK_SIZE;

		Disk = (MemSlotSeg << 4) + MemSlotOff;

//		WriteMemSlotNum( HwDeviceExtension, MemSlotSeg);
		Delay(g_dwDelayTime);		
		//WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, SlotNum ); // block으로 이동..
		WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, 0x00000000 ); // block으로 이동..
		
		//위
		
		//아래
//		ReadWriteMemSlot( MemoryMappedAddress + MemSlotOff , Buffer + (i * SIZE_SECTOR), SIZE_SECTOR);

		//ReadWriteMemSlot(Buffer + (i * SIZE_SECTOR), MemoryMappedAddress + MemSlotOff, SIZE_SECTOR);		
/*
		for (j = 0; j < SIZE_SECTOR / 4; j++) {
			Delay(g_dwDelayTime);
			MemoryMappedAddress[j]  = Buffer[j];
		} //end for
*/

//		ReadWriteMemSlot( MemoryMappedAddress , Buffer + (i * SIZE_SECTOR), SIZE_SECTOR);
		MemoryReadWrite( MemoryMappedAddress + Disk ,Buffer + (i * SIZE_SECTOR), SIZE_SECTOR);
		
		/*
		for (j = 0; j < 0x200; j++) {
			WRITE_PORT_UCHAR(&MemoryMappedAddress[j], Buffer[j]);
		}
		*/
		//WRITE_PORT_UCHAR(MemoryMappedAddress, Buffer[i]);
	}
	//DbgPrint(("MemoryMappedAddress=0x%x, Buffer=0x%x\n", MemoryMappedAddress, Buffer));

	return 1;
}
/***********************************************************************************
함수명 : MemoryReadWrite
인  자 : 
			PULONG dst
			PULONG src
			ULONG count
리턴값 : VOID
설  명 : MemCopy src에서 dst로 copy
***********************************************************************************/
VOID MemoryReadWrite(PULONG dst, PULONG src, ULONG count)
{
	ULONG i = 0L;

	for (i = 0; i < count/4; i++) {

		Delay(g_dwDelayTime);
		dst[i] = src[i];
	} //end for
	
}
/***********************************************************************************
함수명 : 
인  자 :
리턴값 : 
설  명 : 
***********************************************************************************/
