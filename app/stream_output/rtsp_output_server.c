/****************************************************************************
*
* rtsp_output_server.c
*============================================================================
*  作用： 主要用于输出流的SERVER的设置。
*
*
*============================================================================
*****************************************************************************/
//test for git
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "mid_mutex.h"
#include "stream_output_struct.h"
#include "rtsp_server.h"
#include "rtsp_output_server.h"
#include "stream_output_common.h"
#include "stream_output_client.h"

#include "log_common.h"

typedef struct RTSP_SERVER_OUTPUT_T {
	int s_used;
	rtsp_server_config rtsp_config;
	//int status; // active,pause,stop
	stream_output_server_config config;
	Stream_Output_Client *client[RTSP_MAX_CLIENT_NUM];
	mid_mutex_t mutex; //锁
} RTSP_Output_Server_Handle;

#define RTSP_SERVER_CONFIG_INI "rtsp_server_config.ini"

#define RTSP_SERVER_GLOBAL_CONFIG_INI "rtsp_server_global_config.ini"

static RTSP_Output_Server_Handle *g_rtsp_shandle = NULL;

/*********************************static*****function****start*******************************************/
static int rtsp_handle_delete(RTSP_Output_Server_Handle **handle);
static void rtsp_handle_printf(RTSP_Output_Server_Handle *handle);
//static int rtsp_client_find(char *ip, int vport, int aport, int type, RTSP_Output_Server_Handle *handle, int *free_num);
//static int rtsp_client_is_free(Stream_Output_Client *client);
//static int rtsp_client_is_repeat(struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, Stream_Output_Client *client);
//static int rtsp_client_create(struct sockaddr_in *video_addr,	struct sockaddr_in *audio_addr, int type, Stream_Output_Client *client);
//static int rtsp_client_delete(Stream_Output_Client *client);
//static int rtsp_client_config(Stream_Output_Client *client, stream_output_server_config *config);
//static int rtsp_porting_client_add(char *ip, int vport, int aport, int type);
//static int rtsp_porting_client_delete(char *ip, int vport, int aport, int type);
static int app_rtsp_global_config_fwrite(rtsp_server_config *config);
static int app_rtsp_global_config_fread(rtsp_server_config *config);
static int app_rtsp_config_fwrite(int reset);
static int app_rtsp_config_fread(RTSP_Output_Server_Handle *handle);
static int app_rtsp_set_active(int status);
static int app_rtsp_server_start(stream_output_server_config *config);
static int app_rtsp_server_stop(stream_output_server_config *config);
//static int app_rtsp_server_status(void);
static void rtsp_ginfo_print(rtsp_server_config *config);
static int stream_rtsp_set_clinet_tc(int num, int tc_flag, int tc_rate, char *dst_ip, unsigned short port);

/*********************************static*****function******end*******************************************/



static int rtsp_handle_delete(RTSP_Output_Server_Handle **handle)
{
	if(*handle == NULL) {
		PRINTF("warnning,the handle is NULL.\n");
		return -1;
	}

	RTSP_Output_Server_Handle *temp = *handle;

	if(temp->mutex != NULL) {
		mid_mutex_destroy(&(temp->mutex));
	}

	temp->mutex = NULL;

	free(*handle);
	*handle = NULL;
	return 0;

}

static void rtsp_handle_printf(RTSP_Output_Server_Handle *handle)
{
	if(handle == NULL) {
		PRINTF("warnning,the handle is NULL.\n");
		return  ;
	}

	PRINTF("the rtsp handle = %p\n", handle);
	PRINTF("the rtsp used =%d,status=%d,type=%d\n", handle->s_used, handle->config.status, handle->config.type);
	PRINTF("the rtsp ip =%s:%d-%d,mtu=%d,qos=%x,ttl=%d,tc=%d\n",
	       handle->config.main_ip, handle->config.video_port, handle->config.audio_port, handle->config.mtu,
	       handle->config.tos, handle->config.ttl, handle->config.tc_flag);

	return ;
}

#if 0
/*==============================================================================
    Function: 	<rtsp_client_find>
    Input:
    Output:
    Description:  查找节点，节点不存在，返回第一个空闲的空间
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.05
==============================================================================*/
static int rtsp_client_find(char *ip, int vport, int aport, int type, RTSP_Output_Server_Handle *handle, int *free_num)
{
	int i = 0;
	struct sockaddr_in video_addr;
	struct sockaddr_in audio_addr;
	int have_audio = 0;
	//int free_num = 0;
	*free_num = -1;
	memset(&video_addr, 0, sizeof(struct sockaddr_in));
	memset(&audio_addr, 0, sizeof(struct sockaddr_in));

	if(aport != 0) {
		have_audio = 1;
	}

	inet_pton(AF_INET, ip, &video_addr.sin_addr);
	video_addr.sin_port = htons(vport);
	video_addr.sin_family = AF_INET;

	if(have_audio) {
		inet_pton(AF_INET, ip, &audio_addr.sin_addr);
		audio_addr.sin_port = htons(aport);
		audio_addr.sin_family = AF_INET;
	}

	//check ip and port is repeat ,is successed
	//	mid_mutex_lock(g_rtsp_shandle->mutex);
	for(i = 0; i < RTSP_MAX_CLIENT_NUM ; i++) {
		if(rtsp_client_is_repeat(&video_addr, &audio_addr, &(g_rtsp_shandle->client[i])) == 1) {
			PRINTF("Warnning,the client %s:%d-%d ,is repeat.\n", ip, vport, aport);
			return i;
		}

		if(*free_num == -1 && rtsp_client_is_free(&(g_rtsp_shandle->client[i])) == 1) {
			PRINTF("debug,the first free client is %d.\n", i);
			*free_num = i;
		}
	}

	//check if have free client
	if(*free_num == -1) {
		PRINTF("Warnning,there is not have free client\n");
		//	mid_mutex_unlock(g_rtsp_shandle->mutex);
		return -1;
	}

	return -1;

}


/*==============================================================================
    Function: 	<rtsp_client_is_free>
    Input:
    Output: 	0 表示free
    Description:
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.04
==============================================================================*/
static int rtsp_client_is_free(Stream_Output_Client *client)
{
	if(client == NULL) {
		PRINTF("Error,the client is NULL\n");
		return -1;
	}

	if(client->c_used == NOTUSED) {
		return 1;
	} else {
		return -1;
	}
}

/*==============================================================================
    Function: 	<rtsp_client_is_repeat>
    Input:
    Output: 	0  is repeat
    Description:
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.04
==============================================================================*/
static int rtsp_client_is_repeat(struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, Stream_Output_Client *client)
{
	if(video_addr == NULL || audio_addr == NULL || client == NULL) {
		PRINTF("Error,the client is NULL\n");
		return -1;
	}

	if(memcmp(video_addr, &(client->video_addr), sizeof(struct sockaddr_in)) != 0) {
		return -1;
	}

	if(memcmp(audio_addr, &(client->audio_addr), sizeof(struct sockaddr_in)) != 0) {
		return -1;
	}

	return 1;
}

/*==============================================================================
    Function: 	<rtsp_client_creat>
    Input:
    Output:
    Description:   创建一个udp send client
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.04
==============================================================================*/
static int rtsp_client_create(struct sockaddr_in *video_addr,	struct sockaddr_in *audio_addr, int type, Stream_Output_Client *client)
{

	int video_socket = 0;
	int audio_socket = 0;
	int have_audio = 0;

	if(audio_addr->sin_port != 0) {
		have_audio = 1;
	}

	//假定video audio必须不同的port
	video_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if(video_socket < 0) {
		PRINTF("ERROR,video socket is error\n");
		return -1;
	}

	memcpy(&(client->video_addr), video_addr, sizeof(struct sockaddr_in));
	client->video_socket = video_socket;

	if(have_audio) {
		audio_socket = socket(AF_INET, SOCK_DGRAM, 0);

		if(audio_socket < 0) {
			PRINTF("ERROR,audio socket is error\n");
			return -1;
		}

		memcpy(&(client->audio_addr), audio_addr, sizeof(struct sockaddr_in));
		client->audio_socket = audio_socket;
	}

	client->type = type;
	client->c_used = ISUSED;
	client->send_status = S_ACTIVE;
	//设置 ttl ,qos
	//设置是否开启流量整形
	//stream_server_set_tc(config->num);

	return 0;

}

static int rtsp_client_delete(Stream_Output_Client *client)
{
	if(client == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	if(client->video_socket > 0) {
		PRINTF("I will close the rtsp video udp socket = %d\n", client->video_socket);
		close(client->video_socket);
	}

	if(client->audio_socket > 0) {
		PRINTF("I will close the rtsp audio udp socket = %d\n", client->audio_socket);
		close(client->audio_socket);
	}

	memset(client, 0, sizeof(Stream_Output_Client));
	return 0;

}

/*==============================================================================
    Function: 	<rtsp_client_config>
    Input:
    Output:
    Description:  用来设置tos,ttl等值
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.05
==============================================================================*/
static int rtsp_client_config(Stream_Output_Client *client, stream_output_server_config *config)
{
	if(client == NULL || config == NULL) {
		PRINTF("ERROR\n");
		return -1;
	}

	if(client->c_used == NOTUSED || client->video_socket <= 0 || client->video_addr.sin_port == 0) {
		PRINTF("ERROR\n");
		return -1;
	}

	//set qos

	//set ttl
	return  0;
}

/*==============================================================================
    Function: 	<rtsp_porting_client_add>
    Input:
    Output:
    Description:  用于rtsp线程调用，建立一个新的client
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.04
==============================================================================*/
static int rtsp_porting_client_add(char *ip, int vport, int aport, int type)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	//	int ret = -1;
	int r_num = -1;
	int free_num = -1;
	struct sockaddr_in video_addr;
	struct sockaddr_in audio_addr;
	int have_audio = 0;

	mid_mutex_lock(g_rtsp_shandle->mutex);
	r_num =  rtsp_client_find(ip, vport, aport, type, g_rtsp_shandle, &free_num);

	if(r_num >= 0) {
		PRINTF("Warnning,the client %s:%d is repeat %d client\n", ip, vport, r_num);

		if(mid_ip_is_multicast(ip) == 1) {
			g_rtsp_shandle->client[r_num].usenum ++;
			PRINTF("Debug,the %d client is multicast,the usenum=%d\n", r_num, g_rtsp_shandle->client[r_num].usenum);
			mid_mutex_unlock(g_rtsp_shandle->mutex);
			return 0;
		}
	}

	if(free_num == -1) {
		PRINTF("Warnning,there is not have free client\n");
		mid_mutex_unlock(g_rtsp_shandle->mutex);
		return -1;
	}


	memset(&video_addr, 0, sizeof(struct sockaddr_in));
	memset(&audio_addr, 0, sizeof(struct sockaddr_in));

	if(aport != 0) {
		have_audio = 1;
	}

	inet_pton(AF_INET, ip, &video_addr.sin_addr);
	video_addr.sin_port = htons(vport);
	video_addr.sin_family = AF_INET;

	if(have_audio) {
		inet_pton(AF_INET, ip, &audio_addr.sin_addr);
		audio_addr.sin_port = htons(aport);
		audio_addr.sin_family = AF_INET;
	}

	//create new client
	PRINTF("Debug,the %d client will create for the %s:%d-%d\n", free_num, ip, vport, aport);
	rtsp_client_create(&video_addr, &audio_addr, type, &(g_rtsp_shandle->client[free_num]));

	if(mid_ip_is_multicast(ip) == 1) {
		g_rtsp_shandle->client[free_num].usenum ++;
		PRINTF("Debug,the %d client is multicast,the usenum=%d\n", free_num, g_rtsp_shandle->client[free_num].usenum);
	}

	//config the client
	rtsp_client_config(&(g_rtsp_shandle->client[free_num]), &(g_rtsp_shandle->config));

	//set tc ip ,port
	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}



/*==============================================================================
    Function: 	<rtsp_porting_client_delete>
    Input:
    Output:
    Description:    主要用于关闭一个rtsp 客户端
    Modify:
    Auth: 		zhangmin
    Date: 		2012.04.05
==============================================================================*/
static int rtsp_porting_client_delete(char *ip, int vport, int aport, int type)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	//	int ret = -1;
	int r_num = -1;
	int free_num = -1;
	mid_mutex_lock(g_rtsp_shandle->mutex);
	r_num =  rtsp_client_find(ip, vport, aport, type, g_rtsp_shandle, &free_num);

	if(r_num < 0) {
		PRINTF("Warnning,the %s:%d client is not found\n", ip, vport);
		mid_mutex_unlock(g_rtsp_shandle->mutex);
		return -1;
	}


	PRINTF("Debug,the client %s:%d is find, %d client\n", ip, vport, r_num);

	if(mid_ip_is_multicast(ip) == 1) {
		g_rtsp_shandle->client[r_num].usenum --;
		PRINTF("Debug,the %d client is multicast,the usenum=%d\n", r_num, g_rtsp_shandle->client[r_num].usenum);
	}

	//delete the client
	if(g_rtsp_shandle->client[r_num].usenum == 0) {
		rtsp_client_delete(&(g_rtsp_shandle->client[r_num]));
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}
#endif

static int app_rtsp_global_config_fwrite(rtsp_server_config *config)
{
	if(config == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	int ret = 0;
	char bak_name[512] = {0};
	char cmd[512] = {0};
	int value = 0;
	//	int used = 0;
	sprintf(bak_name, "%s.bak", RTSP_SERVER_GLOBAL_CONFIG_INI);

	value = config->s_port;
	ret = app_ini_write_int(bak_name, "rtsp_config", "s_port", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->active;
	ret = app_ini_write_int(bak_name, "rtsp_config", "active", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}


	value = config->s_mtu;
	ret = app_ini_write_int(bak_name, "rtsp_config", "s_mtu", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->mult_flag;
	ret = app_ini_write_int(bak_name, "rtsp_config", "mult_flag", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}


	ret = app_ini_write_string(bak_name, "rtsp_config", "mult_ip", config->mult_ip);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config->mult_port;
	ret = app_ini_write_int(bak_name, "rtsp_config", "mult_port", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	/*拷贝bak文件到ini*/
	sprintf(cmd, "cp -rf %s %s;rm -rf %s", bak_name, RTSP_SERVER_GLOBAL_CONFIG_INI, bak_name);
	system(cmd);
	PRINTF("i will run cmd:%s\n", cmd);
	return 0;

}

static int app_rtsp_global_config_fread(rtsp_server_config *config)
{
	char 			string[512] = {0};
	//	int 			ret  = -1 ;
	int 	value = 0;
	char 			config_file[256] = {0};
	rtsp_server_config  temp;
	memset(&temp, 0, sizeof(rtsp_server_config));

	strcpy(config_file, RTSP_SERVER_GLOBAL_CONFIG_INI);


	if(app_ini_read_int(config_file, "rtsp_config", "s_port", &value) != -1) {
		temp.s_port = value;
	} else {
		temp.s_port = 554;
	}

	if(app_ini_read_int(config_file, "rtsp_config", "active", &value) != -1) {
		temp.active = value;
	} else {
		temp.active = 60;
	}

	if(app_ini_read_int(config_file, "rtsp_config", "s_mtu", &value) != -1) {
		temp.s_mtu = value;
	} else {
		temp.s_mtu = 1500;
	}

	if(app_ini_read_int(config_file, "rtsp_config", "mult_flag", &value) != -1) {
		temp.mult_flag = value;
	} else {
		temp.mult_flag = 0;
	}

	if(app_ini_read_string(config_file, "rtsp_config", "mult_ip", string, sizeof(string)) != -1) {
		strcpy(temp.mult_ip, string);
	} else {
		strcpy(temp.mult_ip, "234.5.6.9");
	}


	if(app_ini_read_int(config_file, "rtsp_config", "mult_port", &value) != -1) {
		temp.mult_port = value;
	} else {
		temp.mult_port = 9000;
	}

	memcpy(config, &temp, sizeof(rtsp_server_config));
	return 0;
}

int app_rtsp_config_save()
{
	app_rtsp_config_fwrite(1);
}

static int app_rtsp_config_fwrite(int reset)
{
	if(g_rtsp_shandle == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	//	int total = 0;
	int ret = 0;
	//int value = 0;
	//	int i = 0;
	char bak_name[512] = {0};
	//	char section[256] = {0};
	char cmd[512] = {0};
	int value = 0;
	int used = 0;
	sprintf(bak_name, "%s.bak", RTSP_SERVER_CONFIG_INI);

	stream_output_server_config config ;
	mid_mutex_lock(g_rtsp_shandle->mutex);
	config = g_rtsp_shandle->config;
	used = g_rtsp_shandle->s_used;
	mid_mutex_unlock(g_rtsp_shandle->mutex);
	/*写接口到bak文件*/

	value = config.tc_flag;
	ret = app_ini_write_int(bak_name, "rtsp", "tc", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	//	value = config.active;
	//	ret = app_ini_write_int(bak_name, "rtsp", "active", value);

	//	if(ret == -1) {
	//		ERR_PRN("\n");
	//		return -1;
	//	}

	value = config.tos;
	ret = app_ini_write_int(bak_name, "rtsp", "tos", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.ttl;
	ret = app_ini_write_int(bak_name, "rtsp", "ttl", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.mtu;
	ret = app_ini_write_int(bak_name, "rtsp", "mtu", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.audio_port;
	ret = app_ini_write_int(bak_name, "rtsp", "audio_port", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.video_port;
	ret = app_ini_write_int(bak_name, "rtsp", "video_port", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	//ip
	ret = app_ini_write_string(bak_name, "rtsp", "main_ip", config.main_ip);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.type;
	ret = app_ini_write_int(bak_name, "rtsp", "type", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.status;
	ret = app_ini_write_int(bak_name, "rtsp", "status", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = used;

	if(reset == 1) {
		value = 0;
	}

	ret = app_ini_write_int(bak_name, "rtsp", "used", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	value = config.tc_rate;
	ret = app_ini_write_int(bak_name, "rtsp", "tc_rate", value);

	if(ret == -1) {
		ERR_PRN("\n");
		return -1;
	}

	/*拷贝bak文件到ini*/
	if(ret != -1) {
		sprintf(cmd, "cp -rf %s %s;rm -rf %s", bak_name, RTSP_SERVER_CONFIG_INI, bak_name);
		system(cmd);
		PRINTF("i will run cmd:%s\n", cmd);
	}

	/*拷贝bak文件到ini*/

	return ret;
}

static int app_rtsp_config_fread(RTSP_Output_Server_Handle *handle)
{
	char 			temp[512] = {0};
	int 			ret  = -1 ;
	int 	value = 0;
	char 			config_file[256] = {0};

	strcpy(config_file, RTSP_SERVER_CONFIG_INI);

	ret = app_ini_read_int(config_file, "rtsp", "used", &value);

	if(ret == -1 || value == NOTUSED) {
		PRINTF("warning ,the rtsp is not used\n");
		return -1;
	}

	handle->s_used = value;

	if(app_ini_read_int(config_file, "rtsp", "status", &value) != -1) {
		handle->config.status = value;
	}

	if(app_ini_read_int(config_file, "rtsp", "type", &value) != -1) {
		if(value == TYPE_RTSP) {
			handle->config.type = value;
		} else {
			handle->config.type = TYPE_RTSP;
		}
	}

	if(app_ini_read_string(config_file, "rtsp", "main_ip", temp, sizeof(temp)) != -1) {
		strcpy(handle->config.main_ip, temp);
	}

	if(app_ini_read_int(config_file, "rtsp", "video_port", &value) != -1) {
		handle->config.video_port = value;
	}

	if(app_ini_read_int(config_file, "rtsp", "audio_port", &value) != -1) {
		handle->config.audio_port = value;
	}

	if(app_ini_read_int(config_file, "rtsp", "mtu", &value) != -1) {
		if(value > 228 || value < 1500) {
			handle->config.mtu = value;
		} else {
			handle->config.mtu = 1500;
		}
	}

	if(app_ini_read_int(config_file, "rtsp", "ttl", &value) != -1) {
		handle->config.ttl = value;
	}

	if(app_ini_read_int(config_file, "rtsp", "tos", &value) != -1) {
		handle->config.tos = value;
	}

	//	if(app_ini_read_int(config_file, "rtsp", "active", &value) != -1) {
	//		handle->config.active = value;
	//	}

	if(app_ini_read_int(config_file, "rtsp", "tc", &value) != -1) {
		handle->config.tc_flag = value;
	}

	if(app_ini_read_int(config_file, "rtsp", "tc_rate", &value) != -1) {
		handle->config.tc_rate = value;
	}

	//check config

	//set config


	return 0;
}

void app_rtsp_server_init()
{
	if(g_rtsp_shandle != NULL) {
		PRINTF("warnning,the handle is already init\n");
		return ;
	}

	int ret = 0;
	//init handle  and set the handle default config
	g_rtsp_shandle = (RTSP_Output_Server_Handle *)malloc(sizeof(RTSP_Output_Server_Handle));

	if(g_rtsp_shandle == NULL) {
		PRINTF("Error,malloc handle is failed\n");
		return ;
	}

	memset(g_rtsp_shandle, 0, sizeof(RTSP_Output_Server_Handle));
	g_rtsp_shandle->mutex = mid_mutex_create();

	if(g_rtsp_shandle->mutex == NULL) {
		PRINTF("Error,create mutex is failed\n");
		rtsp_handle_delete(&g_rtsp_shandle);
		g_rtsp_shandle = NULL;
		return;
	}

	g_rtsp_shandle->config.type = TYPE_RTSP ;
	g_rtsp_shandle->config.status = S_BEGIN;

	app_rtsp_global_config_fread(&(g_rtsp_shandle->rtsp_config));
	//read from flash
	ret = app_rtsp_config_fread(g_rtsp_shandle);

	if(ret == -1) {
		PRINTF("there is no rtsp server\n");
		return ;
	}

	char buff[64] = {0};
	rtsp_get_local_ip(buff, sizeof(buff));
	strcpy(g_rtsp_shandle->config.main_ip, buff);

	/*set rtsp main_port*/
	RtspSetPort(g_rtsp_shandle->rtsp_config.s_port);
	rtsp_set_keepactive_time(g_rtsp_shandle->rtsp_config.active);
	//if active ,will open rtsp server
	app_rtsp_server_start(&(g_rtsp_shandle->config));
	//then set the rtsp server config

	app_rtsp_config_fwrite(0);
	app_rtsp_global_config_fwrite(&(g_rtsp_shandle->rtsp_config));

	rtsp_handle_printf(g_rtsp_shandle);

#ifdef TEST
	//	char temp[64] = {0};
	//	app_get_local_ip(temp);
	//	test_set_old_mult(temp, g_rtsp_shandle->config.main_port, 2);
#endif
}


static int app_rtsp_server_start(stream_output_server_config *config)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	int save_flag = 1;
	stream_output_server_config *temp = NULL;
	mid_mutex_lock(g_rtsp_shandle->mutex);
	temp = &(g_rtsp_shandle->config);

	if(config->status == temp->status) {
		save_flag = 0;
		PRINTF("Warning,the staus is same,the status is %d.\n", config->status);
	}

#if 0

	if(strcmp(config->main_ip, temp->main_ip) != 0) {
		PRINTF("Debug,the rtsp server send ip is changed ,the new ip is %s\n", config->main_ip);
	}

#endif
	g_rtsp_shandle->config.status = config->status;
	app_rtsp_set_active(g_rtsp_shandle->config.status);
	mid_mutex_unlock(g_rtsp_shandle->mutex);
	//set the ip and port to the lib
	//RtspSetPort(g_rtsp_shandle->config.main_port);

	//save flash
	if(save_flag == 1) {
		app_rtsp_config_fwrite(0);
	}

	return 0;
}

static int app_rtsp_server_stop(stream_output_server_config *config)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	int save_flag = 1;
	stream_output_server_config *temp = NULL;
	mid_mutex_lock(g_rtsp_shandle->mutex);
	temp = &(g_rtsp_shandle->config);

	if(config->status == temp->status) {
		save_flag = 0;
		PRINTF("Warning,the staus is same,the status is %d.\n", config->status);
	}

#if 0

	if(strcmp(config->main_ip, temp->main_ip) != 0) {
		PRINTF("Debug,the rtsp server send ip is changed ,the new ip is %s\n", config->main_ip);
	}

#endif
	g_rtsp_shandle->config.status = config->status;
	app_rtsp_set_active(g_rtsp_shandle->config.status);
	mid_mutex_unlock(g_rtsp_shandle->mutex);

	//set the ip and port to the lib
	if(save_flag == 1) {
		//close all the client
		rtsp_close_all_client();
		//close all the client send

		//save to flash
		app_rtsp_config_fwrite(0);
	}

	return 0;
}


int app_rtsp_server_add(stream_output_server_config *config, stream_output_server_config *newconfig)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	if((config->status == S_DEL) || (config->type != TYPE_RTSP)) {
		ERR_PRN("Error,status=%d,type=%d\n", config->status, config->type);
		return -1;
	}


	//int i = 0;
	mid_mutex_lock(g_rtsp_shandle->mutex);

	memcpy(&(g_rtsp_shandle->config), config, sizeof(stream_output_server_config));
	g_rtsp_shandle->config.status = S_ACTIVE;
	g_rtsp_shandle->s_used = ISUSED;
	app_rtsp_set_active(g_rtsp_shandle->config.status);
	mid_mutex_unlock(g_rtsp_shandle->mutex);

	//save to flash
	app_rtsp_config_fwrite(0);

	return 0;
}

int app_rtsp_server_delete()
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	int i = 0;
	Stream_Output_Client *client = NULL;

	//close all the client

	//close all the client send
	for(i = 0 ; i < RTSP_MAX_CLIENT_NUM ; i++) {
		client = (g_rtsp_shandle->client[i]);
		stream_close_rtsp_client(i, client);
	}


	mid_mutex_lock(g_rtsp_shandle->mutex);
	memset(&(g_rtsp_shandle->config), 0, sizeof(stream_output_server_config));
	g_rtsp_shandle->s_used = NOTUSED;
	app_rtsp_set_active(g_rtsp_shandle->config.status);
	mid_mutex_unlock(g_rtsp_shandle->mutex);

	PRINTF("\n");
	//save to flash
	app_rtsp_config_fwrite(0);
	return 0;
}

#if 0
/*获取rtsp的状态*/
static int app_rtsp_server_status()
{
	if(g_rtsp_shandle == NULL) {
		//PRINTF("\n");
		return -1;
	}

	int status  = 0;
	mid_mutex_lock(g_rtsp_shandle->mutex);

	if(g_rtsp_shandle->s_used == ISUSED && g_rtsp_shandle->config.status == S_ACTIVE) {
		status = 1;
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	//PRINTF("status = %d\n", status);
	return status;
}
#endif
int app_rtsp_server_used()
{
	if(g_rtsp_shandle == NULL) {
		//PRINTF("\n");
		return -1;
	}

	int status  = 0;
	mid_mutex_lock(g_rtsp_shandle->mutex);

	if(g_rtsp_shandle->s_used == ISUSED) {
		status = 1;
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	PRINTF("status = %d\n", status);
	return status;
}

static void rtsp_ginfo_print(rtsp_server_config *config)
{
	PRINTF("rtsp_ginfo_print: config= %p\n", config);
	PRINTF("s_port=%d,active=%d,s_mtu=%d,mult_flag=%d,ip=%s:%d\n", config->s_port, config->active, config->s_mtu, config->mult_flag, config->mult_ip, config->mult_port);
}
int app_rtsp_server_set_global_info(rtsp_server_config *config, rtsp_server_config *newconfig)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	rtsp_ginfo_print(config);
	int need_save = 0;
	mid_mutex_lock(g_rtsp_shandle->mutex);
	rtsp_server_config *old = &(g_rtsp_shandle->rtsp_config);

	/*rtsp listen port*/
	if((config->s_port != old->s_port) && (config->s_port >= 81 && config->s_port <= 65535)) {
		old->s_port = config->s_port;
		need_save = 1;
	}

	/*rtsp active time 10-900000*/
	if(config->active != old->active) {
		if(config->active >= 10 && config->active <= 900000) {
			old->active = config->active;
			need_save  = 1;
			//set rtsp lib the active
			rtsp_set_keepactive_time(g_rtsp_shandle->rtsp_config.active);

		}
	}

	//mtu 228--1500
	if(config->s_mtu != old->s_mtu) {
		if(config->s_mtu >= 228 && config->s_mtu <= 1500) {
			old->s_mtu = config->s_mtu ;
			need_save = 1;
		}
	}

	if(config->mult_flag == 0 || config->mult_flag == 1) {
		if(config->mult_flag != old->mult_flag) {
			//multicast
			if(config->mult_flag == 1) {
				old->mult_flag = config->mult_flag;
				strcpy(old->mult_ip, config->mult_ip);
				old->mult_port = config->mult_port;
				need_save = 1;


			} else {
				old->mult_flag = config->mult_flag;
				need_save = 1;

			}
		} else if((config->mult_flag == old->mult_flag) && (config->mult_flag == 1)) {
			if(strcmp(old->mult_ip, config->mult_ip) != 0 || old->mult_port != config->mult_port) {
				strcpy(old->mult_ip, config->mult_ip);
				old->mult_port = config->mult_port;
				need_save = 1;


			}
		}
	}


	//save flash
	if(need_save == 1) {
		app_rtsp_global_config_fwrite(old);
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}
int app_rtsp_server_get_global_info(rtsp_server_config *config)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	mid_mutex_lock(g_rtsp_shandle->mutex);
	memcpy(config, &(g_rtsp_shandle->rtsp_config), sizeof(rtsp_server_config));
	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}

int app_rtsp_server_get_common_info(stream_output_server_config *config)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	mid_mutex_lock(g_rtsp_shandle->mutex);
	char buff[64] = {0};
	rtsp_get_local_ip(buff, sizeof(buff));
	strcpy(g_rtsp_shandle->config.main_ip, buff);
	memcpy(config, &(g_rtsp_shandle->config), sizeof(stream_output_server_config));
	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}



//不对状态进行操作
int app_rtsp_server_set_common_info(stream_output_server_config *in, stream_output_server_config *out)
{
	if(in == NULL || out == NULL) {
		PRINTF("Error.\n");
		return -1;
	}

	int tc_change = 0;
	int ttl_change = 0;

	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	mid_mutex_lock(g_rtsp_shandle->mutex);

	if(in->mtu >= 228 && in->mtu <= 1500) {
		g_rtsp_shandle->config.mtu = in->mtu;
		PRINTF("set mtu=%d\n", in->mtu);
	}

	if(in->ttl >= 1 && in->ttl <= 255) {
		g_rtsp_shandle->config.ttl = in->ttl;
		PRINTF("set ttl=%d\n", in->ttl);
		ttl_change = 2;
	}

	if(in->tos >= 0 && in->tos <= 255) {
		g_rtsp_shandle->config.tos = in->tos;
		PRINTF("set tos=%d\n", in->tos);
		ttl_change = 2;
	}

	if(in->tc_flag != g_rtsp_shandle->config.tc_flag) {
		tc_change = 1;
		g_rtsp_shandle->config.tc_flag = in->tc_flag;
		PRINTF("tc_flag change to %d\n", in->tc_flag);
	}

	if(in->tc_rate != g_rtsp_shandle->config.tc_rate) {
		tc_change = 1;
		g_rtsp_shandle->config.tc_rate = in->tc_rate;
		PRINTF("tc_rate change to %d\n", in->tc_rate);
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);

	//add code for tc by zm
	if(tc_change == 1) {
		stream_rtsp_set_all_tc();
	}

	if(ttl_change == 2) {
		stream_rtsp_set_all_config();
	}

	app_rtsp_config_fwrite(0);

	return 0;

}


int app_rtsp_server_set_status(stream_output_server_config *in, stream_output_server_config *out)
{
	if(in == NULL || out == NULL) {
		PRINTF("Error.\n");
		return -1;
	}

	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	if(in->status == S_ACTIVE) {
		app_rtsp_server_start(in);
	} else if(in->status == S_STOP) {
		app_rtsp_server_stop(in);
	}

	app_rtsp_server_get_common_info(out);
	return 0;
}


static int g_rtsp_is_active = 0;
int app_rtsp_get_active()
{
	return g_rtsp_is_active;
}

static int app_rtsp_set_active(int status)
{
	if(status == S_ACTIVE) {
		g_rtsp_is_active = 1;
	} else {
		g_rtsp_is_active = 0;
	}

	return 0;
}

int stream_server_config_printf(stream_output_server_config *config)
{
	if(config == NULL) {
		return -1;
	}

	PRINTF("server config %p,the conf nume=%d,type=%d,status=%d\n", config, config->num, config->type, config->status);
	PRINTF("ip:%s:%d/%d,mtu=%d,ttl=%d,tos=0x%x,tc_flag =%d.ceile=%d\n", config->main_ip, config->video_port, config->audio_port, config->mtu, config->ttl, config->tos, config->tc_flag, config->tc_rate);
	return 0;
}

/*==============================================================================
	函数: stream_get_rtsp_ginfo
	作用:
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
int stream_get_rtsp_ginfo(int *mult, char *ip, unsigned short *vport, unsigned short *aport)
{
	if(g_rtsp_shandle == NULL) {
		PRINTF("\n");
		return -1;
	}

	mid_mutex_lock(g_rtsp_shandle->mutex);
	*mult = g_rtsp_shandle->rtsp_config.mult_flag;

	if(*mult == 1) {
		strcpy(ip, g_rtsp_shandle->rtsp_config.mult_ip);
		*vport = g_rtsp_shandle->rtsp_config.mult_port;
		*aport = *vport + 2;
	} else {
		*vport  = g_rtsp_shandle->config.video_port;
		*aport = g_rtsp_shandle->config.audio_port;
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}



/*==============================================================================
	函数: stream_create_rtsp_client
	作用:
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
void *stream_create_rtsp_client(int pos, struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr)
{
	//int ret = 0;
	int num = 0;
	int tc_flag = 0;
	int tc_rate = 0;
	char dst_ip[16] ;
	int ttl = 0;
	int tos = 0;
	unsigned short port = 0;

	Stream_Output_Client *client = NULL;
	stream_output_server_config config;
	client = stream_client_is_repeat(video_addr, audio_addr);

	if(client != NULL) {
		stream_client_set_mnum(client, 1);
		return (void *)client;
	}

	PRINTF("\n");
	client = stream_client_get_free();

	if(client == NULL) {
		PRINTF("Error,there is not free client\n");
		return NULL;
	}


	stream_client_open(video_addr, audio_addr, TYPE_RTP,  client);
	mid_mutex_lock(g_rtsp_shandle->mutex);
	g_rtsp_shandle->client[pos] = client;
	ttl = g_rtsp_shandle->config.ttl;
	tos =  g_rtsp_shandle->config.tos;
	mid_mutex_unlock(g_rtsp_shandle->mutex);

	//set ttl
	stream_client_set(client, ttl, tos);
	//set tos



	//add tc code by zm
	memset(&config, 0, sizeof(stream_output_server_config));
	app_rtsp_server_get_common_info(&config);

	num = client->num + 1;
	tc_flag = config.tc_flag;
	tc_rate = config.tc_rate;

	inet_ntop(AF_INET, &(video_addr->sin_addr), dst_ip, 16);
	port = ntohs(video_addr->sin_port);

	PRINTF("num = %d,tc_flag =%d,tc_Rate=%d,dsp_ip=%s,port=%u.\n", num, tc_flag, tc_rate, dst_ip, port);
	stream_rtsp_set_clinet_tc(num, tc_flag, tc_rate, dst_ip,  port);


	return (void *)client;
}

/*==============================================================================
	函数: stream_close_rtsp_client
	作用: 关闭一个rtsp 的 发送的client
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
int stream_close_rtsp_client(int pos, void *p)
{
	if(p == NULL) {
		PRINTF("client is NULL\n");
		return -1;
	}

	Stream_Output_Client *client = (Stream_Output_Client *)p;
	stream_client_set_mnum(client, -1);

	if(steram_client_get_mnum(client) <= 0) {
		PRINTF("i will close the client\n");

		//add by zhangmin
		stream_rtsp_del_client_tc(client->num + 1);

		stream_client_close(client);
		mid_mutex_lock(g_rtsp_shandle->mutex);
		g_rtsp_shandle->client[pos] = NULL;
		mid_mutex_unlock(g_rtsp_shandle->mutex);
		return 0;
	}


	return 0;
}


/*==============================================================================
	函数: stream_get_rtsp_mtu
	作用:  获取 rtsp的rtp的mtu
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
int stream_get_rtsp_mtu()
{
	int mtu = 1450;

	if(RtspClientIsNull() == 0 ||  g_rtsp_shandle == NULL) {
		return mtu ;
	}

	mid_mutex_lock(g_rtsp_shandle->mutex);

	if(g_rtsp_shandle->config.mtu >= 228 && g_rtsp_shandle->config.mtu <= 1500) {
		mtu = g_rtsp_shandle->config.mtu;
		//PRINTF("mtu=%d\n",mtu);
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return mtu;
}

/*==============================================================================
	函数: stream_rtsp_get_active
	作用: 获取当前是否存在rtsp客户端
	输入:
	输出:
	作者:  modify by zm    2012.04.26
==============================================================================*/
int stream_rtsp_get_active()
{
	if(g_rtsp_shandle == NULL) {
		return 0;
	}

	int active = app_rtsp_get_active();

	if(active != S_ACTIVE) {
		return 0;
	}

	return RtspClientIsNull();
}

int stream_rtsp_set_all_tc()
{
	int i = 0;
	int num = 0;
	int tc_flag = 0;
	int tc_rate = 0;
	char dst_ip[16] ;
	unsigned short port = 0;

	Stream_Output_Client *client = NULL;
	stream_output_server_config config;

	memset(&config, 0, sizeof(stream_output_server_config));
	app_rtsp_server_get_common_info(&config);

	mid_mutex_lock(g_rtsp_shandle->mutex);

	//close all the client send
	for(i = 0 ; i < RTSP_MAX_CLIENT_NUM ; i++) {
		client = (g_rtsp_shandle->client[i]);

		if(client == NULL || client->c_used == NOTUSED) {
			continue;
		}

		//stream_close_rtsp_client(client);
		num = client->num + 1;
		tc_flag = config.tc_flag;
		tc_rate = config.tc_rate;

		inet_ntop(AF_INET, &(client->video_addr.sin_addr), dst_ip, 16);
		port = ntohs(client->video_addr.sin_port);

		PRINTF("num = %d,tc_flag =%d,tc_Rate=%d,dsp_ip=%s,port=%u.\n", num, tc_flag, tc_rate, dst_ip, port);
		stream_rtsp_set_clinet_tc(num, tc_flag, tc_rate, dst_ip,  port);
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}

int stream_rtsp_add_client_tc(int num, int tc_flag, int tc_rate, char *dst_ip, unsigned short port)
{
	int vrate = 0;
	int vcbr = 0;
	int arate = 0;
	int trate = 0;
	int crate = 0;
	int ret = 0;
	//	int tc_flag =0;
	int tc_num = num + 1;


	app_get_encode_rate(&vrate, &arate, &vcbr);

	//	if(tc_flag == NOTUSED || vcbr == 0) {
	//		if( tc_num != -1) {
	//			tc_del_element(tc_num);
	//node->tc_num = -1;
	//		}

	//		return 0;
	//	}
	if(vcbr == 0) {
		return -1;
	}

	//	if(tc_flag == ISUSED) {
	//if(node->client == NULL) {
	//		if(node->tc_num != -1) {
	//			tc_del_element(node->tc_num);
	//			node->tc_num = -1;
	//		}
	//
	//			return 0;
	//		}

	//num =  ;

	if(vcbr == 1) {
		trate = app_trans_rate(vrate, arate, TYPE_RTP);
		crate = trate * (tc_rate + 100) / 100;
		//set rate
		ret = tc_add_element(num, dst_ip, port, trate, crate);

		if(ret < 0) {
			PRINTF("Warnning\n");
			return -1;
			//node->tc_num = -1;
		} else {
			return 0;
			//node->tc_num = num;
		}
	}

	return -1;
}

int stream_rtsp_del_client_tc(int num)
{
	PRINTF("tc num = %d\n", num);
	tc_del_element(num);
	return 0;
}

static int stream_rtsp_set_clinet_tc(int num, int tc_flag, int tc_rate, char *dst_ip, unsigned short port)
{
	int vrate = 0;
	int vcbr = 0;
	int arate = 0;
	int trate = 0;
	int crate = 0;
	int ret = 0;
	//	int tc_flag =0;
	int tc_num = num ;

	tc_del_element(tc_num);

	app_get_encode_rate(&vrate, &arate, &vcbr);

	if(vcbr == 0 || tc_flag == NOTUSED) {
		PRINTF("vcbr = %d,tc_flag =%d\n", vcbr, tc_flag);
		return -1;
	}

	if(vcbr == 1) {
		trate = app_trans_rate(vrate, arate, TYPE_RTP);
		crate = trate * (tc_rate + 100) / 100;
		//set rate
		ret = tc_add_element(num, dst_ip, port, trate, crate);

		if(ret < 0) {
			PRINTF("Warnning\n");
			return -1;
			//node->tc_num = -1;
		} else {
			return 0;
			//node->tc_num = num;
		}
	}

	return -1;

}
#if 0
/*设置某个节点的状态*/
static int app_mult_set_node_tc(MULTICAST_Output_Server_NODE *node)
{
	if(node == NULL) {
		PRINTF("Error\n");
		return -1;
	}

	int num = 0;
	int vrate = 0;
	int vcbr = 0;
	int arate = 0;
	int trate = 0;
	int crate = 0;
	int ret = 0;

	app_get_encode_rate(&vrate, &arate, &vcbr);

	if(node->config.tc_flag == NOTUSED || vcbr == 0) {
		if(node->tc_num != -1) {
			tc_del_element(node->tc_num);
			node->tc_num = -1;
		}

		return 0;
	}

	if(node->config.tc_flag == ISUSED) {
		if(node->client == NULL) {
			if(node->tc_num != -1) {
				tc_del_element(node->tc_num);
				node->tc_num = -1;
			}

			return 0;
		}

		num = node->client->num + 1;


		if(vcbr == 1) {
			trate = app_trans_rate(vrate, arate, node->config.type);
			crate = trate * (node->config.tc_rate + 100) / 100;
			//set rate
			ret = tc_add_element(num, node->config.main_ip, node->config.video_port, arate, crate);

			if(ret < 0) {
				PRINTF("Warnning\n");
				node->tc_num = -1;
			} else {
				node->tc_num = num;
			}
		}
	}

	return 0;
}

#endif

int stream_rtsp_set_all_config()
{
	int i = 0;

	int tos = 0;
	int ttl = 0;
	char dst_ip[16] ;
	unsigned short port = 0;

	Stream_Output_Client *client = NULL;
	stream_output_server_config config;

	memset(&config, 0, sizeof(stream_output_server_config));
	app_rtsp_server_get_common_info(&config);

	mid_mutex_lock(g_rtsp_shandle->mutex);

	//close all the client send
	for(i = 0 ; i < RTSP_MAX_CLIENT_NUM ; i++) {
		client = (g_rtsp_shandle->client[i]);

		if(client == NULL || client->c_used == NOTUSED) {
			continue;
		}

		tos = config.tos;
		ttl = config.ttl;
		stream_client_set(client, ttl, tos);
	}

	mid_mutex_unlock(g_rtsp_shandle->mutex);
	return 0;
}


