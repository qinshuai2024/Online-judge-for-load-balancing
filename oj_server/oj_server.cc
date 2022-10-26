#include <iostream>
#include <string>
#include <signal.h>

#include "../common/httplib.h"
#include "oj_controller.hpp"

using namespace httplib;
using namespace ns_control;

Control* ctrl_ptr = nullptr;

void handler(int p)
{
    ctrl_ptr->RecoveryMachine();
}


int main()
{
    Server svr;
    Control ctrl;

    ctrl_ptr = &ctrl;
    // 注册回调函数
    signal(SIGINT, handler);

    svr.Get("/all_questions", [&ctrl](const Request& req, Response& resp){
        std::string html;
        ctrl.ShowAllQuestions(&html);
        resp.set_content(html, "text/html;charset=utf-8");
    });
    // 用户要根据题目编号，获取题目的内容
    // /question/100 -> 正则匹配
    // R"()", 原始字符串raw string,保持字符串内容的原貌，不用做相关的转义
    svr.Get(R"(/question/(\d+))", [&ctrl](const Request& req, Response& resp){
        std::string number = req.matches[1];
        std::string html;
        ctrl.ShowOneQuestions(number, &html);
        resp.set_content(html, "text/html;charset=utf-8");
    }) ;

    svr.Post(R"(/judge/(\d+))", [&ctrl](const Request& req, Response& resp){
        std::string number = req.matches[1];
        std::string out_json;
        ctrl.Judge(number, req.body, &out_json);
        resp.set_content(out_json, "application/json;charset=utf-8");
    });

    svr.set_base_dir("./wwwroot");
    svr.listen("0.0.0.0", 8080);
    
    return 0;
}