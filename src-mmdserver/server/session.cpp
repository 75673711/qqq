#include "session.h"

#include "/opt/include/boost/asio/strand.hpp"

#include <iostream>

namespace mmd
{
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>

WSSession::WSSession(boost::asio::ip::tcp::socket &&socket, REMOVESESSIONHANDLE&& remove_handler)
    : socket_(std::move(socket)),
      can_write_(false),
      removed_(false),
      remove_handler_(remove_handler)
{
}
WSSession::~WSSession()
{
    std::cout << "WSSession destruction" << std::endl;
}

void WSSession::Run()
{
    socket_.binary(true);
    socket_.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    socket_.set_option(websocket::stream_base::decorator(
        [](websocket::response_type &res) {
            res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
        }));

    //Accept the websocket handshake
    socket_.async_accept(
        beast::bind_front_handler(
            &WSSession::OnAccept,
            shared_from_this()));
}

void WSSession::OnAccept(beast::error_code ec)
{
    if (ec)
    {
        PrintMsg(ec, "session accept");
        OnError(ec.value());
        return;
    }

    can_write_ = true;
    OnCanWrite();
    // Read a message
    DoRead();
}

void WSSession::Close()
{
    socket_.async_close(
        boost::beast::websocket::normal,
        beast::bind_front_handler(
            &WSSession::OnClose,
            shared_from_this()));
}

void WSSession::DoWrite(const char *ptr_buffer, std::size_t len)
{
    // net::make_strand只能保证回调函数原子执行  这里需要我们手动上锁
    std::lock_guard<std::mutex> locker(r_mutex_);
    if (can_write_)
    {
        // 在写操作完成前不能调用异步写操作
        std::cout << "write len " << len << std::endl;
        can_write_ = false;
        socket_.binary(true);      
        socket_.async_write(
            boost::asio::buffer(ptr_buffer, len),
            beast::bind_front_handler(
                &WSSession::OnWrite,
                shared_from_this()));
    }
    else
    {
        write_buffer_ += std::string(ptr_buffer, len);
    }
}

void WSSession::OnWrite(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec == websocket::error::closed)
    {
        PrintMsg(ec, "close");
        OnCloseComplete();
        return;
    }

    if (ec)
    {
        PrintMsg(ec, "write");
        OnError(ec.value());
        return;
    }

    std::lock_guard<std::mutex> locker(r_mutex_);
    if (write_buffer_.length() != 0)
    {
        std::cout << "write len " << write_buffer_.length() << std::endl;
        socket_.binary(true);      
        socket_.async_write(
            boost::asio::buffer(write_buffer_.c_str(), write_buffer_.length()),
            beast::bind_front_handler(
                &WSSession::OnWrite,
                shared_from_this()));
        write_buffer_ = std::string();
    }
    else
    {
        can_write_ = true;
    }
}

void WSSession::DoRead()
{
    socket_.async_read(
        read_buffer_,
        beast::bind_front_handler(
            &WSSession::OnRead,
            shared_from_this()));
}

void WSSession::OnRead(beast::error_code ec, std::size_t bytes_transferred)
{
    // This indicates that the session was closed
    if (ec == websocket::error::closed)
    {
        PrintMsg(ec, "close");
        OnCloseComplete();
        return;
    }

    if (ec)
    {
        PrintMsg(ec, "read fail");
        OnError(ec.value());
        return;
    }

    // Echo the message
    //socket_.text(socket_.got_text());
    socket_.binary(socket_.got_binary());

    OnReadData((const char *)(read_buffer_.cdata().data()), bytes_transferred);
    read_buffer_.consume(read_buffer_.size());
    DoRead();
}

void WSSession::OnClose(beast::error_code const &ec)
{
    OnCloseComplete();
}

/************************************************************************* */

void WSSession::WriteData(const std::string& data)
{
    DoWrite(data.c_str(), data.size());
}

void WSSession::WriteData(const char *ptr_data, std::size_t len)
{
    DoWrite(ptr_data, len);
}

void WSSession::OnReadData(const char *ptr_data, std::size_t len)
{
    std::cout << "read data: " << std::string(ptr_data, len).c_str() << std::endl;
}

void WSSession::OnError(int error_code)
{
    std::cout << "error happen and close manually" << std::endl;
    Close();
}

void WSSession::OnCloseComplete()
{
    // todo: remove from server
    if (!removed_)
    {
        std::shared_ptr<WSSession> ptr_session = shared_from_this();
        remove_handler_(ptr_session);
        removed_ = true;
    }
}

void WSSession::OnCanWrite()
{

}

void WSSession::PrintMsg(beast::error_code ec, const std::string &what)
{
    std::cout << what.c_str() << ": " << ec.message() << "\n";
}

} // namespace mmd