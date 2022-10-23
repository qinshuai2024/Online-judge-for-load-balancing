#pragma once

// 只负责程序运行功能，将输入，输出，异常结果保存到指定文件

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "../common/util.hpp"
#include "../common/log.hpp"

namespace ns_runner
{
    class Runner
    {
    public:
        Runner()
        {}
        ~Runner()
        {}
        static void SetProcLimit(int cpu_limit, int mem_limit)
        {
            struct rlimit rlimit_cpu;
            rlimit_cpu.rlim_cur = cpu_limit; // 软件限制最大
            rlimit_cpu.rlim_max = RLIM_INFINITY; // 硬件无限制
            setrlimit(RLIMIT_CPU, &rlimit_cpu);

            struct rlimit rlimit_mem;
            rlimit_mem.rlim_cur = mem_limit * 1024; // 转化为KB
            rlimit_mem.rlim_max = RLIM_INFINITY;
            setrlimit(RLIMIT_AS, &rlimit_mem);
        }

        /**
         * 返回值 > 0: 程序异常了，退出时收到了信号，返回值就是对应的信号编号
         * 返回值 == 0: 正常运行完毕的，结果保存到了对应的临时文件中
         * 返回值 < 0: 内部错误，异常，错误信息也会放到指定文件
         * 
         * cpu_limit: 该程序运行的时候，可以使用的最大cpu资源上限
         * mem_limit: 改程序运行的时候，可以使用的最大的内存大小(KB)
         **/
        static int Run(const std::string &file_name, int cpu_limit, int mem_limit)
        {
            /**
             * 对于一个程序题而言，如果编译成功，运行的结果有三种
             * 1. 代码正常运行，结果正确
             * 2. 代码正常运行，结果不正确
             * 3. 代码不正常运行，有异常
             * 
             * 结果是否正确是由测试用例决定的，这里主要负责异常情况
             * 
             **/
            // 创建三个属于要运行程序的输入输出文件，并重定向到标准输入输出
            std::string path_stdin = ns_util::PathUtil::GetRunIn(file_name);
            std::string path_stdout = ns_util::PathUtil::GetRunOut(file_name);
            std::string path_stderr = ns_util::PathUtil::GetRunError(file_name);

            umask(0);
            int fd_stdin = open(path_stdin.c_str(), O_CREAT | O_RDONLY, 0644);
            int fd_stdout = open(path_stdout.c_str(), O_CREAT | O_WRONLY, 0644);
            int fd_stderr = open(path_stderr.c_str(), O_CREAT | O_WRONLY, 0644);
            if (fd_stderr < 0 || fd_stdin < 0 || fd_stdout < 0)
            {
                ns_log::LOG(ERROR) << "输入输出文件打开失败" << std::endl;
                return -1; // 表示文件打开失败
            }

            pid_t pid = fork();
            if (pid < 0)
            {
                // 创建子进程失败，关闭文件描述符
                close(fd_stderr);
                close(fd_stdout);
                close(fd_stdin);
                ns_log::LOG(ERROR) << "进程创建失败" << std::endl;
                return -2; // 表示进程创建失败
            }
            else if (pid == 0)
            {
                // 重定向
                dup2(fd_stdin, 0);
                dup2(fd_stdout, 1);
                dup2(fd_stderr, 2);

                SetProcLimit(cpu_limit, mem_limit);
                // 拿到要替换的可执行程序
                std::string execute = ns_util::PathUtil::GetExe(file_name);

                // 程序替换
                execl(execute.c_str(), execute.c_str(), nullptr);
                exit(1);
            }
            else 
            {
                close(fd_stderr);
                close(fd_stdout);
                close(fd_stdin);
                int status = 0;
                waitpid(pid, &status, 0);
                // 如果执行的程序异常终止，通过 status & 0x7F 拿到终止信号    -- 值大于0发生异常
                // 如果执行的程序正常结束，那么status的低7位全为0，与的结果为0 -- 等于0正常结束

                ns_log::LOG(INFO) << "运行完毕, info: " << (status & 0x7F) << "\n"; 
                return status & 0x7F; 
            }

        }
    };
}