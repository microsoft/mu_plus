#include "main.h"
#include <Windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <fstream>

using namespace winrt;
using namespace Windows::Foundation;
using namespace std;

//
// Elevate current process system environment privileges to access UEFI variables
//
static int
ElevateCurrentPrivileges (
  )
{
  HANDLE            ProcessHandle = GetCurrentProcess ();
  DWORD             DesiredAccess = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
  HANDLE            hProcessToken;
  TOKEN_PRIVILEGES  tp;
  LUID              luid;
  int               Status = 0;

  if (!LookupPrivilegeValue (NULL, L"SeSystemEnvironmentPrivilege", &luid)) {
    Status = GetLastError ();
    cout << "Failed to lookup privilege value. Errno " << Status << endl;
    return Status;
  }

  if (!OpenProcessToken (ProcessHandle, DesiredAccess, &hProcessToken)) {
    Status = GetLastError ();
    cout << "Failed to open process token. Errno " << Status << endl;
    return Status;
  }

  tp.PrivilegeCount           = 1;
  tp.Privileges[0].Luid       = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  if (!AdjustTokenPrivileges (hProcessToken, FALSE, &tp, sizeof (TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
    Status = GetLastError ();
    cout << "Failed to adjust token privileges. Errno " << Status << endl;
    return Status;
  }

  if (GetLastError () == ERROR_NOT_ALL_ASSIGNED) {
    Status = (int)ERROR_NOT_ALL_ASSIGNED;
    cout << "The token does not have the specified privilege. Errno " << Status << endl;
    return Status;
  }

  CloseHandle (ProcessHandle);
  return SUCCESS;
}

//
// Create log file by retrieving AdvancedLogger variables from UEFI interface
//
int
ReadLogFromUefiInterface (
  fstream  &lfile
  )
{
  int    Status = 0;
  int    i      = 0;
  DWORD  length = 0;
  DWORD  err    = 0;

  stringstream  varName;
  char          *varBuffer = (char *)malloc (MAX_VAR_LENGTH + 1);

  // string to LPCWSTR conversion
  string   tmpGuid  = "{a021bf2b-34ed-4a98-859c-420ef94f3e94}";
  wstring  tmpGuidW = wstring (tmpGuid.begin (), tmpGuid.end ());
  LPCWSTR  guid     = tmpGuidW.c_str ();

  //
  // Parse variables by index until reached end of log
  //
  while (Status == 0) {
    string   tmpVarName   = "V" + to_string (i);
    wstring  tmpVarNameW  = wstring (tmpVarName.begin (), tmpVarName.end ());
    LPCWSTR  varNameConst = tmpVarNameW.c_str ();

    // Retrieve one advanced logger indexed variable via kernel32 API
    length = GetFirmwareEnvironmentVariableW (varNameConst, guid, varBuffer, MAX_VAR_LENGTH);

    if (length == 0) {
      err = GetLastError ();

      // If error is ERROR_NOT_FOUND (203), reached end of variables
      if (err != 203) {
        Status = EFI_ERROR;
        cout << "Error reading variable " << tmpVarName << " errno: " << err << endl;
        return Status;
      } else {
        Status = (int)err;
      }
    }

    if (Status == 0) {
      i += 1;
      streamsize  varSize = (streamsize)length;
      lfile.write (varBuffer, varSize);
      if (lfile.fail ()) {
        cout << "Failed to write to file\n";
        Status = CONS_ERROR;
        return Status;
      }
    } else if (i == 0) {
      cout << "No variables found.\n";
      return Status;
    } else {
      cout << i << " variables read. " << lfile.tellg () << " chars written.\n";
    }

    if (varBuffer) {
      ZeroMemory (varBuffer, MAX_VAR_LENGTH);
    }
  }

  free (varBuffer);
  return SUCCESS;
}

int
main (
  )
{
  fstream     logfile;
  const char  *newRawFilename = ".\\new_logfile.bin";
  int         Status          = 0;

  Status = ElevateCurrentPrivileges ();
  if (Status != 0) {
    cout << "Failed to elevate privileges, errno:" << Status << endl;
    return Status;
  }

  // Create new binary logfile
  logfile.open (newRawFilename, ios::out | ios::binary);
  if (!logfile) {
    cout << "Error opening file.\n";
    Status = FILE_ERROR;
    return Status;
  }

  Status = ReadLogFromUefiInterface (logfile);
  if (Status != SUCCESS) {
    cerr << "Error reading log, exiting.\n";
    return LOG_ERROR;
  }

  logfile.close ();
  if (logfile.fail ()) {
    cout << "Error closing file.\n";
    return FILE_ERROR;
  }

  return SUCCESS;
}
