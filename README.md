About QSsh
==========

QSsh provides SSH and SFTP support for Qt applications. The aim of this project 
is to provide a easy way to use these protocols in any Qt application.

This project is based on Qt Creator's libQtcSsh.so. All the credits to
Qt Creator's team! (http://qt.gitorious.org/qt-creator)

### Compiling botan
Note: 将mingw32的bit目录放到环境变量中

1. 编译botan

1) 生成Makefile

    python configure.py --os=mingw --cpu=x86_32 --cc-bin=i686-w64-mingw32-g++.exe --ar-command=i686-w64-mingw32-gcc-ar

2) 编译

mingw32-make 

3) 编译安装

提前修改Makefile, 将里面的\Mingw 替换成./Mingw 即安装目录为当前目录的mingw, 再直行下面指令，产生如下图文件目录

mingw32-make install


### Compiling QSsh

qssh路径为已经可以直接通过mingw32编译的源文件，其他文件为QSsh源文件。

Prerequisites:
   * Qt 4.7.4 (May work with older versions)
   * On Windows: MinGW 4.4 or later, Visual Studio 2008 or later
   * On Mac: XCode 2.5 or later, compiling on 10.4 requires setting the
     environment variable QTC_TIGER_COMPAT before running qmake

Steps:
```bash
mkdir $BUILD_DIRECTORY
cd $BUILD_DIRECTORY
qmake $SOURCE_DIRECTORY/qssh.pro
make (or mingw32-make or nmake depending on your platform)
```

### Examples

Directory examples/SecureUploader/ contains an example on how to upload 
a file using SFTP
