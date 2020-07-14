
#include "writer.h"
#include <Library/ShellLib.h>
#include <Guid/FileInfo.h>

EFI_STATUS
OpenVolume(
  OUT EFI_FILE_PROTOCOL** Volume
)
{
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fsProto = NULL;
  EFI_STATUS status;
  *Volume = NULL;

  status = gBS->LocateProtocol(
    &gEfiSimpleFileSystemProtocolGuid,
    NULL,
    (VOID**)&fsProto
  );

  if (EFI_ERROR(status))
  {
    return status;
  }

  status = fsProto->OpenVolume(
    fsProto,
    Volume
  );

  return status;
}

EFI_STATUS
OpenFile(
  IN  EFI_FILE_PROTOCOL* Volume,
  OUT EFI_FILE_PROTOCOL** File,
  IN  CHAR16* Path
)
{
  EFI_STATUS status;
  *File = NULL;

  //  from root file we open file specified by path
  status = Volume->Open(
    Volume,
    File,
    Path,
    EFI_FILE_MODE_CREATE |
    EFI_FILE_MODE_WRITE |
    EFI_FILE_MODE_READ,
    0
  );

  return status;
}

EFI_STATUS
CloseFile(
  IN EFI_FILE_PROTOCOL* File
)
{
  //  flush unwritten data
  File->Flush(File);
  //  close file
  File->Close(File);

  return EFI_SUCCESS;
}

EFI_STATUS
WriteDataToFile(
  IN VOID* Buffer,
  IN UINTN BufferSize,
  IN EFI_FILE_PROTOCOL* File
)
{
  UINTN infoBufferSize = 0;
  EFI_FILE_INFO* fileInfo = NULL;

  //  retrieve file info to know it size
  EFI_STATUS status = File->GetInfo(
    File,
    &gEfiFileInfoGuid,
    &infoBufferSize,
    (VOID*)fileInfo
  );

  if (EFI_BUFFER_TOO_SMALL != status)
  {
    return status;
  }

  fileInfo = AllocatePool(infoBufferSize);

  if (NULL == fileInfo)
  {
    status = EFI_OUT_OF_RESOURCES;
    return status;
  }

  //	we need to know file size
  status = File->GetInfo(
    File,
    &gEfiFileInfoGuid,
    &infoBufferSize,
    (VOID*)fileInfo
  );

  if (EFI_ERROR(status))
  {
    goto FINALLY;
  }

  //	we move carriage to the end of the file
  status = File->SetPosition(
    File,
    fileInfo->FileSize
  );

  if (EFI_ERROR(status))
  {
    goto FINALLY;
  }

  //	write buffer
  status = File->Write(
    File,
    &BufferSize,
    Buffer
  );

  if (EFI_ERROR(status))
  {
    goto FINALLY;
  }

  //	flush data
  status = File->Flush(File);

FINALLY:

  if (NULL != fileInfo)
  {
    FreePool(fileInfo);
  }

  return status;
}

EFI_STATUS
WriteToFile(
  VOID
)
{
  CHAR16 path[] = L"\\example.txt";
  EFI_FILE_PROTOCOL* file = NULL;
  EFI_FILE_PROTOCOL* volume = NULL;
  CHAR16 something[] = L"Hello from UEFI driver";

  //
  //  Open file
  //
  EFI_STATUS status = OpenVolume(&volume);

  if (EFI_ERROR(status))
  {
    return status;
  }

  status = OpenFile(volume, &file, path);

  if (EFI_ERROR(status))
  {
    CloseFile(volume);
    return status;
  }

  status = WriteDataToFile(something, sizeof(something), file);

  CloseFile(file);
  CloseFile(volume);

  return status;
}

EFI_STATUS
WriteToFile2(
  VOID
)
{
  SHELL_FILE_HANDLE fileHandle = NULL;
  CHAR16 path[] = L"fs1:\\example2.txt";
  CHAR16 something[] = L"Hello from UEFI driver";
  UINTN writeSize = sizeof(something);

  EFI_STATUS status = ShellInitialize();

  if (EFI_ERROR(status))
  {
    return status;
  }

  status = ShellOpenFileByName(path,
    &fileHandle,
    EFI_FILE_MODE_CREATE |
    EFI_FILE_MODE_WRITE |
    EFI_FILE_MODE_READ,
    0);

  if (EFI_ERROR(status))
  {
    return status;
  }

  status = ShellWriteFile(fileHandle, &writeSize, something);

  ShellCloseFile(&fileHandle);
  return status;
}