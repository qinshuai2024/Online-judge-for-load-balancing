// 负责调用编译和运行模块并处理结果
#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <jsoncpp/json/json.h>

#include "../common/util.hpp"
#include "../common/log.hpp"
#include "compiler.hpp"
#include "runner.hpp"

namespace ns_compile_and_run
{
    using namespace ns_util;
    using namespace ns_log;
    using namespace ns_compiler;
    using namespace ns_runner;

    class CompileAndRun
    {
    public:
        /**
         * @brief
         * 用来执行整个编译运行流程
         * @param in_json
         * 输入为一个json字符串，里面包含如下字段
         * code: 代码
         * input: 用户输入
         * cpu_limit: 时间要求
         * mem_limit: 内存要求
         * @param out_json
         * 输出也为一个json字符串
         * status: 退出状态码
         * reason: 执行结果，由数字表示
         * 选填字段
         * stdout: 成员运行之后的结果
         * stderr: 程序运行错误的结果
         *
         **/
        static void Start(const std::string &in_json, std::string *out_json)
        {
            // 先读取输入字段
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);
            std::string in_code = in_value["code"].asString();
            std::string in_input = in_value["input"].asString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();

            int status_code = 0;   // 返回的状态码
            int run_result = 0;    // 运行结果码
            std::string file_name; // 生成的唯一文件名, 没有后缀
            if (in_code.size() == 0)
            {
                // 代码为空
                status_code = -1;
                goto END;
            }

            // 获取唯一文件名，存储代码，这里采用时间戳
            file_name = FileUtil::UniqueFileName();

            // 往指定源文件写入代码
            if (!FileUtil::WriteFile(PathUtil::GetSrc(file_name), in_code))
            {
                status_code = -2;
                goto END;
            }

            // 编译
            if (!Compiler::Compile(file_name))
            {
                status_code = -3; // 编译失败
                goto END;
            }
            run_result = Runner::Run(file_name, cpu_limit, mem_limit);
            if (run_result < 0)
            {
                status_code = -2; // 内部未知错误
            }
            else if (run_result > 0)
            {
                // 程序异常结束，收到信号
                status_code = run_result;
            }
            else
            {
                // 运行成功
                status_code = 0;
            }
        END:
            // 根据状态码格式化输出结果
            
            Json::Value out_value;
            Json::StyledWriter out_writer;
            out_value["status"] = status_code;
            out_value["reason"] = StatusCodeToDesc(status_code, file_name);
            if (status_code == 0)
            {
                // 运行错误
                std::string str_stdout;
                FileUtil::ReaderFile(PathUtil::GetRunOut(file_name), &str_stdout, true);

                std::string str_stderr;
                FileUtil::ReaderFile(PathUtil::GetRunError(file_name), &str_stderr, true);

                out_value["stdout"] = str_stdout;
                out_value["stderr"] = str_stderr;
            }
            // 返回结果
            *out_json = out_writer.write(out_value);

            //RemoveFile(file_name);
        }

        // 用执行状态码生成对应描述内容
        static std::string StatusCodeToDesc(int code, const std::string &file_name)
        {
            std::string desc;
            switch (code)
            {
            case 0:
                desc = "运行成功";
                break;
            case -1:
                desc = "代码为空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3: // 编译失败，需要返回编译报错
                FileUtil::ReaderFile(PathUtil::GetCompileError(file_name), &desc, true);
                break;
            case SIGABRT: // 6
                desc = "超出内存限制";
                break;
            case SIGXCPU: // 24
                desc = "超出时间限制";
                break;
            case SIGFPE: // 8
                desc = "浮点数运算溢出错误";
                break;
            default:
                desc = "未知：" + std::to_string(code);
                break;
            }
            return desc;
        }
        static void RemoveFile(const std::string& file_name)
        {
            std::string path_src = PathUtil::GetSrc(file_name);
            if (FileUtil::IsFileExists(path_src)) unlink(path_src.c_str());

            std::string path_exe = PathUtil::GetExe(file_name);
            if (FileUtil::IsFileExists(path_exe)) unlink(path_exe.c_str());

            std::string path_compile_err = PathUtil::GetCompileError(file_name);
            if (FileUtil::IsFileExists(path_compile_err)) unlink(path_compile_err.c_str());

            std::string path_run_err = PathUtil::GetRunError(file_name);
            if (FileUtil::IsFileExists(path_run_err)) unlink(path_run_err.c_str());

            std::string path_run_in = PathUtil::GetRunIn(file_name);
            if (FileUtil::IsFileExists(path_run_in)) unlink(path_run_in.c_str());

            std::string path_run_out = PathUtil::GetRunOut(file_name);
            if (FileUtil::IsFileExists(path_run_out)) unlink(path_run_out.c_str());
        }
    };
}