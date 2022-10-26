#pragma once

#include <iostream>
#include <string>
#include <mutex>
#include <vector>
#include <algorithm>
#include <jsoncpp/json/json.h>

#include "../common/util.hpp"
#include "../common/log.hpp"
#include "../common/httplib.h"
#include "oj_view.hpp"
#include "oj_model.hpp"

namespace ns_control
{
    using namespace ns_model;
    using namespace ns_view;
    using namespace ns_log;
    using namespace ns_util;
    using namespace httplib;

    class Machine
    {
    public:
        std::string ip;
        int port;
        uint64_t load;
        std::mutex *mtx; // 锁不能拷贝，所以使用指针
    public:
        Machine(): ip(""), port(0), load(0), mtx(nullptr)
        {}
        ~Machine()
        {}

        // 提升主机负载
        void IncLoad()
        {
            if (mtx) mtx->lock();
            ++ load;
            if (mtx) mtx->unlock();
        }

        // 减少主机负载
        void DecLoad()
        {
            if (mtx) mtx->lock();
            -- load;
            if (mtx) mtx->unlock();
        }
        
        // 重置主机load
        void ResetLoad()
        {
            if (mtx) mtx->lock();
            load = 0;
            if (mtx) mtx->unlock();
        }

        // 获取主机负载
        uint64_t GetLoad()
        {
            uint64_t _load = 0;
            if (mtx) mtx->lock();
            _load = load;
            if (mtx) mtx->unlock();

            return _load;
        }
    };
    // server_list
    const std::string machine_path = "./conf/server_machine.conf";
    class LoadBalance
    {
    private:
        // 存放所有可以提供编译服务的主机
        // 下标充当对应主机ID
        std::vector<Machine> machines;
        // 在线主机的ID
        std::vector<int> online;
        // 离线主机的ID 
        std::vector<int> offline;
        // 保证本类分配主机的安全
        std::mutex mtx;
    public:
        LoadBalance()
        {
            assert(LoadConf(machine_path));
            LOG(INFO) << "加载 " << machine_path << " 成功"<< "\n";
        }
        ~LoadBalance()
        {
        }

        bool LoadConf(const std::string& machine_conf)
        {
            std::ifstream in(machine_conf);
            if (!in.is_open())
            {
                LOG(ERROR) << "加载服务器地址" << machine_conf << "失败" << std::endl;
                return false;
            }
            std::string line;
            while (std::getline(in, line))
            {
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, ":");
                if (tokens.size() != 2)
                {
                    LOG(WARING) << "切分主机端口" << line << "失败" << std::endl;
                    continue;
                }

                Machine m;
                m.ip = tokens[0];
                m.port = atoi(tokens[1].c_str());
                m.load = 0;
                m.mtx = new std::mutex;
                // 当前主机的ID -- 要插入machines的下标
                online.push_back(machines.size());
                machines.push_back(m);
            }
            in.close();
            return true;
        }
        // 两个输出型参数，选择一个负载最小的服务器
        // *id 是选择的服务器的ID
        // **m 是选择服务器的详细信息
        bool SmartChoice(int *id, Machine **m)
        {
            mtx.lock();

            // 可使用的服务器数量
            int online_num = online.size();
            if (online_num == 0)
            {
                mtx.unlock();
                LOG(FATAL) << "所有的后端编译主机均已下线，无法执行后续程序" << std::endl;
                return false;
            }
            // 负载均衡算法
            // 1. 随机数 + hash
            // 2. 轮询 + hash
            
            // 使用第二种，通过遍历的方式，找到一个负载最小的主机
            *id = online[0];
            *m = &machines[online[0]];
            uint64_t min_load = machines[online[0]].GetLoad();
            for (int i = 1; i < online_num; i ++ )
            {
                uint64_t cur_load = machines[online[i]].GetLoad();
                if (min_load > cur_load)
                {
                    min_load = cur_load;
                    *id = online[i];
                    *m = &machines[online[i]];
                }
            }
            mtx.unlock();
            return true;
        }

        void OfflineMachine(int id)
        {
            mtx.lock();
            for (auto iter = online.begin(); iter != online.end(); iter ++ )
            {
                if (*iter == id)
                {
                    // 离线主机找到
                    machines[id].ResetLoad();
                    online.erase(iter);
                    offline.push_back(id);
                    break; // 只删除一个，所以不需要关心迭代器失效的问题
                }
            }
            mtx.unlock();
        }
        void OnlineMachine()
        {
            mtx.lock();
            // 采用全部上线，最终是否可用，负载均衡那里会检查
            online.insert(online.end(), offline.begin(), offline.end());
            offline.clear();
            mtx.unlock();

            LOG(INFO) << "所有的主机有上线啦!" << "\n";
        }
        //for test
        void ShowMachines()
        {
             mtx.lock();
             std::cout << "当前在线主机列表: ";
             for(auto &id : online)
             {
                 std::cout << id << " ";
             }
             std::cout << std::endl;
             std::cout << "当前离线主机列表: ";
             for(auto &id : offline)
             {
                 std::cout << id << " ";
             }
             std::cout << std::endl;
             mtx.unlock();
        }
    };

    // 核心控制模块
    class Control
    {
    private:
        Model model_;
        View view_;
        LoadBalance load_balance_;
    public:
        Control()
        {}
        ~Control()
        {}

        // 重新上线主机
        void RecoveryMachine()
        {
            load_balance_.OnlineMachine();
        }

        // 返回所有题目构成的网页
        bool ShowAllQuestions(std::string *html)
        {
            std::vector<Question> all;
            if (model_.GetAllQuestions(&all))
            {
                // 对要展示的题目排序
                sort(all.begin(), all.end(), [](const struct Question &q1, const struct Question &q2){
                    return atoi(q1.number.c_str()) < atoi(q2.number.c_str());
                });
                view_.AllExpandHtml(all, html);
            }
            else
            {
                *html = "获取题目失败, 形成题目列表失败";
                return false;
            }

            return true;
        }
        // 返回一个题目构成的网页，包括题目描述，代码编写区域
        bool ShowOneQuestions(const std::string &number, std::string *html)
        {
            Question q;
            if (model_.GetOneQuestion(number, &q))
            {
                view_.OneExpandHtml(q, html);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在!";
                return false;
            }
            return true;
        }
        // 判断
        // 传入一个json字符串
        // code : "#include <iostream> ..."
        // input : ""

        void Judge(const std::string& number, const std::string& in_json, std::string* out_json)
        {
            // 0. 根据题目编号，拿到题目细节
            Question q;
            model_.GetOneQuestion(number, &q);
            // 1. 将in_json进行反序列化，拿到对应数据
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);
            std::string code = in_value["code"].asString();

            // 2. 拼接用户代码加测试用例代码，形成完整的程序代码
            Json::Value compile_value;
            compile_value["code"] = code + "\n" + q.tail;
            compile_value["input"] = in_value["input"].asString();
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.mem_limit;
        
            Json::FastWriter writer;
            std::string compile_str = writer.write(compile_value);

            // 3. 选择负载最低的主机
            // 选取规则：一直选择，直到主机可用
            while (true)
            {
                int id = 0;
                Machine* m = nullptr;
                if (!load_balance_.SmartChoice(&id, &m))
                {
                    break; // 无可用主机
                }

                // 4. 找到主机，发起http请求，得到运行编译运行结果
                Client cli(m->ip, m->port);
                m->IncLoad();
                LOG(INFO) << " 选择主机成功, 主机id: " << id << " 详情: " << m->ip << ":" << m->port << " 当前主机的负载是: " << m->GetLoad() << "\n";
                if (auto res = cli.Post("/compile_and_run", compile_str, "application/json;charset=utf-8"))
                {
                    // 5. 将结果赋值给out_json
                    m->DecLoad();
                    if (res->status == 200)
                    {
                        *out_json = res->body;
                        LOG(INFO) << "请求编译和运行服务成功..." << "\n";
                        break;
                    }
                }
                else
                {
                    //请求失败
                    LOG(ERROR) << " 当前请求的主机id: " << id << " 详情: " << m->ip << ":" << m->port << " 可能已经离线"<< "\n";
                    load_balance_.OfflineMachine(id);
                    load_balance_.ShowMachines(); // 调试
                }
            }

        }
    };
}
