/*
**************************************************************************************
函数介绍:  哈哈
1.void RtmpInit()
    功能介绍:
        该函数主要完成RTSP 各项参数初始化
            a.初始化连接客户信息
            b.初始化锁
            c.分配音频打包所需的内存空间

2.void RtmpAddClient(struct sockaddr_in client_addr,int sock,int nPos)
    功能介绍:
        该函数主要完成增加客户端npos的网络地址
    参数说明:
        client_addr:连接的客户端的网络地址
        sock:连接客户的socket
        nPos:要连接的客户端的序号
3.int  RtmpDelClient(int nPos)
    功能介绍:
        该函数用于断开序号为nPos的客户端
    参数说明:
        nPos:要断开的客户端的序号
4.int  RtmpGetNullClientIndex()
    功能介绍:
        该函数用于获取一个空的客户号，最大客户号为5(最多能连6个客户)
5.int  RtmpGetClientNum()
    功能介绍:
        该函数主要返回客户端的数目，用于判断是否有客户连接
6.int  RtmpExit()
    功能介绍:
        退出 RTMP 协议
            a.锁销毁
            b.释放分配的内存    
7.void RtmpSetupConnection(int nPos)
	功能介绍:
		建立与第n 个客户之间的连接
	参数说明:
		 nPos:要连接的客户端的序号
		 
8.RtmpAudioPack(int nLen, unsigned char * pData)
    功能介绍:
        该函数主要完成音频的rtmp打包
    参数说明:
        nLen:要打包的aac视频长度
        pData:要打包的aac视频地址
        
9.RtmpVideoPack(int nLen, unsigned char *pData)
    功能介绍:
        该函数主要完成视频的rtmp打包
    参数说明:
        nLen:要打包的264视频长度
        pData:要打包的264视频地址
10.char *RtmpGetVersion()
    功能介绍澹�
        该函数主要用于获取rtmp 协议的版本号
**************************************************************************************
*/

#define RTMP_LISTEN_PORT                1935// 4212//

#define PRINT_DEUBG

extern char *RtmpGetVersion();
extern int  RtmpGetClientNum();
extern void RtmpTask();



extern void RtmpAudioPack(int nLen, unsigned char *pData);
extern void RtmpVideoPack(int nLen, unsigned char *pData);

