#include "server.h"

#include "session.h"

#include "/opt/include/boost/asio/strand.hpp"

#include <iostream>
#include <boost/bind.hpp>

namespace mmd
{

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

#define RECLOCK std::lock_guard<std::recursive_mutex> locker(re_mutex_);
#define LISTLOCK std::lock_guard<std::recursive_mutex> locker(list_mutex_);

WSServer::WSServer(boost::asio::io_context& ioc)
    : is_listening_(false),
      max_session_(5000),
      max_thread_(4),
      ioc_(ioc),
      //ioc_(net::io_context{max_thread_}),
      acceptor_(ioc)
{
}

WSServer::~WSServer()
{
    {
        LISTLOCK
        for (auto it : session_list_)
        {
            it->Close();
        }
        session_list_.clear();
    }

    for (auto &it : thread_vec_)
    {
        if (it.joinable())
        {
            it.join();
            it.detach();
        }
    }
    thread_vec_.clear();
}

bool WSServer::Listen(const std::string &addr, unsigned short port)
{
    RECLOCK

    if (is_listening_)
        return true;

    auto address = net::ip::make_address(addr);

    end_point_ = tcp::endpoint{address, port};

    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(end_point_.protocol(), ec);
    if (ec)
    {
        PrintMsg(ec, "open");
        return false;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
    {
        PrintMsg(ec, "set_option");
        return false;
    }

    // Bind to the server address
    acceptor_.bind(end_point_, ec);
    if (ec)
    {
        PrintMsg(ec, "bind");
        return false;
    }

    // Start listening for connections
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        PrintMsg(ec, "listen");
        return false;
    }

    is_listening_ = true;

    DoAccept();

    thread_vec_.reserve(max_thread_);
    for (auto i = max_thread_; i > 0; --i)
    {
        thread_vec_.emplace_back([this] {
            ioc_.run();
        });
    }

    return is_listening_;
}

void WSServer::DoAccept()
{
    acceptor_.async_accept(
        net::make_strand(ioc_),  //net::make_strand保证回调操作原子性  且是队列执行方式，提高性能
        beast::bind_front_handler(
            &WSServer::OnAccept,
            shared_from_this()));
}

void WSServer::OnAccept(beast::error_code ec, tcp::socket socket)
{
    if (ec)
    {
        PrintMsg(ec, "server accept");
    }
    else
    {
        // Create the session and run it
        //std::shared_ptr<WSSession> ptr_session = std::make_shared<WSSession>(std::move(socket));
        std::shared_ptr<WSSession> ptr_session = OnNewConnection(std::move(socket));
        AddSession(ptr_session);
        OnNewSession(ptr_session);
    }

    if (is_listening_)
    {
        DoAccept();
    }
}

void WSServer::AddSession(std::shared_ptr<WSSession> ptr_session)
{
    std::cout << "AddSession  --------------" << std::endl;
    LISTLOCK
    session_list_.push_back(ptr_session);
}

void WSServer::RemoveSession(std::shared_ptr<WSSession> ptr_session)
{
    std::cout << "RemoveSession  --------------" << std::endl;
    LISTLOCK
    session_list_.remove(ptr_session);
}

void WSServer::StopListen()
{
    RECLOCK
    if (is_listening_)
    {
        is_listening_ = false;
        acceptor_.close();
    }
}

std::shared_ptr<WSSession> WSServer::OnNewConnection(boost::asio::ip::tcp::socket socket)
{
    return std::make_shared<WSSession>(std::move(socket), boost::bind(&WSServer::RemoveSession, shared_from_this(), _1));
}

void WSServer::OnNewSession(const std::shared_ptr<WSSession> &ptr_session)
{
    ptr_session->Run();
}

void WSServer::Clear()
{
}

void WSServer::PrintMsg(beast::error_code ec, const std::string &what)
{
    std::cout << what.c_str() << ": " << ec.message() << "\n";
}
} // namespace mmd