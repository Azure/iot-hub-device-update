
/**
 * @file source_update_cache_utils.cpp
 * @brief utils for source_update_cache
 *
 * @copyright Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License.
 */

#include "aduc/source_update_cache_utils.h"
#include <aduc/aduc_inode.h> // ADUC_INODE_SENTINEL_VALUE
#include <aduc/file_utils.hpp> // aduc::findFilesInDir
#include <aduc/logging.h>
#include <aduc/workflow_utils.h>
#include <algorithm> // std::
#include <queue> // std::priority_queue
#include <set>
#include <string>
#include <sys/stat.h> // stat
#include <sys/types.h> // ino_t
#include <time.h> // time_t
#include <unistd.h> // unlink
#include <vector>

struct LessThan_UpdateCachePurgeFile
{
};

class UpdateCachePurgeFile
{
public:
    UpdateCachePurgeFile(ino_t fileNodeId, time_t lastModifiedTime, off_t fileSize, const std::string& filePath) :
        inode(fileNodeId), mtime(lastModifiedTime), size(fileSize), path(filePath)
    {
    }

    bool operator<(const UpdateCachePurgeFile& other) const
    {
        return mtime < other.mtime;
    }

    ino_t GetInode() const
    {
        return inode;
    }

    time_t GetLastModifiedTime() const
    {
        return mtime;
    }

    off_t GetSizeInBytes() const
    {
        return size;
    }

    std::string GetFilePath() const
    {
        return path;
    }

private:
    ino_t inode; ///< the inode from stat call.
    time_t mtime; ///< last modified time from stat call.
    off_t size; ///< The size of the file in bytes.
    std::string path; ///< the abs path to the file.
};

EXTERN_C_BEGIN

/**
 * @brief Deletes oldest files from the update cache until given totalSize is freed, or no more files exist.
 * Excludes payload files of the current update.
 * @param workflowHandle The workflow handle.
 * @param totalSize The maximum total size in bytes to be freed up in the update cache.
 * @param updateCacheBasePath The path to the base of update cache. NULL for default.
 * @return int 0 on success.
 */
int ADUC_SourceUpdateCacheUtils_PurgeOldestFromUpdateCache(
    const ADUC_WorkflowHandle workflowHandle, off_t totalSize, const char* updateCacheBasePath)
{
    int result = -1;

    // Algorithm
    //
    // 1. Create priority queue of type (UpdateCachePurgeFile*), where UpdateCachePurgeFile items sort by last modified time
    // 2. Find all files under the dir and add a corresponding UpdateCachePurgeFile entry to the priority queue
    // 3. from workflowHandle, get the inode for each payload file and remove from the priority queue (if exists)
    //         (the inode is saved at the time of moving the payload from sandbox to cache)
    // 4. while pq has items and totalSize > 0
    //        pop pq and delete the non-payload file at that item's path
    //
    try
    {
        // get the files currently in the update cache
        std::vector<std::string> filesInCache;
        aduc::findFilesInDir(
            updateCacheBasePath == nullptr ? ADUC_DELTA_DOWNLOAD_HANDLER_SOURCE_UPDATE_CACHE_DIR : updateCacheBasePath,
            &filesInCache);

        std::priority_queue<UpdateCachePurgeFile> oldestCacheFiles;

        // populate set of inodes of any payloads that have been moved to cache from sandbox
        std::set<ino_t> updatePayloadInodes;
        size_t countPayloads = workflow_get_update_files_count(workflowHandle);
        for (size_t index = 0; index < countPayloads; ++index)
        {
            ino_t payload_inode = workflow_get_update_file_inode(workflowHandle, index);
            if (payload_inode != ADUC_INODE_SENTINEL_VALUE)
            {
                updatePayloadInodes.insert(payload_inode);
            }
        }

        // filter out the current update's payloads that were moved to cache to avoid removal
        if (updatePayloadInodes.size() > 0)
        {
            Log_Debug("Removing %d payload paths from the cache purge list.", updatePayloadInodes.size());

            auto newEnd = std::remove_if(
                filesInCache.begin(), filesInCache.end(), [&updatePayloadInodes](const std::string& filePath) {
                    struct stat st
                    {
                    };
                    if (stat(filePath.c_str(), &st) != 0)
                    {
                        Log_Warn("filter - stat '%s', errno: %d", filePath.c_str(), errno);
                        st.st_ino = ADUC_INODE_SENTINEL_VALUE;
                        return false; // err on the side of not removing it
                    }

                    const ino_t& updateCacheInode = st.st_ino;

                    // Remove from list of files to be purged when the file's inode is in the set of update payload inodes
                    std::set<ino_t>::iterator iter = updatePayloadInodes.find(updateCacheInode);
                    if (iter == updatePayloadInodes.end())
                    {
                        return false;
                    }

                    return true;
                });

            filesInCache.erase(newEnd);
        }

        // insert into purge list
        std::for_each(filesInCache.begin(), filesInCache.end(), [&oldestCacheFiles](const std::string& filePath) {
            struct stat st
            {
            };
            if (stat(filePath.c_str(), &st) != 0)
            {
                Log_Warn("pq push - stat '%s', errno: %d", filePath.c_str(), errno);
            }
            else
            {
                oldestCacheFiles.emplace(st.st_ino, st.st_mtime, st.st_size, filePath);
            }
        });

        // delete files until made enough room as per totalSize needed for new upate payloads
        while (!oldestCacheFiles.empty() && totalSize > 0)
        {
            UpdateCachePurgeFile cachePurgeFile = oldestCacheFiles.top();
            oldestCacheFiles.pop();
            off_t fileSize = cachePurgeFile.GetSizeInBytes();

            std::string filePathForDelete{ std::move(cachePurgeFile.GetFilePath()) };
            int res = unlink(filePathForDelete.c_str());
            if (res != 0)
            {
                Log_Error("unlink '%s', inode %d - errno: %d", filePathForDelete.c_str(), cachePurgeFile.GetInode(), errno);
                result = -1; // overall it is a failure, but keep going to attempt to free up space.
                continue;
            }
            else
            {
                totalSize -= fileSize; // off_t is signed
            }
        }

        result = 0;
    }
    catch (const std::exception& e)
    {
        const char* what = e.what();
        Log_Error("Unhandled std exception: %s", what);
    }
    catch (...)
    {
        Log_Error("Unhandled exception");
    }

    return result;
}

EXTERN_C_END
