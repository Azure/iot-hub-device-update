/**
 * @file content_handler_factory.hpp
 * @brief Definition of the ContentHandlerFactory.
 *
 * @copyright Copyright (c) 2019, Microsoft Corp.
 */
#ifndef ADUC_CONTENT_HANDLER_FACTORY_HPP
#define ADUC_CONTENT_HANDLER_FACTORY_HPP

#include <memory>
#include <string>

class ContentHandler;

/**
 * @struct ContentHandlerCreateData
 * @brief Data that needs to be passed to ContentHandlerFactory::Create.
 */
class ContentHandlerCreateData
{
public:
    // Creates an empty ContentHandlerCreateData.
    // Used to call IsInstalled when outside of a deployment.
    ContentHandlerCreateData() = default;

    ContentHandlerCreateData(
        const std::string& workFolder,
        const std::string& logFolder,
        const std::string& filename,
        const std::string& fileHash) :
        _workFolder(workFolder),
        _logFolder(logFolder), _filename(filename), _fileHash(fileHash)
    {
    }

    const std::string& WorkFolder() const
    {
        return _workFolder;
    }
    const std::string& LogFolder() const
    {
        return _logFolder;
    }
    const std::string& Filename() const
    {
        return _filename;
    }
    const std::string& FileHash() const
    {
        return _fileHash;
    }

private:
    std::string _workFolder;
    std::string _logFolder;

    // TODO(Nox): For now we only support one file.
    // eventually we will want to support a list of files
    // with different types.
    std::string _filename;
    std::string _fileHash;
};

namespace ContentHandlerFactory
{
std::unique_ptr<ContentHandler> Create(const char* updateType, const ContentHandlerCreateData& data);
} // namespace ContentHandlerFactory

#endif // ADUC_CONTENT_HANDLER_FACTORY_HPP
