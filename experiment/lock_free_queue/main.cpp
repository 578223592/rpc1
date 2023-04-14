#include<vector>
#include<unistd.h>
#include<iostream>
#include<thread>
#include "lock_free.h"
LockFreeQueue<string> q;

void func1()
{
    std::vector<string> vec{"1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
    for(int i = 0; i < vec.size(); ++i)
        q.push(vec[i]);    
}

void func2()
{
    std::vector<string> vec{"11", "12", "13", "14", "15", "16", "17", "18", "19", "20"};
    for(int i = 0; i < vec.size(); ++i)
    {
        q.push(vec[i]);
    }
}

void func3()
{
    
    for(int i = 0; i < 10; ++i)
    {
        string a;
        while(!q.pop(a)){}
        std::cout << "func3 a = " << a << "\n";
    }
}

void func4()
{
    
    for(int i = 0; i < 10; ++i)
    {
        string a;
        while(!q.pop(a)){}
        std::cout << "func4 a = " << a << "\n";
    }
}

int main()
{
    std::thread t1(func1);
    std::thread t2(func2);
    std::thread t3(func3);
    std::thread t4(func4);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
}



// #include <iostream>
// #include <atomic>

// int main() {
//     std::atomic<int> counter(0);

//     int expected_value = 0;
//     int new_value = 1;

//     bool result = counter.compare_exchange_weak(expected_value, new_value);

//     if (result) {
//         std::cout << "Counter was " << expected_value << ", now set to " << new_value << std::endl;
//     } else {
//         std::cout << "Counter was not " << expected_value << ", did not update" << std::endl;
//     }

//     return 0;
// }