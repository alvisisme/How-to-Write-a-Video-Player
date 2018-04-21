# 视频截图

## 概述

我们需要预先了解下一个视频文件的基本组件，首先，文件本身我们称为 **容器(container)** ，不同的容器内部数据的组织方式并不相同．我们见到的文件一般都带有avi或者mp4等后缀，我们称之为 **封装格式(format)** ，代表的就是文件内部的数据组织方式，封装格式的主要作用是把视频码流和音频码流按照一定的格式存储在一个文件中，将音视频数据合并存入一个文件的过程我们称为**封装**，从文件中将音视频数据分离出来分别处理的过程我们称为**解封装**.

所谓的 **流(streams)** 其实是个动态的概念，表示一连串随着时间被读取写入的数据，一个视频文件内通常有一个音频流和视频流，我们会循环从流中读入数据然后进行画面输出和声音播放．

我们从流中读取的数据是 **数据包(packets)** ，数据包内可能包含多个**数据帧(frames)**，数据包是网络层的概念，数据帧是数据链路层的概念，我们的视频和音频的最小数据单元就是帧，但是在网络传输中，为了便于在不同网络
中传输，我们会对帧数据进行包装，变成数据包进行统一收发．所以我们进行音视频处理时，也是会先处理数据包，再处理数据帧．

每个流内的数据为了便于传输都会进行特殊的编码操作，进行这种操作的东西我们称为**编解码器(codec)**，编解码器定义了真实数据是如何被编码和解码的，相关的例子有DivX和MP3等，这些数据被对应的编解码器解码后才能被我们的应用所使用．

一个基本的处理视频和音频流的过程如下
```text
10 OPEN streams FROM video.avi
20 READ packets FROM streams INTO frames
30 IF frames NOT COMPLETE GOTO 20
40 DO SOMETHING WITH frames
50 GOTO 20
```

使用ffmpeg处理多媒体文件的流程主要就是分为上面描述的几个步骤，只不过很多应用在对数据帧的处理上非常复杂．

我们现在按照这个流程编写一个例子，先打开一个视频文件，从里面读取视频流，然后将读取的的数据包里面的前5个数据帧都写入PPM文件中，实现一个简单的视频截图功能．

## 关于PPM文件

PPM(Portable PixMap)是可移植像素图片，是由netpbm项目定义的一系列的可移植图片格式中的一种，主要的优点就是格式非常简单．

netpbm项目定义的几种图片格式是通过其表示的颜色类型来区别的，PBM(Portable BitMap)表示位图，只有黑色和白色，PGM(Portable GrayMap)表示灰度图片，PPM(Portable PixMap)则代表完整的RGB颜色的图片。

PPM文件由两个部分组成：第一个部分是三行ASCII码，这个部分决定了图像的存储格式以及图像的特征，第二个部分就是图像的数据部分，其实就是每一个点的RGB值．
前三行的ASCII码中，第一行表示存储格式，P3表示以ASCII码存储，P6表示以二进制存储．第二行表明图片大小，为图片宽度值和高度值，中间由空格分割．第三行描述像素的最大颜色组成，取值0-255.

## 步骤

我们需要对视频文件进行处理，先从文件中读取数据，进行解封装，这里主要依赖的就是**libavformat**库.然后进行音视频数据的解码操作，这里主要依靠的是**libavcodec**库.后面将视频帧输出为ppm图片文件，涉及到了色彩装换和缩放，使用的库为**libswscale**，外加一些通用库函数，主要依赖**libavutil**.

我们首先需要做的就是进行一系列的初始化操作,这些库中需要初始化的就是libavformat，关键函数是`av_register_all`,
打开文件获得数据流的步骤关键函数是`avformat_open_input`和`avformat_find_stream_info`．
关键代码如下：
```C
AVFormatContext *pFormatCtx = NULL;
int i, videoStream = -1;
...
av_register_all();

avformat_open_input(&pFormatCtx, src_filename, NULL, NULL);

avformat_find_stream_info(pFormatCtx, NULL) < 0);

for (i = 0; i < pFormatCtx->nb_streams; i++)
{
  if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
  {
    videoStream = i;
    break;
  }
}
```

之后我们需要对视频进行数据解码．ffmpeg本身预置了很多编解码库，能够自动识别几乎所有的文件编解码信息，我们只需要简单的传递几个参数就能够实现数据的解码，关键函数为`avcodec_find_decoder`,`avcodec_parameters_to_context`和`avcodec_open2`,完成后就可以开始对数据包进行处理.
关键代码如下:
```C
AVCodecContext *vCodecCtx = NULL;
AVCodec *vCodec = NULL;
...
vCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);

vCodecCtx = avcodec_alloc_context3(vCodec);

avcodec_parameters_to_context(vCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

avcodec_open2(vCodecCtx, vCodec, &videoOptionsDict);
```

现在我们完成了必要的准备工作，开始处理数据包和数据帧．







## 参考引用

* [netpbm wiki](https://en.wikipedia.org/wiki/Netpbm_format)
* [ffmpeg tutorial01 source code](https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c)
* [ffmpeg video decoding with libavcodec API example](http://www.ffmpeg.org/doxygen/3.4/decode_video_8c-example.html)



