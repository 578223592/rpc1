#include <string>
#include <iostream>
#include <functional>
#include <algorithm>
#include <vector>
#include <map>
#include <algorithm> // 包含 std::generate_n() 和 std::generate() 函数的头文件
#include <random> // 包含 std::uniform_int_distribution 类型的头文件
using namespace std;
// 32 位的 Fowler-Noll-Vo 哈希算法
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
static uint32_t FNVHash(std::string key) {
    const int p = 16777619;
    uint32_t hash = 2166136261;
    for (int idx = 0; idx < key.size(); ++idx) {
        hash = (hash ^ key[idx]) * p;
    }
    hash += hash << 13;
    hash ^= hash >> 7;
    hash += hash << 3;
    hash ^= hash >> 17;
    hash += hash << 5;
    if (hash < 0) {
        hash = -hash;
    }
    return hash;
}
static int hs(string s)
{
    // return abs((int32_t)hash<string>()(s) % INT32_MAX); // 使用std::hash函数从任意对象得到hash值
    return FNVHash(s);
};

/// @brief 服务节点
class Node
{
private:
    string NodeAddr;
    vector<Node> CliNodesAddr;  //服务节点采用这个值，客户端是用不到这个变量
    // size_t NodeHashValue;
public:
    Node(){};
    Node(string _addr) : NodeAddr(_addr)
    {
    }
    string getNode()
    {
        return NodeAddr;
    }
    void setNodeAddr(string _addr)
    {
        this->NodeAddr = _addr;
    }
    void addCliNode(Node cli)
    {
        CliNodesAddr.push_back(cli);
    }
    void printNodeState()
    {
        cout << "当前节点地址：" << NodeAddr << "   哈希值：" << hs(NodeAddr) << endl;
        for (Node &n : CliNodesAddr)
        {
            cout << "客户端节点地址：" << n.getNode() << "   哈希值：" << hs(n.getNode()) << endl;
        }
        cout << endl
             << endl
             << endl;
    }
};

class HashRing
{
public:
    HashRing(int _virtualNodeNum = 100) : m_virtualNodeNum(_virtualNodeNum)
    {
    }
    void addNode(string _addr)
    {
        Node newNode(_addr);
        for (int i = 0; i < m_virtualNodeNum; i++)
        {
            size_t jump = INT32_MAX / m_virtualNodeNum;
            m_ServerNodes.insert(make_pair(hs(_addr +to_string(jump * i)), newNode)); // 移动拷贝！！！
        }
        
        m_nodeNum.insert(make_pair(_addr,0));
    }
    void removeNode(string _addr)
    {
        for (int i = 0; i < m_virtualNodeNum; i++)
        {
            size_t jump = INT32_MAX / m_virtualNodeNum;
            size_t hashvalue = hs(_addr) + jump * i;
            m_ServerNodes.erase(hashvalue);
        }
        m_nodeNum.erase(_addr);
    }
    void distributionNode(string _addr)
    {
        auto it = m_ServerNodes.lower_bound(hs(_addr));
        if (it == m_ServerNodes.end())
        {
            it = m_ServerNodes.begin();
        }
        it->second.addCliNode(Node(_addr));
        m_nodeNum[it->second.getNode()]++;
    }
    void printHashRingState()
    {
        // for (auto it = m_ServerNodes.begin(); it != m_ServerNodes.end(); ++it)
        // {
        //     it->second.printNodeState();
        // }
        for(auto it = m_nodeNum.begin(); it!= m_nodeNum.end(); ++it){
            cout<<"server node address:"<<it->first<<" has client num:"<<it->second<<endl;
        }
    }

private:
    map<size_t, Node> m_ServerNodes; //hash:node
    map<string, size_t> m_nodeNum; // node ip:node_client_num
    int m_virtualNodeNum;
};



std::string generate_random_string(int n) {
    std::string str;
    str.resize(n);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dist(0, 61); // 均匀分布的随机数生成器，可以生成 0~61 的整数
    std::generate_n(str.begin(), n, [&]() -> char {
        int r = dist(gen);
        if (r < 10) { // 数字
            return '0' + r;
        } else if (r < 36) { // 大写字母
            return 'A' + r - 10;
        } else { // 小写字母
            return 'a' + r - 36;
        }
    });
    return str;
}
int main()
{
    HashRing hr;
    hr.addNode("192.168.0.1");
    hr.addNode("192.168.10.2");
    hr.addNode("192.168.20.3");
    hr.addNode("192.168.30.4");
    
    for(int i = 0 ;i< 10000;i++){
        hr.distributionNode(generate_random_string(5));
    }
    hr.printHashRingState();
    system("pause");
    return 0;
}
