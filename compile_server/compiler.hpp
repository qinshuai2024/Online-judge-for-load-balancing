#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "../common/util.hpp"
#include "../common/log.hpp"


namespace ns_compiler
{
    using namespace ns_log;
    class Compiler
    {
    public:
        Compiler()
        {}
        ~Compiler()
        {}

        // 文件名称，不包含路径，后缀
        static bool Compile(const std::string& file_name)
        {
            pid_t pid = fork();

            if (pid < 0)
            {
                LOG(ERROE) << "创建子进程失败" << std::endl;
                exit(1);
            }
            else if (pid == 0)
            {
                umask(0);
                // 打开错误输出文件
                int err_file = open(ns_util::PathUtil::GetCompileError(file_name).c_str(), O_CREAT | O_WRONLY, 0644);
                if (err_file < 0)
                {
                    LOG(WARING) << "没有形成错误文件" << std::endl;
                    exit(1);
                }
                // 标准错误输出重定向到指定文件
                dup2(err_file, 2);

                // 子进程程序替换
                execlp("g++", "g++", "-o", ns_util::PathUtil::GetExe(file_name).c_str(),\
                ns_util::PathUtil::GetSrc(file_name).c_str(), "-D", "COMPILER_ONLINE", "-std=c++11", nullptr); 
                LOG(ERROR) << "启动编译器g++失败，可能是参数错误" << "\n";
                exit(2);
            }
            else
            {
                waitpid(pid, nullptr, 0);
                // 是否生成指定EXE文件，以判别正常执行
                if (ns_util::FileUtil::IsFileExists(ns_util::PathUtil::GetExe(file_name)))
                {
                    LOG(INFO) << ns_util::PathUtil::GetSrc(file_name) << " 编译成功!" << "\n";
                    return true;
                }
                LOG(ERROR) << "编译失败，没有形成可执行程序" << "\n";
                return false;
            }
        }

    };
}