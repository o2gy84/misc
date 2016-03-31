#include <boost/asio.hpp>
#include <functional>
#include <iostream>
#include <thread>

#include <boost/bind.hpp>
//#include <boost/function.hpp>

enum { max_len = 1024 };

void sync()
{
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket s(io_service);
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query("localhost", "7789");

    boost::system::error_code ec; 
    auto endpoint = resolver.resolve(query, ec);
    if (ec)
        throw std::runtime_error(ec.message());

    boost::asio::connect(s, endpoint, ec);
    if (ec)
        throw std::runtime_error(ec.message());

    std::string hello = "LOGIN\r\n";
    boost::asio::write(s, boost::asio::buffer(hello, hello.size()), ec);

    char reply[max_len];
    //size_t reply_length = boost::asio::read_some(s, boost::asio::buffer(reply, max_len));
    s.read_some(boost::asio::buffer(reply, max_len), ec);
    std::cout << "Reply is: ";
    std::cout << reply << "\n";

}

class Client: public std::enable_shared_from_this<Client>
{
        boost::asio::ip::tcp::socket m_Sock;
        char m_Buf[1024];

    public:
        Client(boost::asio::io_service &io) : m_Sock(io) {}
        boost::asio::ip::tcp::socket& sock()             { return m_Sock; }
        void read()
        {   
            m_Sock.async_read_some(boost::asio::buffer(m_Buf),
                        boost::bind(&Client::handleRead, shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred)
            );  
        }
        void handleRead(const boost::system::error_code& e, std::size_t bytes_transferred)
        {
            if (e == boost::asio::error::eof)
            {
                std::cerr << "-client: " << m_Sock.remote_endpoint().address().to_string() << std::endl;
            }
            if (e) return;
            std::cerr << "read: " << bytes_transferred << " bytes" << std::endl;
            m_Sock.async_write_some(boost::asio::buffer(m_Buf),
                        [self = shared_from_this()](const boost::system::error_code& e, std::size_t bytes_transferred)->void
                        {
                            // После того, как запишем ответ, можно снова читать
                            self->read();
                        }
            );
        }
};

class Server {
    boost::asio::io_service m_Service;
    boost::asio::ip::tcp::acceptor m_Acceptor;

    void onAccept(std::shared_ptr<Client> c, const boost::system::error_code& e) {
        if (e) return;
        std::cerr << "+client: " << c->sock().remote_endpoint().address().to_string().c_str() << std::endl;
        c->read();
        startAccept();
    }
    void startAccept() {
        std::shared_ptr<Client> c(new Client(m_Service));
        m_Acceptor.async_accept(c->sock(), boost::bind(&Server::onAccept, this, c, boost::asio::placeholders::error));
    }
    void run() { m_Service.run(); }

public:
    Server() : m_Acceptor(m_Service)    {}

    void startServer() {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), 5001);
        m_Acceptor.open(endpoint.protocol());
        m_Acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_Acceptor.bind(endpoint);
        m_Acceptor.listen(1024);
        startAccept();

        std::vector<std::thread> threads;
        for (int i = 0; i < 4; ++i)
            threads.push_back(std::thread(std::bind(&Server::run, this)));
        for (auto &thread: threads)
            thread.join();
    }
};


void func() { std::cerr << "func1" << std::endl; }
struct Struct {
    void operator()() {std::cerr << "Struct::operator()" << std::endl; }
};

class Class : public std::enable_shared_from_this<Class>
{
    public:
        void funcA(int arg) { std::cerr << "Class::func().int: " << arg << std::endl; }
        void funcB(int arg1, const std::string &arg2) { std::cerr << "Class::func(). arg1: " << arg1 << ", arg2: " << arg2 << std::endl; }
        void funcC() { std::cerr << "Class::func2()" << std::endl; }

};

void queue_test()
{
    boost::asio::io_service io;
    io.post([]()->void { std::cerr << "lambda" << std::endl; });
    io.post(func);
    io.post(Struct());

    Class c;
    auto f1 = std::bind(&Class::funcC, &c);
    io.post(f1);

    std::shared_ptr<Class> c2 = std::make_shared<Class>();
    auto f2 = std::bind(&Class::funcC, c2->shared_from_this());
    io.post(f2);

    io.run_one();
    io.run_one();
    io.run_one();
    io.run_one();
    io.run_one();

    //io.run();
}

void bind_test()
{
    Class c;
    auto f1 = std::bind(&Class::funcA, &c, std::placeholders::_1);
    auto f2 = std::bind(&Class::funcB, &c, std::placeholders::_1, std::placeholders::_2);
    f1(123);
    f2(456, "abcd");

    std::shared_ptr<Class> c2 = std::make_shared<Class>();
    auto f3 = std::bind(&Class::funcB, c2->shared_from_this(), std::placeholders::_1, std::placeholders::_2);
    f3(789, "zxcv");

    auto f0 = std::bind(&Class::funcC, &c);
    f0();
}


int main(int argc, char *argv[])
{
    //sync();
    //queue_test();
    //
    Server serv;
    serv.startServer();
    return 0;
}

