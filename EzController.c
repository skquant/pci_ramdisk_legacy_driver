#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#pragma optimize("", off)
/***********************************************************************************
함수명 : EzController
인  자 :
			IN PVOID HwDeviceExtension,
			IN PVOID Context,
			IN PVOID BusInformation,
			IN PCHAR ArgumentString,
			IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			OUT PBOOLEAN Again
리턴값 : ULONG
설  명 : 읽어들인 하드웨어 구성정보를 얻어 어덥터를 설정합니다.
***********************************************************************************/
ULONG EzController(
    IN PVOID HwDeviceExtension,
    IN PVOID Context,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again
    )
{	
 	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
	unsigned *Access = FALSE;


	DbgPrint(("\nEzController routine start \n"));
		
	
	if (++g_AdapterCount > 1) {
	    return SP_RETURN_NOT_FOUND;
	}

	// 우리 디바이스의 정보..
	Access = (unsigned *)ConfigInfo->AccessRanges;
	DeviceExtension->m_PciInfo[0].m_dwBaseAddress	= Access[0];
	DeviceExtension->m_PciInfo[0].m_Access1			= Access[1];
	DeviceExtension->m_PciInfo[0].m_dwLength		= Access[2];
	DeviceExtension->m_PciInfo[0].m_dwMemType		= Access[3];

	DeviceExtension->m_PciInfo[1].m_dwBaseAddress	= Access[4];
	DeviceExtension->m_PciInfo[1].m_Access1			= Access[5];
	DeviceExtension->m_PciInfo[1].m_dwLength		= Access[6];
	DeviceExtension->m_PciInfo[1].m_dwMemType		= Access[7];
	DeviceExtension->m_dwInterruptVectorNumber		= ConfigInfo->BusInterruptVector;

	
	KdPrint((" - DeviceExtension->m_PciInfo[0].m_dwBaseAddress = Access[0] => 0x%x\n", Access[0]));
	KdPrint((" - DeviceExtension->m_PciInfo[0].m_Access1       = Access[1] => 0x%x\n", Access[1]));
	KdPrint((" - DeviceExtension->m_PciInfo[0].m_dwLength      = Access[2] => 0x%x\n", Access[2]));
	KdPrint((" - DeviceExtension->m_PciInfo[0].m_dwMemType     = Access[3] => 0x%x\n", Access[3]));
	KdPrint((" - DeviceExtension->m_PciInfo[1].m_dwBaseAddress = Access[4] => 0x%x\n", Access[4]));
	KdPrint((" - DeviceExtension->m_PciInfo[0].m_Access1       = Access[5] => 0x%x\n", Access[5]));
	KdPrint((" - DeviceExtension->m_PciInfo[1].m_dwLength      = Access[6] => 0x%x\n", Access[6]));
	KdPrint((" - DeviceExtension->m_PciInfo[1].m_dwMemType     = Access[7] => 0x%x\n", Access[7]));

	ConfigInfo->AdapterInterfaceType = PCIBus;//추가 0x05 == PCIBus

	ConfigInfo->NumberOfBuses = 1;//기본이 0이다.
	ConfigInfo->MaximumNumberOfTargets = 1;//기본이 SCSI_MAXIMUM_TARGETS이다.
//	ConfigInfo->BusInterruptVector = (ULONG)(PciCommonConfig.u.type0.InterruptLine); //(miniport.h)	
	KdPrint((" - ConfigInfo->BusInterruptVector: %d\n",	ConfigInfo->BusInterruptVector)); 
	KdPrint((" - ConfigInfo->BusInterruptLevel: %d\n",		ConfigInfo->BusInterruptLevel)); 
	//BusInterruptLevel도 값을 같을 것이다.

	ConfigInfo->InterruptMode = LevelSensitive; //Latched; 이미 0x0이다 0x0 == LevelSensitive
	
	//8*0x4000 * 4096;
	ConfigInfo->MaximumTransferLength = SP_UNINITIALIZED_VALUE;//0x10000;/*(64k) or (32K * 8 == 256K) or (page(4k) * 8 == 약 32K)*/
	//SP_UNINITIALIZED_VALUE는 제한 없는 최대 전송사이즈를 의미 (ULONG)~0 == SP_UNINITIALIZED_VALUE	
	ConfigInfo->NumberOfPhysicalBreaks = 0x11;//17;//if Raid ? 7 else 8
	//NumberOfPhysicalBreaks는 0x11이었음.0x11 == 17 이다. 기본으로 넘어온 값이다.
	
	ConfigInfo->DmaChannel		= SP_UNINITIALIZED_VALUE;	
	ConfigInfo->DmaPort			= SP_UNINITIALIZED_VALUE;	
	ConfigInfo->DmaWidth		= Width32Bits;	
	ConfigInfo->DmaSpeed		= Compatible;

	ConfigInfo->AlignmentMask = 3;//1;//7;//3;  // 00000011: 4 byte 단위의 data expecting //몇바이트 단위로 정렬할건지의 경계
	//if AlignmentMask가 0x00000001이면 2바이트 정렬을 가리틴다. word지 2바이트면.
//  ConfigInfo->InitiatorBusId = ?

	//TRUE == 1 FALSE == 0
	ConfigInfo->ScatterGather		= TRUE; // for BUS Master
	ConfigInfo->Master				= TRUE; // for BUS Master
	ConfigInfo->Dma32BitAddresses	= TRUE;

	ConfigInfo->DemandMode = FALSE; // for system DMA
//	ConfigInfo->BufferAccessScsiPortControlled = TRUE; // for 95 ???
//	ConfigInfo->AtdiskPrimaryClaimed = FALSE;
//	ConfigInfo->AtdiskSecondaryClaimed = FALSE;

	KdPrint((" - ConfigInfo->AddiskPrimaryClaimed = %d\n",		ConfigInfo->AtdiskPrimaryClaimed));
	KdPrint((" - ConfigInfo->AtdiskSecondaryClaimed = %d\n",	ConfigInfo->AtdiskSecondaryClaimed));
//	NEW !!!
//	ConfigInfo->Dma64BitAddresses			= FALSE;
//	ConfigInfo->ResetTargetSupported		= FALSE;
//	ConfigInfo->MaximumNumberOfLogicalUnits	=;
//	ConfigInfo->WmiDataProvider				=;

	
	DbgPrint(("\nEzController routine end\n"));
    return SP_RETURN_FOUND;
}
/***********************************************************************************
함수명 : 
인  자 :
리턴값 : 
설  명 : 
***********************************************************************************/
