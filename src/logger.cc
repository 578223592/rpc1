#include "logger.h"
#include <time.h>
#include <iostream>

// 获取日志的单例
Logger& Logger::GetInstance()
{
    static Logger logger;  //懒汉式加载 对c++11新的static特性的使用
    return logger;
}

Logger::Logger()
{
    // 启动专门的写日志线程
// std::thread is a class in the C++ standard library that represents a single execution thread. It allows you to create a new thread of execution within your program, which can run in parallel with other threads.
// To use std::thread, you typically create an instance of the class and provide it with a function to execute. The function must have no arguments and return void. 
    std::thread writeLogTask([&]() -> void {
        for (;;)
        {
            // 获取当前的日期，然后取日志信息，写入相应的日志文件当中 a+
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char file_name[128];
            sprintf(file_name, "%d-%d-%d-log.txt", nowtm->tm_year+1900, nowtm->tm_mon+1, nowtm->tm_mday);

            FILE *pf = fopen(file_name, "a+");
            if (pf == nullptr)
            {
                std::cout << "logger file : " << file_name << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::string msg = m_lckQue.Pop();

            char time_buf[128] = {0};
            sprintf(time_buf, "%d:%d:%d =>[%s] ", 
                    nowtm->tm_hour, 
                    nowtm->tm_min, 
                    nowtm->tm_sec,
                    (m_loglevel == INFO ? "info" : "error"));
            msg.insert(0, time_buf);
            msg.append("\n");

            fputs(msg.c_str(), pf);
            fclose(pf);
            //todo：这里每写一次日志就会关闭文件，后续可以优化
        }
    });
    // 设置分离线程，守护线程
    writeLogTask.detach();
}

// 设置日志级别 
void Logger::SetLogLevel(LogLevel level)
{
    m_loglevel = level;
}

// 写日志， 把日志信息写入lockqueue缓冲区当中
void Logger::Log(std::string msg)
{
    m_lckQue.Push(msg);
}

void LOG_INFO(const char* logmsgformat,...){
    Logger &logger = Logger::GetInstance();
    logger.SetLogLevel(INFO); 
    char c[1024] = {0}; 
    va_list args;
    va_start(args,logmsgformat);
    vsprintf(c,logmsgformat,args);
    va_end(args);
    // 如果在使用可变参数函数后没有调用va_end，会导致程序出现未定义的行为，因为va_end用于释放可变参数列表所占用的资源，包括堆栈等。如果不调用va_end，这些资源可能会一直占用，导致内存泄漏或其他问题。
    logger.Log(c);
}

void LOG_ERR(const char *logmsgformat, ...)
{
    Logger &logger = Logger::GetInstance();
    logger.SetLogLevel(ERROR); 
    char c[1024] = {0}; 
    va_list args;
    va_start(args,logmsgformat);
    vsprintf(c,logmsgformat,args);
    va_end(args);
    // 如果在使用可变参数函数后没有调用va_end，会导致程序出现未定义的行为，因为va_end用于释放可变参数列表所占用的资源，包括堆栈等。如果不调用va_end，这些资源可能会一直占用，导致内存泄漏或其他问题。
    logger.Log(c);
}
