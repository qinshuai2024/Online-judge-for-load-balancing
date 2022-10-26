#ifndef COMPILER_ONLINE
#include "header.cpp" // 仅仅为了写测试用例的时候引入对应类，不报错，真正编译的时候需要去掉
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