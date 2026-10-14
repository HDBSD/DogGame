/* Ordinary per-user save files; retain only the metadata native menus need. */
#include "title_saves.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>
extern BOOL (*g_xbox_host_path_hook)(const char *, wchar_t *, DWORD);
/* Retain the established storage directory so the DogGame rename keeps existing saves. */
static wchar_t s_save_directory[MAX_PATH], s_metadata_directory[MAX_PATH];
static const char *s_guest_slot_names[] = {"02B312278FA2", "02B312278FA3", "02B312278FA4"};

static BOOL save_path(const char *guest, wchar_t *out, DWORD count)
{
    const char *prefix = "\\Device\\Harddisk0\\Partition1\\UDATA\\5451001f";
    size_t n = strlen(prefix);
    if (_strnicmp(guest, prefix, n) || (guest[n] && guest[n] != '\\'))
    {
        return FALSE;
    }
    const char *relative_path = guest + n;
    while (*relative_path == '\\')
    {
        relative_path++;
    }
    for (unsigned i = 0; i < 3; i++)
    {
        char file[80];
        snprintf(file, sizeof file, "%s\\GameData.xsv", s_guest_slot_names[i]);
        if (!_stricmp(relative_path, file))
        {
            return swprintf_s(out, count, L"%s\\save%u.sav", s_save_directory, i + 1) >= 0;
        }
    }
    wchar_t suffix[MAX_PATH];
    if (!MultiByteToWideChar(CP_ACP, 0, relative_path, -1, suffix, MAX_PATH))
    {
        return FALSE;
    }
    /* Only this title's known metadata tree is redirected. */
    if (strstr(relative_path, "..") || strchr(relative_path, ':') || strchr(relative_path, '/'))
    {
        return FALSE;
    }
    return swprintf_s(out, count, L"%s\\%s", s_metadata_directory, suffix) >= 0;
}

static int ensure_save_directory(const wchar_t *path)
{
    wchar_t absolute[MAX_PATH];
    DWORD n = GetFullPathNameW(path, MAX_PATH, absolute, NULL);
    if (!n || n >= MAX_PATH)
    {
        return 0;
    }
    int status = SHCreateDirectoryExW(NULL, absolute, NULL);
    return status == ERROR_SUCCESS || status == ERROR_ALREADY_EXISTS || status == ERROR_FILE_EXISTS;
}

static int copy_legacy_save(const wchar_t *from, const wchar_t *to)
{
    if (GetFileAttributesW(to) != INVALID_FILE_ATTRIBUTES)
    {
        return 1;
    }
    if (CopyFileW(from, to, TRUE))
    {
        return 1;
    }
    DWORD error = GetLastError();
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_FILE_EXISTS)
    {
        return 1;
    }
    fprintf(stderr, "[SAVE] Migration failed: %lu\n", error);
    return 0;
}

int title_saves_init(const char *game, char *storage, unsigned size)
{
    wchar_t appdata[MAX_PATH], local[MAX_PATH], game_directory_wide[MAX_PATH], legacy_path[MAX_PATH],
        destination_path[MAX_PATH], marker[MAX_PATH], legacy_marker[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata)) ||
        FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, local)))
    {
        return 0;
    }
    if (swprintf_s(s_save_directory, MAX_PATH, L"%s\\sd100f\\saves", appdata) < 0 ||
        swprintf_s(s_metadata_directory, MAX_PATH, L"%s\\.metadata\\UDATA\\5451001f", s_save_directory) < 0)
    {
        return 0;
    }
    if (!ensure_save_directory(s_metadata_directory))
    {
        return 0;
    }
    swprintf_s(destination_path, MAX_PATH, L"%s\\.metadata", s_save_directory);
    SetFileAttributesW(destination_path, FILE_ATTRIBUTE_HIDDEN);
    if (!MultiByteToWideChar(CP_UTF8, 0, game, -1, game_directory_wide, MAX_PATH))
    {
        return 0;
    }
    /* Deleting the saves folder must not reset legacy-import state. Honor
     * the earlier marker when upgrading, then persist it outside that folder. */
    if (swprintf_s(destination_path, MAX_PATH, L"%s\\sd100f\\storage", local) < 0 ||
        !ensure_save_directory(destination_path))
    {
        return 0;
    }
    if (swprintf_s(marker, MAX_PATH, L"%s\\save-migration-v1.done", destination_path) < 0 ||
        swprintf_s(legacy_marker, MAX_PATH, L"%s\\migration-v1.done", s_metadata_directory) < 0)
    {
        return 0;
    }
    int migrated = GetFileAttributesW(marker) != INVALID_FILE_ATTRIBUTES ||
                   GetFileAttributesW(legacy_marker) != INVALID_FILE_ATTRIBUTES;
    for (unsigned i = 0; i < 3; i++)
    {
        swprintf_s(destination_path, MAX_PATH, L"%s\\%S", s_metadata_directory, s_guest_slot_names[i]);
        if (!ensure_save_directory(destination_path))
        {
            return 0;
        }
        if (migrated)
        {
            continue;
        }
        swprintf_s(legacy_path, MAX_PATH, L"%s\\UDATA\\5451001f\\%S\\GameData.xsv", game_directory_wide,
                   s_guest_slot_names[i]);
        swprintf_s(destination_path, MAX_PATH, L"%s\\save%u.sav", s_save_directory, i + 1);
        if (!copy_legacy_save(legacy_path, destination_path))
        {
            return 0;
        }
        swprintf_s(legacy_path, MAX_PATH, L"%s\\UDATA\\5451001f\\%S\\SaveMeta.xbx", game_directory_wide,
                   s_guest_slot_names[i]);
        swprintf_s(destination_path, MAX_PATH, L"%s\\%S\\SaveMeta.xbx", s_metadata_directory,
                   s_guest_slot_names[i]);
        if (!copy_legacy_save(legacy_path, destination_path))
        {
            return 0;
        }
    }
    const wchar_t *metadata_files[] = {L"SaveImage.xbx", L"TitleImage.xbx", L"TitleMeta.xbx"};
    for (unsigned i = 0; !migrated && i < 3; i++)
    {
        swprintf_s(legacy_path, MAX_PATH, L"%s\\UDATA\\5451001f\\%s", game_directory_wide, metadata_files[i]);
        swprintf_s(destination_path, MAX_PATH, L"%s\\%s", s_metadata_directory, metadata_files[i]);
        if (!copy_legacy_save(legacy_path, destination_path))
        {
            return 0;
        }
    }
    if (GetFileAttributesW(marker) == INVALID_FILE_ATTRIBUTES)
    {
        HANDLE file = CreateFileW(marker, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS)
        {
            return 0;
        }
        if (file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(file);
        }
    }
    swprintf_s(destination_path, MAX_PATH, L"%s\\sd100f\\storage", local);
    if (!WideCharToMultiByte(CP_UTF8, 0, destination_path, -1, storage, size, NULL, NULL))
    {
        return 0;
    }
    g_xbox_host_path_hook = save_path;
    fprintf(stderr, "[SAVE] Slots: %ls\\save1.sav through save3.sav\n", s_save_directory);
    return 1;
}
