// tutorial03.c
//
// 一个简单的程序，展示如何快速地读取视频帧和播放声音(声画不同步)
//
// 使用Makefile编译
//
// 使用方法：
// tutorial03 sample.mpg
//
// 参考文档：
// https://github.com/mpenkov/ffmpeg-tutorial/blob/master/tutorial03.c

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

static const char *src_filename = NULL;

typedef struct PacketQueue
{
  AVPacketList *first_pkt, *last_pkt;
  int nb_packets;
  int size;
  SDL_mutex *mutex;
  SDL_cond *cond;
} PacketQueue;

PacketQueue audioq;

int quit = 0;

void packet_queue_init(PacketQueue *q)
{
  memset(q, 0, sizeof(PacketQueue));
  q->mutex = SDL_CreateMutex();
  q->cond = SDL_CreateCond();
}
int packet_queue_put(PacketQueue *q, AVPacket *pkt)
{

  AVPacketList *pkt1;
  int ret;
  // if (av_dup_packet(pkt) < 0)
  // {
  //   return -1;
  // }
  pkt1 = av_malloc(sizeof(AVPacketList));
  if (!pkt1)
    return -1;

  if ((ret = av_packet_ref(&pkt1->pkt, pkt) < 0))
  {
    av_free(pkt1);
    return -1;
  }
  // pkt1->pkt = *pkt;
  // pkt1->next = NULL;

  SDL_LockMutex(q->mutex);

  if (!q->last_pkt)
    q->first_pkt = pkt1;
  else
    q->last_pkt->next = pkt1;
  q->last_pkt = pkt1;
  q->nb_packets++;
  q->size += pkt1->pkt.size;
  SDL_CondSignal(q->cond);

  SDL_UnlockMutex(q->mutex);
  return 0;
}
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
  AVPacketList *pkt1;
  int ret;

  SDL_LockMutex(q->mutex);

  for (;;)
  {

    if (quit)
    {
      ret = -1;
      break;
    }

    pkt1 = q->first_pkt;
    if (pkt1)
    {
      q->first_pkt = pkt1->next;
      if (!q->first_pkt)
        q->last_pkt = NULL;
      q->nb_packets--;
      q->size -= pkt1->pkt.size;
      *pkt = pkt1->pkt;
      av_free(pkt1);
      ret = 1;
      break;
    }
    else if (!block)
    {
      ret = 0;
      break;
    }
    else
    {
      SDL_CondWait(q->cond, q->mutex);
    }
  }
  SDL_UnlockMutex(q->mutex);
  return ret;
}

int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size)
{

  static AVPacket pkt;
  static uint8_t *audio_pkt_data = NULL;
  static int audio_pkt_size = 0;
  static AVFrame frame;

  int len1, data_size = 0;

  for (;;)
  {
    while (audio_pkt_size > 0)
    {
      int got_frame = 0;
      len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
      if (len1 < 0)
      {
        /* if error, skip frame */
        audio_pkt_size = 0;
        break;
      }
      audio_pkt_data += len1;
      audio_pkt_size -= len1;
      if (got_frame)
      {
        data_size =
            av_samples_get_buffer_size(
                NULL,
                aCodecCtx->channels,
                frame.nb_samples,
                aCodecCtx->sample_fmt,
                1);
        memcpy(audio_buf, frame.data[0], data_size);
      }
      if (data_size <= 0)
      {
        /* No data yet, get more frames */
        continue;
      }
      /* We have data, return it and come back for more later */
      return data_size;
    }

    if (pkt.data)
      av_packet_unref(&pkt);

    if (quit)
    {
      return -1;
    }

    if (packet_queue_get(&audioq, &pkt, 1) < 0)
    {
      return -1;
    }
    audio_pkt_data = pkt.data;
    audio_pkt_size = pkt.size;
  }
}

int audio_decode_frame2(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size)
{
  static AVPacket pkt;
  static uint8_t *audio_pkt_data = NULL;
  static int audio_pkt_size = 0;
  static AVFrame frame;

  int len1, data_size = 0;

  for (;;)
  {
    while (audio_pkt_size > 0)
    {
      fprintf(stderr, "%d: start read audio packet %d\n", __LINE__, pkt.size);
      int ret = avcodec_send_packet(aCodecCtx, &pkt);
      fprintf(stdout, "%d: avcodec_send_packet %d\n", __LINE__, ret);
      if (ret < 0)
      {
        fprintf(stderr, "Error sending a packet for decoding\n");
        audio_pkt_size = 0;
        break;
      }
      audio_pkt_data += pkt.size;
      audio_pkt_size -= pkt.size;
      
      while (ret >= 0)
      {
        ret = avcodec_receive_frame(aCodecCtx, &frame);
        fprintf(stdout, "%d: avcodec_receive_frame %d\n", __LINE__, ret);

        exit(1);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
          // audio_pkt_size = 0;
          fprintf(stderr, "%d: avcodec_receive_frame error %d\n", __LINE__, ret);
          continue;
        }
        else if (ret < 0)
        {
          fprintf(stderr, "Error during decoding\n");
          audio_pkt_size = 0;
          break;
        }
      }
      // return data_size;
    }

    if (pkt.data)
    {
      av_packet_unref(&pkt);
    }
    if (quit)
    {
      return -1;
    }
    fprintf(stderr, "%d: packet_queue_get\n", __LINE__);
    if (packet_queue_get(&audioq, &pkt, 1) < 0)
    {
      fprintf(stderr, "%d: packet_queue_get error\n", __LINE__);
      return -1;
    }
    fprintf(stderr, "%d: packet_queue_get success\n", __LINE__);
    audio_pkt_data = pkt.data;
    audio_pkt_size = pkt.size;
    fprintf(stderr, "%d: get packet size %d\n", __LINE__, audio_pkt_size);
  }
}
/**
 * stream是声音数据，len是声音长度
 */
void audio_callback(void *userdata, Uint8 *stream, int len)
{

  AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
  int len1, audio_size;

  static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
  static unsigned int audio_buf_size = 0;
  static unsigned int audio_buf_index = 0;

  while (len > 0)
  {
    if (audio_buf_index >= audio_buf_size)
    {
      // 读取　audio_buf 大小的音频数据
      /* We have already sent all our data; get more */
      audio_size = audio_decode_frame2(aCodecCtx, audio_buf, audio_buf_size);
      printf("%d: size %d buf size %d \n", __LINE__, audio_size, audio_buf_size);
      if (audio_size < 0)
      {
        /* If error, output silence */
        audio_buf_size = 1024; // arbitrary?
        memset(audio_buf, 0, audio_buf_size);
      }
      else
      {
        audio_buf_size = audio_size;
      }
      audio_buf_index = 0;
    }
    //
    len1 = audio_buf_size - audio_buf_index;
    if (len1 > len)
    {
      len1 = len;
    }
    // 最终目的就是往　stream　里写入　audio buffer 数据
    memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
    len -= len1;
    stream += len1;
    audio_buf_index += len1;
  }
}

int main(int argc, char *argv[])
{
  AVFormatContext *pFormatCtx = NULL;
  int i, videoStream, audioStream;
  AVCodecContext *vCodecCtx = NULL;
  AVCodec *vCodec = NULL;
  AVFrame *pFrame = NULL;
  AVPacket packet;

  AVCodecContext *aCodecCtx = NULL;
  AVCodec *aCodec = NULL;

  SDL_Overlay *bmp = NULL;
  SDL_Surface *screen = NULL;
  SDL_Rect rect;
  SDL_Event event;
  SDL_AudioSpec wanted_spec, spec;

  struct SwsContext *sws_ctx = NULL;
  AVDictionary *videoOptionsDict = NULL;
  AVDictionary *audioOptionsDict = NULL;

  if (argc < 2)
  {
    fprintf(stdout, "Usage: tutorial03 <file>\n");
    exit(1);
  }
  src_filename = argv[1];
  // 初始化libavformat并注册所有的封装器，解封装器等
  av_register_all();
  // 初始化SDL库
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
  {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  // 打开视频文件输入流，必须使用avformat_close_input关闭文件流
  if (avformat_open_input(&pFormatCtx, src_filename, NULL, NULL) != 0)
  {
    fprintf(stderr, "Couldn't open file.\n");
    pFormatCtx = NULL;
    return -1;
  }
  // 检索流信息
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
  {
    fprintf(stderr, "Couldn't find stream information.\n");
    avformat_close_input(&pFormatCtx);
    return -1;
  }

  // 将运行过程中的信息全部输出到标准错误中
  // av_dump_format(pFormatCtx, 0, argv[1], 0);

  // 找到视频流和音频流
  videoStream = -1;
  audioStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
  {
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
        videoStream < 0)
    {
      videoStream = i;
    }
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO &&
        audioStream < 0)
    {
      audioStream = i;
    }
  }
  if (videoStream == -1)
  {
    fprintf(stderr, "Didn't find a video stream.\n");
    return -1;
  }
  if (audioStream == -1)
  {
    fprintf(stderr, "Didn't find a audio stream.\n");
    return -1;
  }

  /** 开始处理音频流 **/

  // 获得对应的音频解码器
  aCodec = avcodec_find_decoder(pFormatCtx->streams[audioStream]->codecpar->codec_id);
  if (aCodec == NULL)
  {
    fprintf(stderr, "Unsupported audio codec!\n");
    avformat_close_input(&pFormatCtx);
    return -1;
  }
  // 从音频流中获得编解码器上下文的指针
  aCodecCtx = avcodec_alloc_context3(aCodec);
  if (!aCodecCtx)
  {
    fprintf(stderr, "Failed to allocate the audio codec context\n");
    avformat_close_input(&pFormatCtx);
    return -1;
  }
  // 复制输入流的编解码器的参数到输出编解码器上下文中
  if (avcodec_parameters_to_context(aCodecCtx, pFormatCtx->streams[audioStream]->codecpar) < 0)
  {
    fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&aCodecCtx);
    return -1;
  }

  // 根据编解码器的信息设置音频参数，用于传入SDL库
  wanted_spec.freq = aCodecCtx->sample_rate;
  wanted_spec.format = AUDIO_S16SYS;
  wanted_spec.channels = aCodecCtx->channels;
  wanted_spec.silence = 0;
  wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
  wanted_spec.callback = audio_callback;
  wanted_spec.userdata = aCodecCtx;

  if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
  {
    fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
    return -1;
  }

  // 打开音频编解码器
  if (avcodec_open2(aCodecCtx, aCodec, &audioOptionsDict) < 0)
  {
    fprintf(stderr, "Could not open audio codec!\n");
    avcodec_free_context(&aCodecCtx);
    avformat_close_input(&pFormatCtx);
    return -1;
  }

  packet_queue_init(&audioq);
  SDL_PauseAudio(0);

  /**开始处理视频数据**/
  // 获得对应的视频编解码器
  vCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
  if (vCodec == NULL)
  {
    fprintf(stderr, "Unsupported video codec!\n");
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
    avformat_close_input(&pFormatCtx);
    return -1;
  }

  // 打开视频编解码器
  if (avcodec_open2(vCodecCtx, vCodec, &videoOptionsDict) < 0)
  {
    fprintf(stderr, "Could not open video codec!\n");
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
    else if (packet.stream_index == audioStream)
    {
       int ret;
      ret = avcodec_send_packet(aCodecCtx, &packet);
      if (ret < 0)
      {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
      }
      while (ret >= 0)
      {
        ret = avcodec_receive_frame(aCodecCtx, pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
          break;
        }
        else if (ret < 0)
        {
          fprintf(stderr, "Error during decoding\n");
          exit(1);
        }
        
      }
      // packet_queue_put(&audioq, &packet);
    }
    else
    {
      // 释放由av_read_frame分配内存的数据包的内存
      av_packet_unref(&packet);
    }
    SDL_PollEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT:
      quit = 1;
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
  avcodec_close(aCodecCtx);
  avcodec_close(vCodecCtx);
  // 关闭文件流
  avformat_close_input(&pFormatCtx);

  return 0;
}