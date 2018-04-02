// tutorial01.c
// 一个简单的程序，展示如何使用libavformat和libavcodec读取视频文件，并
// 将视频的前５帧数据保存为PPM格式的图片．
//
// 使用Makefile编译
//
// 使用方法：
// tutorial01 samplempg.mp4
//
// 参考文档：
// http://www.ffmpeg.org/doxygen/3.4/decode_video_8c-example.html
// https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial01.c

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

static const char *src_filename = NULL;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
  FILE *pFile;
  char szFilename[32];
  int y;

  // 打开输出ppm文件
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile = fopen(szFilename, "wb");
  if (pFile == NULL)
    return;

  // 写入头
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // 写入像素数据
  for (y = 0; y < height; y++)
    fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);

  // 关闭文件
  fclose(pFile);
}

int main(int argc, char *argv[])
{
  AVFormatContext *pFormatCtx = NULL;
  int i, videoStream;
  AVCodecContext *pCodecCtx = NULL;
  AVCodec *pCodec = NULL;
  AVFrame *pFrame = NULL;
  AVFrame *pFrameRGB = NULL;
  AVPacket packet;
  int frameFinished;
  int numBytes;
  uint8_t *buffer = NULL;

  AVDictionary *optionsDict = NULL;
  struct SwsContext *sws_ctx = NULL;

  if (argc < 2)
  {
    fprintf(stdout, "Usage: tutorial01 <file>\n");
    return -1;
  }
  src_filename = argv[1];
  // 初始化libavformat并注册所有的封装器，解封装器等
  av_register_all();

  // 打开视频文件输入流，必须使用avformat_close_input关闭文件流
  if (avformat_open_input(&pFormatCtx, src_filename, NULL, NULL) != 0)
  {
    fprintf(stderr, "Couldn't open file.\n");
    return -1;
  }
  // 检索流信息
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
  {
    fprintf(stderr, "Couldn't find stream information.\n");
    return -1;
  }

  // 将运行过程中的信息全部输出到标准错误中
  av_dump_format(pFormatCtx, 0, argv[1], 0);

  // 找到视频流，我们只需要处理视频数据
  videoStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      videoStream = i;
      break;
    }
  if (videoStream == -1)
  {
    fprintf(stderr, "Didn't find a video stream.\n");
    return -1;
  }

  // 从视频流中获得编解码器上下文的指针
  // 该接口已被废弃(warning: ‘codec’ is deprecated)
  // pCodecCtx = pFormatCtx->streams[videoStream]->codec;

  // 获得对应的解码器
  pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
  if (pCodec == NULL)
  {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }

  // 从视频流中获得编解码器上下文的指针
  pCodecCtx = avcodec_alloc_context3(pCodec);
  if (!pCodecCtx)
  {
    fprintf(stderr, "Failed to allocate the codec context\n");
    return -1;
  }

  // 复制输入流的编解码器的参数到输出编解码器上下文中
  if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
    return -1;
  }

  // 打开编解码器
  if (avcodec_open2(pCodecCtx, pCodec, &optionsDict) < 0)
  {
    fprintf(stderr, "Could not open codec!\n");
    return -1;
  }

  // 分配一个数据帧用于存放后续从视频流中获得的帧数据
  pFrame = av_frame_alloc();
  if (pFrame == NULL)
  {
    fprintf(stderr, "Allocate AVFrame failed!\n");
    return -1;
  }
  // 分配一个帧结构用于后续存放RGB图像
  pFrameRGB = av_frame_alloc();
  if (pFrameRGB == NULL)
  {
    fprintf(stderr, "Allocate AVFrame failed!\n");
    return -1;
  }

  // 确定保存图片需要的缓冲区大小，并且分配缓冲区空间
  // 该接口已被废弃(warning: ‘avpicture_get_size’ is deprecated)
  // numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
  numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
  buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

  sws_ctx =
      sws_getContext(
          pCodecCtx->width,
          pCodecCtx->height,
          pCodecCtx->pix_fmt,
          pCodecCtx->width,
          pCodecCtx->height,
          AV_PIX_FMT_RGB24,
          SWS_BILINEAR,
          NULL,
          NULL,
          NULL);

  // 把缓冲区中合适的部分指派到pFrameRGB中的图像面板．
  // 该接口已被废弃(warning: ‘avpicture_fill’ is deprecated) 需要注意的是，AVFrame是AVPicture的一个超集，可以强制转换．
  // avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
  av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);

  // 读取帧数据并且将前５帧数据存入磁盘
  i = 0;
  while (av_read_frame(pFormatCtx, &packet) >= 0)
  {
    // 判断数据包是否来自视频流
    if (packet.stream_index == videoStream)
    {
      // 从数据包中提取解码数据帧

      // 该接口已被废弃(‘avcodec_decode_video2’ is deprecated)
      // avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
      // // 是否拿到了完整的一帧
      // if (frameFinished)
      // {
      //   // 将图像从原生格式转换到RGB格式
      //   sws_scale(
      //       sws_ctx,
      //       (uint8_t const *const *)pFrame->data,
      //       pFrame->linesize,
      //       0,
      //       pCodecCtx->height,
      //       pFrameRGB->data,
      //       pFrameRGB->linesize);

      //   // 保存数据帧到磁盘
      //   if (++i <= 5)
      //   {
      //     SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
      //   }
      // }
      int ret;
      ret = avcodec_send_packet(pCodecCtx, &packet);
      if (ret < 0)
      {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
      }
      while (ret >= 0)
      {
        ret = avcodec_receive_frame(pCodecCtx, pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
          break;
        }
        else if (ret < 0)
        {
          fprintf(stderr, "Error during decoding\n");
          exit(1);
        }

        // 一帧读取结束
        // 将图像从pFrame的原生格式转换到pFrameRGB的RGB格式
        sws_scale(
            sws_ctx,
            (uint8_t const *const *)pFrame->data,
            pFrame->linesize,
            0,
            pCodecCtx->height,
            pFrameRGB->data,
            pFrameRGB->linesize);

        // 保存数据帧到磁盘
        if (++i <= 5)
        {
          SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
        }
      }
    }

    // 释放由av_read_frame分配内存的数据包的内存
    // 该接口已被废弃(‘av_free_packet’ is deprecated)
    // av_free_packet(&packet);
    av_packet_unref(&packet);
  }

  // 释放RGB图像
  av_free(buffer);
  av_free(pFrameRGB);
  // 释放YUV帧
  av_free(pFrame);
  // 关闭编解码器
  avcodec_close(pCodecCtx);
  // 关闭视频文件流
  avformat_close_input(&pFormatCtx);

  return 0;
}