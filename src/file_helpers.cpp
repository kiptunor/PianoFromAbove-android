#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <sys/stat.h>

#include "config/config.h"
#include "file_helpers.h"
#include "logger.h"
#include "mb_types.h"














std::string FileHelpers::config_dir;
std::string FileHelpers::lists_dir;

std::string human_readable_size(u64 bytes)
{
    const char *units[]    = { "B", "KB", "MB", "GB", "TB", "PB" };
    int         unit_index = 0;
    f64         size       = static_cast<f64>(bytes);

    while(size >= 1024 && unit_index < 5)
    {
        size /= 1024;
        ++unit_index;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return oss.str();
}

std::vector<std::string> FileHelpers::GetFilesByExtension(const std::string &path, const std::string &extensions)
{
    // I'm so fucking lazy to do this shit
    std::vector<std::string> result;

    // Parse extensions
    std::vector<std::string> exts;
    std::string              ext;

    // Get specified extensions separated by '|'
    for(char c : extensions)
    {
        if(c == '|')
        {
            if(!ext.empty())
                exts.push_back(ext);

            ext.clear();
        }
        else
            ext += c;
    }
    if(!ext.empty())
        exts.push_back(ext);

    // Convert to lowercase
    for(auto &e : exts)
        std::transform(e.begin(), e.end(), e.begin(), ::tolower);

    // Scan directory
    for(const auto &entry : std::filesystem::directory_iterator(path))
    {
        if(entry.is_regular_file())
        {
            std::string fileExt = entry.path().extension().string();
            std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::tolower);

            for(const auto &e : exts)
            {
                if(fileExt == e)
                {
                    result.push_back(entry.path().string());
                    break;
                }
            }
        }
    }

    return result;
}

FileHelpers::FileInfo FileHelpers::GetFileInfo(const std::string &path)
{
    FileHelpers::FileInfo res;

    struct stat           f_stat;
    std::time_t           t;
    std::tm              *tm_ptr;

    // Get basic file information and populate the the struct members
    if(stat(path.c_str(), &f_stat) == 0)
    {
        t             = f_stat.st_mtime;
        tm_ptr        = std::localtime(&t);
        res.file_name = path;

        std::ostringstream temp;
        temp << std::put_time(tm_ptr, "D: %Y-%m-%d | T: %H:%M:%S"); // Create date & time string
        res.last_mod = temp.str();
        res.size     = human_readable_size(f_stat.st_size);
        res.success  = true;
    }
    else
    {
        res.success = false;
        res.err << "File does not exist !!\n";
        Log::error("File to get file information: %s", res.err.str().c_str());
    }

    return res;
}

static u32 ReadVLQ(const u8 *data, size_t maxSize, size_t &pos)
{
    u32 value = 0;
    u32 shift = 0;
    while(pos < maxSize)
    {
        u8 byte = data[pos++];
        value |= (byte & 0x7F) << shift;
        shift += 7;
        if(!(byte & 0x80))
            break;
    }
    return value;
}

FileHelpers::MidiParseInfo FileHelpers::ParseMidiFile(const std::string &path)
{
    MidiParseInfo info;
    FILE *fp = std::fopen(path.c_str(), "rb");
    if(!fp) return info;

    u8 header[14];
    if(std::fread(header, 1, 14, fp) != 14) { std::fclose(fp); return info; }
    if(std::memcmp(header, "MThd", 4) != 0) { std::fclose(fp); return info; }

    info.ppqn = (header[12] << 8) | header[13];
    if(info.ppqn & 0x8000) { std::fclose(fp); info.ppqn = 0; return info; }

    while(true)
    {
        u8 chunk[8];
        if(std::fread(chunk, 1, 8, fp) != 8) break;

        if(std::memcmp(chunk, "MTrk", 4) != 0)
        {
            u32 skip = (chunk[4] << 24) | (chunk[5] << 16) | (chunk[6] << 8) | chunk[7];
            std::fseek(fp, skip, SEEK_CUR);
            continue;
        }

        u32 trackLen = (chunk[4] << 24) | (chunk[5] << 16) | (chunk[6] << 8) | chunk[7];
        if(trackLen == 0) break;

        std::vector<u8> td(trackLen);
        if(std::fread(td.data(), 1, trackLen, fp) != trackLen) break;

        size_t pos = 0;
        u8 lastStatus = 0;
        while(pos < td.size())
        {
            ReadVLQ(td.data(), td.size(), pos);
            if(pos >= td.size()) break;

            u8 ev = td[pos++];
            if(ev == 0xFF)
            {
                if(pos >= td.size()) break;
                u8 metaType = td[pos++];
                size_t dataStart = pos;
                u32 dataLen = ReadVLQ(td.data(), td.size(), pos);
                if(pos + dataLen > td.size()) break;

                if(metaType == 0x51 && dataLen >= 3)
                {
                    u32 usPerQ = (td[pos] << 16) | (td[pos+1] << 8) | td[pos+2];
                    if(usPerQ > 0)
                        info.bpm = 60000000.0 / usPerQ;
                    if(info.timeSigNum > 0 && info.bpm > 0.0) break;
                }
                else if(metaType == 0x58 && dataLen >= 4)
                {
                    info.timeSigNum = td[pos];
                    int denomPow = td[pos+1];
                    info.timeSigDen = 1 << denomPow;
                    if(info.timeSigNum > 0 && info.bpm > 0.0) break;
                }
                pos += dataLen;
            }
            else if(ev == 0xF0 || ev == 0xF7)
            {
                u32 dataLen = ReadVLQ(td.data(), td.size(), pos);
                pos += dataLen;
            }
            else if(ev >= 0x80 && ev <= 0xEF)
            {
                lastStatus = ev;
                int statusNib = ev & 0xF0;
                if(statusNib == 0xC0 || statusNib == 0xD0)
                    { if(pos < td.size()) pos++; }
                else
                    { if(pos + 1 < td.size()) pos += 2; }
            }
            else
            {
                int statusNib = lastStatus & 0xF0;
                if(statusNib == 0xC0 || statusNib == 0xD0) {}
                else { if(pos < td.size()) pos++; }
            }
        }
        break;
    }

    std::fclose(fp);
    info.success = (info.ppqn > 0);
    return info;
}

#ifndef PLATFORM_ANDROID
void FileHelpers::createConfigDirs()
{
    std::ostringstream config_dir_path;
    std::ostringstream lists_dir_path;
    config_dir_path << std::getenv("HOME") << CONFIG_DIR;
    lists_dir_path << std::getenv("HOME") << CONFIG_LISTS;

    config_dir = config_dir_path.str();
    lists_dir  = lists_dir_path.str();

    if(!std::filesystem::exists(config_dir_path.str()))
        if(!std::filesystem::create_directory(config_dir_path.str()))
            Log::error("Failed to create config directory: %s", config_dir_path.str().c_str());

    if(!std::filesystem::exists(lists_dir_path.str()))
        if(!std::filesystem::create_directory(lists_dir_path.str()))
            Log::error("Failed to create lists directory: %s", lists_dir_path.str().c_str());
}
#endif
