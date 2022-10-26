#pragma once

#include <iostream>
#include <string>
#include "util.hpp"


namespace ns_log
{
    // 错误等级
    enum
    {
        INFO,
        DEBUG,
        WARING,
        ERROR,
        FATAL
    };

    inline std::ostream& Log(const std::string& level, const std::string& file_name, int line)
    {
        std::string message = "[";
        message += level;
        message += "]";

        message += "[";
        message += file_name;
        message += "]";

        message += "[";
        message += std::to_string(line);
        message += "]";

        // 添加时间戳
        message += "[";
        message += ns_util::TimeUtil::GetTimeStamp();
        message += "]";

        std::cout << message; // 不加回车换行，数据会存在输出缓冲区里面

        return std::cout; 
    }
    // 使用方式 LOG(Level) << "xxxx" << "\n";
    // 此处编写的时候出现神级错误，宏结尾加了分号; 找了好久
    #define LOG(level) Log(#level, __FILE__, __LINE__)

}