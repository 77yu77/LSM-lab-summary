#pragma once

#include <sstream>
#include <sys/stat.h>
#include <vector>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include <windows.h>
#endif
#if defined(linux) || defined(__MINGW32__) || defined(__APPLE__)
#include <dirent.h>
#include <unistd.h>
#endif

namespace utils{
    /**
     * Check whether directory exists
     * @param path directory to be checked.
     * @return ture if directory exists, false otherwise.
     */
    bool dirExists(std::string path);

    /**
     * list all filename in a directory
     * @param path directory path.
     * @param ret all files name in directory.
     * @return files number.
     */
    #if defined(_WIN32) && !defined(__MINGW32__) 
    int scanDir(std::string path, std::vector<std::string> &ret){
        std::string extendPath;
        if(path[path.size() - 1] == '/'){
            extendPath = path + "*";
        }
        else{
            extendPath = path + "/*";
        }
        WIN32_FIND_DATA fd;
        HANDLE h = FindFirstFileA(extendPath.c_str(), &fd);
        if(h == INVALID_HANDLE_VALUE){
            return 0;
        }
        while(true){
            std::string ss(fd.cFileName);
            if(ss[0] != '.'){
                ret.push_back(ss);
            }
            if(FindNextFile(h, &fd) ==false){
                break;
            }
        }
        return ret.size();
    }
    #endif
    #if defined(linux) || defined(__MINGW32__) || defined(__APPLE__)
    int scanDir(std::string path, std::vector<std::string> &ret);
    #endif

    /**
     * Create directory
     * @param path directory to be created.
     * @return 0 if directory is created successfully, -1 otherwise.
     */
    int _mkdir(const char *path);

    /**
     * Create directory recursively
     * @param path directory to be created.
     * @return 0 if directory is created successfully, -1 otherwise.
     */
    int mkdir(const char *path);

    /**
     * Delete a empty directory
     * @param path directory to be deleted.
     * @return 0 if delete successfully, -1 otherwise.
     */
    int rmdir(const char *path);

    /**
     * Delete a file
     * @param path file to be deleted.
     * @return 0 if delete successfully, -1 otherwise.
     */
    int rmfile(const char *path);


    
}
