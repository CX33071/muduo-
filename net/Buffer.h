#pragma once
#include <vector>
#include <string>
namespace muduo{
    namespace net{
        class Buffer{
            public:
             const static size_t kCheapPrepend = 8;//头部预留8字节,用来在数据包前加长度，解除粘包
             const static size_t kInitialSize = 1024;//初始大小1k
             Buffer();//初始化一个空缓冲区，初始化readerIndex_和writeIndex_
             //三个核心大小查询
             size_t readableBytes() const;//可读数据长度
             size_t writeableBytes() const;//可写空间长度
             size_t prependableBytes() const;//头部预留空间
// readableBytes() = writerIndex_ - readerIndex_ writableBytes() =buffer.size() - writerIndex_
// prependableBytes() = readerIndex_
             void swap(Buffer& rhs);//交换两个缓冲区
             const char* peek() const;//获取读指针，返回第一个可读字节的指针begin()+readerIndex_
             //只移动指针，不删除数据
             void retrieve(size_t len);//读了len字节，指针往后挪len
             void retrieveUntil(const char* end);//读到某个位置
             void retrieveAll();//全部读完，指针复位
             std::string retrieveAllAsString();//取出数据，返回字符串。自动移动读指针
             std::string retrieveAsString(size_t len);
             void append(const char* data, size_t len);//往缓冲区写数据，空间不足则自动扩容
             void ensureWritableBytes(size_t len);//确保有足够空间可写，如果空间不够调用makeSpace扩容
             char* beginWrite();//返回当前可写位置的指针
             const char* beginWrite() const;
             void hasWritten(size_t len);//写完数据移动写指针
             void prepend(const void* data, size_t len);//头部插入数据的长度，解决粘包
             size_t internalCapacity() const;//内部容量
             ssize_t readFd(int, int* saveErrno);//从socket直接读到缓冲区，非阻塞IO必须用这个，一次性读尽可能多的数据
             private:
              char* begin();//缓冲区起始地址
              const char* begin() const;
              void makeSpace(size_t len);//扩容
              std::vector<char> buffer_;//底层内存(自动扩容)
              size_t readerIndex_;//读指针
              size_t writerIndex_;//写指针
        };
        }  // namespace net
}