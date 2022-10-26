
// 测试C语言连接MySQL

#include <iostream>
#include "./include/mysql.h"

const std::string host="127.0.0.1";
const std::string user="root";
const std::string password = "QSqin7172001#";
const std::string db = "test9_18";
const unsigned int port = 3466;

int main()
{
    // 1. 创建MySQL句柄
    MYSQL* my = mysql_init(nullptr);

    // 2. 连接数据库，TCP/IP
    if (mysql_real_connect(my, host.c_str(), user.c_str(), password.c_str(), db.c_str(), port, nullptr, 0) == nullptr)
    {
        std::cout << "connect failed" << std::endl;
        return 1;
    }
    // 2.1 设置链接编码格式
    mysql_set_character_set(my, "utf-8");
    std::cout << "Connection succeeded" << std::endl;

    // 3. 访问数据库
    // 插入数据
    // std::string sql = "insert into grade values(7, \'123\', \'A\')";
    // int res = mysql_query(my, sql.c_str());
    // if (res == 0) {
    //     std::cout << "insert succeeded" << std::endl;
    // }
    // // 删改，也是如此，它们没有从MySQL获取数据，操作很简单
    // std::string sql = "delete from grade where id = 6;";
    // std::string sql = "update grade set grade=\'F\' where name = \'张三\';";

    std::string sql = "select * from grade";
    int code = mysql_query(my, sql.c_str());
    if(code != 0)
    {
        std::cout << "execute: " << sql << " failed" << std::endl;
        return 2;
    }

    // 3.2 解析数据，拿到行数和列数
    MYSQL_RES *result = mysql_store_result(my);
    int rows = mysql_num_rows(result); // 行
    int cols = mysql_num_fields(result); // 列

    // 3.3 解析数据 -- 获取表中列名
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    for(int i = 0; i < cols; i++)
    {
        std::cout << fields[i].name << "\t";
    }
    std::cout << std::endl;

    // 3.4 拿到每个数据
    for (int i = 0; i < rows; i ++ ) 
    {
        // 拿到完整一行
        MYSQL_ROW line = mysql_fetch_row(result); 
        for (int j = 0; j < cols; j ++ ) 
        {
            std::cout << line[j] << "\t"; // 依次打印每列的值
        }
        std::cout << std::endl;
    }

    // 4. 关闭MySQL
    free(result);
    mysql_close(my);
    return 0;
}



// 测试ctemplate
// #include <iostream>
// #include <string>
// #include <ctemplate/template.h>
// int main()
// {
//     const std::string in_html = "./test.html";
//     //建立ctemplate参数目录结构，键值对的形式
//     ctemplate::TemplateDictionary dict("root");
//     // 向结构中添加对应变量的值
//     dict.SetValue("table_name", "表单"); // 前面是设置在html中变量，后面是要替换的值

//     // 向循环中添加变量值
//     for (int i = 0; i < 2; i ++ ) {
//         // 创建一个空的字典，它的父类是dict，返回值为该字典的地址
//         // 其中传入的参数，就是循环的名称
//         ctemplate::TemplateDictionary* table_dict = dict.AddSectionDictionary("TABLE");
//         // 同样可以为对应变量设置数据
//         table_dict->SetValue("field1", "1");
//         table_dict->SetIntValue("field2", 3);
//         // 设置格式化的值
//         table_dict->SetFormattedValue("field3", "%d", i);
//     }

//     // 获取被渲染的对象
//     ctemplate::Template* tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);//DO_NOT_STRIP：保持html网页原貌
//     //开始渲染，返回新的网页结果到out_html
//     std::string out_html;
//     tpl->Expand(&out_html, &dict);
//     std::cout << "渲染的带参html是:" << std::endl;
//     std::cout << out_html << std::endl;
//     return 0;
// }