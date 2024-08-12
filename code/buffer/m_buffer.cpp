#include "m_buffer.h"

Buffer::Buffer(int initBuffersize):m_buffer(initBuffersize),m_read_pos(0),m_write_pos(0){}

size_t Buffer::ReadableBytes() const{
    return m_write_pos-m_read_pos;
}

size_t Buffer::WritableBytes()const{
    return m_buffer.size()-m_write_pos;
}

size_t Buffer::PrependableBytes()const{
    return m_read_pos;
}

const char* Buffer::Peek()const{
    return BeginPtr_()+m_read_pos;
}

void Buffer::Retrieve(size_t len){
    assert(len<=ReadableBytes());
    m_read_pos+=len;
}

void Buffer::RetrieveUntil(const char* end){
    assert(Peek()<=end);
    Retrieve(end-Peek());
}

void Buffer::RetrieveAll(){
    //set n bytes to 0
    bzero(&m_buffer[0],m_buffer.size());
    m_read_pos=0;
    m_write_pos=0;
}

std::string Buffer::RetrieveAllToStr(){
    std::string str(Peek(),ReadableBytes());
    RetrieveAll();
    return str;
}

const char* Buffer::BeginWriteConst()const{
    return BeginPtr_()+m_write_pos;
}

char *Buffer::BeginWrite(){
    return BeginPtr_()+m_write_pos;
}

void Buffer::HasWritten(size_t len){
    m_write_pos+=len;
}

void Buffer::Append(const std::string& str){
    Append(str.data(),str.length());
}
void Buffer::Append(const void* data,size_t len){
    assert(data);
    Append(static_cast<const char*>(data),len);
}

void Buffer::Append(const char* str,size_t len){
    assert(str);
    EnsureWritable(len);
    std::copy(str,str+len,BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const Buffer& buffer){
    Append(buffer.Peek(),buffer.ReadableBytes());
}

void Buffer::EnsureWritable(size_t len){
    if(WritableBytes()<len){
        MakeSpace_(len);
    }
    assert(WritableBytes()>=len);
}   

ssize_t Buffer::ReadFd(int fd,int* saveErrno){
    char buff[65535];
    /*
    iovec 结构体是 POSIX 标准中定义的一个数据结构
    用于高效地进行内存到内存或内存到 I/O（输入/输出）的传输。
    */
    struct iovec iov[2];
    const size_t writable=WritableBytes();
    

    iov[0].iov_base=BeginPtr_()+m_write_pos;
    iov[0].iov_len=writable;

    iov[1].iov_base=buff;
    iov[1].iov_len = sizeof(buff);
    // 先写满第一个缓冲区，然后写第二个缓冲区。
    const ssize_t len=readv(fd,iov,2);

    if(len<0) *saveErrno=errno;
    else if(static_cast<size_t>(len)<=writable) m_write_pos+=len;
    else {
        // 已经写满了第一个缓冲区
        m_write_pos=m_buffer.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd,int* saveErrno){
    size_t readSize=ReadableBytes();
    ssize_t len=write(fd,Peek(),readSize);
    if(len<0){
        *saveErrno=errno;
        return len;
    }
    m_read_pos+=len;
    return len;
}

char* Buffer::BeginPtr_(){
    return &*m_buffer.begin();
}

const char* Buffer::BeginPtr_()const{
    return &*m_buffer.begin();
}

void Buffer::MakeSpace_(size_t len){
    if(WritableBytes()+PrependableBytes()<len){
        m_buffer.resize(m_write_pos+len+1);
    }
    else{
        // 向前挪动
        size_t readable=ReadableBytes();
        // first,last,result;
        std::copy(BeginPtr_()+m_read_pos,BeginPtr_()+m_write_pos,BeginPtr_());
        m_read_pos=0;
        m_write_pos=readable;
        assert(readable==ReadableBytes());
    }
}