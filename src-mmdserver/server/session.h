#pragma once

#include "/opt/include/boost/beast/core.hpp"
#include "/opt/include/boost/beast/websocket.hpp"

#include <string>
#include <functional>

namespace mmd
{

class WSSession;

typedef std::function<void(std::shared_ptr<WSSession> ptr_session)> REMOVESESSIONHANDLE;

class WSSession : public std::enable_shared_from_this<WSSession>
{
public:
    explicit WSSession(boost::asio::ip::tcp::socket &&socket, REMOVESESSIONHANDLE&& remove_handler);
    virtual ~WSSession();

    void Run();
    void Close();

public:
    // 外部调用写入数据
    void WriteData(const std::string& data);
    void WriteData(const char *ptr_data, std::size_t len);
    // 子类派生  处理读取到的数据
    virtual void OnReadData(const char *ptr_data, std::size_t len);
    // 子类派生  处理读取到的数据
    virtual void OnError(int error_code);
    // 基类会将自己从server中移除  
    virtual void OnCloseComplete();
    // 新建连接后还需要accept才可以写入数据
    virtual void OnCanWrite();

protected:
    virtual void DoWrite(const char *ptr_buffer, std::size_t len);
    virtual void OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred);

    virtual void DoRead();
    virtual void OnRead(boost::beast::error_code ec, std::size_t bytes_transferred);

    virtual void OnAccept(boost::beast::error_code ec);
    virtual void PrintMsg(boost::beast::error_code ec, const std::string &what);

    virtual void OnClose(boost::beast::error_code const &ec);

private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> socket_;
    boost::beast::flat_buffer read_buffer_;

    std::mutex r_mutex_;
    std::string write_buffer_;

    std::atomic<bool> can_write_;
    std::atomic<bool> removed_;
    REMOVESESSIONHANDLE remove_handler_;
};
} // namespace mmd