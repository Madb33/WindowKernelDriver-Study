#include "Source.h"

#define NEXTDEVICENAME L"\\Device\\Hook_MSG1"
UNICODE_STRING DriverName, DosDeviceName, NextDeviceName;

#define IOCTL_EXIT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x4000, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TEXTRW CTL_CODE(FILE_DEVICE_UNKNOWN, 0x5000, METHOD_BUFFERED, FILE_ANY_ACCESS)

void Unload(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS stat = STATUS_SUCCESS;
	UNICODE_STRING name;

	IoDeleteDevice(pDriverObject->DeviceObject);
}

NTSTATUS Create(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ret = STATUS_SUCCESS;

	return ret;
}

NTSTATUS Close(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS ret = STATUS_SUCCESS;

	return ret;
}
NTSTATUS Read(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS stat = STATUS_SUCCESS;
	PIO_STACK_LOCATION Stack;
	char* buffer = "Hello from Kernel";
	char* system_buffer = Irp->AssociatedIrp.SystemBuffer;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	RtlZeroMemory(system_buffer, Stack->Parameters.Read.Length);
	RtlCopyMemory(system_buffer, buffer, strlen(buffer));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = strlen(buffer);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return stat;
}

NTSTATUS Write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS stat = STATUS_SUCCESS;
	PIO_STACK_LOCATION Stack;
	char buffer[256];
	char* system_buffer = Irp->AssociatedIrp.SystemBuffer;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	RtlZeroMemory(buffer, Stack->Parameters.Write.Length);
	RtlCopyMemory(buffer, system_buffer, strlen(system_buffer));

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = strlen(buffer);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	
	return stat;
}

NTSTATUS DeviceIOController(PDEVICE_OBJECT DeviceOBJ, PIRP irp)
{
	PDEVICE_EXTENSION DevExt = (PDEVICE_EXTENSION)DeviceOBJ->DeviceExtension;
	PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(irp);
	ULONG InputIO = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputIO = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = Stack->Parameters.DeviceIoControl.IoControlCode;
	PVOID buf = (PVOID)irp->AssociatedIrp.SystemBuffer;;
	NTSTATUS stat = STATUS_SUCCESS;
	ULONG info;

	switch (code)
	{
	case IOCTL_EXIT:
	{
		IoCallDriver(DevExt->NextLayerDeviceObject, irp);
		char Exit[] = "Message2 Hook Driver Exit";
		if (OutputIO < strlen(Exit))
		{
			stat = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		//OutputIO = irp->UserBuffer;
		RtlCopyMemory(buf, Exit, sizeof(Exit) + 1);
		info = sizeof(Exit);
		break;
	}

	case IOCTL_TEXTRW:
	{
		char msg[] = "Message2 Write Success";
		if (OutputIO < strlen(msg))
		{
			stat = STATUS_INVALID_BUFFER_SIZE;
			break;
		}
		DbgPrint("[%s]", buf);
		RtlCopyMemory(buf, msg, sizeof(msg));
		info = sizeof(msg);
		break;
	}
	}
	irp->IoStatus.Status = stat;
	irp->IoStatus.Information = info;
	IoCompleteRequest(irp, IO_NO_INCREMENT, info);
	IoCallDriver(DevExt->NextLayerDeviceObject, irp);
}

NTSTATUS Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS stat = STATUS_SUCCESS;
	PIO_STACK_LOCATION Stack;

	Stack = IoGetCurrentIrpStackLocation(Irp);

	switch (Stack->MinorFunction)
	{
	case IRP_MJ_CREATE:
	{
		Create(DeviceObject, Irp);
		break;
	}
	case IRP_MJ_CLOSE:
	{
		Close(DeviceObject, Irp);
		break;
	}
	case IRP_MJ_READ:
	{
		Read(DeviceObject, Irp);
		break;
	}
	case IRP_MJ_WRITE:
	{
		Write(DeviceObject, Irp);
		break;
	}
	break;

	return stat;
	}
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObj, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS stat = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject= 0;
	PDEVICE_EXTENSION DeviceExtension;
	
	RtlInitUnicodeString(&NextDeviceName, NEXTDEVICENAME);
	DriverObj->MajorFunction[IRP_MJ_CREATE] = Create;
	DriverObj->MajorFunction[IRP_MJ_CLOSE] = Close;
	DriverObj->MajorFunction[IRP_MJ_READ] = Read;
	DriverObj->MajorFunction[IRP_MJ_WRITE] = Write;
	DriverObj->MajorFunction[IRP_MJ_PNP] = Dispatch;
	DriverObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIOController;
	DriverObj->DriverUnload = Unload;

	stat = IoCreateDevice(DriverObj, sizeof(DEVICE_EXTENSION), 0, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);
	DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	if (stat == STATUS_SUCCESS)
		DbgPrint("DriverEntry() - IoCreateDevice() Success\n");
	else{
		DbgPrint("DriverEntry() - IoCreateDevice() Fail\n");
		return stat;
	}

	stat = IoAttachDevice(DeviceObject, &NextDeviceName, &DeviceExtension->NextLayerDeviceObject);
	if (stat == STATUS_SUCCESS)
		DbgPrint("DriverEntry() - IoAttachDevice() Success\n %08x", stat);
	else{
		DbgPrint("DriverEntry() - IoAttachDevice() Fail\n %08x", stat); 
		IoDeleteDevice(DriverObj->DeviceObject);
	}

	return stat;
}
