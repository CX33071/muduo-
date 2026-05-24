#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace mulib
{
    class noncopyable{
    public:
        noncopyable(const noncopyable &) = delete;
        noncopyable &operator=(const noncopyable &) = delete;

    protected:
        noncopyable() = default;
        ~noncopyable() = default;
    };
}

#endif // NONCOPYABLE_H

//禁止拷贝类，子类继承这个功能，避免拷贝类出现资源重复释放等
//禁止拷贝
//a=b编译器实际会翻译成a.operator(b),operator=是实现赋值的函数，禁止赋值，如果允许a=b,两个变量同时管理一个fd
//构造函数只给子类用，外部不能调用
//=default生成默认函数
