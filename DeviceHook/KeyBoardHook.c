#include "hook.h"

#define path "\\Device"
#define DeviceName L"\\Device\\KeyHook"
#define keyborad L"\\Device\\KeyboardClass0"

UNICODE_STRING Keyboard;
PDEVICE_OBJECT KeyboardObj;
FAST_MUTEX Log;
ULONG g_nop;

void Unload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("Unload KeyboardHooker.sys\n");
	PDEVICE_EXTENSION pDeviceExtension = (PDEVICE_EXTENSION)pDriverObject->DeviceObject->DeviceExtension;
	IoDetachDevice(pDeviceExtension->NextLayerDeviceObject);
	IoDeleteDevice(pDriverObject->DeviceObject);
}

NTSTATUS ReadComplete(IN PDEVICE_OBJECT DeviceObj, IN PIRP Irp, IN PVOID Context)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PKEYBOARD_INPUT_DATA keyData;
	int numKey;

	DbgPrint("ReadComplete Start\n");

	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		keyData = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
		numKey = Irp->IoStatus.Information / sizeof(KEYBOARD_INPUT_DATA);

		for (int i = 0; i < numKey; i++)
		{
			DbgPrint("ScanCode: %x\n", keyData->MakeCode);

			if (keyData->MakeCode == 0x1e)
				keyData->MakeCode = 0x25;
		}
	}

	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}
	DbgPrint("ReadComplete End!\n");

	return STATUS_SUCCESS;
}

NTSTATUS Read(IN PDEVICE_OBJECT DeviceObj, IN PIRP Irp)
{
	NTSTATUS stat;
	PIO_STACK_LOCATION currentIrpStack;
	PIO_STACK_LOCATION nextIrpStack;

	currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
	nextIrpStack = IoGetNextIrpStackLocation(Irp);

	*nextIrpStack = *currentIrpStack;

	DbgPrint("[Read]\n");
	IoSetCompletionRoutine(
		Irp,
		ReadComplete,
		DeviceObj, 
		TRUE, 
		TRUE, 
		TRUE);
	stat = IoCallDriver(((PDEVICE_EXTENSION)DeviceObj->DeviceExtension)->NextLayerDeviceObject, Irp);
	KdPrint(("READ END\n"));
	return stat;
}

NTSTATUS DispatchsSkip(IN PDEVICE_OBJECT DeviceObj, IN PIRP Irp)
{
	NTSTATUS stat;

	DbgPrint("Skip!\n");

	
	IoSkipCurrentIrpStackLocation(Irp);
	stat = IoCallDriver(((PDEVICE_EXTENSION)DeviceObj->DeviceExtension)->NextLayerDeviceObject, Irp);

	return stat;
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObj, IN PUNICODE_STRING RegistryPath)
{
	PDEVICE_OBJECT DeviceObj;
	PDEVICE_OBJECT pKbClassDeviceObject;
	PDEVICE_EXTENSION pDeviceExtension = NULL;
	UNICODE_STRING uDeviceName;
	UNICODE_STRING uTargetDeviceName;
	//PFILE_OBJECT fileObject;
	NTSTATUS stat = STATUS_SUCCESS;

	for (ULONG i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObj->MajorFunction[i] = DispatchsSkip;
	
	DriverObj->MajorFunction[IRP_MJ_READ] = Read;
	DriverObj->DriverUnload = Unload;

	DbgPrint("DriverEntry Start!\n");

	RtlInitUnicodeString(&uDeviceName, DeviceName);
	RtlInitUnicodeString(&uTargetDeviceName, keyborad);

	stat = IoCreateDevice(DriverObj, 
		sizeof(DEVICE_EXTENSION), 
		&uDeviceName, 
		FILE_DEVICE_KEYBOARD, 
		0, 
		FALSE, 
		&DeviceObj);
	if (!NT_SUCCESS(stat))
	{
		return stat;
	}

	RtlZeroMemory(DeviceObj->DeviceExtension, sizeof(DEVICE_EXTENSION));
	pDeviceExtension = (PDEVICE_EXTENSION)DeviceObj->DeviceExtension;

	stat = IoAttachDevice(DeviceObj, 
		&uTargetDeviceName, 
		&(pDeviceExtension->NextLayerDeviceObject));
	if (!NT_SUCCESS(stat))
		return stat;
	
	DeviceObj->Flags = DeviceObj->Flags | (DO_BUFFERED_IO | DO_POWER_PAGABLE);
	DeviceObj->Flags = DeviceObj->Flags & ~DO_DEVICE_INITIALIZING;

	DbgPrint("Device Attach Success\n");

	return stat;
}
