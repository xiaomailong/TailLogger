# TailLogger
使用一个缓存保存由CI发送过来的日志数据，当CI程序终止时可以及时保存程序退出时的日志信息

# 背景
联锁机正常运行会产生大量的无用的信息，这部分日志信息占用了很大部分的磁盘空间，为节省资源，使用TailLogger可以只记录程序异常退出时最后的日志信息，这样在节省磁盘空间的同时又保证了可以正确追踪程序异常退出的原因。

# 部署
在联锁程序的ci_profile.sh当中会有启动配置，如下：


```bash
# run tail logger
nohup /root/TailLogger/taillogger > /dev/null&

# run ci at background
CI_HOME="/root/ci"
cd ${CI_HOME}/build
# copy core if system crashed
./tar.sh > /dev/null&
# run ci
./ci -a > /dev/null&
```

该脚本会在开机后自动被加载。  

# 需要注意的问题
1. TailLogger默认使用`/root/ci_log`目录作为日志记录目录。
2. TailLogger使用的是一个循环的数组作为缓存，默认大小1M。
3. TailLogger使用的是Unix套接字，其服务端使用 `/tmp/CI_UNIX_SOCK_SRV` 作为套接字，联锁程序应该使用`/tmp/CI_UNIX_SOCK_CLIENT`作为客户端套接字。