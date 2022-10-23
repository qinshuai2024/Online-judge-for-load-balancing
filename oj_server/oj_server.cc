#include <iostream>
#include <string>

#include "../common/httplib.h"

using namespace httplib;
int main()
{
    Server svr;
    // 获取所有题目列表
    svr.Get("/all_questions", [](const Request& req, Response& resp){
        // 使用control模块(调用view)，将题库列表渲染成网页返回
        resp.set_content("所有题目列表", "text/plain;charset=utf-8");
    });
    
}