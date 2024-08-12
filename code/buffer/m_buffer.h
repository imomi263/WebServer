#ifndef M_BUFFER_H
#define M_BUFFER_H
#include<cstring>
#include<iostream>
#include<unistd.h>
#include<sys/uio.h>
#include<vector>
#include<atomic>
#include<assert.h>
class Buffer{
public:
    Buffer(int initBuffersize=1024);
    ~Buffer()=default;

    //const 关键字表示这个成员函数
    //不会修改它所属对象的状态
    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;
    //看看读到哪里了
    const char* Peek() const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);

    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst()const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char* str,size_t len);
    void Append(const void* data,size_t len);
    void Append(const Buffer& buffer);

    //size_t 是一个无符号整数类型
    //ssize_t 是一个有符号整数类型
    ssize_t ReadFd(int fd,int *Errno);
    ssize_t WriteFd(int fd,int *Errno);

private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    std::vector<char>m_buffer;
    std::atomic<std::size_t> m_read_pos;
    std::atomic<std::size_t> m_write_pos;
};

#endif //M_BUFFER_H