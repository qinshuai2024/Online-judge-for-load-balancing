#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <boost/algorithm/string.hpp>


namespace ns_util
{

    const std::string temp_path = "./temp/";

    // 文件路径相关工具
    class PathUtil
    {
    public:
        // 添加后缀
        static std::string AddSuffix(const std::string file_name, const std::string&  suffix)
        {
            std::string path_name = temp_path;
            path_name += file_name;
            path_name += suffix;
            return path_name;
        }
        
        // 下面三个函数可以获得编译时相关存储文件路径
        static std::string GetSrc(const std::string& file_name)
        {
            return AddSuffix(file_name, ".cpp");
        }
        static std::string GetExe(const std::string& file_name)
        {
            return AddSuffix(file_name, ".exe");
        }
        static std::string GetCompileError(const std::string& file_name)
        {
            return AddSuffix(file_name, ".compile_error");
        }
        // 下面三个函数可以获得运行时，标准输入、输出、错误输出的文件路径
        static std::string GetRunIn(const std::string& file_name)
        {
            return AddSuffix(file_name, ".stdin");
        }
        static std::string GetRunOut(const std::string& file_name)
        {
            return AddSuffix(file_name, ".stdout");
        }
        static std::string GetRunError(const std::string& file_name)
        {
            return AddSuffix(file_name, ".stderr");
        }
    };
    
    class TimeUtil
    {
    public:
        static std::string GetTimeStamp()
        {
            timeval t_val;
            gettimeofday(&t_val, nullptr);
            return std::to_string(t_val.tv_sec); // 获取到现在的秒数
        }
        // 获得毫秒时间戳
        static std::string GetTimeMS()
        {
            struct timeval t_val;
            gettimeofday(&t_val, nullptr);
            // 秒、毫秒、微秒
            long long ms = t_val.tv_sec * 1000 + t_val.tv_usec / 1000;
            return std::to_string(ms);
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExists(const std::string& path_name)
        {
            // int stat(const char *path, struct stat *buf); 返回0表示成功，文件存在。 buf为输出参数，存放文件信息
            struct stat st;
            if (stat(path_name.c_str(), &st) == 0)
            {
                return true;
            }
            return false;
        }
        // 构建一个唯一的文件名，没有后缀
        static std::string UniqueFileName()
        {
            static std::atomic_uint id(0);
            id ++ ; // 原子性递增
            std::string ms = TimeUtil::GetTimeMS();
            return ms + "_" + std::to_string(id);
        }
        
        static bool WriteFile(const std::string& path_name, const std::string& content)
        {
            std::ofstream ofs(path_name); // 默认打开写功能
            if (!ofs.is_open())
            {
                return false;
            }

            ofs.write(content.c_str(), content.size());
            ofs.close();
            return true;
        }

        static bool ReaderFile(const std::string& path_name, std::string* out_content, bool keep_exp = false)
        {
            out_content->clear();
            std::ifstream ifs(path_name);
            if (!ifs.is_open())
            {
                return false;
            }
            // 按行读取
            std::string line;
            while (std::getline(ifs, line))
            {
                if (keep_exp) line += "\n";
                *out_content += line;
            }
            ifs.close();
            return true;
        }
    };

    class StringUtil
    {
    public:
        static void SplitString(const std::string& target, std::vector<std::string> *out, const std::string& sep)
        {
            boost::split(*out, target, boost::is_any_of(sep), boost::algorithm::token_compress_on);
        }
    };

}