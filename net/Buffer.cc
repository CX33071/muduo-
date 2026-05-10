#include "Buffer.h"
#include <assert.h>
#include <algorithm>
#include <sys/uio.h>
using namespace muduo::net;
Buffer::Buffer():buffer_(kCheapPrepend+kInitialSize),readerIndex_(kCheapPrepend),writerIndex_(kCheapPrepend){
    assert(readableBytes() == 0);//assert如果条件不满足就打印信息然后退出程序
    assert(writeableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
}
size_t Buffer::readableBytes()const{
    return writerIndex_ - readerIndex_;
}
size_t Buffer::writeableBytes()const{
    return buffer_.size() - writerIndex_;
}
size_t Buffer::prependableBytes()const{
    return readerIndex_;
}
void Buffer::swap(Buffer &rhs){
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}
char*Buffer::begin(){
    return &*buffer_.begin();
}
const char*Buffer::begin()const{
    return &*buffer_.begin();
}
const char*Buffer::peek()const{
    return begin() + readerIndex_;
}
void Buffer::retrieve(size_t len){
    assert(len <= readableBytes());
    readerIndex_ += len;
}
void Buffer::retrieveUntil(const char*end){
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
}
void Buffer::retrieveAll(){
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}
std::string Buffer::retrieveAllAsString(){
    return retrieveAsString(readableBytes());
}
std::string Buffer::retrieveAsString(size_t len){
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
}
void Buffer::ensureWritableBytes(size_t len){
    if(writeableBytes()<len){
        makeSpace(len);
    }
    assert(writeableBytes() >= len);
}
void Buffer::append(const char*data,size_t len){
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}
char*Buffer::beginWrite(){
    return begin() + writerIndex_;
}
const char*Buffer::beginWrite()const{
    return begin() + writerIndex_;
}
void Buffer::hasWritten(size_t len){
    writerIndex_ += len;
}
void Buffer::prepend(const void *data,size_t len){
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + readerIndex_);
}
size_t Buffer::internalCapacity()const{
    return buffer_.capacity();
}
//一次性从socket读到缓冲区，空间不够就用栈临时存
ssize_t Buffer::readFd(int fd,int *saveErrno){//出错把错误码存在saveErrno
    char extrabuf[65535];//在栈上开64KB临时缓冲区，Buffer满了临时放在这
    struct iovec vec[2];//readv要用的分散读数组，可以一次读数据到两块不同内存
    const size_t writable = writeableBytes();//缓冲区当前可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;//第一块内存：Buffer自己的可写区域
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);//第二块内存：栈上临时缓冲区
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;//如果Buffer空间<64KB,用两块内存(Buffer+栈)，否则只用Buffer
    const ssize_t n = ::readv(fd, vec, iovcnt);//一次系统调用，把数据读到两块内存，速度比read块很多，数据先放vec[0],满了自动放vec[1]
    if(n<0){
        *saveErrno = errno;
    } else if (static_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    }else{
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);//数据装不下放在extrabuf里，调用append把栈上数据搬进Buffer,Buffer自动扩容
    }
    return n;
}
//makeSpace要么整体扩容，要么把数据挪到最前面，腾出后面空间
void Buffer::makeSpace(size_t len){
    if(writeableBytes()+prependableBytes()<len+kCheapPrepend){//后面剩余空间+前面已读空间
        buffer_.resize(writerIndex_ + len);
    }else{
        assert(kCheapPrepend < readerIndex_);
        std::copy(begin() + readerIndex_, begin() + writerIndex_,
                  begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readableBytes();
        assert(readable == readableBytes());
    }
}
