#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "GlobalDefs.h"
//////////////////////////////////////////////////////////////////////////

//MD5 ������
class CMD5Encrypt
{
	//��������
private:
	//���캯��
	CMD5Encrypt() {}

	//���ܺ���
public:
	//��������
	static void EncryptData(const char * pszSrcData, char szMD5Result[33]);
};

/////////////////////////////////////////////////////////////////////////////////
//DES ������
#include <stdlib.h>
#define DES_ENCRYPT	1
#define DES_DECRYPT	0
//#define DES_LONG unsigned long
#define DES_LONG unsigned int
typedef unsigned char DES_cblock[8];
typedef /* const */ unsigned char const_DES_cblock[8];
typedef DES_LONG t_DES_SPtrans[8][64];
/* With "const", gcc 2.8.1 on Solaris thinks that DES_cblock *
* and const_DES_cblock * are incompatible pointer types. */
  
extern t_DES_SPtrans MyDES_SPtrans;

typedef struct DES_ks
{
	union
	{
		DES_cblock cblock;
		/* make sure things are correct size on machines with
		* 8 byte longs */
		DES_LONG deslong[2];
	} ks[16];
} DES_key_schedule;

class CDESEncrypt
{
	//��������
public:
	//���캯��
	CDESEncrypt() {}

	//���ܺ���
public:
	void SetKey(const_DES_cblock *key);

	/**
	* @brief DES���ܽ���
	* @param inData �������ݣ�����ʱΪ���ģ�����ʱΪ���ģ�
	* @param inSize �������ݳ���
	* @param outBuff ���������
	* @param buffSize �����������С
	* @param enc true ���ܣ�false ����
	* @return unsigned int  <0���ܻ����ʧ�ܣ�>=0 �������ռ�����������ʵ�ʴ�С
	*/
	int EncdecData(void* inData, unsigned int inSize, void* outBuff, unsigned int buffSize, bool enc);

private:
	int encdec_des(unsigned char * inData, unsigned int inSize, unsigned char *outBuff, unsigned int buffSize, bool enc);
private:
	DES_key_schedule	m_key_des;
	unsigned int		m_enc_mask;
	unsigned int		m_dec_mask;
};


#endif /* ENCRYPT_H */

