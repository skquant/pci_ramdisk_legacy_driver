#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#pragma optimize("", off)

ULONG			g_dwDelayTime;

/***********************************************************************************
�Լ��� : Delay
��  �� :
			ULONG fact
���ϰ� : VOID
��  �� : 
***********************************************************************************/
VOID Delay(ULONG fact)
{
	ULONG i;

	for (i = 0; i < fact; i++) {
		_asm	in	al, 61h
	} //end for
}
/***********************************************************************************
�Լ��� : ReadWriteMemSlot
��  �� :
			PULONG dst
			PULONG src
			ULONG count
���ϰ� : VOID
��  �� : 
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
�Լ��� : WriteMemSlotNum
��  �� :
			PVOID HwDeviceExtension
			ULONG SlotNum
���ϰ� : VOID
��  �� : 
***********************************************************************************/
VOID WriteMemSlotNum( PVOID HwDeviceExtension, ULONG SlotNum )
{
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

	Delay(g_dwDelayTime);
	WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, SlotNum ); // block���� �̵�..
}
/***********************************************************************************
�Լ��� : ReadFromSector
��  �� :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
���ϰ� : ULONG
��  �� : ���Ϳ� ���ϴ�.
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
�Լ��� : ReadFromSector
��  �� :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
���ϰ� : ULONG
��  �� : ���Ϳ��� �о� ���Դϴ�.
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
�Լ��� : ReadSector
��  �� :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
���ϰ� : ULONG
��  �� : ReadFromSector(DeviceExtension, 0, 1, tempBuffer);
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
		//WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, SlotNum ); // block���� �̵�..
		WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, 0x00000000 ); // block���� �̵�..
		//��
		//�Ʒ�
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
�Լ��� : WriteSector
��  �� :
			PVOID HwDeviceExtension
			ULONG StartingSector
			ULONG SectorCount
			PUCHAR Buffer
���ϰ� : ULONG
��  �� : WriteToSector(DeviceExtension, 0, 1, PartBuffer);//DeviceExtension->DiskImage
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
		//WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, SlotNum ); // block���� �̵�..
		WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, 0x00000000 ); // block���� �̵�..
		
		//��
		
		//�Ʒ�
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
�Լ��� : MemoryReadWrite
��  �� : 
			PULONG dst
			PULONG src
			ULONG count
���ϰ� : VOID
��  �� : MemCopy src���� dst�� copy
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
�Լ��� : 
��  �� :
���ϰ� : 
��  �� : 
***********************************************************************************/
