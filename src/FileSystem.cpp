#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <direct.h>
#include <new>
#include <windows.h>
#elif __cplusplus >= 201703L
#include <filesystem>
#else // Assume POSIX
#include <sys/stat.h>
#endif

#include "FileSystem.hpp"
#include "pbg3/Pbg3Archive.hpp"
#include "utils.hpp"

u32 g_LastFileSize;

FILE *FileSystem::FopenUTF8(const char *filepath, const char *mode)
{
#ifndef _WIN32
    FILE *f = std::fopen(filepath, mode);
    if (f != NULL) return f;

    // Nintendo Switch / FAT32 alias resolving:
    // Handles files copied without Japanese locale (where 紅魔郷*.DAT -> *.DAT, 東方紅魔郷.cfg -> .cfg)
    const char *p = filepath;
    if (std::strstr(p, ".cfg") || std::strstr(p, ".CFG"))
    {
        if (std::strchr(mode, 'r'))
        {
            const char *c[] = {".cfg", "th06.cfg", "東方紅魔郷.cfg", NULL};
            for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
        }
        else
        {
            f = std::fopen(".cfg", mode);
            if (f) return f;
            return std::fopen("th06.cfg", mode);
        }
    }
    if (std::strstr(p, "CM.DAT") || std::strstr(p, "CM.dat") || std::strstr(p, "cm.dat"))
    {
        const char *c[] = {"CM.DAT", "cm.dat", "KOUMAKYO_CM.dat", "KOUMAKYO_CM.DAT", "紅魔郷CM.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    if (std::strstr(p, "ED.DAT") || std::strstr(p, "ED.dat") || std::strstr(p, "ed.dat"))
    {
        const char *c[] = {"ED.DAT", "ed.dat", "KOUMAKYO_ED.dat", "KOUMAKYO_ED.DAT", "紅魔郷ED.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    if (std::strstr(p, "IN.DAT") || std::strstr(p, "IN.dat") || std::strstr(p, "in.dat"))
    {
        const char *c[] = {"IN.DAT", "in.dat", "KOUMAKYO_IN.dat", "KOUMAKYO_IN.DAT", "紅魔郷IN.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    if (std::strstr(p, "MD.DAT") || std::strstr(p, "MD.dat") || std::strstr(p, "md.dat"))
    {
        const char *c[] = {"MD.DAT", "md.dat", "KOUMAKYO_MD.dat", "KOUMAKYO_MD.DAT", "紅魔郷MD.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    if (std::strstr(p, "ST.DAT") || std::strstr(p, "ST.dat") || std::strstr(p, "st.dat"))
    {
        const char *c[] = {"ST.DAT", "st.dat", "KOUMAKYO_ST.dat", "KOUMAKYO_ST.DAT", "紅魔郷ST.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    if (std::strstr(p, "TL.DAT") || std::strstr(p, "TL.dat") || std::strstr(p, "tl.dat"))
    {
        const char *c[] = {"TL.DAT", "tl.dat", "KOUMAKYO_TL.dat", "KOUMAKYO_TL.DAT", "紅魔郷TL.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    if (std::strstr(p, "score.dat") || std::strstr(p, "SCORE.DAT"))
    {
        const char *c[] = {"score.dat", "SCORE.DAT", NULL};
        for (int i = 0; c[i]; i++) { f = std::fopen(c[i], mode); if (f) return f; }
    }
    return NULL;
#else
    u32 filepathWLen = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, NULL, 0) * 2;
    u32 modeWLen = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0) * 2;

    if (filepathWLen == 0 || modeWLen == 0)
    {
        return NULL;
    }

    wchar_t *filepathW = new wchar_t[filepathWLen];
    wchar_t *modeW = new wchar_t[modeWLen];

    MultiByteToWideChar(CP_UTF8, 0, filepath, -1, filepathW, filepathWLen / 2);
    MultiByteToWideChar(CP_UTF8, 0, mode, -1, modeW, modeWLen / 2);

    FILE *f = _wfopen(filepathW, modeW);

    delete[] filepathW;
    delete[] modeW;

    return f;
#endif
}

void FileSystem::CreateDir(const char *path)
{
#ifdef _WIN32
    _mkdir(path);
#elif __cplusplus >= 201703L
    auto p = std::filesystem::path(path);
    std::filesystem::create_directory(p);
#else
    mkdir(path, 0755);
#endif
}

u8 *FileSystem::OpenPath(const char *filepath, int isExternalResource)
{
    u8 *data;
    FILE *file;
    size_t fsize;
    i32 entryIdx;
    const char *entryname;
    i32 pbg3Idx;

    entryIdx = -1;
    if (isExternalResource == 0)
    {
        entryname = std::strrchr(filepath, '\\');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        entryname = std::strrchr(entryname, '/');
        if (entryname == (char *)0x0)
        {
            entryname = filepath;
        }
        else
        {
            entryname = entryname + 1;
        }
        if (g_Pbg3Archives != NULL)
        {
            for (pbg3Idx = 0; pbg3Idx < 0x10; pbg3Idx += 1)
            {
                if (g_Pbg3Archives[pbg3Idx] != NULL)
                {
                    entryIdx = g_Pbg3Archives[pbg3Idx]->FindEntry(entryname);
                    if (entryIdx >= 0)
                    {
                        break;
                    }
                }
            }
        }
        if (entryIdx < 0)
        {
            return NULL;
        }
    }
    if (entryIdx >= 0)
    {
        utils::DebugPrint2("%s Decode ... \n", entryname);
        data = g_Pbg3Archives[pbg3Idx]->ReadDecompressEntry(entryIdx, entryname);
        g_LastFileSize = g_Pbg3Archives[pbg3Idx]->GetEntrySize(entryIdx);
    }
    else
    {
        utils::DebugPrint2("%s Load ... \n", filepath);
        file = FopenUTF8(filepath, "rb");
        if (file == NULL)
        {
            utils::DebugPrint2("error : %s is not found.\n", filepath);
            return NULL;
        }
        else
        {
            std::fseek(file, 0, SEEK_END);
            fsize = std::ftell(file);
            g_LastFileSize = fsize;
            std::fseek(file, 0, SEEK_SET);
            data = (u8 *)std::malloc(fsize);
            std::fread(data, 1, fsize, file);
            std::fclose(file);
        }
    }
    return data;
}

int FileSystem::WriteDataToFile(const char *path, const void *data, size_t size)
{
    FILE *f;

    f = FopenUTF8(path, "wb");
    if (f == NULL)
    {
        return -1;
    }
    else
    {
        if (std::fwrite(data, 1, size, f) != size)
        {
            std::fclose(f);
            return -2;
        }
        else
        {
            std::fclose(f);
            return 0;
        }
    }
}
