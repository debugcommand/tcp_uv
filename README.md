# 简介
基于libuv的tcp封装------->https://github.com/libuv/libuv

基于hiredis的tcp封装----->https://github.com/redis/hiredis

支持：x86、x64|windwos and linux

IDE：vs2017 and CMakeLists.txt

git submodule update --init
目录介绍：

bin---------运行环境，编译后可执行程序会自动复制

library-----第三方库，暂时只有openssl和libuv

tcp_uv--|

      inc--头文件
      
      src--源文件
      
      uvnet--libuv封装
      
      redis--hiredis源文件

tcp server and client 测试结果：
最高连接数10000，未出现内存及CPU高峰值
