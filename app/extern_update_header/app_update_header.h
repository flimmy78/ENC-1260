#ifndef _APP_UPDATE_HEADER_H__
#define _APP_UPDATE_HEADER_H__

typedef struct {
	int info_len; //sizeof(UP_HEADER_INFO)
	unsigned int checksum; //�ṹ���checksum
	char product_id[16]; //ENC1200VGA/SDI
	int major_version;         // Ԥ��, ���汾����һ�£��������� ����3.0,2.0��
	int minor_version;		  // Ԥ�� 0
	int pcb_version; //Ԥ��,
	int kernel_version; 		//Ԥ��,
	int fpga_version; 			//Ԥ��,
	int vss_version;		  // Ԥ��,
	int force;                //Ԥ��,ǿ��  //���ع�ע���汾��һ����???
	int compressmode;         //Ԥ��,ѹ��ģʽ��Ԥ��
	unsigned int filesystem;  //Ԥ��,�ļ�ϵͳ yaffs ,jaffs
	char target[16];          //Ԥ�� ������kernel ,app��
	unsigned char custom_info[32];   //���Ʋ�Ʒ��Ϣ
	char reserve[8];		//Ԥ��
} UP_HEADER_INFO; //len

#define FIRST_UPDATE_HEADER "##################SZREACH#######"  //32 char 

#define UPDATE_HEARER_LEN 256 //��λ���0xff





#endif


