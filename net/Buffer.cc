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
