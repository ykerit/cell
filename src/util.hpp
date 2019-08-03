//
// Created by some yuan on 2019/7/30.
//

#ifndef CELL_UTIL_HPP
#define CELL_UTIL_HPP

#include <iostream>
#include <boost/filesystem.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace cell
{
    namespace util
    {
        #define SHARED_FOLDERS "share"
        namespace FS = boost::filesystem;

        using Dict = std::unordered_map<std::string, std::pair<std::string, int64_t>>;


        static void GenerateFileList(Dict& dict)
        {
            // get share folder path
            FS::path shareFoldersPath = FS::current_path().parent_path() / SHARED_FOLDERS;
            // judgment of existence
            if (FS::exists(shareFoldersPath) && FS::is_directory(shareFoldersPath))
            {
                FS::directory_iterator iter(shareFoldersPath);
                while (iter != FS::directory_iterator{})
                {
                    if (FS::is_directory(*iter))
                        continue;
                    else
                        dict[iter->path().filename().string()] =
                                std::make_pair(iter->path().string(), FS::file_size(iter->path()));
                    ++iter;
                }
            } else
            {
                FS::create_directory(shareFoldersPath);
            }
        }
    }
}


#endif //CELL_UTIL_HPP
