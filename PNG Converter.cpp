#include <windows.h>
#include <string>
#include <iostream>
#include <Magick++.h>
#include <codecvt>

// Link against necessary libraries, I choose to do it from here and not from project setting for the sake of simplicity
// but you still need to add the lib folder to your project setting under Linker > General > Additional Library Directories
 #pragma comment(lib, "CORE_RL_Magick++_.lib")
 #pragma comment(lib, "CORE_RL_MagickCore_.lib")
 #pragma comment(lib, "CORE_RL_MagickWand_.lib")

using namespace std;

// Function to convert wstring to string (UTF-8)
string WStringToString(const wstring& wstr) {
    if (wstr.empty()) return string();
    // Use wstring_convert with codecvt_utf8 for conversion
    wstring_convert<codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

// Function to get the executable path
wstring GetExecutablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return wstring(path);
}

// Function to add context menu entry
bool AddContextMenuEntry() {
    HKEY hKey;
    // This is why it needs Administrator privileges, because I use HKEY_LOCAL_MACHINE instead of HKEY_CURRENT_USER, you can change it if you want
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\*\\shell", 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        wcerr << L"Failed to open registry key for adding context menu." << endl;
        return false;
    }

    HKEY hSubKey;
    // Create a new key for our context menu entry
    result = RegCreateKeyExW(hKey, L"ConvertToPNG", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        wcerr << L"Failed to create registry subkey." << endl;
        return false;
    }

    // Set the default value (the menu text)
    wstring menuText = L"Convert to PNG";
    RegSetValueExW(hSubKey, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(menuText.c_str()),
        (menuText.size() + 1) * sizeof(wchar_t));

    // Set the Icon value
    wstring exePath = GetExecutablePath();
    wstring iconValue = L"\"" + exePath + L"\",0"; // Icon index 0
    RegSetValueExW(hSubKey, L"Icon", 0, REG_SZ, reinterpret_cast<const BYTE*>(iconValue.c_str()),
        (iconValue.size() + 1) * sizeof(wchar_t));

    // Create the "command" subkey
    HKEY hCommandKey;
    result = RegCreateKeyExW(hSubKey, L"command", 0, NULL, 0, KEY_WRITE, NULL, &hCommandKey, NULL);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hSubKey);
        RegCloseKey(hKey);
        wcerr << L"Failed to create command subkey." << endl;
        return false;
    }

    // Set the command to execute
    wstring command = L"\"" + exePath + L"\" \"" + L"%1" + L"\"";
    RegSetValueExW(hCommandKey, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()),
        (command.size() + 1) * sizeof(wchar_t));

    // Close all opened registry keys
    RegCloseKey(hCommandKey);
    RegCloseKey(hSubKey);
    RegCloseKey(hKey);

  //  wcout << L"Context menu entry added successfully with icon." << endl;
    return true;
}


// Function to remove context menu entry
bool RemoveContextMenuEntry() {
    HKEY hKey;
    // Use HKEY_LOCAL_MACHINE to match where we added the entry
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\*\\shell", 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        wcerr << L"Failed to open registry key for removing context menu." << endl;
        return false;
    }

    // Delete the "ConvertToPNG" subkey
    result = RegDeleteTreeW(hKey, L"ConvertToPNG");
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        wcerr << L"Failed to delete context menu entry." << endl;
        return false;
    }

    RegCloseKey(hKey);
 //   wcout << L"Context menu entry removed successfully." << endl;
    return true;
}

// Function to check if context menu entry exists
bool IsContextMenuEntryExists() {
    HKEY hKey;
    // Use HKEY_LOCAL_MACHINE to match where we added the entry
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\*\\shell\\ConvertToPNG", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// Function to check if a file is a valid image
bool IsValidImage(const string& imagePath) {
    try {
        Magick::Image image;
        image.ping(imagePath); // Ping reads image metadata without loading the entire image
        return true;
    }
    catch (Magick::Exception& error_) {
     //   wcerr << L"Invalid image file: ";
     //   WStringToString(wstring(imagePath.begin(), imagePath.end())), wcerr << error_.what() << endl;
        return false;
    }
}

// Function to convert image to PNG without deleting the original
bool ConvertImageToPNG(const wstring& imagePath) {
    try {
        // Initialize ImageMagick
        Magick::InitializeMagick(NULL);

        // Convert wstring to string (UTF-8)
        string imagePathStr = WStringToString(imagePath);

        // Check if the file is a valid image
        if (!IsValidImage(imagePathStr)) {
     //       wcerr << L"The file is not a valid image or is unsupported: " << imagePath << endl;
            return false;
        }

        // Read the image
        Magick::Image image;
        image.read(imagePathStr);

        // Define the new file path with .png extension
        wstring newPath = imagePath;
        size_t pos = newPath.find_last_of(L".");
        if (pos != wstring::npos) {
            newPath = newPath.substr(0, pos) + L".png";
        }
        else {
            newPath += L".png";
        }

        // Convert newPath to string
        string newPathStr = WStringToString(newPath);

        // Write the image as PNG
        image.write(newPathStr);

 //       wcout << L"Image converted to PNG successfully: " << newPath << endl;
        return true;
    }
    catch (Magick::Exception& error_) {
 //       wcerr << L"Error converting image: " << error_.what() << endl;
        return false;
    }
}

bool IsRunAsAdmin()
{
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = NULL;

    // Allocate and initialize a SID for the Administrators group.
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &ntAuthority,
        2, // Sub-authority count
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &administratorsGroup))
    {
      //  wcerr << L"AllocateAndInitializeSid Error: " << GetLastError() << endl;
        return false;
    }

    // Check whether the token of the current process is a member of the Administrators group.
    if (!CheckTokenMembership(NULL, administratorsGroup, &isAdmin))
    {
       // wcerr << L"CheckTokenMembership Error: " << GetLastError() << endl;
        isAdmin = FALSE;
    }

    // Free the SID once done.
    FreeSid(administratorsGroup);

    return isAdmin;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc == 1) {
        // No arguments: toggle context menu entry
        if (IsContextMenuEntryExists()) {
            // Remove it
            if (!IsRunAsAdmin()) {
                wcerr << L"Please run the program as an administrator in order to remove context menu entry." << endl;
                system("pause");
                return 1;
            }
            if (!RemoveContextMenuEntry()) {
                return 1;
            }
            cout << "The program has been removed from the context menu." << endl;
            system("pause");
        }
        else {
            // Add it
            cout << "It's your first time running the program." << endl;
            if (!IsRunAsAdmin()) {
                wcerr << L"Please run the program as an administrator in order to add context menu entry." << endl;
                system("pause");
				return 1;
            }
            if (!AddContextMenuEntry()) {
                return 1;
            }
            cout << "The program has been added to the context menu and is ready to use." << endl;
            cout << "In order to remove the program from the context menu, please run the program again as adminstatrator." << endl;
            system("pause");
        }
    }
    else if (argc == 2) {
        // One argument: assume it's the image path
        wstring imagePath = argv[1];
        // Check if the file exists
        if (GetFileAttributesW(imagePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
         //   wcerr << L"File does not exist: " << imagePath << endl;
            return 1;
        }

        // Convert the image
        if (!ConvertImageToPNG(imagePath)) {
            return 1;
        }
    }
    else {
     //   wcerr << L"Invalid number of arguments." << endl;
        return 1;
    }

    return 0;
}
