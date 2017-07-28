///////////////////////////////////////////////////////////////////////////////
// SysUtils.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _SYS_UTILS_H_
#define _SYS_UTILS_H_

#include "Options.h"

#ifdef _COMPILER_WIN
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#include <windows.h>
#endif

#ifdef _COMPILER_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <iostream>
#include <string>
#endif

#include "GlobalDefs.h"


///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class Buffer;
class StrList;

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

// �ļ����Ҽ�¼
struct FileFindItem
{
    INT64 fileSize;         // �ļ���С
    std::string fileName;   // �ļ���(����·��)
    UINT attr;              // �ļ�����
};

typedef std::vector<FileFindItem> FileFindResult;

///////////////////////////////////////////////////////////////////////////////
// �����

//-----------------------------------------------------------------------------
//-- �ַ�������:

/*
* �������� isIntStr
* ���ܣ�   �ж��ַ����Ƿ�Ϊ����
* ������   str 
* ����ֵ�� true ��  false �� 
*/
bool isIntStr(const std::string& str);

/*
* �������� isInt64Str
* ���ܣ�   �ж��ַ����Ƿ�Ϊ64λ����
* ������   str
* ����ֵ�� true ��  false ��
*/
bool isInt64Str(const std::string& str);

/*
* �������� isFloatStr
* ���ܣ�   �ж��ַ����Ƿ�ΪFloat
* ������   str
* ����ֵ�� true ��  false ��
*/
bool isFloatStr(const std::string& str);

/*
* �������� isFloatStr
* ���ܣ�   �ж��ַ����Ƿ�ΪBool
* ������   str
* ����ֵ�� true ��  false ��
*/
bool isBoolStr(const std::string& str);

/*
* �������� formatString
* ���ܣ�   ��ʽ���ַ���
* ������   format
           vlist
* ����ֵ�� std::string
*/
std::string formatString(const char *format, ...);

/*
* �������� formatString
* ���ܣ�   ��ʽ���ַ���
* ������   format
vlist
* ����ֵ�� std::string
*/
void formatStringV(std::string& result, const char *format, va_list argList);

/*
* �������� strToInt
* ���ܣ�   string ת�� int 
* ������   str          �ַ���
           defaultVal   Ĭ��ֵ
* ����ֵ�� int
*/
int strToInt(const std::string& str, int defaultVal = 0);

/*
* �������� strToInt64
* ���ܣ�   string ת�� INT64
* ������   str          �ַ���
		   defaultVal   Ĭ��ֵ
* ����ֵ�� INT64
*/
INT64 strToInt64(const std::string& str, INT64 defaultVal = 0);

/*
* �������� strToInt64
* ���ܣ�   string ת�� INT64
* ������   int          
* ����ֵ�� string
*/
std::string intToStr(int value);

/*
* �������� strToInt64
* ���ܣ�   string ת�� INT64
* ������   int
* ����ֵ�� string
*/
std::string intToStr(INT64 value);

/*
* �������� strToFloat
* ���ܣ�   string ת�� double
* ������   std::string
* ����ֵ�� double
*/
double strToFloat(const std::string& str, double defaultVal = 0);

/*
* �������� floatToStr
* ���ܣ�   double ת�� string
* ������   double
* ����ֵ�� string
*/
std::string floatToStr(double value, const char *format = "%f");

/*
* �������� strToBool
* ���ܣ�   string ת�� bool
* ������   string
* ����ֵ�� bool
*/
bool strToBool(const std::string& str, bool defaultVal = false);

/*
* �������� boolToStr
* ���ܣ�   bool ת�� string
* ������   bool
* ����ֵ�� string
*/
std::string boolToStr(bool value, bool useBoolStrs = false);

/*
* �������� sameText
* ���ܣ�   �Ƚ������ַ����Ƿ����   (�����ִ�Сд)
* ������   str
* ����ֵ�� true ��  false ��
*/
bool sameText(const std::string& str1, const std::string& str2);

/*
* �������� compareText
* ���ܣ�   �Ƚ������ַ����Ƿ����   (�����ִ�Сд)
* ������   char*
* ����ֵ�� true ��  false ��
*/
int compareText(const char* str1, const char* str2);

/*
* �������� trimString
* ���ܣ�   ȥ���ַ���ͷβ�Ŀհ��ַ� (ASCII <= 32)
* ������   str
* ����ֵ�� str
*/
std::string trimString(const std::string& str);

/*
* �������� upperCase
* ���ܣ�   �ַ������д
* ������   str
* ����ֵ�� str
*/
std::string upperCase(const std::string& str);

/*
* �������� lowerCase
* ���ܣ�   �ַ�����Сд
* ������   str
* ����ֵ�� str
*/
std::string lowerCase(const std::string& str);

/*
*  ������  repalceString
*  ����: �ַ����滻
*  ����:
*    sourceStr			- Դ��
*    oldPattern			- Դ���н����滻���ַ���
*    newPattern			- ȡ�� oldPattern ���ַ���
*    replaceAll			- �Ƿ��滻Դ��������ƥ����ַ���(��Ϊfalse����ֻ�滻��һ��)
*    caseSensitive		- �Ƿ����ִ�Сд
*  ����:
*   �����滻����֮����ַ���
*/
std::string repalceString(const std::string& sourceStr, const std::string& oldPattern,
	const std::string& newPattern, bool replaceAll = false, bool caseSensitive = true);

//-----------------------------------------------------------------------------
// ����: �ָ��ַ���
// ����:
//   sourceStr   - Դ��
//   splitter  - �ָ���
//   strList     - ��ŷָ�֮����ַ����б�
//   trimResult - �Ƿ�Էָ��Ľ������ trim ����
// ʾ��:
//   ""          -> []
//   " "         -> [" "]
//   ","         -> ["", ""]
//   "a,b,c"     -> ["a", "b", "c"]
//   ",a,,b,c,"  -> ["", "a", "", "b", "c", ""]
//-----------------------------------------------------------------------------
void splitString(const std::string& sourceStr, char splitter, StrList& strList,
	bool trimResult = false);

//-----------------------------------------------------------------------------
// ����: �ָ��ַ�����ת�����������б�
// ����:
//   sourceStr  - Դ��
//   splitter - �ָ���
//   intList    - ��ŷָ�֮����������б�
//-----------------------------------------------------------------------------
void splitStringToInt(const std::string& sourceStr, char splitter, IntegerArray& intList);

//-----------------------------------------------------------------------------
// ����: ��Դ���л�ȡһ���Ӵ�
//
// For example:
//   inputStr(before)   delimiter  del       inputStr(after)   result(after)
//   ----------------   -----------  ----------    ---------------   -------------
//   "abc def"           ' '         false         "abc def"         "abc"
//   "abc def"           ' '         true          "def"             "abc"
//   " abc"              ' '         false         " abc"            ""
//   " abc"              ' '         true          "abc"             ""
//   ""                  ' '         true/false    ""                ""
//-----------------------------------------------------------------------------
std::string fetchStr(std::string& inputStr, char delimiter = ' ', bool del = true);

//-----------------------------------------------------------------------------
// ����: �������м���붺�Ž������ݷ���
//-----------------------------------------------------------------------------
std::string addThousandSep(const INT64& number);

//-----------------------------------------------------------------------------
// Converts a string to a quoted string.
// For example:
//    abc         ->     "abc"
//    ab'c        ->     "ab'c"
//    ab"c        ->     "ab""c"
//    (empty)     ->     ""
//-----------------------------------------------------------------------------
std::string getQuotedStr(const char* str, char quoteChar = '"');

//-----------------------------------------------------------------------------
//
// For example:
//    str(before)    Returned string        str(after)
//    ---------------    ---------------        ---------------
//    "abc"               abc                    '\0'
//    "ab""c"             ab"c                   '\0'
//    "abc"123            abc                    123
//    abc"                (empty)                abc"
//    "abc                abc                    '\0'
//    (empty)             (empty)                '\0'
//-----------------------------------------------------------------------------
std::string extractQuotedStr(const char*& str, char quoteChar = '"');

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
// For example:
//    "abc"     ->     abc
//    "ab""c"   ->     ab"c
//    "abc      ->     "abc
//    abc"      ->     abc"
//    (empty)   ->    (empty)
//-----------------------------------------------------------------------------
std::string getDequotedStr(const char* str, char quoteChar = '"');




//-----------------------------------------------------------------------------
//-- ϵͳ���:

/*
* �������� getLastSysError
* ���ܣ�   ��ȡ���һ�δ�����Ϣ
* ������
* ����ֵ�� int
*/
int getLastSysError();

/*
* �������� getCurThreadId
* ���ܣ�   ��ȡ��ǰ�߳�id
* ������
* ����ֵ�� THREAD_ID
*/
THREAD_ID getCurThreadId();

/*
* �������� sysErrorMessage
* ���ܣ�   ��ȡϵͳ������Ϣ
* ������   code
* ����ֵ�� string
*/
std::string sysErrorMessage(int errorCode);

/*
* �������� getCurTicks
* ���ܣ�   ��ȡ��ǰ������
* ������   
* ����ֵ�� UINT64
*/
UINT64 getCurTicks();


/*
* �������� getTickDiff
* ���ܣ�   ȡ������ Ticks ֮��
* ������   oldTicks   newTicks
* ����ֵ�� UINT64
*/
UINT64 getTickDiff(UINT64 oldTicks, UINT64 newTicks);

/*
* �������� sleepSeconds
* ���ܣ�   ˯�� seconds �룬�ɾ�ȷ�����롣
* ������   seconds        - ˯�ߵ���������ΪС�����ɾ�ȷ������ (ʵ�ʾ�ȷ��ȡ���ڲ���ϵͳ)
           allowInterrupt - �Ƿ������ź��ж�
* ����ֵ�� 
*/
void sleepSeconds(double seconds, bool allowInterrupt);

//-----------------------------------------------------------------------------
//-- �����:
/*
* �������� randomize
* ���ܣ�   ����� "���������"
* ������  
* ����ֵ�� 
*/
void randomize();


/*
* �������� randomize
* ���ܣ�   ���� [min..max] ֮���һ��������������߽�
* ������
* ����ֵ��
*/
int getRandom(int min, int max);

//-----------------------------------------------------------------------------
//-- �ļ���Ŀ¼:

/*
* �������� fileExists
* ���ܣ�   ����ļ��Ƿ����
* ������   fileName
* ����ֵ��
*/
bool fileExists(const std::string& fileName);

/*
* �������� directoryExists
* ���ܣ�   ���Ŀ¼�Ƿ����
* ������   fileName
* ����ֵ��
*/
bool directoryExists(const std::string& dir);


/*
* �������� createDir
* ���ܣ�   ����Ŀ¼
			createDir("C:\\test");
  			createDir("/home/test");
*/
bool createDir(const std::string& dir);

/*
* �������� deleteDir
* ���ܣ�   ɾ��Ŀ¼
* ������   dir       - ��ɾ����Ŀ¼
           recursive - �Ƿ�ݹ�ɾ��
* ����ֵ�� true   - �ɹ�  false  - ʧ��
*/
bool deleteDir(const std::string& dir, bool recursive = false);

/*
* �������� extractFilePath
* ���ܣ�   ���ļ����ַ�����ȡ���ļ�·��
* ������
* ����ֵ��
* ʾ��:
//   extractFilePath("C:\\MyDir\\test.c");         ����: "C:\\MyDir\\"
//   extractFilePath("C:");                        ����: "C:\\"
//   extractFilePath("/home/user1/data/test.c");   ����: "/home/user1/data/"
*/
std::string extractFilePath(const std::string& fileName);

/*
* �������� extractFileName
* ���ܣ�   ���ļ����ַ�����ȡ���������ļ���
* ������   fileName - ����·�����ļ���
* ����ֵ��
// ʾ��:
//   extractFileName("C:\\MyDir\\test.c");         ����: "test.c"
//   extractFilePath("/home/user1/data/test.c");   ����: "test.c"
*/
std::string extractFileName(const std::string& fileName);

/*
* �������� extractFileExt
* ���ܣ�   ���ļ����ַ�����ȡ���ļ���չ��
* ������   fileName - �ļ��� (�ɰ���·��)
* ����ֵ��
// ʾ��:
//   extractFileExt("C:\\MyDir\\test.txt");         ����:  ".txt"
//   extractFileExt("/home/user1/data/test.c");     ����:  ".c"
*/
std::string extractFileExt(const std::string& fileName);

/*
* �������� changeFileExt
* ���ܣ�   �ı��ļ����ַ����е��ļ���չ��
* ������   fileName - ԭ�ļ��� (�ɰ���·��)
           ext      - �µ��ļ���չ��
* ����ֵ�� �µ��ļ���
// ʾ��:
			changeFileExt("c:\\test.txt", ".new");        ����:  "c:\\test.new"
//			changeFileExt("test.txt", ".new");            ����:  "test.new"
//			changeFileExt("test", ".new");                ����:  "test.new"
//			changeFileExt("test.txt", "");                ����:  "test"
//			changeFileExt("test.txt", ".");               ����:  "test."
//			changeFileExt("/home/user1/test.c", ".new");  ����:  "/home/user1/test.new"
*/
std::string changeFileExt(const std::string& fileName, const std::string& ext);

/*
* �������� forceDirectories
* ���ܣ�   ǿ�ƴ���Ŀ¼
* ������    
* ����ֵ��  true   - �ɹ�  false  - ʧ��
*/
bool forceDirectories(std::string dir);

/*
* �������� deleteFile
* ���ܣ�   ɾ���ļ�
* ������
* ����ֵ��  true   - �ɹ�  false  - ʧ��
*/
bool deleteFile(const std::string& fileName);

/*
* �������� removeFile
* ���ܣ�   ɾ���ļ�
* ������
* ����ֵ�� true   - �ɹ�  false  - ʧ��
*/
bool removeFile(const std::string& fileName);

/*
* �������� renameFile
* ���ܣ�   �ļ�������
* ������
* ����ֵ�� true   - �ɹ�  false  - ʧ��
*/
bool renameFile(const std::string& oldFileName, const std::string& newFileName);

/*
* �������� getFileSize
* ���ܣ�   ȡ���ļ��Ĵ�С����ʧ���򷵻�-1
* ������
* ����ֵ�� true   - �ɹ�  false  - ʧ��
*/
INT64 getFileSize(const std::string& fileName);


/*
* �������� findFiles
* ���ܣ�   ��ָ��·���²��ҷ����������ļ�
* ������    path    - ָ�����ĸ�·���½��в��ң�������ָ��ͨ���
			attr      - ֻ���ҷ��ϴ����Ե��ļ�
			findResult - ���ز��ҽ��
* ����ֵ�� 
// ʾ��:
//   findFiles("C:\\test\\*.*", FA_ANY_FILE & ~FA_HIDDEN, fr);
//   findFiles("/home/*.log", FA_ANY_FILE & ~FA_SYM_LINK, fr);
*/
void findFiles(const std::string& path, UINT attr, FileFindResult& findResult);

/*
* �������� pathWithSlash
* ���ܣ�   ��ȫ·���ַ�������� "\" �� "/"
* ������
* ����ֵ��
*/
std::string pathWithSlash(const std::string& path);

/*
* �������� pathWithoutSlash
* ���ܣ�   ȥ��·���ַ�������� "\" �� "/"
* ������
* ����ֵ��
*/
std::string pathWithoutSlash(const std::string& path);

/*
* �������� getAppExeName
* ���ܣ�   ȡ�ÿ�ִ���ļ�������
* ������
* ����ֵ��
*/
std::string getAppExeName(bool includePath = true);

/*
* �������� getAppPath
* ���ܣ�   ȡ�ÿ�ִ���ļ����ڵ�·��
* ������
* ����ֵ��
*/
std::string getAppPath();

/*
* �������� getAppSubPath
* ���ܣ�   ȡ�ÿ�ִ���ļ����ڵ�·������Ŀ¼
* ������
* ����ֵ��
*/
std::string getAppSubPath(const std::string& subDir = "");

//ȡС
template <typename T>
const T& min(const T& a, const T& b) { return ((a < b) ? a : b); }

//ȡ��
template <typename T>
const T& max(const T& a, const T& b) { return ((a < b) ? b : a); }

//��Χ�ж�
template <typename T>
const T& ensureRange(const T& value, const T& minVal, const T& maxVal)
{
	return (value > maxVal) ? maxVal : (value < minVal ? minVal : value);
}

//����ֵ
template <typename T>
void swap(T& a, T& b) { T temp(a); a = b; b = temp; }


//�Ƚ�ֵ
template <typename T>
int compare(const T& a, const T& b) { return (a < b) ? -1 : (a > b ? 1 : 0); }

#endif 
