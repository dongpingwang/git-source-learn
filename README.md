
# 源码分析：git第一个提交是怎么样的？

说明：第一个版本与后续版本差别很大，本文不讨论git内部设计，感兴趣的可以参考我之前写的一篇博客(https://blog.csdn.net/wangadping/article/details/126337953)

目前正在学习c/cpp，git第一次提交源码很少，总代码行数大约1k来行，当做学习c语言

## 切换到first-commit
```shell
git clone https://github.com/git/git.git
git log --reverse
git checkout e83c5163316f89bfbde7d9ab23ca2e25604af290
```


## 编译
编译环境：Ubuntu-14.04

安装一些依赖：
```shell
sudo apt-get install libssl-dev
```
修改Makefile
```
-LIBS= -lssl
+LIBS= -lssl -lz -lcrypto
```

## 源码分析


