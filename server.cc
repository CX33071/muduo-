#include <muduo/net/Buffer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include <iostream>
#include <string>
#include <unordered_map>

class DictServer {
   public:
    DictServer(int port)
        /* 构造muduo::net::TcpServer _server。
        第一个参数EventLoop* loop:
        用于处理服务器接收到的连接请求以及后续的数据读写事件。
        其中，这个EventLoop对象是在服务器的主线程中创建和运行的。
        第二个参数const InetAddress& listenAddr: 这是服务器监听的地址。
        其中，InetAddress封装了IP地址和端口号，用于指定服务器应该监听哪个网络接口和端口。
        第三个参数const string& nameArg: 服务器的名称。
        第四个参数Option option = kNoReusePort: 指定服务器套接字的一些选项。
        kNoReusePort是默认值，表示不允许端口重用，若设置为kReusePort，表示允许端口重用。
        */
        : _server(&_baseloop,
                  muduo::net::InetAddress("0.0.0.0", port),
                  "DictServer",
                  muduo::net::TcpServer::kReusePort) {
        // 设置连接事件（连接建立/管理）的回调，注意，由于类的非静态成员函数有this指针，所有这里要bind设计一下
        _server.setConnectionCallback(
            std::bind(&DictServer::onConnection, this, std::placeholders::_1));
        // 设置连接消息的回调，同理，由于类的非静态成员函数有this指针，所有这里要bind设计一下
        _server.setMessageCallback(
            std::bind(&DictServer::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));
    }
    void start() {
        _server.start();   // 先开始监听
        _baseloop.loop();  // 然后loop内部死循环事件监控
    }

   private:
    // 定义连接建立成功或失败的回调函数，参数为TcpConnectionPtr类型。
    void onConnection(const muduo::net::TcpConnectionPtr& conn) {
        if (conn->connected())
            std::cout << "连接建立！\n";
        else
            std::cout << "连接断开！\n";
    }
    // 定义发送信息时的回调函数，参数有如下类型
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buf,
                   muduo::Timestamp) {
        static std::unordered_map<std::string, std::string> dict_map = {
            {"hello", "你好"}, {"world", "世界"}, {"bite", "比特"}};
        std::string msg = buf->retrieveAllAsString();
        std::string res;
        auto it = dict_map.find(msg);
        if (it != dict_map.end())
            res = it->second;
        else
            res = "未知单词！";
        conn->send(res);
    }

   private:
    // 注意：_baseloop一定要放在_server上面，因为_server是用_baseloop参数来构造的
    muduo::net::EventLoop _baseloop;
    muduo::net::TcpServer _server;
};

int main() {
    DictServer server(8888);
    server.start();
    return 0;
}