用来实现无锁队列 改进日志模块

直接运行  ./run.sh 即可

注意打印出来的a并不是严格按照顺序的，因为取出和打印之间不是原子的，这两个操作之间其他线程可能已经取出来了内容。