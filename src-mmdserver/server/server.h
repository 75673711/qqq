#pragma once

#include "/opt/include/boost/beast/core.hpp"
#include "/opt/include/boost/beast/websocket.hpp"

#include <string>
#include <thread>
#include <vector>
#include <list>

namespace mmd
{
class WSSession;

class WSServer : public std::enable_shared_from_this<WSServer>
{
public:
    explicit WSServer(boost::asio::io_context &ioc);
    virtual ~WSServer();

    bool Listen(const std::string &addr, unsigned short port);
    void StopListen();

    virtual std::shared_ptr<WSSession> OnNewConnection(boost::asio::ip::tcp::socket socket);
    virtual void OnNewSession(const std::shared_ptr<WSSession> &ptr_session);

    void Clear();

    virtual void PrintMsg(boost::beast::error_code ec, const std::string &what);

    void AddSession(std::shared_ptr<WSSession> ptr_session);
    void RemoveSession(std::shared_ptr<WSSession> ptr_session);

protected:
    void DoAccept();
    void OnAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

private:
    std::list<std::shared_ptr<WSSession>> session_list_;
    std::vector<std::thread> thread_vec_;

    std::atomic<bool> is_listening_;
    std::size_t max_session_;
    std::size_t max_thread_;
    boost::asio::io_context &ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::endpoint end_point_;

    std::recursive_mutex re_mutex_;
    std::recursive_mutex list_mutex_;
};
} // namespace mmd