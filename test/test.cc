#include <iostream>
#include <string>
#include <ctemplate/template.h>


int main()
{
    const std::string in_html = "./test.html";
    //建立ctemplate参数目录结构，键值对的形式
    ctemplate::TemplateDictionary dict("root");
    // 向结构中添加对应变量的值
    dict.SetValue("table_name", "表单"); // 前面是设置在html中变量，后面是要替换的值

    // 向循环中添加变量值
    for (int i = 0; i < 2; i ++ ) {
        // 创建一个空的字典，它的父类是dict，返回值为该字典的地址
        // 其中传入的参数，就是循环的名称
        ctemplate::TemplateDictionary* table_dict = dict.AddSectionDictionary("TABLE");
        // 同样可以为对应变量设置数据
        table_dict->SetValue("field1", "1");
        table_dict->SetIntValue("field2", 3);
        // 设置格式化的值
        table_dict->SetFormattedValue("field3", "%d", i);
    }

    // 获取被渲染的对象
    ctemplate::Template* tpl = ctemplate::Template::GetTemplate(in_html, ctemplate::DO_NOT_STRIP);//DO_NOT_STRIP：保持html网页原貌
    //开始渲染，返回新的网页结果到out_html
    std::string out_html;
    tpl->Expand(&out_html, &dict);
    std::cout << "渲染的带参html是:" << std::endl;
    std::cout << out_html << std::endl;
    return 0;
}