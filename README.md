# 简介
一个简单的rpc通信框架，基于c++和protobuf实现。

# 使用说明
## 环境准备
protobuf、gdb、muduo、boost
## 使用准备
以一个最简单的获取frinedList功能为例介绍，介绍如何使用。

step1：rpc通信需要序列化和反序列化，这个过程依赖protobuf，因此先要编写protobuf所需的`protoc`文件。文件在example/friend.proto中。
注意：因为还使用到了protobuf提供的服务（stub）功能，因此需要设置：`option cc_generic_services = true;`

step2:使用protobuf生成pb文件:`protoc --cpp_out=./ friend.proto`，可以看到已经生成了对应的pb.h和pb.cc文件(`example/friend.pb.cc`)。 

step3：编写对应的friendservice.cc文件
```c++
#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include "logger.h"

class FriendService : public fixbug::FiendServiceRpc
{
public:
    std::vector<std::string> GetFriendsList(uint32_t userid)
    {
        std::cout << "do GetFriendsList service! userid:" << userid << std::endl;
        std::vector<std::string> vec;
        vec.push_back("gao yang");
        vec.push_back("liu hong");
        vec.push_back("wang shuo");
        return vec;
    }

    // 重写基类方法
    void GetFriendsList(::google::protobuf::RpcController* controller,
                       const ::fixbug::GetFriendsListRequest* request,
                       ::fixbug::GetFriendsListResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t userid = request->userid();
        std::vector<std::string> friendsList = GetFriendsList(userid);
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        for (std::string &name : friendsList)
        {
            std::string *p = response->add_friends();
            *p = name;
        }
        done->Run();
    }
};

int main(int argc, char **argv)
{
    // LOG_ERR("log ");
    LOG_INFO("log start");

    // 调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new FriendService());

    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}
```
step4: 编译文件，依次执行命令。
```bash
swx@swx-virtual-machine1:~/mprpc$ ls
autobuild.sh  bin  build  CMakeLists.txt  example  experiment  lib  README.md  src  test
swx@swx-virtual-machine1:~/mprpc$ cd build/
swx@swx-virtual-machine1:~/mprpc/build$ ls
CMakeCache.txt  CMakeFiles  cmake_install.cmake  example  Makefile  src
swx@swx-virtual-machine1:~/mprpc/build$ make clear
make: *** No rule to make target 'clear'.  Stop.
swx@swx-virtual-machine1:~/mprpc/build$ make clean
swx@swx-virtual-machine1:~/mprpc/build$ ls
CMakeCache.txt  CMakeFiles  cmake_install.cmake  example  Makefile  src
swx@swx-virtual-machine1:~/mprpc/build$ cmake ..
-- Configuring done
-- Generating done
-- Build files have been written to: /home/swx/mprpc/build
swx@swx-virtual-machine1:~/mprpc/build$ make
[  6%] Building CXX object src/CMakeFiles/mprpc.dir/mprpcapplication.cc.o
[ 13%] Building CXX object src/CMakeFiles/mprpc.dir/mprpcconfig.cc.o
[ 20%] Building CXX object src/CMakeFiles/mprpc.dir/rpcheader.pb.cc.o
[ 26%] Building CXX object src/CMakeFiles/mprpc.dir/rpcprovider.cc.o
[ 33%] Building CXX object src/CMakeFiles/mprpc.dir/mprpcchannel.cc.o
[ 40%] Building CXX object src/CMakeFiles/mprpc.dir/mprpccontroller.cc.o
[ 46%] Building CXX object src/CMakeFiles/mprpc.dir/logger.cc.o
[ 53%] Building CXX object src/CMakeFiles/mprpc.dir/zookeeperutil.cc.o
[ 60%] Linking CXX static library ../../lib/libmprpc.a
[ 60%] Built target mprpc
[ 66%] Building CXX object example/callee/CMakeFiles/provider.dir/friendservice.cc.o
[ 73%] Building CXX object example/callee/CMakeFiles/provider.dir/__/friend.pb.cc.o
[ 80%] Linking CXX executable ../../../bin/provider
[ 80%] Built target provider
[ 86%] Building CXX object example/caller/CMakeFiles/consumer.dir/callfriendservice.cc.o
[ 93%] Building CXX object example/caller/CMakeFiles/consumer.dir/__/friend.pb.cc.o
[100%] Linking CXX executable ../../../bin/consumer
[100%] Built target consumer
```
step5:在bin中已经生成了对应的服务提供者和消费者，运行即可。
# 通信原理解读

# todo列表
todo列表
- [] 通信原理解读
- [] 监控节点数据改变和节点 的删除客户端应该执行相应的操作。
- [x]客户端实现一致性hash算法实现达到负载均衡

> 果然不复习就会忘记，一段时间没碰这个项目了，感觉都要忘记完了。。。
> 正好借着写README的机会回顾一下项目。


