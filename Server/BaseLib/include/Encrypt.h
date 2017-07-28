#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "GlobalDefs.h"
//////////////////////////////////////////////////////////////////////////

//MD5 加密类
class CMD5Encrypt
{
	//函数定义
private:
	//构造函数
	CMD5Encrypt() {}

	//功能函数
public:
	//生成密文
	static void EncryptData(const char * pszSrcData, char szMD5Result[33]);
};

/////////////////////////////////////////////////////////////////////////////////
//DES 加密类
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
	//函数定义
public:
	//构造函数
	CDESEncrypt() {}

	//功能函数
public:
	void SetKey(const_DES_cblock *key);

	/**
	* @brief DES加密解密
	* @param inData 输入数据（加密时为明文，解密时为密文）
	* @param inSize 输入数据长度
	* @param outBuff 输出缓冲区
	* @param buffSize 输出缓冲区大小
	* @param enc true 加密，false 解密
	* @return unsigned int  <0加密或解密失败，>=0 输出数据占输出缓冲区的实际大小
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

