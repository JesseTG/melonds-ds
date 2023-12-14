/*
    Copyright 2023 Jesse Talavera-Greenberg

    melonDS DS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS DS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS DS. If not, see http://www.gnu.org/licenses/.
*/

#define SKIP_STDIO_REDEFINES

#include <cerrno>
#include <memory>
#include <system_error>
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include <file/file_path.h>
#include <Platform.h>
#include <streams/file_stream.h>
#include <streams/file_stream_transforms.h>
#include <retro_assert.h>
#include <compat/strl.h>
#include <vfs/vfs.h>
#include <string/stdstring.h>

#include "../config/config.hpp"
#include "environment.hpp"
#include "tracy.hpp"
#include "utils.hpp"

using namespace melonDS;
using namespace melonDS::Platform;
using std::unique_ptr;
using std::unordered_map;

constexpr unsigned GetRetroVfsFileAccessFlags(FileMode mode) noexcept {
    unsigned retro_mode = 0;
    if (mode & FileMode::Read)
        retro_mode |= RETRO_VFS_FILE_ACCESS_READ;

    if (mode & FileMode::Write)
        retro_mode |= RETRO_VFS_FILE_ACCESS_WRITE;

    if (mode & FileMode::Preserve)
        retro_mode |= RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING;

    return retro_mode;
}

unsigned GetRetroVfsFileAccessHints(const std::string& path) noexcept {
    // TODO: If this path matches an SD card path, NAND path, or firmware path,
    // then we should return RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS

    if (string_ends_with(path.c_str(), ".bin")) {
        // TODO: When I submit that PR with those new Platform calls,
        // use them instead of relying on a .bin file extension
        return RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS;
    }

    return RETRO_VFS_FILE_ACCESS_HINT_NONE;
}

constexpr unsigned GetRetroVfsFileSeekOrigin(FileSeekOrigin origin) noexcept {
    switch (origin) {
        case FileSeekOrigin::Start:
            return RETRO_VFS_SEEK_POSITION_START;
        case FileSeekOrigin::Current:
            return RETRO_VFS_SEEK_POSITION_CURRENT;
        case FileSeekOrigin::End:
            return RETRO_VFS_SEEK_POSITION_END;
        default:
            return 0;
    }
}

struct melonDS::Platform::FileHandle {
    RFILE *file;
    unsigned hints;
};

Platform::FileHandle *Platform::OpenFile(const std::string& path, FileMode mode) {
    ZoneScopedN(TracyFunction);
    if ((mode & FileMode::ReadWrite) == FileMode::None)
    { // If we aren't reading or writing, then we can't open the file
        retro::error("Attempted to open \"{}\" in neither read nor write mode (FileMode {:#x})\n", path, (unsigned)mode);
        return nullptr;
    }

    bool file_exists = path_is_valid(path.c_str());

    if (!file_exists && (mode & FileMode::NoCreate)) {
        // If the file doesn't exist, and we're not allowed to create it...
        retro::warn("Attempted to open \"{}\" in FileMode {:#x}, but the file doesn't exist and FileMode::NoCreate is set\n", path, (unsigned)mode);
        return nullptr;
    }

    Platform::FileHandle *handle = new Platform::FileHandle;
    handle->hints = GetRetroVfsFileAccessHints(path);
    handle->file = filestream_open(path.c_str(), GetRetroVfsFileAccessFlags(mode), handle->hints);

    if (!handle->file) {
        retro::error("Attempted to open \"{}\" in FileMode {:#x}, but failed", path, (unsigned)mode);
        delete handle;
        return nullptr;
    }

    retro::debug("Opened \"{}\" in FileMode {:#x}", path, (unsigned)mode);

    return handle;
}

Platform::FileHandle *Platform::OpenLocalFile(const std::string& path, FileMode mode) {
    ZoneScopedN(TracyFunction);
    if (path_is_absolute(path.c_str())) {
        return OpenFile(path, mode);
    }

    std::string sysdir = retro::get_system_directory().value_or("");
    char fullpath[PATH_MAX];
    size_t pathLength = fill_pathname_join_special(fullpath, sysdir.c_str(), path.c_str(), sizeof(fullpath));
    pathname_make_slashes_portable(fullpath);

    if (pathLength >= sizeof(fullpath)) {
        Log(LogLevel::Warn, "Path \"%s\" is too long to be joined with system directory \"%s\"", path.c_str(), sysdir.c_str());
    }

    return OpenFile(fullpath, mode);
}

bool Platform::FileExists(const std::string& name)
{
    return path_is_valid(name.c_str());
}

bool Platform::LocalFileExists(const std::string& name)
{
    ZoneScopedN(TracyFunction);
    if (name.empty()) {
        return false;
    }

    if (path_is_absolute(name.c_str())) {
        return path_is_valid(name.c_str());
    }

    std::string sysdir = retro::get_system_directory().value_or("");
    char fullpath[PATH_MAX];
    size_t pathLength = fill_pathname_join_special(fullpath, sysdir.c_str(), name.c_str(), sizeof(fullpath));
    pathname_make_slashes_portable(fullpath);

    if (pathLength >= sizeof(fullpath)) {
        Log(LogLevel::Warn, "Path \"%s\" is too long to be joined with system directory \"%s\"", name.c_str(), sysdir.c_str());
    }

    return path_is_valid(fullpath);
}

/// Close a file opened with \c OpenFile.
/// @returns \c true if the file was closed successfully, false otherwise.
bool Platform::CloseFile(FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file) {
        return false;
    }

    char path[PATH_MAX];
    strlcpy(path, filestream_get_path(file->file), sizeof(path));
    retro::debug("Closing \"{}\"", path);
    bool ok = (filestream_close(file->file) == 0);

    if (!ok) {
        retro::error("Failed to close \"{}\"", path);
    }
    delete file;

    return ok;
}

/// Returns true if there is no more data left to read in this file.
bool Platform::IsEndOfFile(FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file)
        return false;

    return filestream_eof(file->file) == EOF;
}

bool Platform::FileReadLine(char* str, int count, FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file || !str)
        return false;

    return filestream_gets(file->file, str, count);
}

bool Platform::FileSeek(FileHandle* file, s64 offset, FileSeekOrigin origin)
{
    ZoneScopedN(TracyFunction);
    if (!file)
        return false;

    return filestream_seek(file->file, offset, GetRetroVfsFileSeekOrigin(origin)) == 0;
}

void Platform::FileRewind(FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (file)
        filestream_rewind(file->file);
}

u64 Platform::FileRead(void* data, u64 size, u64 count, FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file || !data)
        return 0;

    int64_t bytesRead = filestream_read(file->file, data, size * count);
    if (bytesRead < 0) {
        retro::error("Failed to read from file \"{}\"", filestream_get_path(file->file));
    } else if (bytesRead != size * count) {
        retro::warn("Read {} bytes from file \"{}\", expected {}", bytesRead, filestream_get_path(file->file), size * count);
    }

    return bytesRead;
}

bool Platform::FileFlush(FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file)
        return false;

    return filestream_flush(file->file) == 0;
}

u64 Platform::FileWrite(const void* data, u64 size, u64 count, FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file || !data)
        return 0;

    u64 result = filestream_write(file->file, data, size * count);

    return result;
}

u64 Platform::FileWriteFormatted(FileHandle* file, const char* fmt, ...)
{
    ZoneScopedN(TracyFunction);
    if (!file || !fmt)
        return 0;

    va_list args;
    va_start(args, fmt);
    u64 ret = filestream_vprintf(file->file, fmt, args);
    va_end(args);
    return ret;
}

u64 Platform::FileLength(FileHandle* file)
{
    ZoneScopedN(TracyFunction);
    if (!file)
        return 0;

    int64_t size = filestream_get_size(file->file);
    if (filestream_error(file->file)) {
        retro::error("Failed to get size of file \"{}\"", filestream_get_path(file->file));
    }
    return size;
}