
#include "Encrypt.h"

#include <stdio.h>
#include <string.h>
//数据大小
#define ENCRYPT_KEY_LEN					5									//密钥长度

//////////////////////////////////////////////////////////////////////////

#define S11		7
#define S12		12
#define S13		17
#define S14		22
#define S21		5
#define S22		9
#define S23 	14
#define S24 	20
#define S31 	4
#define S32 	11
#define S33 	16
#define S34 	23
#define S41 	6
#define S42 	10
#define S43 	15
#define S44 	21

#define F(x,y,z) (((x)&(y))|((~x)&(z)))
#define G(x,y,z) (((x)&(z))|((y)&(~z)))
#define H(x,y,z) ((x)^(y)^(z))
#define I(x,y,z) ((y)^((x)|(~z)))

#define ROTATE_LEFT(x,n) (((x)<<(n))|((x)>>(32-(n))))

#define FF(a,b,c,d,x,s,ac)													\
{																			\
	(a)+=F((b),(c),(d))+(x)+(unsigned long int)(ac);						\
	(a)=ROTATE_LEFT((a),(s));												\
	(a)+=(b);																\
}

#define GG(a,b,c,d,x,s,ac)													\
{																			\
	(a)+=G((b),(c),(d))+(x)+(unsigned long int)(ac);						\
	(a)=ROTATE_LEFT ((a),(s));												\
	(a)+=(b);																\
}

#define HH(a,b,c,d,x,s,ac)													\
{																			\
	(a)+=H((b),(c),(d))+(x)+(unsigned long int)(ac);						\
	(a)=ROTATE_LEFT((a),(s));												\
	(a)+=(b);																\
}

#define II(a,b,c,d,x,s,ac)													\
{																			\
	(a)+=I((b),(c),(d))+(x)+(unsigned long int)(ac);						\
	(a)=ROTATE_LEFT((a),(s));												\
	(a)+=(b);																\
}

//////////////////////////////////////////////////////////////////////////

//MD5 加密类
class CMD5
{
	//变量定义
private:
	unsigned long int				state[4];
	unsigned long int				count[2];
	unsigned char					buffer[64];
	unsigned char					PADDING[64];

	//函数定义
public:
	//构造函数
	CMD5() { MD5Init(); }

	//功能函数
public:
	//最终结果
	void MD5Final(unsigned char digest[16]);
	//设置数值
	void MD5Update(unsigned char * input, unsigned int inputLen);

	//内部函数
private:
	//初始化
	void MD5Init();
	//置位函数
	void MD5_memset(unsigned char * output, int value, unsigned int len);
	//拷贝函数
	void MD5_memcpy(unsigned char * output, unsigned char * input, unsigned int len);
	//转换函数
	void MD5Transform(unsigned long int state[4], unsigned char block[64]);
	//编码函数
	void Encode(unsigned char * output, unsigned long int * input, unsigned int len);
	//解码函数
	void Decode(unsigned long int *output, unsigned char * input, unsigned int len);
};

//////////////////////////////////////////////////////////////////////////

//初始化
void CMD5::MD5Init()
{
	count[0]=0;
	count[1]=0;
	state[0]=0x67452301;
	state[1]=0xefcdab89;
	state[2]=0x98badcfe;
	state[3]=0x10325476;
	MD5_memset(PADDING,0,sizeof(PADDING));
	*PADDING=0x80;
	return;
}

//更新函数
void CMD5::MD5Update (unsigned char * input, unsigned int inputLen)
{
	unsigned int i,index,partLen;
	index=(unsigned int)((this->count[0]>>3)&0x3F);
	if ((count[0]+=((unsigned long int)inputLen<<3))<((unsigned long int)inputLen<<3)) count[1]++;
	count[1]+=((unsigned long int)inputLen>>29);
	partLen=64-index;
	if (inputLen>=partLen) 
	{
		MD5_memcpy((unsigned char*)&buffer[index],(unsigned char *)input,partLen);
		MD5Transform(state,buffer);
		for (i=partLen;i+63<inputLen;i+=64) MD5Transform(state,&input[i]);
		index=0;
	}
	else i=0;
	MD5_memcpy((unsigned char*)&buffer[index],(unsigned char *)&input[i],inputLen-i);
	return;
}

//最终结果
void CMD5::MD5Final(unsigned char digest[16])
{
	unsigned char bits[8];
	unsigned int index,padLen;
	Encode(bits,count,8);
	index=(unsigned int)((count[0]>>3)&0x3f);
	padLen=(index<56)?(56-index):(120-index);
	MD5Update( PADDING,padLen);
	MD5Update(bits,8);
	Encode(digest,state,16);
	MD5_memset((unsigned char*)this,0,sizeof (*this));
	MD5Init();
	return;
}

//转换函数
void CMD5::MD5Transform(unsigned long int state[4], unsigned char block[64])
{
	unsigned long int a=state[0],b=state[1],c=state[2],d=state[3],x[16];
	Decode(x,block,64);

	FF(a,b,c,d,x[ 0],S11,0xd76aa478); /* 1 */
	FF(d,a,b,c,x[ 1],S12,0xe8c7b756); /* 2 */
	FF(c,d,a,b,x[ 2],S13,0x242070db); /* 3 */
	FF(b,c,d,a,x[ 3],S14,0xc1bdceee); /* 4 */
	FF(a,b,c,d,x[ 4],S11,0xf57c0faf); /* 5 */
	FF(d,a,b,c,x[ 5],S12,0x4787c62a); /* 6 */
	FF(c,d,a,b,x[ 6],S13,0xa8304613); /* 7 */
	FF(b,c,d,a,x[ 7],S14,0xfd469501); /* 8 */
	FF(a,b,c,d,x[ 8],S11,0x698098d8); /* 9 */
	FF(d,a,b,c,x[ 9],S12,0x8b44f7af); /* 10 */
	FF(c,d,a,b,x[10],S13,0xffff5bb1); /* 11 */
	FF(b,c,d,a,x[11],S14,0x895cd7be); /* 12 */
	FF(a,b,c,d,x[12],S11,0x6b901122); /* 13 */
	FF(d,a,b,c,x[13],S12,0xfd987193); /* 14 */
	FF(c,d,a,b,x[14],S13,0xa679438e); /* 15 */
	FF(b,c,d,a,x[15],S14,0x49b40821); /* 16 */

	GG(a,b,c,d,x[ 1],S21,0xf61e2562); /* 17 */
	GG(d,a,b,c,x[ 6],S22,0xc040b340); /* 18 */
	GG(c,d,a,b,x[11],S23,0x265e5a51); /* 19 */
	GG(b,c,d,a,x[ 0],S24,0xe9b6c7aa); /* 20 */
	GG(a,b,c,d,x[ 5],S21,0xd62f105d); /* 21 */
	GG(d,a,b,c,x[10],S22,0x2441453);  /* 22 */
	GG(c,d,a,b,x[15],S23,0xd8a1e681); /* 23 */
	GG(b,c,d,a,x[ 4],S24,0xe7d3fbc8); /* 24 */
	GG(a,b,c,d,x[ 9],S21,0x21e1cde6); /* 25 */
	GG(d,a,b,c,x[14],S22,0xc33707d6); /* 26 */
	GG(c,d,a,b,x[ 3],S23,0xf4d50d87); /* 27 */
	GG(b,c,d,a,x[ 8],S24,0x455a14ed); /* 28 */
	GG(a,b,c,d,x[13],S21,0xa9e3e905); /* 29 */
	GG(d,a,b,c,x[ 2],S22,0xfcefa3f8); /* 30 */
	GG(c,d,a,b,x[ 7],S23,0x676f02d9); /* 31 */
	GG(b,c,d,a,x[12],S24,0x8d2a4c8a); /* 32 */

	HH(a,b,c,d,x[ 5],S31,0xfffa3942); /* 33 */
	HH(d,a,b,c,x[ 8],S32,0x8771f681); /* 34 */
	HH(c,d,a,b,x[11],S33,0x6d9d6122); /* 35 */
	HH(b,c,d,a,x[14],S34,0xfde5380c); /* 36 */
	HH(a,b,c,d,x[ 1],S31,0xa4beea44); /* 37 */
	HH(d,a,b,c,x[ 4],S32,0x4bdecfa9); /* 38 */
	HH(c,d,a,b,x[ 7],S33,0xf6bb4b60); /* 39 */
	HH(b,c,d,a,x[10],S34,0xbebfbc70); /* 40 */
	HH(a,b,c,d,x[13],S31,0x289b7ec6); /* 41 */
	HH(d,a,b,c,x[ 0],S32,0xeaa127fa); /* 42 */
	HH(c,d,a,b,x[ 3],S33,0xd4ef3085); /* 43 */
	HH(b,c,d,a,x[ 6],S34,0x4881d05);  /* 44 */
	HH(a,b,c,d,x[ 9],S31,0xd9d4d039); /* 45 */
	HH(d,a,b,c,x[12],S32,0xe6db99e5); /* 46 */
	HH(c,d,a,b,x[15],S33,0x1fa27cf8); /* 47 */
	HH(b,c,d,a,x[ 2],S34,0xc4ac5665); /* 48 */

	II(a,b,c,d,x[ 0],S41,0xf4292244); /* 49 */
	II(d,a,b,c,x[ 7],S42,0x432aff97); /* 50 */
	II(c,d,a,b,x[14],S43,0xab9423a7); /* 51 */
	II(b,c,d,a,x[ 5],S44,0xfc93a039); /* 52 */
	II(a,b,c,d,x[12],S41,0x655b59c3); /* 53 */
	II(d,a,b,c,x[ 3],S42,0x8f0ccc92); /* 54 */
	II(c,d,a,b,x[10],S43,0xffeff47d); /* 55 */
	II(b,c,d,a,x[ 1],S44,0x85845dd1); /* 56 */
	II(a,b,c,d,x[ 8],S41,0x6fa87e4f); /* 57 */
	II(d,a,b,c,x[15],S42,0xfe2ce6e0); /* 58 */
	II(c,d,a,b,x[ 6],S43,0xa3014314); /* 59 */
	II(b,c,d,a,x[13],S44,0x4e0811a1); /* 60 */
	II(a,b,c,d,x[ 4],S41,0xf7537e82); /* 61 */
	II(d,a,b,c,x[11],S42,0xbd3af235); /* 62 */
	II(c,d,a,b,x[ 2],S43,0x2ad7d2bb); /* 63 */
	II(b,c,d,a,x[ 9],S44,0xeb86d391); /* 64 */

	state[0]+=a;
	state[1]+=b;
	state[2]+=c;
	state[3]+=d;

	MD5_memset((unsigned char *)x,0,sizeof(x));

	return;
}

//编码函数
void CMD5::Encode(unsigned char * output, unsigned long int * input,unsigned int len)
{
	unsigned int i, j;
	for (i=0,j=0;j<len;i++,j+=4)
	{
		output[j]=(unsigned char)(input[i]&0xff);
		output[j+1]=(unsigned char)((input[i]>>8)&0xff);
		output[j+2]=(unsigned char)((input[i]>>16)&0xff);
		output[j+3]=(unsigned char)((input[i]>>24)&0xff);
	}
	return;
}

//解码函数
void CMD5::Decode(unsigned long int *output, unsigned char *input, unsigned int len)
{
	unsigned int i,j;
	for (i=0,j=0;j<len;i++,j+=4)
	{
		output[i]=((unsigned long int)input[j])|(((unsigned long int)input[j+1])<<8)|
			(((unsigned long int)input[j+2])<<16)|(((unsigned long int)input[j+3])<< 24);
	}
	return;
}

//拷贝函数
void CMD5::MD5_memcpy(unsigned char * output, unsigned char * input,unsigned int len)
{
	for (unsigned int i=0;i<len;i++) output[i]=input[i];
}

//置位函数
void CMD5::MD5_memset (unsigned char * output, int value, unsigned int len)
{
	for (unsigned int i=0;i<len;i++) ((char *)output)[i]=(char)value;
}

//////////////////////////////////////////////////////////////////////////

//生成密文
void CMD5Encrypt::EncryptData(const char * pszSrcData, char szMD5Result[16])
{
	//加密密文
	CMD5 MD5Encrypt;
	unsigned char szResult[16];
	MD5Encrypt.MD5Update((unsigned char *)pszSrcData,(unsigned int)strlen(pszSrcData)*sizeof(char));
	MD5Encrypt.MD5Final(szResult);

	//输出结果
	szMD5Result[0]=0;
	for (int i=0;i<16;i++) 
		sprintf(&szMD5Result[i*2],"%02x",szResult[i]);

	return;
}

//////////////////////////////////////////////////////////////////////////
//DES

extern const DES_LONG (*sp)[8][64];

#define ROTATE(a,n)     (((a)>>(int)(n))|((a)<<(32-(int)(n))))
#define DES_KEY_SZ 	(sizeof(DES_cblock))

//#define	ROTATE(a,n)	(_lrotr(a,n))

#define LOAD_DATA(R,S,u,t,E0,E1,tmp) \
	u=R^s[S  ]; \
	t=R^s[S+1]
#define LOAD_DATA_tmp(a,b,c,d,e,f) LOAD_DATA(a,b,c,d,e,f,g)

#define D_ENCRYPT(LL,R,S) {\
	LOAD_DATA_tmp(R,S,u,t,E0,E1); \
	t=ROTATE(t,4); \
	LL^=\
	(*sp)[0][(u>> 2L)&0x3f]^ \
	(*sp)[2][(u>>10L)&0x3f]^ \
	(*sp)[4][(u>>18L)&0x3f]^ \
	(*sp)[6][(u>>26L)&0x3f]^ \
	(*sp)[1][(t>> 2L)&0x3f]^ \
	(*sp)[3][(t>>10L)&0x3f]^ \
	(*sp)[5][(t>>18L)&0x3f]^ \
	(*sp)[7][(t>>26L)&0x3f]; }

#define PERM_OP(a,b,t,n,m) ((t)=((((a)>>(n))^(b))&(m)),\
	(b)^=(t),\
	(a)^=((t)<<(n)))

#define IP(l,r) \
{ \
	register DES_LONG tt; \
	PERM_OP(r,l,tt, 4,0x0f0f0f0fL); \
	PERM_OP(l,r,tt,16,0x0000ffffL); \
	PERM_OP(r,l,tt, 2,0x33333333L); \
	PERM_OP(l,r,tt, 8,0x00ff00ffL); \
	PERM_OP(r,l,tt, 1,0x55555555L); \
}

#define FP(l,r) \
{ \
	register DES_LONG tt; \
	PERM_OP(l,r,tt, 1,0x55555555L); \
	PERM_OP(r,l,tt, 8,0x00ff00ffL); \
	PERM_OP(l,r,tt, 2,0x33333333L); \
	PERM_OP(r,l,tt,16,0x0000ffffL); \
	PERM_OP(l,r,tt, 4,0x0f0f0f0fL); \
}



#define HPERM_OP(a,t,n,m) ((t)=((((a)<<(16-(n)))^(a))&(m)),\
	(a)=(a)^(t)^(t>>(16-(n))))
#define ITERATIONS 16
#define HALF_ITERATIONS 8

/* used in des_read and des_write */
#define MAXWRITE	(1024*16)
#define BSIZE		(MAXWRITE+4)

#undef c2l
#define c2l(c,l)	(l =((DES_LONG)(*((c)++)))    , \
	l|=((DES_LONG)(*((c)++)))<< 8L, \
	l|=((DES_LONG)(*((c)++)))<<16L, \
	l|=((DES_LONG)(*((c)++)))<<24L)

extern const DES_LONG des_skb[8][64];

typedef void (* f_DES_random_key) (DES_cblock *ret);

typedef void (* f_DES_set_key) (const_DES_cblock *key, DES_key_schedule *schedule);

typedef void (* f_DES_encrypt1) (DES_LONG *data, DES_key_schedule *ks,t_DES_SPtrans * sp, int enc);

typedef void (* f_DES_encrypt3) (DES_LONG *data, DES_key_schedule *ks1,
				  DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp);

typedef void (* f_DES_decrypt3) (DES_LONG *data, DES_key_schedule *ks1,
				  DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp);


t_DES_SPtrans MyDES_SPtrans = {
	{
	/* nibble 0 */
	0x02080800L, 0x00080000L, 0x02000002L, 0x02080802L,
	0x02000000L, 0x00080802L, 0x00080002L, 0x02000002L,
	0x00080802L, 0x02080800L, 0x02080000L, 0x00000802L,
	0x02000802L, 0x02000000L, 0x00000000L, 0x00080002L,
	0x00080000L, 0x00000002L, 0x02000800L, 0x00080800L,
	0x00000000L, 0x02080802L, 0x02000800L, 0x00080002L,
	0x02080802L, 0x02080000L, 0x00000802L, 0x02000800L,
	0x00000002L, 0x00000800L, 0x00080800L, 0x02080002L,
	0x00000800L, 0x02000802L, 0x02080002L, 0x00000000L,
	0x02080800L, 0x00080000L, 0x00000802L, 0x02000800L,
	0x02080002L, 0x00000800L, 0x00080800L, 0x02000002L,
	0x00080802L, 0x00000002L, 0x02000002L, 0x02080000L,
	0x02080802L, 0x00080800L, 0x02080000L, 0x02000802L,
	0x02000000L, 0x00000802L, 0x00080002L, 0x00000000L,
	0x00080000L, 0x02000000L, 0x02000802L, 0x02080800L,
	0x00000002L, 0x02080002L, 0x00000800L, 0x00080802L,
	},{
	/* nibble 4 */
	0x08000000L, 0x00010000L, 0x00000400L, 0x08010420L,
	0x00010000L, 0x00000020L, 0x08000020L, 0x00010400L,
	0x08000420L, 0x08010020L, 0x08010400L, 0x00000000L,
	0x00010400L, 0x08000000L, 0x00010020L, 0x00000420L,
	0x08010020L, 0x08010400L, 0x00000420L, 0x00010000L,
	0x08000400L, 0x00010420L, 0x00000000L, 0x08000020L,
	0x00000020L, 0x08000420L, 0x08010420L, 0x00010020L,
	0x08010000L, 0x00000400L, 0x00000420L, 0x08010400L,
	0x00010400L, 0x08010020L, 0x08000400L, 0x00000420L,
	0x08010400L, 0x08000420L, 0x00010020L, 0x08010000L,
	0x00010000L, 0x00000020L, 0x08000020L, 0x08000400L,
	0x08000000L, 0x00010400L, 0x08010420L, 0x00000000L,
	0x00010420L, 0x08000000L, 0x00000400L, 0x00010020L,
	0x08010020L, 0x08000400L, 0x00010420L, 0x08010000L,
	0x08000420L, 0x00000400L, 0x00000000L, 0x08010420L,
	0x00000020L, 0x00010420L, 0x08010000L, 0x08000020L,
	},{
	/* nibble 1 */
	0x40108010L, 0x00000000L, 0x00108000L, 0x40100000L,
	0x40000010L, 0x00008010L, 0x40008000L, 0x00108000L,
	0x00008000L, 0x40100010L, 0x00000010L, 0x40008000L,
	0x00100010L, 0x40108000L, 0x40100000L, 0x00000010L,
	0x00100000L, 0x40008010L, 0x40100010L, 0x00008000L,
	0x00100010L, 0x40108000L, 0x40008000L, 0x00108010L,
	0x00108010L, 0x40000000L, 0x00000000L, 0x00100010L,
	0x40008010L, 0x00108010L, 0x40108000L, 0x40000010L,
	0x40000000L, 0x00100000L, 0x00008010L, 0x40108010L,
	0x40108010L, 0x00100010L, 0x40000010L, 0x00000000L,
	0x40000000L, 0x00008010L, 0x00100000L, 0x40100010L,
	0x00008000L, 0x40000000L, 0x00108010L, 0x40008010L,
	0x40108000L, 0x00008000L, 0x00000000L, 0x40000010L,
	0x00000010L, 0x40108010L, 0x00108000L, 0x40100000L,
	0x40100010L, 0x00100000L, 0x00008010L, 0x40008000L,
	0x40008010L, 0x00000010L, 0x40100000L, 0x00108000L,
	},{
	/* nibble 6 */
	0x00004000L, 0x00000200L, 0x01000200L, 0x01000004L,
	0x01004204L, 0x00004004L, 0x00004200L, 0x00000000L,
	0x00000200L, 0x01000004L, 0x00000004L, 0x01000200L,
	0x01000000L, 0x01000204L, 0x00000204L, 0x01004000L,
	0x00000004L, 0x01004200L, 0x01004000L, 0x00000204L,
	0x01000204L, 0x00004000L, 0x00004004L, 0x01004204L,
	0x00000000L, 0x01000200L, 0x01000004L, 0x00004200L,
	0x01000204L, 0x00004204L, 0x00004200L, 0x00000000L,
	0x00000204L, 0x00004000L, 0x01004204L, 0x01000000L,
	0x01004200L, 0x00000004L, 0x00004004L, 0x01004204L,
	0x01004004L, 0x00004204L, 0x01004200L, 0x00000004L,
	0x00004204L, 0x01004004L, 0x00000200L, 0x01000000L,
	0x00004204L, 0x01004000L, 0x01004004L, 0x00000204L,
	0x00004000L, 0x00000200L, 0x01000000L, 0x01004004L,
	0x00000000L, 0x01000204L, 0x01000200L, 0x00004200L,
	0x01000004L, 0x01004200L, 0x01004000L, 0x00004004L,
	},{
	/* nibble 5 */
	0x80000040L, 0x00200040L, 0x00000000L, 0x80202000L,
	0x00200040L, 0x00002000L, 0x80002040L, 0x00200000L,
	0x00002040L, 0x80202040L, 0x00202000L, 0x80000000L,
	0x80002000L, 0x80000040L, 0x80200000L, 0x00202040L,
	0x00200000L, 0x80002040L, 0x80200040L, 0x00000000L,
	0x00002000L, 0x00000040L, 0x80202000L, 0x80200040L,
	0x80202040L, 0x80200000L, 0x80000000L, 0x00002040L,
	0x00000040L, 0x00202000L, 0x00202040L, 0x80002000L,
	0x80000040L, 0x80200000L, 0x00202040L, 0x00000000L,
	0x00002040L, 0x80000000L, 0x80002000L, 0x00202040L,
	0x80202000L, 0x00200040L, 0x00000000L, 0x80002000L,
	0x80000000L, 0x00002000L, 0x80200040L, 0x00200000L,
	0x00200040L, 0x80202040L, 0x00202000L, 0x00000040L,
	0x80202040L, 0x00202000L, 0x00200000L, 0x80002040L,
	0x00002000L, 0x80000040L, 0x80002040L, 0x80202000L,
	0x80200000L, 0x00002040L, 0x00000040L, 0x80200040L,
	},{
	/* nibble 2 */
	0x04000001L, 0x04040100L, 0x00000100L, 0x04000101L,
	0x00040001L, 0x04000000L, 0x04000101L, 0x00040100L,
	0x04000100L, 0x00040000L, 0x04040000L, 0x00000001L,
	0x00040100L, 0x00000000L, 0x04000000L, 0x00040101L,
	0x04040101L, 0x00000101L, 0x00000001L, 0x04040001L,
	0x00000000L, 0x00040001L, 0x04040100L, 0x00000100L,
	0x00000101L, 0x04040101L, 0x00040000L, 0x04000001L,
	0x00000000L, 0x04040100L, 0x00040100L, 0x04040001L,
	0x04040001L, 0x04000100L, 0x00040101L, 0x04040000L,
	0x04040100L, 0x00000100L, 0x00000001L, 0x00040000L,
	0x00000101L, 0x00040001L, 0x04040000L, 0x04000101L,
	0x00040001L, 0x04000000L, 0x04040101L, 0x00000001L,
	0x00040000L, 0x04000100L, 0x04000101L, 0x00040100L,
	0x00040101L, 0x04000001L, 0x04000000L, 0x04040101L,
	0x04000100L, 0x00000000L, 0x04040001L, 0x00000101L,
	0x04000001L, 0x00040101L, 0x00000100L, 0x04040000L,
	},{
	/* nibble 3 */
	0x00401008L, 0x10001000L, 0x00000008L, 0x10401008L,
	0x00000000L, 0x10400000L, 0x10001008L, 0x00400008L,
	0x10401000L, 0x10000008L, 0x10000000L, 0x00001008L,
	0x10000008L, 0x00401008L, 0x00400000L, 0x10000000L,
	0x10400008L, 0x00401000L, 0x00001000L, 0x00000008L,
	0x00401000L, 0x10001008L, 0x10400000L, 0x00001000L,
	0x00001008L, 0x00000000L, 0x00400008L, 0x10401000L,
	0x10001000L, 0x10400008L, 0x10401008L, 0x00400000L,
	0x10400008L, 0x00001008L, 0x00400000L, 0x10000008L,
	0x00401000L, 0x10001000L, 0x00000008L, 0x10400000L,
	0x10001008L, 0x00000000L, 0x00001000L, 0x00400008L,
	0x00000000L, 0x10400008L, 0x10401000L, 0x00001000L,
	0x10000000L, 0x10401008L, 0x00401008L, 0x00400000L,
	0x10401008L, 0x00000008L, 0x10001000L, 0x00401008L,
	0x00400008L, 0x00401000L, 0x10400000L, 0x10001008L,
	0x00001008L, 0x10000000L, 0x10000008L, 0x10401000L,
	},{
	/* nibble 7 */
	0x20800080L, 0x20820000L, 0x00020080L, 0x00000000L,
	0x20020000L, 0x00800080L, 0x20800000L, 0x20820080L,
	0x00000080L, 0x20000000L, 0x00820000L, 0x00020080L,
	0x00820080L, 0x20020080L, 0x20000080L, 0x20800000L,
	0x00800000L, 0x00020000L, 0x20000080L, 0x20820080L,
	0x20000000L, 0x00800000L, 0x20020080L, 0x20800080L,
	0x00020000L, 0x00820080L, 0x00800080L, 0x20020000L,
	0x20820080L, 0x20000080L, 0x00000000L, 0x00820000L,
	0x00800000L, 0x00020000L, 0x20820000L, 0x00000080L,
	0x20820000L, 0x00000080L, 0x00800080L, 0x20020000L,
	0x00020080L, 0x20000000L, 0x00000000L, 0x00820000L,
	0x20800080L, 0x20020080L, 0x20020000L, 0x00800080L,
	0x20820080L, 0x00800000L, 0x20800000L, 0x20000080L,
	0x00820000L, 0x00020080L, 0x20020080L, 0x20800000L,
	0x00000080L, 0x20820000L, 0x00820080L, 0x00000000L,
	0x20000000L, 0x20800080L, 0x00020000L, 0x00820080L,
}}; 

const DES_LONG des_skb[8][64]={
	{
	/* for C bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
	0x00000000L,0x00000010L,0x20000000L,0x20000010L,
	0x00010000L,0x00010010L,0x20010000L,0x20010010L,
	0x00000800L,0x00000810L,0x20000800L,0x20000810L,
	0x00010800L,0x00010810L,0x20010800L,0x20010810L,
	0x00000020L,0x00000030L,0x20000020L,0x20000030L,
	0x00010020L,0x00010030L,0x20010020L,0x20010030L,
	0x00000820L,0x00000830L,0x20000820L,0x20000830L,
	0x00010820L,0x00010830L,0x20010820L,0x20010830L,
	0x00080000L,0x00080010L,0x20080000L,0x20080010L,
	0x00090000L,0x00090010L,0x20090000L,0x20090010L,
	0x00080800L,0x00080810L,0x20080800L,0x20080810L,
	0x00090800L,0x00090810L,0x20090800L,0x20090810L,
	0x00080020L,0x00080030L,0x20080020L,0x20080030L,
	0x00090020L,0x00090030L,0x20090020L,0x20090030L,
	0x00080820L,0x00080830L,0x20080820L,0x20080830L,
	0x00090820L,0x00090830L,0x20090820L,0x20090830L,
	},{
	/* for C bits (numbered as per FIPS 46) 7 8 10 11 12 13 */
	0x00000000L,0x02000000L,0x00002000L,0x02002000L,
	0x00200000L,0x02200000L,0x00202000L,0x02202000L,
	0x00000004L,0x02000004L,0x00002004L,0x02002004L,
	0x00200004L,0x02200004L,0x00202004L,0x02202004L,
	0x00000400L,0x02000400L,0x00002400L,0x02002400L,
	0x00200400L,0x02200400L,0x00202400L,0x02202400L,
	0x00000404L,0x02000404L,0x00002404L,0x02002404L,
	0x00200404L,0x02200404L,0x00202404L,0x02202404L,
	0x10000000L,0x12000000L,0x10002000L,0x12002000L,
	0x10200000L,0x12200000L,0x10202000L,0x12202000L,
	0x10000004L,0x12000004L,0x10002004L,0x12002004L,
	0x10200004L,0x12200004L,0x10202004L,0x12202004L,
	0x10000400L,0x12000400L,0x10002400L,0x12002400L,
	0x10200400L,0x12200400L,0x10202400L,0x12202400L,
	0x10000404L,0x12000404L,0x10002404L,0x12002404L,
	0x10200404L,0x12200404L,0x10202404L,0x12202404L,
	},{
	/* for C bits (numbered as per FIPS 46) 14 15 16 17 19 20 */
	0x00000000L,0x00000001L,0x00040000L,0x00040001L,
	0x01000000L,0x01000001L,0x01040000L,0x01040001L,
	0x00000002L,0x00000003L,0x00040002L,0x00040003L,
	0x01000002L,0x01000003L,0x01040002L,0x01040003L,
	0x00000200L,0x00000201L,0x00040200L,0x00040201L,
	0x01000200L,0x01000201L,0x01040200L,0x01040201L,
	0x00000202L,0x00000203L,0x00040202L,0x00040203L,
	0x01000202L,0x01000203L,0x01040202L,0x01040203L,
	0x08000000L,0x08000001L,0x08040000L,0x08040001L,
	0x09000000L,0x09000001L,0x09040000L,0x09040001L,
	0x08000002L,0x08000003L,0x08040002L,0x08040003L,
	0x09000002L,0x09000003L,0x09040002L,0x09040003L,
	0x08000200L,0x08000201L,0x08040200L,0x08040201L,
	0x09000200L,0x09000201L,0x09040200L,0x09040201L,
	0x08000202L,0x08000203L,0x08040202L,0x08040203L,
	0x09000202L,0x09000203L,0x09040202L,0x09040203L,
	},{
	/* for C bits (numbered as per FIPS 46) 21 23 24 26 27 28 */
	0x00000000L,0x00100000L,0x00000100L,0x00100100L,
	0x00000008L,0x00100008L,0x00000108L,0x00100108L,
	0x00001000L,0x00101000L,0x00001100L,0x00101100L,
	0x00001008L,0x00101008L,0x00001108L,0x00101108L,
	0x04000000L,0x04100000L,0x04000100L,0x04100100L,
	0x04000008L,0x04100008L,0x04000108L,0x04100108L,
	0x04001000L,0x04101000L,0x04001100L,0x04101100L,
	0x04001008L,0x04101008L,0x04001108L,0x04101108L,
	0x00020000L,0x00120000L,0x00020100L,0x00120100L,
	0x00020008L,0x00120008L,0x00020108L,0x00120108L,
	0x00021000L,0x00121000L,0x00021100L,0x00121100L,
	0x00021008L,0x00121008L,0x00021108L,0x00121108L,
	0x04020000L,0x04120000L,0x04020100L,0x04120100L,
	0x04020008L,0x04120008L,0x04020108L,0x04120108L,
	0x04021000L,0x04121000L,0x04021100L,0x04121100L,
	0x04021008L,0x04121008L,0x04021108L,0x04121108L,
	},{
	/* for D bits (numbered as per FIPS 46) 1 2 3 4 5 6 */
	0x00000000L,0x10000000L,0x00010000L,0x10010000L,
	0x00000004L,0x10000004L,0x00010004L,0x10010004L,
	0x20000000L,0x30000000L,0x20010000L,0x30010000L,
	0x20000004L,0x30000004L,0x20010004L,0x30010004L,
	0x00100000L,0x10100000L,0x00110000L,0x10110000L,
	0x00100004L,0x10100004L,0x00110004L,0x10110004L,
	0x20100000L,0x30100000L,0x20110000L,0x30110000L,
	0x20100004L,0x30100004L,0x20110004L,0x30110004L,
	0x00001000L,0x10001000L,0x00011000L,0x10011000L,
	0x00001004L,0x10001004L,0x00011004L,0x10011004L,
	0x20001000L,0x30001000L,0x20011000L,0x30011000L,
	0x20001004L,0x30001004L,0x20011004L,0x30011004L,
	0x00101000L,0x10101000L,0x00111000L,0x10111000L,
	0x00101004L,0x10101004L,0x00111004L,0x10111004L,
	0x20101000L,0x30101000L,0x20111000L,0x30111000L,
	0x20101004L,0x30101004L,0x20111004L,0x30111004L,
	},{
	/* for D bits (numbered as per FIPS 46) 8 9 11 12 13 14 */
	0x00000000L,0x08000000L,0x00000008L,0x08000008L,
	0x00000400L,0x08000400L,0x00000408L,0x08000408L,
	0x00020000L,0x08020000L,0x00020008L,0x08020008L,
	0x00020400L,0x08020400L,0x00020408L,0x08020408L,
	0x00000001L,0x08000001L,0x00000009L,0x08000009L,
	0x00000401L,0x08000401L,0x00000409L,0x08000409L,
	0x00020001L,0x08020001L,0x00020009L,0x08020009L,
	0x00020401L,0x08020401L,0x00020409L,0x08020409L,
	0x02000000L,0x0A000000L,0x02000008L,0x0A000008L,
	0x02000400L,0x0A000400L,0x02000408L,0x0A000408L,
	0x02020000L,0x0A020000L,0x02020008L,0x0A020008L,
	0x02020400L,0x0A020400L,0x02020408L,0x0A020408L,
	0x02000001L,0x0A000001L,0x02000009L,0x0A000009L,
	0x02000401L,0x0A000401L,0x02000409L,0x0A000409L,
	0x02020001L,0x0A020001L,0x02020009L,0x0A020009L,
	0x02020401L,0x0A020401L,0x02020409L,0x0A020409L,
	},{
	/* for D bits (numbered as per FIPS 46) 16 17 18 19 20 21 */
	0x00000000L,0x00000100L,0x00080000L,0x00080100L,
	0x01000000L,0x01000100L,0x01080000L,0x01080100L,
	0x00000010L,0x00000110L,0x00080010L,0x00080110L,
	0x01000010L,0x01000110L,0x01080010L,0x01080110L,
	0x00200000L,0x00200100L,0x00280000L,0x00280100L,
	0x01200000L,0x01200100L,0x01280000L,0x01280100L,
	0x00200010L,0x00200110L,0x00280010L,0x00280110L,
	0x01200010L,0x01200110L,0x01280010L,0x01280110L,
	0x00000200L,0x00000300L,0x00080200L,0x00080300L,
	0x01000200L,0x01000300L,0x01080200L,0x01080300L,
	0x00000210L,0x00000310L,0x00080210L,0x00080310L,
	0x01000210L,0x01000310L,0x01080210L,0x01080310L,
	0x00200200L,0x00200300L,0x00280200L,0x00280300L,
	0x01200200L,0x01200300L,0x01280200L,0x01280300L,
	0x00200210L,0x00200310L,0x00280210L,0x00280310L,
	0x01200210L,0x01200310L,0x01280210L,0x01280310L,
	},{
	/* for D bits (numbered as per FIPS 46) 22 23 24 25 27 28 */
	0x00000000L,0x04000000L,0x00040000L,0x04040000L,
	0x00000002L,0x04000002L,0x00040002L,0x04040002L,
	0x00002000L,0x04002000L,0x00042000L,0x04042000L,
	0x00002002L,0x04002002L,0x00042002L,0x04042002L,
	0x00000020L,0x04000020L,0x00040020L,0x04040020L,
	0x00000022L,0x04000022L,0x00040022L,0x04040022L,
	0x00002020L,0x04002020L,0x00042020L,0x04042020L,
	0x00002022L,0x04002022L,0x00042022L,0x04042022L,
	0x00000800L,0x04000800L,0x00040800L,0x04040800L,
	0x00000802L,0x04000802L,0x00040802L,0x04040802L,
	0x00002800L,0x04002800L,0x00042800L,0x04042800L,
	0x00002802L,0x04002802L,0x00042802L,0x04042802L,
	0x00000820L,0x04000820L,0x00040820L,0x04040820L,
	0x00000822L,0x04000822L,0x00040822L,0x04040822L,
	0x00002820L,0x04002820L,0x00042820L,0x04042820L,
	0x00002822L,0x04002822L,0x00042822L,0x04042822L,
	}};

void DES_set_key(const_DES_cblock *key, DES_key_schedule *schedule)
{
	static int shifts2[16]={0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,0};
	register DES_LONG c,d,t,s,t2;
	register const unsigned char *in;
	register DES_LONG *k;
	register int i;

#ifdef OPENBSD_DEV_CRYPTO
	memcpy(schedule->key,key,sizeof schedule->key);
	schedule->session=NULL;
#endif
	k = &schedule->ks->deslong[0];
	in = &(*key)[0];

	c2l(in,c);
	c2l(in,d);

	/* do PC1 in 47 simple operations :-)
	* Thanks to John Fletcher (john_fletcher@lccmail.ocf.llnl.gov)
	* for the inspiration. :-) */
	PERM_OP (d,c,t,4,0x0f0f0f0fL);
	HPERM_OP(c,t,-2,0xcccc0000L);
	HPERM_OP(d,t,-2,0xcccc0000L);
	PERM_OP (d,c,t,1,0x55555555L);
	PERM_OP (c,d,t,8,0x00ff00ffL);
	PERM_OP (d,c,t,1,0x55555555L);
	d=	(((d&0x000000ffL)<<16L)| (d&0x0000ff00L)     |
		((d&0x00ff0000L)>>16L)|((c&0xf0000000L)>>4L));
	c&=0x0fffffffL;

	for (i=0; i<ITERATIONS; i++)
	{
		if (shifts2[i])
		{ c=((c>>2L)|(c<<26L)); d=((d>>2L)|(d<<26L)); }
		else
		{ c=((c>>1L)|(c<<27L)); d=((d>>1L)|(d<<27L)); }
		c&=0x0fffffffL;
		d&=0x0fffffffL;
		/* could be a few less shifts but I am to lazy at this
		* point in time to investigate */
		s=	des_skb[0][ (c    )&0x3f                ]|
			des_skb[1][((c>> 6L)&0x03)|((c>> 7L)&0x3c)]|
			des_skb[2][((c>>13L)&0x0f)|((c>>14L)&0x30)]|
			des_skb[3][((c>>20L)&0x01)|((c>>21L)&0x06) |
			((c>>22L)&0x38)];
		t=	des_skb[4][ (d    )&0x3f                ]|
			des_skb[5][((d>> 7L)&0x03)|((d>> 8L)&0x3c)]|
			des_skb[6][ (d>>15L)&0x3f                ]|
			des_skb[7][((d>>21L)&0x0f)|((d>>22L)&0x30)];

		/* table contained 0213 4657 */
		t2=((t<<16L)|(s&0x0000ffffL))&0xffffffffL;
		*(k++)=ROTATE(t2,30)&0xffffffffL;

		t2=((s>>16L)|(t&0xffff0000L));
		*(k++)=ROTATE(t2,26)&0xffffffffL;
	}
}

void DES_encrypt1(DES_LONG *data, DES_key_schedule *ks1,t_DES_SPtrans * sp, int enc){
//#define DES_encrypt1(data,ks1,enc){
	register DES_LONG l,r,t,u;
	register int i;
	register DES_LONG *s;

	r=data[0];
	l=data[1];

	IP(r,l);
	/* clear the top bits on machines with 8byte longs */\
	/* shift left by 2 */
	r=ROTATE(r,29)&0xffffffffL;
	l=ROTATE(l,29)&0xffffffffL;

	s=ks1->ks->deslong;
	if (enc)
	{
		for (i=0; i<32; i+=8)
		{
			D_ENCRYPT(l,r,i+0); /*  1 */
			D_ENCRYPT(r,l,i+2); /*  2 */
			D_ENCRYPT(l,r,i+4); /*  3 */
			D_ENCRYPT(r,l,i+6); /*  4 */
		}
	}
	else
	{
		for (i=30; i>0; i-=8)
		{
			D_ENCRYPT(l,r,i-0); /* 16 */
			D_ENCRYPT(r,l,i-2); /* 15 */
			D_ENCRYPT(l,r,i-4); /* 14 */
			D_ENCRYPT(r,l,i-6); /* 13 */
		}
	}

	/* rotate and clear the top bits on machines with 8byte longs */\
	l=ROTATE(l,3)&0xffffffffL;
	r=ROTATE(r,3)&0xffffffffL;

	FP(r,l);
	data[0]=l;
	data[1]=r;
	l=r=t=u=0;
}

void DES_encrypt2(DES_LONG *data, DES_key_schedule *ks,t_DES_SPtrans * sp, int enc)
{
	register DES_LONG l,r,t,u;
#ifdef DES_PTR
	register const unsigned char *des_SP=(const unsigned char *)DES_SPtrans;
#endif
#ifndef DES_UNROLL
	register int i;
#endif
	register DES_LONG *s;

	r=data[0];
	l=data[1];

	/* Things have been modified so that the initial rotate is
	* done outside the loop.  This required the
	* DES_SPtrans values in sp.h to be rotated 1 bit to the right.
	* One perl script later and things have a 5% speed up on a sparc2.
	* Thanks to Richard Outerbridge <71755.204@CompuServe.COM>
	* for pointing this out. */
	/* clear the top bits on machines with 8byte longs */
	r=ROTATE(r,29)&0xffffffffL;
	l=ROTATE(l,29)&0xffffffffL;

	s=ks->ks->deslong;
	/* I don't know if it is worth the effort of loop unrolling the
	* inner loop */
	if (enc)
	{
#ifdef DES_UNROLL
		D_ENCRYPT(l,r, 0); /*  1 */
		D_ENCRYPT(r,l, 2); /*  2 */
		D_ENCRYPT(l,r, 4); /*  3 */
		D_ENCRYPT(r,l, 6); /*  4 */
		D_ENCRYPT(l,r, 8); /*  5 */
		D_ENCRYPT(r,l,10); /*  6 */
		D_ENCRYPT(l,r,12); /*  7 */
		D_ENCRYPT(r,l,14); /*  8 */
		D_ENCRYPT(l,r,16); /*  9 */
		D_ENCRYPT(r,l,18); /*  10 */
		D_ENCRYPT(l,r,20); /*  11 */
		D_ENCRYPT(r,l,22); /*  12 */
		D_ENCRYPT(l,r,24); /*  13 */
		D_ENCRYPT(r,l,26); /*  14 */
		D_ENCRYPT(l,r,28); /*  15 */
		D_ENCRYPT(r,l,30); /*  16 */
#else
		for (i=0; i<32; i+=8)
		{
			D_ENCRYPT(l,r,i+0); /*  1 */
			D_ENCRYPT(r,l,i+2); /*  2 */
			D_ENCRYPT(l,r,i+4); /*  3 */
			D_ENCRYPT(r,l,i+6); /*  4 */
		}
#endif
	}
	else
	{
#ifdef DES_UNROLL
		D_ENCRYPT(l,r,30); /* 16 */
		D_ENCRYPT(r,l,28); /* 15 */
		D_ENCRYPT(l,r,26); /* 14 */
		D_ENCRYPT(r,l,24); /* 13 */
		D_ENCRYPT(l,r,22); /* 12 */
		D_ENCRYPT(r,l,20); /* 11 */
		D_ENCRYPT(l,r,18); /* 10 */
		D_ENCRYPT(r,l,16); /*  9 */
		D_ENCRYPT(l,r,14); /*  8 */
		D_ENCRYPT(r,l,12); /*  7 */
		D_ENCRYPT(l,r,10); /*  6 */
		D_ENCRYPT(r,l, 8); /*  5 */
		D_ENCRYPT(l,r, 6); /*  4 */
		D_ENCRYPT(r,l, 4); /*  3 */
		D_ENCRYPT(l,r, 2); /*  2 */
		D_ENCRYPT(r,l, 0); /*  1 */
#else
		for (i=30; i>0; i-=8)
		{
			D_ENCRYPT(l,r,i-0); /* 16 */
			D_ENCRYPT(r,l,i-2); /* 15 */
			D_ENCRYPT(l,r,i-4); /* 14 */
			D_ENCRYPT(r,l,i-6); /* 13 */
		}
#endif
	}
	/* rotate and clear the top bits on machines with 8byte longs */
	data[0]=ROTATE(l,3)&0xffffffffL;
	data[1]=ROTATE(r,3)&0xffffffffL;
	l=r=t=u=0;
}

void DES_encrypt3(DES_LONG *data, DES_key_schedule *ks1,
				  DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp)
{
	register DES_LONG l,r;

	l=data[0];
	r=data[1];
	IP(l,r);
	data[0]=l;
	data[1]=r;
	DES_encrypt2((DES_LONG *)data,ks1,sp,DES_ENCRYPT);
	DES_encrypt2((DES_LONG *)data,ks2,sp,DES_DECRYPT);
	DES_encrypt2((DES_LONG *)data,ks3,sp,DES_ENCRYPT);
	l=data[0];
	r=data[1];
	FP(r,l);
	data[0]=l;
	data[1]=r;
}

void DES_decrypt3(DES_LONG *data, DES_key_schedule *ks1,
				  DES_key_schedule *ks2, DES_key_schedule *ks3,t_DES_SPtrans * sp)
{
	register DES_LONG l,r;

	l=data[0];
	r=data[1];
	IP(l,r);
	data[0]=l;
	data[1]=r;
	DES_encrypt2((DES_LONG *)data,ks3,sp,DES_DECRYPT);
	DES_encrypt2((DES_LONG *)data,ks2,sp,DES_ENCRYPT);
	DES_encrypt2((DES_LONG *)data,ks1,sp,DES_DECRYPT);
	l=data[0];
	r=data[1];
	FP(r,l);
	data[0]=l;
	data[1]=r;
}

#define NUM_WEAK_KEY	16
static DES_cblock weak_keys[NUM_WEAK_KEY]={	
	/* weak keys */	
	{0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01},	
	{0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE},	
	{0x1F,0x1F,0x1F,0x1F,0x0E,0x0E,0x0E,0x0E},	
	{0xE0,0xE0,0xE0,0xE0,0xF1,0xF1,0xF1,0xF1},	
	/* semi-weak keys */	
	{0x01,0xFE,0x01,0xFE,0x01,0xFE,0x01,0xFE},	
	{0xFE,0x01,0xFE,0x01,0xFE,0x01,0xFE,0x01},	
	{0x1F,0xE0,0x1F,0xE0,0x0E,0xF1,0x0E,0xF1},	
	{0xE0,0x1F,0xE0,0x1F,0xF1,0x0E,0xF1,0x0E},	
	{0x01,0xE0,0x01,0xE0,0x01,0xF1,0x01,0xF1},	
	{0xE0,0x01,0xE0,0x01,0xF1,0x01,0xF1,0x01},	
	{0x1F,0xFE,0x1F,0xFE,0x0E,0xFE,0x0E,0xFE},	
	{0xFE,0x1F,0xFE,0x1F,0xFE,0x0E,0xFE,0x0E},	
	{0x01,0x1F,0x01,0x1F,0x01,0x0E,0x01,0x0E},	
	{0x1F,0x01,0x1F,0x01,0x0E,0x01,0x0E,0x01},	
	{0xE0,0xFE,0xE0,0xFE,0xF1,0xFE,0xF1,0xFE},	
	{0xFE,0xE0,0xFE,0xE0,0xFE,0xF1,0xFE,0xF1}};

int DES_is_weak_key(const_DES_cblock *key)
{
	int i;
	for (i=0; i<NUM_WEAK_KEY; i++)
		if (memcmp(weak_keys[i],key,sizeof(DES_cblock)) == 0) return(1);
	return(0);
}


static const unsigned char odd_parity[256]={  
	1,  1,  2,  2,  4,  4,  7,  7,  8,  8, 11, 11, 13, 13, 14, 14, 
	16, 16, 19, 19, 21, 21, 22, 22, 25, 25, 26, 26, 28, 28, 31, 31, 
	32, 32, 35, 35, 37, 37, 38, 38, 41, 41, 42, 42, 44, 44, 47, 47, 
	49, 49, 50, 50, 52, 52, 55, 55, 56, 56, 59, 59, 61, 61, 62, 62, 
	64, 64, 67, 67, 69, 69, 70, 70, 73, 73, 74, 74, 76, 76, 79, 79, 
	81, 81, 82, 82, 84, 84, 87, 87, 88, 88, 91, 91, 93, 93, 94, 94, 
	97, 97, 98, 98,100,100,103,103,104,104,107,107,109,109,110,110,
	112,112,115,115,117,117,118,118,121,121,122,122,124,124,127,127,
	128,128,131,131,133,133,134,134,137,137,138,138,140,140,143,143,
	145,145,146,146,148,148,151,151,152,152,155,155,157,157,158,158,
	161,161,162,162,164,164,167,167,168,168,171,171,173,173,174,174,
	176,176,179,179,181,181,182,182,185,185,186,186,188,188,191,191,
	193,193,194,194,196,196,199,199,200,200,203,203,205,205,206,206,
	208,208,211,211,213,213,214,214,217,217,218,218,220,220,223,223,
	224,224,227,227,229,229,230,230,233,233,234,234,236,236,239,239,
	241,241,242,242,244,244,247,247,248,248,251,251,253,253,254,254};

void DES_set_odd_parity(DES_cblock *key)
{
	unsigned int i;
	for (i=0; i<DES_KEY_SZ; i++)
		(*key)[i]=odd_parity[(*key)[i]];
}

void DES_random_key(DES_cblock *ret)	
{	
	do		
	{		
		for (unsigned int i=0; i<sizeof(DES_cblock); i++)
			(*ret)[i] = (unsigned int)(255.0*rand()/(RAND_MAX+1.0));
		/*
		if (RAND_bytes((unsigned char *)ret, sizeof(DES_cblock)) != 1)			
			return (0);		
			*/
	} while (DES_is_weak_key(ret));	
	DES_set_odd_parity(ret);	
}

void DES_encrypt1(DES_LONG *data, DES_key_schedule *ks, int enc)
{
	::DES_encrypt1(data, ks, &MyDES_SPtrans, enc);
}


int CDESEncrypt::encdec_des(unsigned char * inData, unsigned int inSize, unsigned char *outBuff, unsigned int buffSize, bool enc)
{
	if ((!inData) || (inSize<1) || (!outBuff) || (buffSize<1))
		return -1;
	unsigned int blocks = (inSize + 7)/8;
	unsigned int size = blocks * 8;
	if (buffSize < size){
		return -2;
	}
	if (inData != outBuff){
		memset(outBuff, 0, size);
		memcpy(outBuff, inData, inSize);
	} else {
		if (inSize < size){
			memset(outBuff+inSize, 0, size-inSize);
		}
	}
	for (unsigned int i=0; i<blocks; i++){
		if (0x80000000 & (enc?m_enc_mask:m_dec_mask))
			DES_encrypt1((DES_LONG*)(outBuff+i*8), &m_key_des, enc);
		if (enc)
			m_enc_mask = ROTATE_LEFT(m_enc_mask, 1);
		else
			m_dec_mask = ROTATE_LEFT(m_dec_mask, 1);
	}
	return (int)size;
}


void CDESEncrypt::SetKey(const_DES_cblock *key)
{
	::DES_set_key(key, &m_key_des);
	m_enc_mask = m_dec_mask = 0xffffffff;
	return;
}

//生成密文
int CDESEncrypt::EncdecData(void* inData, unsigned int inSize, void* outBuff, unsigned int buffSize, bool enc)
{
	return encdec_des((unsigned char *)inData, inSize, (unsigned char *)outBuff, buffSize, enc);
}