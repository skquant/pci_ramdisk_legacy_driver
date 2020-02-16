#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#pragma optimize("", off)

/***********************************************************************************
�Լ��� : EzHwInitialize
��  �� : 
			HwDeviceExtension
���ϰ� : BOOLEAN
��  �� : �ϵ��� �ʱ�ȭ�մϴ�. 
***********************************************************************************/
BOOLEAN EzHwInitialize(IN PVOID HwDeviceExtension)
{
    PHW_DEVICE_EXTENSION	DeviceExtension = HwDeviceExtension;

	PHYSICAL_ADDRESS		Physical_Address;
	PHYSICAL_ADDRESS		RawAddress, TranslatedAddress;

	UNICODE_STRING			uniDriverPath;
	ULONG					AddressSpace, ulongData;
	
	int Channel = 0;

	DbgPrint(("\nEzHwInitialize routine start\n"));
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Access[0]
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	RawAddress = RtlConvertLongToLargeInteger( DeviceExtension->m_PciInfo[0].m_dwBaseAddress ); 
	
	AddressSpace = 1;
	KdPrint((" - RawAddress = 0x%x\n", RawAddress));

	if(AddressSpace) {
		BOOLEAN bAddressCheck;

		bAddressCheck = HalTranslateBusAddress( PCIBus, 0, RawAddress, &AddressSpace, &TranslatedAddress );
		
		if(bAddressCheck == TRUE) {
			DbgPrint((" - HalTranslateBusAddress TRUE[Confgig_PortNumber] \n"));
		} else {
			DbgPrint((" - HalTranslateBusAddress FALSE[Confgig_PortNumber] \n"));
		} //end if

		DeviceExtension->Confgig_PortNumber = TranslatedAddress.u.LowPart;
	} else { 
		DeviceExtension->Confgig_PortNumber = (ULONG)MmMapIoSpace( RawAddress, 1, FALSE );
	} //end if
	KdPrint((" - DeviceExtension->Confgig_PortNumber = 0x%x\n", DeviceExtension->Confgig_PortNumber));
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//DeviceExtension->Data_LinearAddress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Physical_Address = RtlConvertUlongToLargeInteger( DeviceExtension->m_PciInfo[1].m_dwBaseAddress );//Access[1]
		/*
	if(AddressSpace) {
		BOOLEAN bAddressCheck;

		bAddressCheck = HalTranslateBusAddress( PCIBus, 0, Physical_Address, &AddressSpace, &TranslatedAddress );
		
		if(bAddressCheck == TRUE) {
			DbgPrint((" - HalTranslateBusAddress TRUE[Data_LinearAddress] \n"));
		} else {
			DbgPrint((" - HalTranslateBusAddress FALSE[Data_LinearAddress] \n"));
		} //end if

		DeviceExtension->Data_LinearAddress = (UCHAR *)TranslatedAddress.u.LowPart;
	} else { 
		*/
		DeviceExtension->Data_LinearAddress = (UCHAR *)MmMapIoSpace( Physical_Address, DeviceExtension->m_PciInfo[1].m_dwLength, FALSE );
	//} //end if

	//DeviceExtension->Data_LinearAddress = (UCHAR*)MmMapIoSpace( Physical_Address, DeviceExtension->m_PciInfo[1].m_dwLength, FALSE );

	KdPrint((" - DeviceExtension->Data_LinearAddress(UCHAR *) = 0x%x\n", DeviceExtension->Data_LinearAddress));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ��ũ ����� ������ �´�...  //�̷� ó���� ��ũ ����� ������ ���� ���մϴ�
//DISK_SIZE = READ_PORT_ULONG(DeviceExtension->Confgig_PortNumber+4) + 1;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	WRITE_PORT_ULONG(&DeviceExtension->Confgig_PortNumber, 0x00000000 );	

	DeviceExtension->DiskLength = DEFAULT_DISK_SIZE; //1G

	KdPrint((" - DeviceExtension->DiskLength = 0x%x\n", DeviceExtension->DiskLength));

	DeviceExtension->m_TotalHwseg = ((DeviceExtension->DiskLength)/HWSEGMENT)+1;
	KdPrint((" - DeviceExtension->m_TotalHwseg = 0x%x\n", DeviceExtension->m_TotalHwseg));
	

	RtlInitUnicodeString( &uniDriverPath, DRIVER_REGPATH );

	if(GetRegDWORDInfo( L"Ez74_DELAYTIME", &uniDriverPath, &ulongData ) == STATUS_SUCCESS) {
		
		g_dwDelayTime = ulongData;
    } 
	KdPrint((" - Delay Time Value-->g_dwDelayTime = %d\n", g_dwDelayTime));
	
	DeviceExtension->DiskLength			= DeviceExtension->DiskLength;
    DeviceExtension->BytesPerSector		= 512;
    DeviceExtension->SectorsPerTrack	= 32;
    DeviceExtension->TracksPerCylinder	= 2;
    DeviceExtension->NumberOfCylinders	= (DeviceExtension->DiskLength) / 512 / 32 / 2;

	DbgPrint(("\nEzHwInitialize routine end\n"));
	return TRUE;
}
/***********************************************************************************
�Լ��� : GetRegDWORDInfo
��  �� :
			Arguments : pEntryKey - ��Ʈ�� Ű�� 
			pRegKey -  ������Ʈ�� Ű��  
			pKeyValue - Ű �� ( ���ϵ� )
���ϰ� : NTSTATUS
��  �� : ������Ʈ���� DWORD Type �����͸� �����´�. 
***********************************************************************************/
NTSTATUS GetRegDWORDInfo( PWCHAR pEntryKey, PUNICODE_STRING pRegKey, PULONG pKeyValue )
{
	RTL_QUERY_REGISTRY_TABLE paramTable[2];
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG zero=0;
	PWCHAR pRegKeyToRead;

	pRegKeyToRead = ExAllocatePool(PagedPool, MAXIMUM_FILENAME_LENGTH );
	if( pRegKeyToRead != NULL) {

		RtlZeroMemory( pRegKeyToRead, MAXIMUM_FILENAME_LENGTH );
		RtlMoveMemory( pRegKeyToRead, pRegKey->Buffer, pRegKey->Length );
		RtlZeroMemory( paramTable, sizeof( paramTable) );
		KdPrint( ( "C2D2 : Reg : %ws    Enkey %ws \n" , pRegKeyToRead, pEntryKey) ) ;
   
		paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
		paramTable[0].Name = pEntryKey;
		paramTable[0].EntryContext = pKeyValue;
		paramTable[0].DefaultType = REG_DWORD;
		paramTable[0].DefaultData = &zero;
		paramTable[0].DefaultLength = sizeof( ULONG );
 
		if( !NT_SUCCESS( 
			RtlQueryRegistryValues(
				RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
				pRegKeyToRead,
				&paramTable[0],
				NULL,
				NULL) ) ) {
				
			Status = STATUS_UNSUCCESSFUL;    

		} else {
			Status = STATUS_SUCCESS;    
		} //end if

	ExFreePool(pRegKeyToRead);

	} //end if
	KdPrint(("GetRegDWORDInfo = Status = 0x%x\n"));

	return Status;
}
/***********************************************************************************
�Լ��� : 
��  �� :
���ϰ� : 
��  �� : 
***********************************************************************************/
