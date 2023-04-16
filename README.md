
# 源码分析：git第一个提交是怎么样的？

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

编译后生成的可执行文件如下：
```shell
find ./ -perm /+x -type f 
./read-tree
./show-diff
./update-cache
./cat-file
./write-tree
./init-db
./commit-tree
```

## 分析
- 可以参考我之前写的博客：https://blog.csdn.net/wangadping/article/details/126337953
- github上有问老哥写的挺好的：https://github.com/xiaowenxia/git-first-commit
