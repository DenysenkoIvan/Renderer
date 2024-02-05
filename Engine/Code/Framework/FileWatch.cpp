#include "FileWatch.h"

#include <Application/Application.h>
#include <Framework/Common.h>

FileWatch::~FileWatch()
{
    if (!m_changeHandle)
    {
        return;
    }

    FindCloseChangeNotification(m_changeHandle);
    m_changeHandle = 0;
}

void FileWatch::Start(const std::filesystem::path& folder, bool isWatchSubtree)
{
    Assert(!m_changeHandle);

    std::filesystem::path fullPath = Application::Get()->GetRootPath() / folder;

    m_changeHandle = FindFirstChangeNotificationW(fullPath.c_str(), isWatchSubtree, FILE_NOTIFY_CHANGE_LAST_WRITE);

    m_watchedFiles.clear();
    EnumerateFiles(fullPath, isWatchSubtree);
}

bool FileWatch::IsChanged()
{
    if (!m_changeHandle)
    {
        LogError("FileWatch variable not initialized");
        return false;
    }

    bool hasPotentialChanges = WAIT_OBJECT_0 == WaitForMultipleObjects(1, &m_changeHandle, TRUE, 0);
    FindNextChangeNotification(m_changeHandle);

    if (hasPotentialChanges)
    {
        size_t prevSize = m_changedFiles.size();
        CollectChangedFiles();
        return m_changedFiles.size() > prevSize;
    }

    return false;
}

std::vector<std::filesystem::path> FileWatch::RetrieveChangedFiles()
{
    return std::move(m_changedFiles);
}

void FileWatch::EnumerateFiles(const std::filesystem::path& fullPath, bool isRecursive)
{
    ProfileFunction();

    for (const std::filesystem::path& path : std::filesystem::directory_iterator(fullPath))
    {
        if (std::filesystem::is_directory(path) && isRecursive)
        {
            EnumerateFiles(path, isRecursive);
        }
        else
        {
            std::filesystem::file_time_type lastWriteTime = std::filesystem::last_write_time(path);
            m_watchedFiles.emplace_back(path, lastWriteTime);
        }
    }
}

void FileWatch::CollectChangedFiles()
{
    ProfileFunction();

    m_changedFiles.clear();

    for (WatchedFile& file : m_watchedFiles)
    {
        std::filesystem::file_time_type newLastWriteTime = std::filesystem::last_write_time(file.path);

        if (newLastWriteTime != file.lastWriteTime)
        {
            m_changedFiles.push_back(file.path);
            file.lastWriteTime = newLastWriteTime;
        }
    }
}