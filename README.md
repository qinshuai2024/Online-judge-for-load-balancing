

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

- 后端：`C++`、`Boost库函数`、`cpp-httplib网络库`、`ctemplate开源前端网页渲染库`、`jsoncpp`、`MySQL`、负载均衡设计
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
ns_util::PathUtil::GetSrc(file_name).c_str(), "-D", "COMPILER_ONLINE", "-std=c++11", nullptr);
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

`MVC`即`Model、View、Controller`即模型、视图、控制器。





[理解`MVC`模式](https://zhuanlan.zhihu.com/p/35680070)

# 四、题目录入



