#ifndef CORA_COMMON_SOURCEMANAGER_H
#define CORA_COMMON_SOURCEMANAGER_H

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "SourceLocation.hpp"

namespace cora
{

    struct SourceFile
    {
        uint32_t id;
        std::string path;
        std::string content;

        // Line offset
        std::vector<uint32_t> line;

        SourceFile(uint32_t id, const std::string &path, const std::string &cont)
            : id(id), path(path), content(cont)
        {

            line.push_back(0);
            for (uint32_t i = 0; i < content.size(); ++i)
            {
                if (content[i] == '\n')
                {
                    line.push_back(i + 1);
                }
            }
        };
    };

    class SourceManager
    {
    public:
        uint32_t addFile(const std::string &path, const std::string &content)
        {
            if (pathToFileID.count(path))
            {
                return pathToFileID[path];
            }

            uint32_t id = static_cast<uint32_t>(files.size());
            files.push_back(std::make_unique<SourceFile>(id, path, content));
            pathToFileID[path] = id;
            return id;
        };

        const SourceFile *getFile(uint32_t id) const
        {
            if (id >= files.size())
                return nullptr;
            return files[id].get();
        };

        SourceLocation getLoc(uint32_t fileID, uint32_t offset) const
        {
            const SourceFile *file = getFile(fileID);
            if (!file || offset > file->content.size())
                return {};

            auto it = std::upper_bound(file->line.begin(), file->line.end(), offset);
            uint32_t line = static_cast<uint32_t>(std::distance(file->line.begin(), it));
            uint32_t lineStart = file->line[line - 1];
            uint32_t column = offset - lineStart + 1;

            return {fileID, offset, line, column};
        };

        std::string_view getLineContent(uint32_t fileID, uint32_t line) const
        {
            const SourceFile *file = getFile(fileID);
            if (!file || line == 0 || line > file->line.size())
                return "";

            uint32_t start = file->line[line - 1];
            uint32_t end = (line < file->line.size()) ? file->line[line] - 1 : static_cast<uint32_t>(file->content.size());

            // Trim trailing \r if present (for Windows line endings)
            if (end > start && file->content[end - 1] == '\r')
            {
                end--;
            }

            return std::string_view(file->content.data() + start, end - start);
        };

        std::string_view getSnippet(SourceRange range) const
        {
            const SourceFile *file = getFile(range.start.fileID);
            if (!file || !range.isValid() || range.end.offset < range.start.offset)
                return "";

            return std::string_view(file->content.data() + range.start.offset, range.end.offset - range.start.offset);
        };

    public:
        // private:
        std::vector<std::unique_ptr<SourceFile>> files;
        std::unordered_map<std::string, uint32_t> pathToFileID;
    };

} // namespace cora

#endif // CORA_COMMON_SOURCEMANAGER_H