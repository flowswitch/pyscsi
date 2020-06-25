#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <windows.h>

#define PYSCSIAPI extern "C" __declspec(dllexport)

#define SCSI_IOCTL_DATA_OUT               0
#define SCSI_IOCTL_DATA_IN                1
#define SCSI_IOCTL_DATA_UNSPECIFIED       2

static HANDLE hf = INVALID_HANDLE_VALUE;
static int timeout = 30;

typedef struct _SCSI_PASS_THROUGH_DIRECT {
  USHORT  Length;
  UCHAR  ScsiStatus;
  UCHAR  PathId;
  UCHAR  TargetId;
  UCHAR  Lun;
  UCHAR  CdbLength;
  UCHAR  SenseInfoLength;
  UCHAR  DataIn;
  ULONG  DataTransferLength;
  ULONG  TimeOutValue;
  PVOID  DataBuffer;
  ULONG  SenseInfoOffset;
  UCHAR  Cdb[16];
} SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

static int CallSptd(unsigned char dir, unsigned timeout, unsigned char *cdb, unsigned cdb_size, void *buf, unsigned data_size)
{
	uint8_t cmd[0x50];
	memset(cmd, 0, 0x50);

	SCSI_PASS_THROUGH_DIRECT *sptd = (SCSI_PASS_THROUGH_DIRECT *)cmd;
	sptd->Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptd->TimeOutValue = timeout;
	sptd->PathId = 0;
	sptd->TargetId = 1;
	sptd->Lun = 0;
	sptd->CdbLength = cdb_size;
	sptd->SenseInfoLength = 0x18;
	sptd->DataIn = dir;
	sptd->DataTransferLength = data_size;
	sptd->DataBuffer = buf;
	sptd->SenseInfoOffset = 0x30;
	memcpy(sptd->Cdb, cdb, cdb_size);
	DWORD BytesReturned = 0;
	if (!DeviceIoControl(hf, 0x4D014, cmd, 0x50, cmd, 0x50, &BytesReturned, 0))
	{
		return GetLastError();
	}
	return sptd->ScsiStatus;
}

PYSCSIAPI void ScsiSetTimeout(int tmo)
{
	timeout = tmo;
}

PYSCSIAPI int ScsiGetTimeout(void)
{
	return timeout;
}

PYSCSIAPI int ScsiOut(unsigned char *cdb, unsigned cdb_size, void *buf, unsigned data_size)
{
	return CallSptd(SCSI_IOCTL_DATA_OUT, timeout, cdb, cdb_size, buf, data_size);
}

PYSCSIAPI int ScsiIn(unsigned char *cdb, unsigned cdb_size, void *buf, unsigned data_size)
{
	return CallSptd(SCSI_IOCTL_DATA_IN, timeout, cdb, cdb_size, buf, data_size);
}

PYSCSIAPI int ScsiOpen(const char drive)
{
	char path[] = "\\\\.\\H:";
	path[4] = drive;
	hf = CreateFile(path, 
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, //lpSecurityAttributes
			OPEN_EXISTING,
			0x20000000,
			0 //hTemplateFile
			);
	return (hf==INVALID_HANDLE_VALUE) ? -1 : 0;
}

PYSCSIAPI void ScsiClose()
{
	if (hf!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(hf);			
		hf = INVALID_HANDLE_VALUE;
	}
}
