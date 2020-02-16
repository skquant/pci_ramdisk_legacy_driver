#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#pragma optimize("", off)

/***********************************************************************************
�Լ��� : EzResetController
��  �� :
			IN PVOID HwDeviceExtension,
			IN ULONG PathId
���ϰ� : BOOLEAN
��  �� : Reset
***********************************************************************************/
BOOLEAN EzResetController(IN PVOID HwDeviceExtension,IN ULONG PathId)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;

    DbgPrint(("\nEzResetController routine start\n"));

    if (DeviceExtension->CurrentSrb) {
        ScsiPortCompleteRequest(DeviceExtension,
                                DeviceExtension->CurrentSrb->PathId,
                                DeviceExtension->CurrentSrb->TargetId,
                                DeviceExtension->CurrentSrb->Lun,
                                (ULONG)SRB_STATUS_BUS_RESET);
        DeviceExtension->CurrentSrb = NULL;
        ScsiPortNotification(NextRequest,
                             DeviceExtension,
                             NULL);
    }
			
	
    Ez74HwInitialize(HwDeviceExtension);
	

	DbgPrint(("\nEzResetController routine end\n"));
    return TRUE;
}
/***********************************************************************************
�Լ��� : 
��  �� :
���ϰ� : 
��  �� : 
***********************************************************************************/
