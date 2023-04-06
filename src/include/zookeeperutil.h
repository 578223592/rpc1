#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>
#include <string>
#include <vector>
static const int32_t zoo_path_buf_len = 1024;
static const int32_t zoo_value_buf_len = 10240;

namespace details
{
    static const char *state_to_string(int state)
    {
        if (state == 0)
            return "zoo_closed_state";
        if (state == ZOO_CONNECTING_STATE)
            return "zoo_state_connecting";
        if (state == ZOO_ASSOCIATING_STATE)
            return "zoo_state_associating";
        if (state == ZOO_CONNECTED_STATE)
            return "zoo_state_connected";
        if (state == ZOO_EXPIRED_SESSION_STATE)
            return "zoo_state_expired_session";
        if (state == ZOO_AUTH_FAILED_STATE)
            return "zoo_state_auth_failed";

        return "zoo_state_invalid";
    }

    static const char *type_to_string(int state)
    {
        if (state == ZOO_CREATED_EVENT)
            return "zoo_event_created";
        if (state == ZOO_DELETED_EVENT)
            return "zoo_event_deleted";
        if (state == ZOO_CHANGED_EVENT)
            return "zoo_event_changed";
        if (state == ZOO_CHILD_EVENT)
            return "zoo_event_child";
        if (state == ZOO_SESSION_EVENT)
            return "zoo_event_session";
        if (state == ZOO_NOTWATCHING_EVENT)
            return "zoo_event_notwatching";

        return "zoo_event_unknow";
    }

}
/** error code */
enum zoo_rc
{
    z_ok = 0, /*!< Everything is OK */

    /** System and server-side errors.
     * This is never thrown by the server, it shouldn't be used other than
     * to indicate a range. Specifically error codes greater than this
     * value, but lesser than {@link #api_error}, are system errors. */
    z_system_error = -1,
    z_runtime_inconsistency = -2, /*!< A runtime inconsistency was found */
    z_data_inconsistency = -3,    /*!< A data inconsistency was found */
    z_connection_loss = -4,       /*!< Connection to the server has been lost */
    z_marshalling_error = -5,     /*!< Error while marshalling or unmarshalling data */
    z_unimplemented = -6,         /*!< Operation is unimplemented */
    z_operation_timeout = -7,     /*!< Operation timeout */
    z_bad_arguments = -8,         /*!< Invalid arguments */
    z_invliad_state = -9,         /*!< Invliad zhandle state */

    /** API errors.
     * This is never thrown by the server, it shouldn't be used other than
     * to indicate a range. Specifically error codes greater than this
     * value are API errors (while values less than this indicate a
     * {@link #system_error}).
     */
    z_api_error = -100,
    z_no_node = -101,                   /*!< Node does not exist */
    z_no_auth = -102,                   /*!< Not authenticated */
    z_bad_version = -103,               /*!< Version conflict */
    z_no_children_for_ephemeral = -108, /*!< Ephemeral nodes may not have children */
    z_node_exists = -110,               /*!< The node already exists */
    z_not_empty = -111,                 /*!< The node has children */
    z_session_expired = -112,           /*!< The session has been expired by the server */
    z_invalid_callback = -113,          /*!< Invalid callback specified */
    z_invalid_acl = -114,               /*!< Invalid ACL specified */
    z_auth_failed = -115,               /*!< Client authentication failed */
    z_closing = -116,                   /*!< ZooKeeper is closing */
    z_nothing = -117,                   /*!< (not error) no server responses to process */
    z_session_moved = -118              /*!<session moved to another server, so operation is ignored */
};
/** zoo node info */
struct zoo_state_t
{
    int64_t ctime;          // node create time
    int64_t mtime;          // node last modify time
    int32_t version;        // node version
    int32_t children_count; // the number of children of the node
};

class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    noncopyable(const noncopyable &);
    noncopyable &operator=(const noncopyable &);
};

// 封装的zk客户端类
/*
本项目中zookeeper是用作服务发现，因此在provider运行的时候才会注册服务（provider运行run函数的时候）

*/
class ZkClient : public noncopyable
{
public:
    ZkClient();
    ~ZkClient();
    // zkclient启动连接zkserver
    void Start();
    

    /// @brief // 在zkserver上根据指定的path创建znode节点 
    /// @param path 
    /// @param data 
    /// @param datalen 
    /// @param state 默认为0（永久节点）
    void Create(const char *path, const char *data, int datalen, int state = 0);
    // 根据参数指定的znode节点路径，获取znode节点的值
    std::string GetData(const char *path);
    // 获取节点值
    zoo_rc get_node(const char *path, std::string &out_value, bool watch);
    /// @brief children不是全路径，是短路径
    /// @param path 
    /// @param children 
    /// @param watch 
    /// @return 
    zoo_rc get_children(const char *path, std::vector<std::string> &children, bool watch);
    zoo_rc delete_node(const char *path, int32_t version = -1);
    zoo_rc exists_node(const char *path, zoo_state_t *info = nullptr, bool watch = true);
    /**
     * @brief request watch the path's data change event
     *
     * @param   path            - the node name
     * @param   handler         - the path's data change callback fuction
     * @param   value           - return the node's name if not NULL
     *
     * @return  see {@link #get_node}
     */
    // todo:还未完成，涉及了data_change_event_handler_t，尽量去除掉
    // zoo_rc watch_data_change(const char *path, const data_change_event_handler_t &handler, std::string *value);
    // // 设置节点的子节点变化(增/减)的通知回调函数
    // zoo_rc watch_children_event(const char *path, const child_event_handler_t &handler, std::vector<std::string> *out_children);
    // todo:待完成

    /// @brief 节点删除回调
    /// @param path 
    void on_path_delete(const char *path);
    /// @brief 节点数据改变回调，完成：重新注册watcher和执行其他业务
    /// @param path 
    void on_path_data_change(const char *path);
    void on_path_child_change(const char* path);
private:
    // zk的客户端句柄
    zhandle_t *m_zhandle;
};

struct Context
{
    sem_t* m_sem;
    ZkClient *m_zkclient;
};