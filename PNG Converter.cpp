#include <windows.h>
#include <string>
#include <shlobj.h>
#include <shellapi.h>
#include <iostream>
#include <Magick++.h>
#include <locale>
#include <codecvt>

// Link against necessary libraries
#pragma comment(lib, "Shell32.lib")
//#pragma comment(lib, "CORE_RL_Magick++.lib")
#pragma comment(lib, "CORE_RL_MagickCore_.lib")
#pragma comment(lib, "CORE_RL_MagickWand_.lib")


// Function to convert std::wstring to std::string (UTF-8)
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    // Use std::wstring_convert with std::codecvt_utf8 for conversion
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(wstr);
}

// Function to get the executable path
std::wstring GetExecutablePath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return std::wstring(path);
}

// Function to add context menu entry
bool AddContextMenuEntry() {
    HKEY hKey;
    // Use HKEY_CURRENT_USER to avoid requiring admin privileges
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\*\\shell", 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Failed to open registry key for adding context menu." << std::endl;
        return false;
    }

    HKEY hSubKey;
    // Create a new key for our context menu entry
    result = RegCreateKeyExW(hKey, L"ConvertToPNG", 0, NULL, 0, KEY_WRITE, NULL, &hSubKey, NULL);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcerr << L"Failed to create registry subkey." << std::endl;
        return false;
    }

    // Set the default value (the menu text)
    std::wstring menuText = L"Convert to PNG";
    RegSetValueExW(hSubKey, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(menuText.c_str()),
        (menuText.size() + 1) * sizeof(wchar_t));

    // Set the Icon value
    std::wstring exePath = GetExecutablePath();
    std::wstring iconValue = L"\"" + exePath + L"\",0"; // Icon index 0
    RegSetValueExW(hSubKey, L"Icon", 0, REG_SZ, reinterpret_cast<const BYTE*>(iconValue.c_str()),
        (iconValue.size() + 1) * sizeof(wchar_t));

    // Create the "command" subkey
    HKEY hCommandKey;
    result = RegCreateKeyExW(hSubKey, L"command", 0, NULL, 0, KEY_WRITE, NULL, &hCommandKey, NULL);
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hSubKey);
        RegCloseKey(hKey);
        std::wcerr << L"Failed to create command subkey." << std::endl;
        return false;
    }

    // Set the command to execute
    std::wstring command = L"\"" + exePath + L"\" \"" + L"%1" + L"\"";
    RegSetValueExW(hCommandKey, NULL, 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()),
        (command.size() + 1) * sizeof(wchar_t));

    // Close all opened registry keys
    RegCloseKey(hCommandKey);
    RegCloseKey(hSubKey);
    RegCloseKey(hKey);

  //  std::wcout << L"Context menu entry added successfully with icon." << std::endl;
    return true;
}


// Function to remove context menu entry
bool RemoveContextMenuEntry() {
    HKEY hKey;
    // Use HKEY_LOCAL_MACHINE to match where we added the entry
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\*\\shell", 0, KEY_WRITE, &hKey);
    if (result != ERROR_SUCCESS) {
        std::wcerr << L"Failed to open registry key for removing context menu." << std::endl;
        return false;
    }

    // Delete the "ConvertToPNG" subkey
    result = RegDeleteTreeW(hKey, L"ConvertToPNG");
    if (result != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        std::wcerr << L"Failed to delete context menu entry." << std::endl;
        return false;
    }

    RegCloseKey(hKey);
 //   std::wcout << L"Context menu entry removed successfully." << std::endl;
    return true;
}

// Function to check if context menu entry exists
bool IsContextMenuEntryExists() {
    HKEY hKey;
    // Use HKEY_LOCAL_MACHINE to match where we added the entry
    // This is why it needs Administrator privileges, because I use HKEY_LOCAL_MACHINE instead of HKEY_CURRENT_USER, you can change it if you want
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\*\\shell\\ConvertToPNG", 0, KEY_READ, &hKey);
    if (result == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

// Function to check if a file is a valid image
bool IsValidImage(const std::string& imagePath) {
    try {
        Magick::Image image;
        image.ping(imagePath); // Ping reads image metadata without loading the entire image
        return true;
    }
    catch (Magick::Exception& error_) {
     //   std::wcerr << L"Invalid image file: ";
     //   WStringToString(std::wstring(imagePath.begin(), imagePath.end())), std::wcerr << error_.what() << std::endl;
        return false;
    }
}

// Function to convert image to PNG without deleting the original
bool ConvertImageToPNG(const std::wstring& imagePath) {
    try {
        // Initialize ImageMagick
        Magick::InitializeMagick(NULL);

        // Convert wstring to string (UTF-8)
        std::string imagePathStr = WStringToString(imagePath);

        // Check if the file is a valid image
        if (!IsValidImage(imagePathStr)) {
     //       std::wcerr << L"The file is not a valid image or is unsupported: " << imagePath << std::endl;
            return false;
        }

        // Read the image
        Magick::Image image;
        image.read(imagePathStr);

        // Define the new file path with .png extension
        std::wstring newPath = imagePath;
        size_t pos = newPath.find_last_of(L".");
        if (pos != std::wstring::npos) {
            newPath = newPath.substr(0, pos) + L".png";
        }
        else {
            newPath += L".png";
        }

        // Convert newPath to string
        std::string newPathStr = WStringToString(newPath);

        // Write the image as PNG
        image.write(newPathStr);

 //       std::wcout << L"Image converted to PNG successfully: " << newPath << std::endl;
        return true;
    }
    catch (Magick::Exception& error_) {
 //       std::wcerr << L"Error converting image: " << error_.what() << std::endl;
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
      //  std::wcerr << L"AllocateAndInitializeSid Error: " << GetLastError() << std::endl;
        return false;
    }

    // Check whether the token of the current process is a member of the Administrators group.
    if (!CheckTokenMembership(NULL, administratorsGroup, &isAdmin))
    {
       // std::wcerr << L"CheckTokenMembership Error: " << GetLastError() << std::endl;
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
                std::wcerr << L"Please run the program as an administrator in order to remove context menu entry." << std::endl;
                system("pause");
                return 1;
            }
            if (!RemoveContextMenuEntry()) {
                return 1;
            }
            std::cout << "The program has been removed from the context menu." << std::endl;
            system("pause");
        }
        else {
            // Add it
            std::cout << "It's your first time running the program." << std::endl;
            if (!IsRunAsAdmin()) {
                std::wcerr << L"Please run the program as an administrator in order to add context menu entry." << std::endl;
                system("pause");
				return 1;
            }
            if (!AddContextMenuEntry()) {
                return 1;
            }
            std::cout << "The program has been added to the context menu and is ready to use." << std::endl;
            std::cout << "In order to remove the program from the context menu, please run the program again as adminstatrator." << std::endl;
            system("pause");
        }
    }
    else if (argc == 2) {
        // One argument: assume it's the image path
        std::wstring imagePath = argv[1];
        // Check if the file exists
        if (GetFileAttributesW(imagePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
         //   std::wcerr << L"File does not exist: " << imagePath << std::endl;
            return 1;
        }

        // Convert the image
        if (!ConvertImageToPNG(imagePath)) {
            return 1;
        }
    }
    else {
     //   std::wcerr << L"Invalid number of arguments." << std::endl;
        return 1;
    }

    return 0;
}
