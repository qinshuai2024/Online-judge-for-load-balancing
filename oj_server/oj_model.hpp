#pragma once

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

#include <cassert>
#include <cstdlib>

#include "../common/log.hpp"
#include "../common/util.hpp"

namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    struct Question
    {
        std::string number; // 题目编号
        std::string title; // 题目标题
        std::string star; // 题目难度等级
        std::string desc; // 题目描述
        int cpu_limit; // 运行时间限制 (秒)
        int mem_limit; // 内存限制 (KB)
        std::string header; // 为题目提供的原有代码
        std::string tail; // 为题目准备的测试用例
    };

    const std::string question_list = "./questions/questions.list";
    const std::string question_path = "./questions/";
    class Model
    {
    private:
        std::unordered_map<std::string, Question> questions;
    public:
        Model()
        {
            assert(LoadQuestionsList(question_list));
        }
        bool LoadQuestionsList(const std::string& list_path)
        {
            std::ifstream in(list_path);
            if (!in.is_open())
            {
                LOG(ERROR) << "打开题目列表文件失败" << std::endl;
                return false;
            }

            // 一行一行的读取，一行为一个题目信息
            // 1 判断回文数 简单 1 30000
            std::string line;
            while (std::getline(in, line))
            {
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, " ");

                if (tokens.size() < 5)
                {
                    LOG(ERROR) << "加载部分题库失败" << std::endl;
                    return false;
                }
                Question q;
                q.number = tokens[0];
                q.title = tokens[1];
                q.star = tokens[2];
                q.cpu_limit = atoi(tokens[3].c_str());
                q.mem_limit = atoi(tokens[4].c_str());

                // 再加上题目描述，给定的代码
                std::string path = question_path;
                path += "/" + q.number + "/";

                FileUtil::ReaderFile(path + "desc.txt", &(q.desc), true);
                FileUtil::ReaderFile(path + "header.cpp", &(q.header), true);
                FileUtil::ReaderFile(path + "tail.cpp", &(q.tail), true);


                questions.insert({q.number, q});

            }

            LOG(INFO) << "加载题库成功" << std::endl;
            in.close(); 
            return true;
        }   
        bool GetAllQuestions(std::vector<Question>* out)
        {
            if (questions.empty()) 
            {
                LOG(ERROR) << "获取题库失败" << std::endl;
                return false;
            }

            for (auto& iter : questions)
            {
                out->push_back(iter.second);
            }
            return true;
        }
        // 通过题目编号，获取对应题目信息
        bool GetOneQuestion(const std::string& number, Question* q)
        {
            const auto& iter = questions.find(number);
            if (iter == questions.end())
            {
                LOG(ERROR) << "找不到对应题目编号, 编号为：" << number << std::endl;
                return false;
            }
            
            // 找到了
            *q = iter->second;
            return true;
        }
        ~Model(){}
    };
}