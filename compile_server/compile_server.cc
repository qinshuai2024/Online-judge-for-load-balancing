// 给编译服务模块添加网络通信

#include "compile_run.hpp"
#include "../common/httplib.h"

using namespace ns_compile_and_run;
using namespace httplib;

static void Usage(const std::string &proc)
{
    std::cout << "Usage: " << proc << " port" << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        Usage(argv[0]);
        exit(1);
    }

    Server svr; // 创建一个服务
    svr.Post("/compile_and_run", [](const Request &req, Response &resp)
             {
                 std::string in_json = req.body;
                 std::string out_json;
                 if (!in_json.empty())
                 {
                     CompileAndRun::Start(in_json, &out_json);
                     resp.set_content(out_json, "application/json;charset=utf-8");
                 }
             });
    svr.listen("0.0.0.0", atoi(argv[1])); // 启动服务

    // 测试
    // std::string in_json;
    // Json::Value in_value;
    // in_value["code"] = R"(
    //     #include <iostream>
    //     int main()
    //     {
    //         std::cout << "hhhh" << std::endl;
    //         std::cerr << "error" << std::endl;
    //         return 0;
    //     }
    // )";
    // in_value["input"] = "";
    // in_value["cpu_limit"] = 1;
    // in_value["mem_limit"] = 1024 * 10 * 3; // 运行模块设计的内存占用大小单位为KB
    // Json::StyledWriter writer;
    // in_json = writer.write(in_value);

    // std::cout << in_json << std::endl;

    // std::string out_json;
    // CompileAndRun::Start(in_json, &out_json);
    // std::cout << out_json << std::endl;

    return 0;
}