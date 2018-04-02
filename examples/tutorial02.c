// tutorial02.c
// 一个简单的程序，展示如何使用libavformat和libavcodec读取视频文件，并
// 将视频通过SDL库显示在屏幕上．
//
// 使用Makefile编译
//
// 使用方法：
// tutorial02 sample.mpg
//
// 参考文档：
// http://www.ffmpeg.org/doxygen/3.4/decode_video_8c-example.html
// https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial02.c

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

static const char *src_filename = NULL;

int main(int argc, char *argv[])
{
  AVFormatContext *pFormatCtx = NULL;
  int i, videoStream;
  AVCodecContext *vCodecCtx = NULL;
  AVCodec *vCodec = NULL;
  AVFrame *pFrame = NULL;
  AVPacket packet;

  AVDictionary *optionsDict = NULL;
  struct SwsContext *sws_ctx = NULL;

  SDL_Overlay *bmp = NULL;
  SDL_Surface *screen = NULL;
  SDL_Rect rect;
  SDL_Event event;

  if (argc < 2)
  {
    fprintf(stdout, "Usage: tutorial02 <file>\n");
    return -1;
  }
  src_filename = argv[1];
  // 初始化libavformat并注册所有的封装器，解封装器等
  av_register_all();
  // 初始化SDL库
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
  {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    return -1;
  }

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
  av_dump_format(pFormatCtx, 0, src_filename, 0);

  // 找到视频流，我们只需要处理视频数据
  videoStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
  {
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      videoStream = i;
      break;
    }
  }
  if (videoStream == -1)
  {
    fprintf(stderr, "Didn't find a video stream.\n");
    return -1;
  }

  // 获得对应的解码器
  vCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
  if (vCodec == NULL)
  {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }

  // 从视频流中获得编解码器上下文的指针
  vCodecCtx = avcodec_alloc_context3(vCodec);
  if (!vCodecCtx)
  {
    fprintf(stderr, "Failed to allocate the codec context\n");
    return -1;
  }

  // 复制输入流的编解码器的参数到输出编解码器上下文中
  if (avcodec_parameters_to_context(vCodecCtx, pFormatCtx->streams[videoStream]->codecpar) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
    return -1;
  }

  // 打开编解码器
  if (avcodec_open2(vCodecCtx, vCodec, &optionsDict) < 0)
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

  // 初始化一个屏幕用于播放视频数据
#ifndef __DARWIN__
  screen = SDL_SetVideoMode(vCodecCtx->width, vCodecCtx->height, 0, 0);
#else
  screen = SDL_SetVideoMode(vCodecCtx->width, vCodecCtx->height, 24, 0);
#endif
  if (!screen)
  {
    fprintf(stderr, "SDL: could not set video mode - exiting\n");
    exit(1);
  }

  // 分配空间，在屏幕上存放我们的YUV图像
  bmp = SDL_CreateYUVOverlay(vCodecCtx->width,
                             vCodecCtx->height,
                             SDL_YV12_OVERLAY,
                             screen);

  sws_ctx =
      sws_getContext(
          vCodecCtx->width,
          vCodecCtx->height,
          vCodecCtx->pix_fmt,
          vCodecCtx->width,
          vCodecCtx->height,
          AV_PIX_FMT_YUV420P,
          SWS_BILINEAR,
          NULL,
          NULL,
          NULL);

  i = 0;
  while (av_read_frame(pFormatCtx, &packet) >= 0)
  {
    // 判断数据包是否来自视频流
    if (packet.stream_index == videoStream)
    {
      int ret;
      ret = avcodec_send_packet(vCodecCtx, &packet);
      if (ret < 0)
      {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
      }
      while (ret >= 0)
      {
        ret = avcodec_receive_frame(vCodecCtx, pFrame);
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
        SDL_LockYUVOverlay(bmp);
        AVFrame pict;
        pict.data[0] = bmp->pixels[0];
        pict.data[1] = bmp->pixels[2];
        pict.data[2] = bmp->pixels[1];

        pict.linesize[0] = bmp->pitches[0];
        pict.linesize[1] = bmp->pitches[2];
        pict.linesize[2] = bmp->pitches[1];
        // 将图像从pFrame的原生格式转换到SDL使用的YUV格式
        sws_scale(
            sws_ctx,
            (uint8_t const *const *)pFrame->data,
            pFrame->linesize,
            0,
            vCodecCtx->height,
            pict.data,
            pict.linesize);

        SDL_UnlockYUVOverlay(bmp);
        rect.x = 0;
        rect.y = 0;
        rect.w = vCodecCtx->width;
        rect.h = vCodecCtx->height;
        SDL_DisplayYUVOverlay(bmp, &rect);
      }
    }

    // 释放由av_read_frame分配内存的数据包的内存
    av_packet_unref(&packet);
    SDL_PollEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT:
      SDL_Quit();
      exit(0);
      break;
    default:
      break;
    }
  }

  // 释放YUV帧
  av_free(pFrame);
  // 关闭编解码器
  avcodec_close(vCodecCtx);
  // 关闭视频文件流
  avformat_close_input(&pFormatCtx);

  return 0;
}