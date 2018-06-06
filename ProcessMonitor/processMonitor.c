#include <ntifs.h>
#include <wdm.h>
#include <ntddk.h>

const char dbg_name[] = "[Test]";

VOID Unload(IN PDRIVER_OBJECT DriverObejct);
void CreateProcessNotifyRoutine(IN HANDLE ppid, IN HANDLE pid, IN BOOLEAN create);
//void CreateProcessNotifyRoutineEx(IN OUT PEPROCESS eprocess, IN HANDLE pid, IN PPS_CREATE_NOTIFY_INFO CreateInfo OPTIONAL);

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS ret = STATUS_SUCCESS;
	PDEVICE_OBJECT DeviceObject = 0;

	//DbgSetDebugFilterState(DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, TRUE);
	DbgPrint("%s DriverEntry() Start\n", dbg_name);

	DriverObject->DriverUnload = Unload;

	ret = PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, 0);
	//ret = PsSetCreateProcessNotifyRoutineEx(CreateProcessNotifyRoutine, 0);
	if (ret == STATUS_SUCCESS)
	{
		DbgPrint("%s PsSetCreateProcessNotifyRoutine() Success\n", dbg_name);
	}
	else
	{
		DbgPrint("%s PsSetCreateProcessNotifyRoutine() Fail\n", dbg_name);
		return ret;
	}

	DbgPrint("%s DriverEntry() End\n", dbg_name);

	return ret;
}

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS ret = STATUS_SUCCESS;
	DbgPrint("%s Unload\n", dbg_name);

	ret = PsSetCreateProcessNotifyRoutine(CreateProcessNotifyRoutine, 1);
	if (ret == STATUS_SUCCESS)
	{
		DbgPrint("%s PsSetCreateProcessNotifyRoutine() Success\n", dbg_name);
	}
	else
	{
		DbgPrint("%s PsSetCreateProcessNotifyRoutine() Fail\n", dbg_name);
		return;
	}

	return;
}

void CreateProcessNotifyRoutine(IN HANDLE ppid, IN HANDLE pid, IN BOOLEAN create)
{
	char parent[256], child[256];
	LPTSTR name;
	PEPROCESS proc;
	PsLookupProcessByProcessId(pid, &proc);
	name = (LPTSTR)proc + 0x16c;

	DbgPrint("[%s]\t%s In CreateProcessNotifyRoutine\n", name);

	if (create == 1)
	{
		DbgPrint("[%s]\t %d -> %d Created",name, ppid, pid);
	}
	else
	{
		DbgPrint("[%s]\t %d Deleted\n", name, pid);
	}
	//ZwClose(ppid);
	//ZwClose(pid);

	return;
}
