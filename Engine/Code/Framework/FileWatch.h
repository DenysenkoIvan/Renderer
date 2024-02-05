#pragma once

#include <filesystem>

#include <windows.h>

class FileWatch
{
public:
    FileWatch() = default;
    ~FileWatch();

    void Start(const std::filesystem::path& folder, bool isWatchSubtree);

    bool IsChanged();
    std::vector<std::filesystem::path> RetrieveChangedFiles();

private:
    void EnumerateFiles(const std::filesystem::path& fullPath, bool isWatchSubtree);
    void CollectChangedFiles();

private:
    struct WatchedFile
    {
        std::filesystem::path path;
        std::filesystem::file_time_type lastWriteTime;

        WatchedFile(const std::filesystem::path& path, std::filesystem::file_time_type lastWriteTime)
            : path(path), lastWriteTime(lastWriteTime) {}
    };

private:
    HANDLE m_changeHandle{};
    std::vector<WatchedFile> m_watchedFiles;
    std::vector<std::filesystem::path> m_changedFiles;
};