# 引言

FFmpeg是一个出色的用于创建视频应用或者相关通用工具的库，它帮助用户处理了所有关于视频处理的工作，包括解码，编码，封装和解封装，让用户可以更简单地编写媒体应用．FFmpeg使用C编写，简单，速度快，而且可以解码如今能找到的几乎所有格式，同时也可以编码其它一些格式．

但是关于FFmpeg相关的文档和教程实在太过缺失，国内相对完善的就是雷神（[雷霄骅][leixiaohua1020]）的博客文章了，可惜雷神已逝．官网推荐的由[Stephen Dranger][Stephen Dranger]编写的教程2015年２月就不再更新，源代码也有点过时，由[mpenkov][mpenkov]更新的源代码最后更新于2016年１月６日，作者也表明不会继续维护．出于学习和实践的目的，我照着Stephen Dranger编写的教程思路，尝试使用目前最新版本的FFmpeg源码编写一个简易的播放器，就做为自己的入门总结了．才疏学浅，若有不足，欢迎指正．

教程相关环境如下

系统：
```text
Ubuntu 16.04.4 LTS (Xenial Xerus)
```

编译器：
```text
gcc (Ubuntu 5.4.1-2ubuntu1~16.04) 5.4.1 20160904
```

ffmpeg版本：
```text
ffmpeg version 3.4.1
built with gcc 5.4.1 (Ubuntu 5.4.1-2ubuntu1~16.04) 20160904
libavutil      55. 78.100 / 55. 78.100
libavcodec     57.107.100 / 57.107.100
libavformat    57. 83.100 / 57. 83.100
libavdevice    57. 10.100 / 57. 10.100
libavfilter     6.107.100 /  6.107.100
libswscale      4.  8.100 /  4.  8.100
libswresample   2.  9.100 /  2.  9.100
libpostproc    54.  7.100 / 54.  7.100
```

[leixiaohua1020]:https://blog.csdn.net/leixiaohua1020
[Stephen Dranger]:http://dranger.com/ffmpeg/ffmpeg.html
[mpenkov]:https://github.com/mpenkov/ffmpeg-tutorial