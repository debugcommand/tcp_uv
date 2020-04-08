# 简介
基于libuv的tcp封装------->https://github.com/libuv/libuv

基于hiredis的tcp封装----->https://github.com/redis/hiredis

支持：x86、x64|windwos and linux

IDE：vs2017，会有CMake支持（敬请期待，有空再做 =.=）

目录介绍：

bin---------运行环境，编译后可执行程序会自动复制

library-----第三方库，暂时只有openssl和libuv

tcp_uv--|

      inc--头文件
      
      src--源文件
      
      uvnet--libuv封装
      
      redis--hiredis源文件
      
