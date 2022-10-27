

# 一、项目宏观原理和技术栈

## 1.1 项目宏观结构

在线判题系统，项目属于`B/S`模式。

![image-20221018105201986](https://s2.loli.net/2022/10/18/xNpmqTUHjungtwS.png)

所以可以大体分为四个模块来编写：

1. `compile_server `: 对用户上传的题目提供编译并运行，保存相关结果和数据。
2. `oj_server` : 获取用户请求，负载均衡的分配，并返回结果。
3. `comm` : 公共模块，功能函数，日志等。
4. 前端界面设计，这部分代码也是在`oj_server`里面完成。

## 1.2 技术栈和环境

技术栈：

- 后端：`C++`、`Boost库函数`、`cpp-httplib网络库`、`ctemplate开源前端网页渲染库`、`jsoncpp`、`MySQL`、负载均衡设计，`MVC`
- 前端：`html/css/js/JQuery/ajax`、`Ace前端在线编辑器`

项目环境：`CentOs 7`、`VS Code`、`MySQL`

# 二、编译运行服务开发

模块功能：对题目编译运行，并获得格式化的结果（错误信息、输入输出信息、执行结果等）。

## 2.1 整体框图

![QQ截图20221022110528](https://s2.loli.net/2022/10/22/H4uTksGvaUVejQ3.png)



## 2.2 日志功能

先把日志功能写好，方便后面调试。

日志需要包含日志等级，描述信息，文件名，时间，行数四个基本信息，为了方便打印日志，这里设置返回对象为`ostream`，可以直接使用流输出运算符填写日志信息。

```c++
std::ostream& Log(const std::string& level, const std::string& file_name, int line)
#define LOG(level) Log(#level, __FILE__, __LINE__)
// 使用方式 LOG(LEVEL) << "xxxx" << std::endl;
```

## 2.3 编译模块

编译模块很简单，获得对于文件，调用系统调用程序替换函数，使用`g++`编译即可。

这里主要用到了两个系统调用：
```c++
#include <unistd.h>
int dup2(int oldfd, int newfd); // 将原newfd指向的文件换成oldfd指向的文件
```

```c++
// 标准错误输出重定向到指定文件
dup2(err_file, 2);
// 子进程程序替换
execlp("g++", "g++", "-o", ns_util::PathUtil::GetExe(file_name).c_str(),\
ns_util::PathUtil::GetSrc(file_name).c_str(), "-std=c++11", nullptr);
```

详细使用可以阅读我之前写的博客
[文件IO和重定向](https://blog.csdn.net/qq_52145272/article/details/124057291)
[进程替换](https://blog.csdn.net/qq_52145272/article/details/123114181)

## 2.4 运行模块

只负责程序运行功能，将输入，输出，异常结果保存到指定文件。返回值代表不同的信息。

```c++
#include <sys/time.h>
#include <sys/resource.h>

int setrlimit(int resource, const struct rlimit *rlim);
struct rlimit {
   rlim_t rlim_cur;  /* Soft limit */  
   rlim_t rlim_max;  /* Hard limit (ceiling for rlim_cur) */
};

resource的值：
RLIMIT_CPU：CPU time limit in seconds.
RLIMIT_AS：The maximum size of the process's virtual memory (address space) in bytes.
```

创建结构体对象，添加对应的软件限制，硬件无限制`RLIM_INFINITY`，然后分别设置内存和CPU运行时间限制。
一般而言，内存空间限制的值都很大，几十上百M，所以这里就知道用KB为单位，方便录题。



运行模块可能用到输入、输出、错误输出，所以三个标准流都要重定义到特定的文件夹。同样也是创建子进程使用程序替换，执行程序。
同时，对每一步出现的错误，返回特定的数值。

父进程获得执行程序退出码：

```c++
 waitpid(pid, &status, 0);
// 如果执行的程序异常终止，通过 status & 0x7F 拿到终止信号    -- 值大于0发生异常
// 如果执行的程序正常结束，那么status的低7位全为0，与的结果为0 -- 等于0正常结束
return status & 0x7F; 
```

## 2.5 服务模块

统筹处理编译和运行，主要通过客户端传入的`json`字符串，构建对应代码文件（文件名具有唯一性），分别调用编译和运行模块，对不同的返回结果添加描述字段，同样返回一个`json`串，最后将生成的临时文件全部处理干净。

服务模块的主要函数解析：

```c++
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
static void Start(const std::string &in_json, std::string *out_json);
```

- 生成临时文件

  利用系统时间戳和一个原子性递增的数，生成一个唯一性的文件名（不含目录和后缀）。

```C++
static std::string UniqueFileName()
{
    static std::atomic_uint id(0);
    id ++ ; // 原子性递增
    std::string ms = TimeUtil::GetTimeMS();
    return ms + "_" + std::to_string(id);
}
```

```c++
// 获取时间戳的系统调用
// 获得毫秒时间戳
#include <sys/time.h>

int gettimeofday(struct timeval *tv, struct timezone *tz);
struct timeval {
   time_t      tv_sec;     /* seconds */
   suseconds_t tv_usec;    /* microseconds */
};
// timezone类型不用管，tz参数传入空即可
struct timeval t_val;
gettimeofday(&t_val, nullptr);
```

为所需的文件（编译、运行需要重定向的三个流文件）添加后缀：

```c++
const std::string temp_path = "./temp/";  // 将产生的所有临时文件放在temp目录下

// 添加后缀
static std::string AddSuffix(const std::string file_name, const std::string&  suffix)
{
    // 拼接形成完整文件名
    std::string path_name = temp_path;
    path_name += file_name;
    path_name += suffix;
    return path_name;
}
```

- 清理临时文件：
  因为不知道生成了哪些文件，所以需要使用系统调用判断文件是否存在，存在再删除。

```c++
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

int stat(const char *path, struct stat *buf);
// 返回0表示成功，文件存在。 buf为输出参数，存放文件信息
struct stat st;
stat(path_name.c_str(), &st)
```

```c++
delete a name and possibly the file it refers to
#include <unistd.h>
int unlink(const char *pathname); // 删除指定文件
```

- 打包成网络服务：
  这里使用`cpp-httplib`库。`httplib库和json库`在上一个项目中有介绍。[点击查看](https://github.com/qinshuai2024/Search-Engines)

```c++
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
```

## 2.6 使用Postman测试

先进行本地测试：手动添加`json`串，调用函数。执行程序后，查看输出结果。

```c++
// 测试
std::string in_json;
Json::Value in_value;
in_value["code"] = R"(
    #include <iostream>
    int main()
    {
        std::cout << "hhhh" << std::endl;
        std::cerr << "error" << std::endl;
        return 0;
    }
)";
in_value["input"] = "";
in_value["cpu_limit"] = 1;
in_value["mem_limit"] = 1024 * 10 * 3; // 运行模块设计的内存占用大小单位为KB  30 * 1024 kB =  30 M
Json::StyledWriter writer;
in_json = writer.write(in_value);

std::cout << in_json << std::endl;

std::string out_json;
CompileAndRun::Start(in_json, &out_json);
std::cout << out_json << std::endl;
```



使用发起`http`请求的测试工具`Postman`来进行测试。

`postman`使用方法：

> 创建一个新的请求，选择`body->raw->json`，在`POST`位置可以设置请求方法， 输入框可以设置请求的`URL`，最后点击`Send`即可。如下图。

```json 
{
    "code":"#include <iostream>\nusing namespace std;\nint main()\n{\ncout << \"hello world\" << std::endl;\nreturn 0;\n}",
    "input":"",
    "cpu_limit": 1,
    "mem_limit": 50000
}
```

![image-20221022161636372](https://s2.loli.net/2022/10/22/9cu7XR8fsoBbU5Z.png)

# 三、基于`MVC`模式的判题服务



## 3.1 认识`MVC`

**`MVC`**即模型(`Model`)－视图(`view`)－控制器(`controller`)的缩写，是一种设计模式。它是用一种业务逻辑、数据与界面显示分离的方法来组织代码，将众多的业务逻辑聚集到一个部件里面，在需要改进和个性化定制界面及用户交互的同时，不需要重新编写业务逻辑，达到减少编码的时间，提高代码复用性。

- M：和数据交互的模块，比如对题库的增删改查。
- V：通常是拿到数据之后，要进行构建网页，渲染网页内容，展示给用户的(浏览器)。
- C：控制器接受用户的输入并调用模型和视图去完成用户的需求。控制器本身不输出任何东西和做任何处理。它只是接收请求并决定调用哪个模型构件去处理请求，然后再确定用哪个视图来显示返回的数据。

使用的`MVC`的目的：在于将M和V的实现代码分离，从而使同一个程序可以使用不同的表现形式。比如，底层题库存储可以使用文件，也可以使用数据库，不会影响最后的展现。

## 3.2 设置服务路由

一个基本的判题系统，需要有如下服务（代码均为测试所用）：

1. 获取所有题目列表

   ```c++
   Server svr;
   // 获取所有题目列表
   svr.Get("/all_questions", [](const Request& req, Response& resp){
       // 使用control模块(调用view)，将题库列表渲染成网页返回
       resp.set_content("所有题目列表", "text/plain;charset=utf-8");
   });
   ```

2. 获取指定题目的编写界面，使用正则表达式匹配题号

   ```c++
   svr.Get(R"(/questions/(\d+))", [&ctrl](const Request& req, Response& resp){
       std::string number = req.matches[1]; // 获得匹配的题号
       resp.set_content("题号：" + number, "text/plain;charset=utf-8");
   }) ;
   ```

3. 获取指定题目的判题界面，执行编译模块

   ```c++
   svr.Get(R"(/judge/(\d+))", [&ctrl](const Request& req, Response& resp){
       std::string number = req.matches[1]; // 获得匹配的题号
       // 实际这里调用编译模块，返回的应该是一个json串
       resp.set_content("判题题号：" + number, "text/plain;charset=utf-8");
   }) ;
   ```

 4. 设置web页面根目录

    ```c++
     svr.set_base_dir("./wwwroot");
    ```

## 3.3 `Model`模块

这设计数据处理模块之前，需要先设计题目字段，见` 4.2 文件版题库`。

`model`模块对数据处理，拿到题号，以及题目对应细节。

对每个题目，可设计如下结构体来表示：
```c++
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
```

使用哈希表存储题号和题目信息的映射关系，方便读取任意题
```c++
std::unordered_map<std::string, Question> questions;
```

这里只需要设计三个主要功能：

1. 从文件加载所有题目列表到内存，放在`questions`中。
   ```c++
   // 从指定文件中读取 若是MySQL存储，就不用这个接口了
   bool LoadQuestionsList(const std::string& list_path); 
   ```

   

2. 提供一个接口，获取所有的题目列表信息。
   ```c++
   // out是一个输出参数
   bool GetAllQuestions(std::vector<Question>* out); 
   ```

   

3. 查询并返回任意题号对应的信息。
   ```c++
   // 通过题目编号，获取对应题目信息
   bool GetOneQuestion(const std::string& number, Question* q)
   ```


## 3.4 `View`模块

### 3.4.1  `ctemplate`的安装和使用

源码安装：

```c++
git clone https://github.com/OlafvdSpek/ctemplate 下载源码
./autogen.sh
./configure
make         //编译
make install     //安装到系统中
// 需要g++版本，有些命令需要root权限
```

`Ctemplate`是Google开源的一个C++版本`html`模板替换库。在C++代码中操作`html`模板是一件非常简单和高效的事。

基本语法：

> 模板中的变量使用{{}}括起来，
>
> 而`{{#循环名}}`和`{{/循环名}}`表示一个循环。

需要处理的`html`

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>测试ctemplate</title>
</head>
<body>
    <div>{{table_name}}</div>
    <table>
        {{#TABLE}}
        <tr>
            <td>{{field1}}</td>
            <td>{{field2}}</td>
            <td>{{field3}}</td>
        </tr>
        {{/TABLE}}
    </table>
</body>
</html>
```

`ctemplate`的使用

```c++
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
```

最后处理的结果，注意编译的时候需要链接`ctemplate`库（`-lctemplate`），由于这个库中使用了原生线程库，所以还需要链接`pthread`（`-lpthread`）。

```html
渲染的带参html是:
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>测试ctemplate</title>
</head>
<body>
    <div>表单</div>
    <table>
        
        <tr>
            <td>1</td>
            <td>3</td>
            <td>0</td>
        </tr>
        
        <tr>
            <td>1</td>
            <td>3</td>
            <td>1</td>
        </tr>
        
    </table>
</body>
</html>
```

### 3.4.2 编写`view`模块

`view`主要做的事情就是，把获得的数据渲染到HTML网页中，使用`ctemplate`可以轻松完成。

使用`ctemplate`之前需要在`html`中设置变量，对于题目列表的显示，可以使用表格：

```html
<table>
    <caption>题目列表</caption>
    <thead>
        <tr>
            <th>编号</th>
            <th>标题</th>
            <th>难度</th>
        </tr>
    </thead>
    <tbody>
        {{#question_list}}
        <tr>
            <td>{{number}}</td>
            <td>{{title}}</td>
            <td>{{star}}</td>
        </tr>
        {{/question_list}}
    </tbody>
</table>
```

单个题目的显示需要布局，代码较多，这里列举需要填充的变量即可
```c++
{{number}}     {{title}}     {{star}}
{{desc}}     // 题目描述
{{pre_code}} // 显示给用户的代码
```

功能实现：

1. 获取所有题目的网页
   ```c++
   // 获取题目列表，并组成HTML返回
   void AllExplandHtml(const std::vector<ns_model::Question>& questions, std::string *out_html);
   ```

2. 获取某一个题目详细内容的网页
   ```c++
   // 获取单个题目信息并返回
   void OneExplandHtml(const ns_model::Question& q, std::string *out_html)
   ```

## 3.5 `Controller`（负载均衡）

这是比较复杂的一个模块，具有承上启下功能。

### 3.5.1  核心代码逻辑

控制模块的核心就是，使用`model`模块获取数据，交给`view`形成网页，最后控制模块将拿到的网页传递到浏览器上展现给用户。

主要功能：

1. 展现题目列表
   ```c++
   // 返回所有题目构成的网页
   bool ShowAllQuestions(std::string *html);
   ```

2. 展现某个题目界面
   ```c++
   // 返回一个题目构成的网页，包括题目描述，代码编写区域
   bool ShowOneQuestions(const std::string &number, std::string *html);
   ```

3. 判题功能，需要负载均衡的交给后端服务器编译。
   ```c++
   // 传入一个json字符串
   // code : "#include <iostream> ..."
   // input : ""
   void Judge(const std::string& number, const std::string& in_json, std::string* out_json)
   ```

   

   > 判题功能主要步骤：
   >
   > 1. 根据题目编号，拿到题目细节
   > 2. 将`in_json`进行反序列化，拿到对应数据
   > 3. 拼接用户代码加测试用例代码，形成完整的程序代码
   > 4. 选择负载最低的主机
   > 5. 找到主机，发起`http`请求，得到运行编译运行结果
   > 6. 将结果赋值给`out_json`

   

### 3.5.2 负载均衡设计

首先需要设计一个服务器类，增加属性和对应方法：

```c++
class Machine
{
public:
    std::string ip;
    int port; 
    uint64_t load; // 负载
    // 对机器负载数量的修改需要原子性 ，锁不能拷贝，所以使用指针
    std::mutex *mtx; 
public:
    Machine(): ip(""), port(0), load(0), mtx(nullptr)
    {}
    ~Machine()
    {}
    // 提升主机负载
    void IncLoad();
    // 减少主机负载
    void DecLoad();
    // 重置主机负载
    void ResetLoad();
    // 获取主机负载
    uint64_t GetLoad();
};
```

管理多个机器，来控制负载均衡，这也需要一个类，其中配置文件中存放可供使用的主机，如：`127.0.0.1:8081`

```c++
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
    // 保证分配主机的安全和唯一
    std::mutex mtx;
public:
    LoadBalance()
    {
        assert(LoadConf(machine_path));
        LOG(INFO) << "加载 " << machine_path << " 成功"<< "\n";
    }
    ~LoadBalance()
    {}
	// 读取配置文件，获取能够使用的主机地址
    bool LoadConf(const std::string& machine_conf);
    // 两个输出型参数，选择一个负载最小的服务器
    // *id 是选择的服务器的ID
    // **m 是选择服务器的详细信息
    bool SmartChoice(int *id, Machine **m);
    // 主机离线
    void OfflineMachine(int id);
    // 主机上线
    void OnlineMachine();
};
```

### 3.5.3 常见的负载均衡算法

当一台机器不能承受访问压力时，通过横向增加多台服务器，来共同承担访问压力，来极大的降低后端的访问压力，提升用户的访问性能。



1. 轮询(Round Robin)

   > 原理：将请求按顺序轮流分配到后台服务器上，均衡的对待每一台服务器，而不关心服务器实际的连接数和当前的系统负载。
   > 适合场景：适合于应用服务器硬件都相同的情况。

2. 加权轮循(Weighted Round Robbin)

   > 原理：在轮询的基础上根据硬件配置不同，按权重分发到不同的服务器。
   > 适合场景：跟配置高、负载低的机器分配更高的权重，使其能处理更多的请求；而性能低、负载高的机器，配置较低的权重，让其处理较少的请求。

3. 随机(Random)

   > 原理：通过系统随机函数，根据后台服务器列表的大小值来随机选取其中一台进行访问。
   >
   > 随着调用量的增大，客户端的请求可以被均匀地分派到所有的后端服务器上，其实际效果越来越接近于平均分配流量到后台的每一台服务器，也就是轮询法的效果。

4. 最少连接(Least Connections)

   > 原理：将请求发送给当前最少连接数的服务器上。由于每个请求的连接时间不一样，使用轮询或者加权轮询算法的话，可能会让一台服务器当前连接数过大，而另一台服务器的连接过小，造成负载不均衡。
   > 记录每个服务器正在处理的请求数，把新的请求分发到最少连接的服务器上，要维护内部状态（服务器和对应连接数）。

5. 源地址散列(IP Hash)

   > 原理：根据服务消费者请求客户端的`IP`地址，通过哈希函数计算得到一个哈希值，将此哈希值和服务器列表的大小进行取模运算，得到的结果便是要访问的服务器地址的序号。
   > 适合场景：根据请求的来源`IP`进行`hash`计算，同一`IP`地址的客户端，当后端服务器列表不变时，它每次都会映射到同一台后端服务器进行访问。用来实现会话粘滞(Sticky Session)。
   >
   > 参考阅读： [会话保持（粘滞会话）](https://www.cnblogs.com/yanggb/p/10970702.html)

本次因为只有一台云服务器，每个编译服务器性能相同，所以使用最简单的轮询即可。
实现也很简单，遍历在线的服务器，选取一个负载最小的服务器使用即可。


## 3.6 `Postman`综合调试

由于只有一台服务器，所以只能开多个终端，多个执行编译服务，充当服务器，一个用来执行OJ服务，处理请求。

注意：编译使用`-D`选项，可以定义宏，处理掉之前因为方便编写测试用例引入的头文件。

完善路由服务功能：

```c++
// 判题功能
svr.Post(R"(/judge/(\d+))", [&ctrl](const Request& req, Response& resp){
        std::string number = req.matches[1];
        std::string out_json;
        ctrl.Judge(number, req.body, &out_json);
        resp.set_content(out_json, "application/json;charset=utf-8");
    });
```

### 3.6.1 正确性验证

使用第一题的`A+B`的代码测试，如下所示：

![image-20221025151454672](https://s2.loli.net/2022/10/25/qUdAaIgko19sZrX.png)

![](https://s2.loli.net/2022/10/25/Y1y8z4qG6hvBOAb.png)

三个服务器执行编译服务，它们的`ip:port`已经在OJ服务中注册（配置文件中），后台日志打印如下：

![](https://s2.loli.net/2022/10/25/9Bh1QrkFqApaZeN.png)

测试服务器停止后，能否正常使用剩余服务器。

![](https://s2.loli.net/2022/10/25/ZEpKmsD7okuScVA.png)

### 3.6.2 压力测试

由于`postman`发送请求很慢，无法进行压力测试，不能看到负载是否均衡。等到实现前端后端交互，使用前端的提交代码（疯狂点击）可以进行压力测试。如下图，可以看到负载均衡：

![](https://s2.loli.net/2022/10/27/XvMaORLlo1GgKrf.png)



### 3.6.3 上线编译服务

**上线挂掉但又重新启动服务器**

当所有或者大量编译服务器挂掉之后，人工重新启动服务器，然后OJ服务器，需要将这些下线的及其重新上线（加入到在线列表），但是具体能不能使用，选择主机的是需要判断的。这里可以使用信号来实现。

```c++
#include <signal.h>
typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler); // 注册信号回调方法
```

![](https://s2.loli.net/2022/10/26/1VmBt6wUSWx8GZO.png)



![](https://s2.loli.net/2022/10/26/Yav1bz9ydWK7UJ8.png)

![](https://s2.loli.net/2022/10/26/BimY4lqTpCkMoy5.png)



# 四、题目录入

## 4.1 文件版题库

首先需要建立一个题目列表，可以用如下几个字段来描述一道题目：

> 题目编号（唯一）
> 题目标题
> 题目难度（简单、中等、困难）
> 时间要求
> 空间要求

可以用一个文本，一行表示一个题目，不同字段之间用空格分隔。
```c++
1 A+B 简单 1 30000
```

然后就是对每个题目的具体细节，对于核心代码模式需要如下字段

> 上面描述的五个字段
> 对题目的描述
> 提供的代码
> 测试代码

可以对每个题目，建立对应题号的目录，存放题目描述和预先代码。
![image-20221023160421395](https://s2.loli.net/2022/10/23/K1sJIWvHkq86lQd.png)

其中`head.cpp`存放预先提供的代码，`tail.cpp`存放测试代码，最后编译的是两个的组合。

例如：

```c++
// desc.txt
输入两个整数，求这两个整数的和是多少。
```

```c++
// head.cpp
#include <iostream>
using namespace std;

class Solution {
public:
    int twoSum(int a, int b) {
        // 代码编写区
    }
};
```

```c++
// tail.cpp
#ifndef COMPILER_ONLINE
#include "head.cpp" // 仅仅为了写测试用例的时候引入对应类，不报错，真正编译的时候需要去掉，g++ -D COMPILER_ONLINE
#endif

bool test1()
{
    int a = 10, b= 20;
    if (a + b == Solution().twoSum(a, b)) 
        return true;
    else 
        return false;
}

bool test2()
{
    int a = 2220, b= 31230;
    if (a + b == Solution().twoSum(a, b)) 
        return true;
    else 
        return false;
}

int main()
{
    if (test1() && test2()) {
        cout << "测试通过" << endl;
    } else {
        cout << "答案错误" << endl;
    }
    return 0;
}
```

## 4.2 `MySQL`版题库

设计表结构

> 数据库：`oj`， 表：`oj_questions`

```sql
create table if not exists `oj_questions`(
	id int primary key auto_increment comment '题目的ID',
    title varchar(64) not null comment '标题',
	star varchar(8) not null comment '难度',
    header text not null comment '给用户看到的代码',
    tail text not null comment '测试代码',
    time_limit int default 1 comment '时间限制',
    cpu_limit int default 5000 comment '空间限制'
)
```

关于C/C++访问MySQL数据库，可以看下面这篇我的笔记。

[使用C语言连接MySQL](http://t.csdn.cn/Tnhwn)

在保证接口不变的同时，完成读取数据，来改造model模块。

# 五、前端界面设计



## 5.1 `ACE`编辑器

[ACE项目地址](https://github.com/ajaxorg/ace)

> Ace是一个用JavaScript编写的可嵌入代码编辑器。它与Sublime，Vim和TextMate等本地编辑器的功能和性能相匹配。它可以轻松地嵌入任何网页和JavaScript应用程序中。

通过`CDN`引入`ACE相关js文件`

> `CDN`——**Content Delivery Network，内容分发网络**。引入服务器的js文件，方便直接编写，使用CND技术引入，加快访问速度。

```html
<script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.2.6/ace.js" type="text/javascript"
    charset="utf-8"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/ace/1.2.6/ext-language_tools.js" type="text/javascript"
    charset="utf-8"></script>

// 引入JQuery的js文件
<script src="http://code.jquery.com/jquery-2.1.1.min.js"></script>
```

ACE 相关使用
```html
<script>
    //初始化对象
    editor = ace.edit("code");

    // 设置主题风格和语言
    theme = "clouds"
    language = "c_cpp"
    editor.setTheme("ace/theme/" + theme);
    editor.session.setMode("ace/mode/" + language);

    // 字体大小
    editor.setFontSize(16);
    // 设置默认制表符的大小:
    editor.getSession().setTabSize(4);

    // 设置只读（true时只读，用于展示代码）
    editor.setReadOnly(false);

    // 启用提示菜单
    ace.require("ace/ext/language_tools");
    editor.setOptions({
        enableBasicAutocompletion: true,
        enableSnippets: true,
        enableLiveAutocompletion: true
    });
</script>
```

## 5.2 前后端交互

这里需要前后端交互的地方只有提交代码，只需设置一个按钮，设置点击事件，使用Ajax的POST方法向后端发送代码，然后获取后端结果。根据结果的状态码，显示不同内容。

```js
function submit(){
    // alert("嘿嘿!");
    // 1. 收集当前页面的有关数据, 1. 题号 2.代码
    var code = editor.getSession().getValue();
    // console.log(code);
    var number = $(".container .part1 .left_desc h3 #number").text();
    // console.log(number);
    var judge_url = "/judge/" + number;
    // console.log(judge_url);
    // 2. 构建json，并通过ajax向后台发起基于http的json请求
    $.ajax({
        method: 'Post',   // 向后端发起请求的方式
        url: judge_url,   // 向后端指定的url发起请求
        dataType: 'json', // 告知server，我需要什么格式
        contentType: 'application/json;charset=utf-8',  // 告知server，我给你的是什么格式
        data: JSON.stringify({
            'code':code,
            'input': ''
        }),
        success: function(data){
            //成功得到结果
            // console.log(data);
            show_result(data);
        }
    });
}
```

