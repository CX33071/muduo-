#ifndef MUDUO_NET_BUFFER_H
#define MUDUO_NET_BUFFER_H

#include <vector>
#include <string>

namespace mulib{
    namespace net{
        class Buffer{
        public:
            const static size_t kCheapPrepend = 8;
            const static size_t kInitialSize = 1024;
            Buffer();
            size_t readableBytes() const;
            size_t writableBytes() const;
            size_t prependableBytes() const;

            void swap(Buffer &rhs);
            const char *peek() const; // 返回当前可读数据的指针

            void retrieve(size_t len);
            void retrieveUntil(const char *end);
            void retrieveAll();

            std::string retrieveAllAsString();
            std::string retrieveAsString(size_t len);

            void append(const char* data,size_t len);
            void ensureWritableBytes(size_t len);

            char* beginWrite();
            const char *beginWrite() const;
            void hasWritten(size_t len);

            void prepend(const void *data, size_t len);
            size_t internalCapacity() const;
            ssize_t readFd(int, int *saveErrno);
        
        private:
            char *begin();

            const char *begin() const;

            void makeSpace(size_t len);

            std::vector<char> buffer_;
            size_t readerIndex_; // 读指针，指向当前可读数据的起始位置
            size_t writerIndex_; // 写指针，指向当前可写数据的起始位置
        };
    }
}

#endif//

//头部预留8字节,用来在数据包前加长度，解除粘包
//初始大小1k
//初始化一个空缓冲区，初始化readerIndex_和writeIndex_
//三个核心大小查询
//可读数据长度
//可写空间长度
//头部预留空间
// readableBytes() = writerIndex_ - readerIndex_ writableBytes() =buffer.size() - writerIndex_
// prependableBytes() = readerIndex_
//交换两个缓冲区
//获取读指针，返回第一个可读字节的指针begin()+readerIndex_
//只移动指针，不删除数据
//读了len字节，指针往后挪len
//读到某个位置
//全部读完，指针复位
//取出数据，返回字符串。自动移动读指针
//往缓冲区写数据，空间不足则自动扩容
//确保有足够空间可写，如果空间不够调用makeSpace扩容
//返回当前可写位置的指针
//写完数据移动写指针
//头部插入数据的长度，解决粘包
//内部容量
//从socket直接读到缓冲区，非阻塞IO必须用这个，一次性读尽可能多的数据
//缓冲区起始地址
//扩容
//底层内存(自动扩容)
//读指针
//写指针
// namespace net
