#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

PFLT_FILTER MiniFilterHandle = NULL;
FLT_PREOP_CALLBACK_STATUS MiniPreCreate(
	PFLT_CALLBACK_DATA Data, 
	PCFLT_RELATED_OBJECTS FltObjects, 
	PVOID *CompletionContex);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(
	PFLT_CALLBACK_DATA Data, 
	PCFLT_RELATED_OBJECTS FltObjects, 
	PVOID CompletionContext, 
	FLT_POST_OPERATION_FLAGS Flags);

NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags);

BOOLEAN equalExtension(
	In const PUNICODE_STRING full,
	In const PUNICODE_STRING tail,
	In BOOLEAN cases)
{
	ULONG i;
	USHORT full_count;
	USHORT tail_count;

	if (full == NULL || tail == NULL) return FALSE;

	full_count = full->Length / sizeof(WCHAR);
	tail_count = tail->Length / sizeof(WCHAR);

	if (full_count < tail_count) return FALSE;
	if (tail_count == 0) return FALSE;

	if (cases)
	{
		for (i = 1; i <= tail_count; ++i)
		{
			if (RtlUpcaseUnicodeChar(full->Buffer[full_count - i]) !=
				RtlUpcaseUnicodeChar(tail->Buffer[tail_count - i]))
				return FALSE;
		}
	}
	else
	{
		for (i = 1; i <= tail_count; ++i)
		{
			if (full->Buffer[full_count - i] != tail->Buffer[tail_count - i])
				return FALSE;
		}
	}

	return TRUE;

}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID *CompletionContex)
{
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(
	PFLT_CALLBACK_DATA Data,
	PCFLT_RELATED_OBJECTS FltObjects,
	PVOID CompletionContext,
	FLT_POST_OPERATION_FLAGS Flags)
{
	PFLT_FILE_NAME_INFORMATION FileNameInfor;
	UNICODE_STRING fileExtension;
	NTSTATUS stat;
	POBJECT_NAME_INFORMATION ObjectNameInformation = NULL;

	switch (Data->Iopb->MajorFunction)
	{
	case IRP_MJ_CREATE:
		if (Data->IoStatus.Information == FILE_CREATED)
		{
			DbgBreakPoint();
			stat = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfor);
			if (NT_SUCCESS(stat))
			{
				stat = FltParseFileNameInformation(FileNameInfor);
				if (NT_SUCCESS(stat)){
					RtlInitUnicodeString(&fileExtension, L"txt");
					if (TRUE == equalExtension(&FileNameInfor->Name, &fileExtension, TRUE))
					{
						stat = FltParseFileNameInformation(FileNameInfor);
						IoQueryFileDosDeviceName(FltObjects->FileObject, &ObjectNameInformation);

						KdPrint(("Create File : %wZ \n", &ObjectNameInformation->Name));
						KdPrint(("\n"));
					}
				}
				FltReleaseFileNameInformation(FileNameInfor);
			}
		}
	}	
	return FLT_POSTOP_FINISHED_PROCESSING;
}

NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags) {
	DbgPrint("Driver Unloaded !! \n");
	FltUnregisterFilter(MiniFilterHandle);

	return STATUS_SUCCESS;
}

NTSTATUS Unload(PDRIVER_OBJECT pDriverObject) {
	DbgPrint("Driver Unloaded !! \n");
	return STATUS_SUCCESS;
}

const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_CREATE, 0, MiniPreCreate, MiniPostCreate },
	{ IRP_MJ_OPERATION_END }
};

const FLT_REGISTRATION FilterRegistration = { sizeof(FLT_REGISTRATION),
FLT_REGISTRATION_VERSION,
0, NULL,
Callbacks,
MiniUnload,
NULL, NULL, NULL, NULL,
NULL, NULL, NULL, NULL
};

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	NTSTATUS status;

	DriverObject->DriverUnload = Unload;

	status = FltRegisterFilter(DriverObject, &FilterRegistration, &MiniFilterHandle);
	if (NT_SUCCESS(status)) {
		status = FltStartFiltering(MiniFilterHandle);
		if (!NT_SUCCESS(status)) {
			DbgPrint(" MiniFilter canot start !!\n");
			FltUnregisterFilter(MiniFilterHandle);
		}
	}
	return status;
}
