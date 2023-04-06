#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
					int state, const char *path, void *watcherCtx)
{

	Context * contextPtr = (Context *)zoo_get_context(zh);
	ZkClient *zkClient = contextPtr->m_zkclient;
	if (type == ZOO_SESSION_EVENT) // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE) // zkclient和zkserver连接成功
		{
			// sem_t *sem = (sem_t*)zoo_get_context(zh); // 把注册的上下文信息拿回来，原来注册了信号量
			// sem_t *sem = &(((Context *)zoo_get_context(zh))->m_sem); // 好像zoo_get_context只能拿回一次消息，第二次调用是拿不回来的
			sem_t *sem = contextPtr->m_sem;
			sem_post(sem);											 // post信号量
		}
	}
	else if (type == ZOO_CREATED_EVENT)
	{
		std::cout << "ZOO_CREATED_EVENT   path:;" << path << std::endl;
		// zkClient->on_path_created(path);
	}
	else if (type == ZOO_DELETED_EVENT)
	{
		std::cout << "ZOO_DELETED_EVENT   path:;" << path << std::endl;
		zkClient->on_path_delete(path);
	}
	else if (type == ZOO_CHANGED_EVENT)
	{
		std::cout << "ZOO_CHANGED_EVENT   path:" << path << std::endl;
		zkClient->on_path_data_change(path);
	}
	else if (type == ZOO_CHILD_EVENT)
	{
		std::cout << "ZOO_CHILD_EVENT   path:" << path << std::endl;
		zkClient->on_path_child_change(path);
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)
{
}

ZkClient::~ZkClient()
{
	if (m_zhandle != nullptr)
	{
		zookeeper_close(m_zhandle); // 关闭句柄，释放资源  MySQL_Conn
	}
}

// 连接zkserver
void ZkClient::Start()
{
	std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
	std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
	std::string connstr = host + ":" + port;

	/*
	zookeeper_mt：多线程版本
	zookeeper的API客户端程序提供了三个线程
	API调用线程
	网络I/O线程  pthread_create  poll
	watcher回调线程 pthread_create
	*/
	m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0);
	if (nullptr == m_zhandle)
	{
		std::cout << "zookeeper_init error!" << std::endl;
		exit(EXIT_FAILURE);
	}
	// 只是创建句柄 成功 ， 不代表 连接zookeeper成功  --因为是异步的
	sem_t sem;
	sem_init(&sem, 0, 0);

	Context c;
	c.m_sem = &sem;
	c.m_zkclient = this;
	zoo_set_context(m_zhandle, &c); // 给指定的zookeeper添加上下文，可以理解成让zookeeper带了一些信息

	sem_wait(&sem); // 调用肯定会阻塞，阻塞到调用回调global_watcher（回调里面写了post）
	// 也应该阻塞，阻塞了才往下走呀
	std::cout << "zookeeper_init success!" << std::endl;
}
// 创建节点，节点名，存放的数据，数据长度，类型（永久节点还是临时节点）
void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
	char path_buffer[128];
	int bufferlen = sizeof(path_buffer);
	int flag = 0;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了
		flag = zoo_create(m_zhandle, path, data, datalen,
						  &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen); // 这个api还不是很清楚
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path_buffer << std::endl;
		}
		else
		{
			std::cout << "znode create error... path:" << path_buffer <<  "  flag:" << flag<< std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
	char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}

zoo_rc ZkClient::get_node(const char *path, std::string &out_value, bool watch)
{
	struct Stat s = {0};

	char buf[zoo_value_buf_len] = {0};
	int buf_size = sizeof(buf);
	char *data_buffer = buf;
	zoo_rc rt = (zoo_rc)zoo_get((zhandle_t *)m_zhandle, path, watch, data_buffer, &buf_size, &s);
	do
	{
		if (rt != z_ok)
		{
			break;
		}

		// check data complete
		if (s.dataLength > buf_size)
		{
			data_buffer = (char *)malloc(s.dataLength + 1);
			buf_size = s.dataLength;
			rt = (zoo_rc)zoo_get((zhandle_t *)m_zhandle, path, watch, data_buffer, &buf_size, &s);
			if (rt != z_ok)
			{
				break;
			}
		}

		out_value = std::move(std::string(data_buffer, buf_size));
	} while (0);

	// try free buffer
	if (data_buffer != buf)
	{
		free(data_buffer);
	}

	return rt;
}

zoo_rc ZkClient::get_children(const char *path, std::vector<std::string> &children, bool watch)
{
	String_vector string_v;
	zoo_rc rt = (zoo_rc)zoo_get_children((zhandle_t *)m_zhandle, path, watch, &string_v);
	if (rt != z_ok)
	{
		return rt;
	}

	for (int32_t i = 0; i < string_v.count; ++i)
	{
		children.push_back(string_v.data[i]);
	}

	return rt;
}

zoo_rc ZkClient::delete_node(const char *path, int32_t version)
{
	return (zoo_rc)zoo_delete((zhandle_t *)m_zhandle, path, version);
}

zoo_rc ZkClient::exists_node(const char *path, zoo_state_t *info, bool watch)
{
	struct Stat s = {0};

	zoo_rc rt = (zoo_rc)zoo_exists((zhandle_t *)m_zhandle, path, (int)watch, &s);

	if (info)
	{
		std::cerr << "not implent info is not nullptr in exists_node" << std::endl;
		assert(false);
	}

	return rt;
}

void ZkClient::on_path_delete(const char *path)
{
	char buff[1024] = {0};
	sprintf(buff, " zk_cpp::on_path_delete, path[%s]\n", path);
	std::cout << buff << std::endl;
}

void ZkClient::on_path_data_change(const char *path)
{
	char buff[1024] = {0};
	sprintf(buff, " zk_cpp::on_path_data_change, path[%s]\n", path);
	std::cout << buff << std::endl;

	std::string outValue;
	zoo_rc rt = get_node(path, outValue, true);
	if (rt != z_ok)
	{
		std::cerr << "get_node error ~~~" << std::endl;
		return;
	}
	// todo:重新读取节点数据写入hash环
}

void ZkClient::on_path_child_change(const char *path)
{
	std::vector<std::string> children;
	bool watch = true;
	zoo_rc rt = get_children(path, children, watch);
	if (rt != z_ok)
	{
		std::cerr << "get_children error ~~~" << std::endl;
		return;
	}
	//todo 根据子节点重新写hash环
}
