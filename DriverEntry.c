#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"

#pragma optimize("", off)
#pragma pack(1)
/***************************************************************************
�Լ��� : DriverEntry
��  �� : IN PVOID DriverObject,IN PVOID Arg
���ϰ� : ULONG
***************************************************************************/
ULONG DriverEntry(IN PVOID DriverObject,IN PVOID Arg)
{
    UCHAR				   VendorId[] = {'1','6','5','F'};
	UCHAR				   DeviceId[] = {'F','D','0','0'};
	
	HW_INITIALIZATION_DATA hwInitializationData;
        
    ULONG                  statusToReturn, newStatus;
	ULONG                  adapterCount;
	    
	
	DbgPrint(("\nEz Ramdisk Board Driver Entry routine gogo!\n"));

    statusToReturn = 0xffffffff;
    memset(&hwInitializationData, 0, sizeof(HW_INITIALIZATION_DATA));

    hwInitializationData.HwInitializationDataSize = sizeof(HW_INITIALIZATION_DATA);

	// Entry Point Set
    hwInitializationData.HwFindAdapter		= EzController;
	hwInitializationData.HwInitialize		= EzHwInitialize;
    hwInitializationData.HwResetBus			= EzResetController;
    hwInitializationData.HwStartIo			= EzStartIo;	
	//hwInitializationData.HwInterrupt		= EzInterrupt;

    hwInitializationData.DeviceExtensionSize		= sizeof(HW_DEVICE_EXTENSION);
    hwInitializationData.SpecificLuExtensionSize	= sizeof(HW_LU_EXTENSION);

    //Indicate PIO device.
    hwInitializationData.MapBuffers					= TRUE;

    hwInitializationData.VendorId             = VendorId; //inode
    hwInitializationData.VendorIdLength       = 4;
    hwInitializationData.DeviceId             = DeviceId;
    hwInitializationData.DeviceIdLength       = 4;

    adapterCount = 0;
    
    hwInitializationData.NumberOfAccessRanges = 2;		// Number Of PNP Resources
    hwInitializationData.AdapterInterfaceType = PCIBus;

    newStatus = ScsiPortInitialize(DriverObject,
                                   Arg,
                                   &hwInitializationData,
                                   &adapterCount);

    if (newStatus < statusToReturn)
        statusToReturn = newStatus;

	DbgPrint(("\nEz Ramdisk Board Driver Entry routine Bye Bye! \n"));

    return statusToReturn;
}
/***********************************************************************************
�Լ��� : 
��  �� :
���ϰ� : 
��  �� : 
***********************************************************************************/
