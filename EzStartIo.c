#include "ntddk.h"
#include "Ez.h"
#include "ntdddisk.h"
#include "ntddscsi.h"
#pragma optimize("", off)

ULONG			StartIoCount = 0;
/***********************************************************************************
함수명 : EzStartIo
인  자 :
			IN PVOID HwDeviceExtension
			IN PSCSI_REQUEST_BLOCK Srb
리턴값 : BOOLEAN
설  명 : StartIo
***********************************************************************************/
BOOLEAN EzStartIo(IN PVOID HwDeviceExtension,IN PSCSI_REQUEST_BLOCK Srb)
{
    PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
    ULONG status;
    
	DbgPrint(("\nEzStartIo routine start\n"));


	if (StartIoCount > 0) {
		_asm	int		3
	}

	StartIoCount++;

	KdPrint((" - EzStartIo --> StartIoCount = %d\n", StartIoCount));
	//

	//기본에 그냥 올리면 0만 잡힌다.
	//Bus Number 0 , Target ID 0, LUN 0
	KdPrint((" - Srb->PathId = %d, Srb->TargetId = %d, Srb->Lun = %d\n",
			Srb->PathId, Srb->TargetId, Srb->Lun));

	//Srb->Lun이 증가할수록 디스크 장치도 증가.
	
	if (Srb->Lun != 0 || Srb->TargetId != 0 || Srb->SrbStatus != 0 || Srb->PathId != 0) {
		status = SRB_STATUS_SELECTION_TIMEOUT;
		
		KdPrint((" - EzStartIo: Skip response about slave HDDs\n"));
		goto Exit;

	}
	
    switch (Srb->Function) {

	case SRB_FUNCTION_EXECUTE_SCSI: 

		DbgPrint((" - EzStartIo: SRB_FUNCTION_EXECUTE_SCSI\n"));
		
		if (DeviceExtension->CurrentSrb) {
			DbgPrint((" - EzStartIo: Already have a request!\n"));
			
			Srb->SrbStatus = SRB_STATUS_BUSY;			
			ScsiPortNotification(RequestComplete, DeviceExtension, Srb);

			DbgPrint((" - EzStartIo: return FALSE.\n"));
			
			StartIoCount--;

			return FALSE;
		} //end if

        DeviceExtension->CurrentSrb = Srb;
		status = DiskSendCommand(HwDeviceExtension, Srb);
				
        // Get logical unit extension.
        //ScsiPortGetLogicalUnit(deviceExtension, Srb->PathId, Srb->TargetId, Srb->Lun);
        
        // Build CCB.
        //BuildCcb(deviceExtension, Srb);

		//status = SRB_STATUS_SUCCESS;
		KdPrint((" - EzStartIo: Srbstatus = %lxh\n", (ULONG)status));
        break;

    case SRB_FUNCTION_ABORT_COMMAND:
		DbgPrint((" - EzStartIo: SRB_FUNCTION_ABORT_COMMAND\n"));
        if (!DeviceExtension->CurrentSrb) {
			DbgPrint((" - EzStartIo: SRB to abort already completed\n"));
            status = SRB_STATUS_ABORT_FAILED;
            break;
        }

        // ------------------------------------------------------------
        // Abort function indicates that a request timed out.
        // Call reset routine. Card will only be reset if
        // status indicates something is wrong.
        // Fall through to reset code.
        // -------------------------------------------------- Fucked up!


    case SRB_FUNCTION_RESET_BUS:		
		DbgPrint((" - EzStartIo: SRB_FUNCTION_RESET_BUS\n"));
		DbgPrint((" - EzStartIo: Reset bus request received\n"));
        if (!Ez74ResetController(DeviceExtension, Srb->PathId)) {
            
			DbgPrint((" - EzStartIo: Reset bus failed\n"));
            ScsiPortLogError(
                HwDeviceExtension,
                NULL,
                0,
                0,
                0,
                SP_INTERNAL_ADAPTER_ERROR,
                5 << 8 // ?
                );
            status = SRB_STATUS_ERROR;
        } else {
            status = SRB_STATUS_SUCCESS;
		}
        break;

    case SRB_FUNCTION_IO_CONTROL:
		DbgPrint((" - EzStartIo: SRB_FUNCTION_IO_CONTROL\n"));
		
		if (DeviceExtension->CurrentSrb) {

            DbgPrint(("AtapiStartIo: Already have a request!\n"));

            Srb->SrbStatus = SRB_STATUS_BUSY;
            ScsiPortNotification(RequestComplete, DeviceExtension, Srb);
            return FALSE;
        } //end if

        //
        // Indicate that a request is active on the controller.
        //

        DeviceExtension->CurrentSrb = Srb;

        if (Ez74StringCmp( ((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,"SCSIDISK",strlen("SCSIDISK"))) {

            DbgPrint(("AtapiStartIo: IoControl signature incorrect. Send %s, expected %s\n",
                        ((PSRB_IO_CONTROL)(Srb->DataBuffer))->Signature,
                        "SCSIDISK"));

            status = SRB_STATUS_INVALID_REQUEST;
            break;
        }

        switch (((PSRB_IO_CONTROL)(Srb->DataBuffer))->ControlCode) {

            case IOCTL_SCSI_MINIPORT_SMART_VERSION: {

                PGETVERSIONINPARAMS versionParameters = (PGETVERSIONINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                UCHAR deviceNumber;

                //
                // Version and revision per SMART 1.03
                //

                versionParameters->bVersion = 1;
                versionParameters->bRevision = 1;
                versionParameters->bReserved = 0;

                //
                // Indicate that support for IDE IDENTIFY, ATAPI IDENTIFY and SMART commands.
                //

                versionParameters->fCapabilities = (CAP_ATA_ID_CMD | CAP_ATAPI_ID_CMD | CAP_SMART_CMD);

                //
                // This is done because of how the IOCTL_SCSI_MINIPORT
                // determines 'targetid's'. Disk.sys places the real target id value
                // in the DeviceMap field. Once we do some parameter checking, the value passed
                // back to the application will be determined.
                //

                deviceNumber = versionParameters->bIDEDeviceMap;

                if (!(DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT) ||
                    (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE)) {

                    status = SRB_STATUS_SELECTION_TIMEOUT;
                    break;
                }


                //
                // NOTE: This will only set the bit
                // corresponding to this drive's target id.
                // The bit mask is as follows:
                //
                //     Sec Pri
                //     S M S M
                //     3 2 1 0
                //

                if (DeviceExtension->NumberChannels == 1) {
                    if (DeviceExtension->PrimaryAddress) {
                        deviceNumber = 1 << Srb->TargetId;
                    } else {
                        deviceNumber = 4 << Srb->TargetId;
                    }
                } else {
                    deviceNumber = 1 << Srb->TargetId;
                }

                versionParameters->bIDEDeviceMap = deviceNumber;

                status = SRB_STATUS_SUCCESS;
                break;
            }

            case IOCTL_SCSI_MINIPORT_IDENTIFY: {

                PSENDCMDOUTPARAMS cmdOutParameters = (PSENDCMDOUTPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                SENDCMDINPARAMS   cmdInParameters = *(PSENDCMDINPARAMS)(((PUCHAR)Srb->DataBuffer) + sizeof(SRB_IO_CONTROL));
                ULONG             i;
                UCHAR             targetId;


                if (cmdInParameters.irDriveRegs.bCommandReg == ID_CMD) {

                    //
                    // Extract the target.
                    //

                    targetId = cmdInParameters.bDriveNumber;

                if (!(DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_DEVICE_PRESENT) ||
                     (DeviceExtension->DeviceFlags[Srb->TargetId] & DFLAGS_ATAPI_DEVICE)) {

                        status = SRB_STATUS_SELECTION_TIMEOUT;
                        break;
                    }

                    //
                    // Zero the output buffer
                    //

                    for (i = 0; i < (sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1); i++) {
                        ((PUCHAR)cmdOutParameters)[i] = 0;
                    }

                    //
                    // Build status block.
                    //

                    cmdOutParameters->cBufferSize = IDENTIFY_BUFFER_SIZE;
                    cmdOutParameters->DriverStatus.bDriverError = 0;
                    cmdOutParameters->DriverStatus.bIDEError = 0;

                    //
                    // Extract the identify data from the device extension.
                    //

                    //ScsiPortMoveMemory (cmdOutParameters->bBuffer, &DeviceExtension->IdentifyData[targetId], IDENTIFY_DATA_SIZE);
					//RtlMoveMemory(cmdOutParameters->bBuffer,&DeviceExtension->IdentifyData[targetId],IDENTIFY_DATA_SIZE);
                    status = SRB_STATUS_SUCCESS;


                } else {
                    status = SRB_STATUS_INVALID_REQUEST;
                }
                break;
            }

            case  IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS:
            case  IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS:
            case  IOCTL_SCSI_MINIPORT_ENABLE_SMART:
            case  IOCTL_SCSI_MINIPORT_DISABLE_SMART:
            case  IOCTL_SCSI_MINIPORT_RETURN_STATUS:
            case  IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE:
            case  IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES:
            case  IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS:

                //status = IdeSendSmartCommand(HwDeviceExtension,Srb);
                break;

            default :

                status = SRB_STATUS_INVALID_REQUEST;
                break;

        }
        break;

    case SRB_FUNCTION_CLAIM_DEVICE:    
		DbgPrint((" - EzStartIo: SRB_FUNCTION_CLAIM_DEVICE\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_RECEIVE_EVENT:  
		DbgPrint((" - EzStartIo: AtapiStartIo: SRB_FUNCTION_RECEIVE_EVENT\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: AtapiStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_RELEASE_QUEUE:   
		DbgPrint((" - EzStartIo: SRB_FUNCTION_RELEASE_QUEUE\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_ATTACH_DEVICE:          
		DbgPrint((" - EzStartIo: SRB_FUNCTION_ATTACH_DEVICE\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_RELEASE_DEVICE:         
		DbgPrint((" - EzStartIo: SRB_FUNCTION_RELEASE_DEVICE\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_SHUTDOWN:               
		DbgPrint((" - EzStartIo: SRB_FUNCTION_SHUTDOWN\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_FLUSH:                  
		DbgPrint((" - EzStartIo: SRB_FUNCTION_FLUSH\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_RELEASE_RECOVERY:       
		DbgPrint((" - EzStartIo: SRB_FUNCTION_RELEASE_RECOVERY\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_RESET_DEVICE:           
		DbgPrint((" - EzStartIo: SRB_FUNCTION_RESET_DEVICE\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
		break;

    case SRB_FUNCTION_TERMINATE_IO:           
		DbgPrint((" - EzStartIo: SRB_FUNCTION_TERMINATE_IO\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

    case SRB_FUNCTION_FLUSH_QUEUE:            
		DbgPrint((" - EzStartIo: SRB_FUNCTION_FLUSH_QUEUE\n"));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
		break;

    case SRB_FUNCTION_REMOVE_DEVICE:          
		DbgPrint((" - EzStartIo: SRB_FUNCTION_REMOVE_DEVICE\n"));
        status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
        break;

	default:
		KdPrint((" - EzStartIo: SRB_FUNCTION_XXXXXXXX: Srb->Function == %lxh\n",Srb->Function));
		status = SRB_STATUS_INVALID_REQUEST;
		DbgPrint((" - EzStartIo: status = SRB_STATUS_INVALID_REQUEST;\n"));
    }

Exit:

    if (status != SRB_STATUS_PENDING) {

		KdPrint((" - EzStartIo: Srb %lxh complete with status %lxh\n",Srb, status));

        DeviceExtension->CurrentSrb = NULL;
        
		Srb->SrbStatus = (UCHAR)status;
        
		ScsiPortNotification(RequestComplete,   DeviceExtension, Srb);

        ScsiPortNotification(NextRequest,  DeviceExtension, NULL);
    } //end if
            
	//Srb->SrbStatus = (UCHAR)status;
	KdPrint((" - EzStartIo: Srb->SrbStatus = %lxh\n",Srb->SrbStatus));
	DbgPrint((" - EzStartIo: return TRUE\n"));

	DeviceExtension->CurrentSrb = 0;

	StartIoCount--;

	DbgPrint(("\nEzStartIo routine end\n"));
    return TRUE;
}
/***********************************************************************************
함수명 : DiskSendCommand
인  자 :
			IN PVOID HwDeviceExtension
			IN PSCSI_REQUEST_BLOCK Srb
리턴값 : ULONG
설  명 : SRB_FUNCTION_EXECUTE_SCSI
***********************************************************************************/
ULONG DiskSendCommand(IN PVOID HwDeviceExtension,IN PSCSI_REQUEST_BLOCK Srb)
{	
	PINQUIRYDATA    pInquiryData;

	ULONG i = 0;
	ULONG status;
	
	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
	ULONG diskSize = DeviceExtension->DiskLength ;
	
	PMODE_PARAMETER_HEADER   modeData;

	static UCHAR PartBuffer[0x200];
	static UCHAR tempBuffer[0x200];//512
	static UCHAR zeroBuffer[0x200];
	PARTI Part;
		
	//Part = 	(PPARTI)DeviceExtension->DiskImage;
	

    KdPrint((" -- DiskSendCommand: Command %lxh to device[Srb->TargetId] %d\n", Srb->Cdb[0], Srb->TargetId));

    switch (Srb->Cdb[0]) {

    case SCSIOP_INQUIRY:
        KdPrint((" -- SCSIOP_INQUIRY to device[Srb->TargetId] %d\n",Srb->TargetId));
		
        pInquiryData  = Srb->DataBuffer;

        memset(Srb->DataBuffer, 0, Srb->DataTransferLength);

        pInquiryData->DeviceType = DIRECT_ACCESS_DEVICE;

		memcpy(pInquiryData->VendorId	, "Ez Disk"	, 8 );
		memcpy(pInquiryData->ProductId	, "Drive"	, 16);

		for (i = 0; i < 4; i += 2) {
			pInquiryData->ProductRevisionLevel[i]	= 0x30;//FirmwareRevison[i+1] 펌웨어 수정???
			pInquiryData->ProductRevisionLevel[i+1] = 0x30;//FirmwareRevison[i]
		}
	    //pInquiryData->RemovableMedia = 1 ;
		pInquiryData->Synchronous = 1;


		KdPrint((" - diskSize = 0x%x, SIZE_SECTOR = 0x%x\n", diskSize, SIZE_SECTOR));
				
		ReadSector(DeviceExtension, 0, 1, tempBuffer);

		if ((tempBuffer[510] == 0x55) && (tempBuffer[511] == 0xAA)) {
			DbgPrint((" - Sector Value (tempBuffer[510] == 0x55) && (tempBuffer[511] == 0xAA))\n"));
			status = SRB_STATUS_SUCCESS;
			break;

		} else {

			Part.indicator	= 0x80;
			Part.starthead	= 0;
			Part.startsec	= 1;
			Part.starttrack = 0;
			Part.endhead	= 0;
			Part.endsec		= (UCHAR)( ( diskSize / SIZE_SECTOR) - 1 - 1);
			Part.endtrack	= 0;
			Part.parttype	= 4;
			Part.bias		= 1;
			Part.partsize	= diskSize / SIZE_SECTOR;
				
			memcpy(PartBuffer + 446, &Part, sizeof(PARTI));
			
			PartBuffer[510] = 0x55;
			PartBuffer[511] = 0xAA;
		} //end if
				
		DeviceExtension->DiskImage = PartBuffer;
		KdPrint((" - PartBuffer = 0x%x, DeviceExtension->DiskImage = 0x%x\n", PartBuffer, DeviceExtension->DiskImage));
		
		WriteSector(DeviceExtension, 0, 1, PartBuffer);

		//이곳 위까지는 0번쩨에 파티션에 대한 정보를 한 섹터만 블록으로 기록을 하였습니당.

		//아래는 다음 1번째 부터 메모리를 초기화 세팅을 해주는 겁니당.

		/*
		for (i = 1; i < 0x10000; i++) {	//65536 = 1024 * 64 64k만큼 loop	
			//1G / 512 Sector =2097152 [ 0x200000 ]
			WriteSector(DeviceExtension, i, 1, zeroBuffer);

		} //end for
		*/
		
		status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_READ_CAPACITY: //크기를 변경하는 부분
		KdPrint((" -- SCSIOP_READ_CAPACITY to device %d\n",Srb->TargetId));
	
		((PREAD_CAPACITY_DATA)Srb->DataBuffer)->BytesPerBlock = 0x20000;	// Big Endian(512Bytes/Secter)

		//i = diskSize / SIZE_SECTOR;
		i = diskSize / SIZE_SECTOR;
		//i = 0x7D82;
		((PREAD_CAPACITY_DATA)Srb->DataBuffer)->LogicalBlockAddress =
            (((PUCHAR)&i)[0] << 24) |
			(((PUCHAR)&i)[1] << 16) |
            (((PUCHAR)&i)[2] << 8) | 
			((PUCHAR)&i)[3];

        KdPrint((" -- disk %lxh - i = diskSize / SIZE_SECTOR : %lxh\n",Srb->TargetId, i));
        status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_READ:   
		KdPrint((" -- SCSIOP_READ to device %d\n",Srb->TargetId));
        status = DiskReadWrite(HwDeviceExtension, Srb);
	    break;
	case SCSIOP_WRITE:
		KdPrint((" -- SCSIOP_WRITE to device %d\n",Srb->TargetId));
        status = DiskReadWrite(HwDeviceExtension, Srb);
	    break;
    
	case SCSIOP_MODE_SENSE:
		KdPrint((" -- SCSIOP_MODE_SENSE to device %d\n",Srb->TargetId));
		modeData = (PMODE_PARAMETER_HEADER)Srb->DataBuffer;
       
		Srb->DataTransferLength = sizeof(MODE_PARAMETER_HEADER);       
		modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;

        status = SRB_STATUS_INVALID_REQUEST;
		//status = SRB_STATUS_SUCCESS;
		break;

	case SCSIOP_MODE_SELECT: 		
		KdPrint((" -- SCSIOP_MODE_SELECT to device %d\n",Srb->TargetId));
		status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_TEST_UNIT_READY:
		KdPrint((" -- SCSIOP_TEST_UNIT_READY to device %d\n",Srb->TargetId));
        status = SRB_STATUS_SUCCESS;
        break;
    case SCSIOP_VERIFY:
		KdPrint((" -- SCSIOP_VERIFY to device %d\n",Srb->TargetId));
		status = SRB_STATUS_SUCCESS;
        break;

    case SCSIOP_START_STOP_UNIT:
		KdPrint((" -- SCSIOP_START_STOP_UNIT to device %d\n",Srb->TargetId));
		status = SRB_STATUS_SUCCESS;
		break;

    case SCSIOP_REQUEST_SENSE:
		KdPrint((" -- SCSIOP_REQUEST_SENSE to device %d\n",Srb->TargetId));
    
	default:
       KdPrint((" -- DiskSendCommand: Unsupported command %lxh\n",
                  Srb->Cdb[0]));

       status = SRB_STATUS_INVALID_REQUEST;

    }
    KdPrint((" -- DiskSendCommand: return status = %lxh\n",status));
    return status;

}
/***********************************************************************************
함수명 : DiskReadWrite
인  자 :
			IN PVOID HwDeviceExtension
			IN PSCSI_REQUEST_BLOCK Srb
리턴값 : ULONG
설  명 : SCSIOP_READ OR SCSIOP_WRITE
***********************************************************************************/
ULONG DiskReadWrite(IN PVOID HwDeviceExtension,IN PSCSI_REQUEST_BLOCK Srb)
{	
	UCHAR *pBuffer;
	ULONG dwSize;
	ULONG startingSector;
	ULONG SectorCount;

	PHW_DEVICE_EXTENSION DeviceExtension = HwDeviceExtension;
		
	//PARTI Part;	
	//////////////////////////////////////////////////////////////////////////////
	pBuffer = (UCHAR *)Srb->DataBuffer;
	/*
	DeviceExtension->DiskImage = (UCHAR *)Srb->DataBuffer;
	
	Part.indicator	= 0x80;
	Part.starthead	= 0;
	Part.startsec	= 1;
	Part.starttrack = 0;
	Part.endhead	= 0;
	Part.endsec		= (UCHAR)( ( (DeviceExtension->DiskLength) / SIZE_SECTOR) - 1 - 1);
	Part.endtrack	=0;
	Part.parttype	= 4;
	Part.bias		= 1;
	Part.partsize	= ( (DeviceExtension->DiskLength) / SIZE_SECTOR) - 1;
		
	memcpy(DeviceExtension->DiskImage + 446, &Part, sizeof(PARTI));
	
	DeviceExtension->DiskImage[510] = 0x55;
	DeviceExtension->DiskImage[511] = 0xAA;
	*/
	//////////////////////////////////////////////////////////////////////////////


	dwSize = Srb->DataTransferLength;
	
    startingSector = ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte3 |
                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte2 << 8 |
                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte1 << 16 |
                        ((PCDB)Srb->Cdb)->CDB10.LogicalBlockByte0 << 24;

	KdPrint((" - DiskReadWrite startingSector = %u\n", startingSector));

	if ((dwSize % SIZE_SECTOR) != 0) {
		_asm	int		3
	}

	SectorCount =  (UCHAR)((dwSize + 0x1FF) / SIZE_SECTOR);
	KdPrint((" - DiskReadWrite SectorCount = %d\n", SectorCount));

	if (startingSector >= ((DeviceExtension->DiskLength) / SIZE_SECTOR)) {
		KdPrint((" - startingSector >= (diskSize = %u / SIZE_SECTOR = %u )\n", (DeviceExtension->DiskLength) , SIZE_SECTOR));
		return SRB_STATUS_SUCCESS;
	} //end if

	if (Srb->SrbFlags & SRB_FLAGS_DATA_IN) { //Read 가 내려왔을 시..0
		KdPrint((" - DiskReadWrite READ command! \n"));
		
		ReadSector( HwDeviceExtension, startingSector, SectorCount, pBuffer );

	} else { // Write 가 내려왔을 시...
		KdPrint((" - DiskReadWrite WRITE command! \n"));
		
		WriteSector( HwDeviceExtension, startingSector, SectorCount, pBuffer );

	} //end if

	return SRB_STATUS_SUCCESS;		
}
/***********************************************************************************
함수명 : Ez74StringCmp
인  자 : 
			PCHAR FirstStr
			PCHAR SecondStr
			ULONG Count
리턴값 : LONG
설  명 : 문자열을 비교
***********************************************************************************/
LONG Ez74StringCmp (PCHAR FirstStr,PCHAR SecondStr,ULONG Count)
{
    UCHAR  first ,last;

    if (Count) {
        do {

            //
            // Get next char.
            //

            first = *FirstStr++;
            last = *SecondStr++;

            if (first != last) {

                //
                // If no match, try lower-casing.
                //

                if (first>='A' && first<='Z') {
                    first = first - 'A' + 'a';
                }
                if (last>='A' && last<='Z') {
                    last = last - 'A' + 'a';
                }
                if (first != last) {

                    //
                    // No match
                    //

                    return first - last;
                }
            }
        }while (--Count && first);
    }

    return 0;
}
/***********************************************************************************
함수명 : 
인  자 :
리턴값 : 
설  명 : 
***********************************************************************************/
