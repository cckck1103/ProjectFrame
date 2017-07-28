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
// 提前声明

class Buffer;
class StrList;

///////////////////////////////////////////////////////////////////////////////
// 类型定义

// 文件查找记录
struct FileFindItem
{
    INT64 fileSize;         // 文件大小
    std::string fileName;   // 文件名(不含路径)
    UINT attr;              // 文件属性
};

typedef std::vector<FileFindItem> FileFindResult;

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//-----------------------------------------------------------------------------
//-- 字符串函数:

/*
* 函数名： isIntStr
* 功能：   判断字符串是否为整数
* 参数：   str 
* 返回值： true 是  false 否 
*/
bool isIntStr(const std::string& str);

/*
* 函数名： isInt64Str
* 功能：   判断字符串是否为64位整数
* 参数：   str
* 返回值： true 是  false 否
*/
bool isInt64Str(const std::string& str);

/*
* 函数名： isFloatStr
* 功能：   判断字符串是否为Float
* 参数：   str
* 返回值： true 是  false 否
*/
bool isFloatStr(const std::string& str);

/*
* 函数名： isFloatStr
* 功能：   判断字符串是否为Bool
* 参数：   str
* 返回值： true 是  false 否
*/
bool isBoolStr(const std::string& str);

/*
* 函数名： formatString
* 功能：   格式化字符串
* 参数：   format
           vlist
* 返回值： std::string
*/
std::string formatString(const char *format, ...);

/*
* 函数名： formatString
* 功能：   格式化字符串
* 参数：   format
vlist
* 返回值： std::string
*/
void formatStringV(std::string& result, const char *format, va_list argList);

/*
* 函数名： strToInt
* 功能：   string 转换 int 
* 参数：   str          字符串
           defaultVal   默认值
* 返回值： int
*/
int strToInt(const std::string& str, int defaultVal = 0);

/*
* 函数名： strToInt64
* 功能：   string 转换 INT64
* 参数：   str          字符串
		   defaultVal   默认值
* 返回值： INT64
*/
INT64 strToInt64(const std::string& str, INT64 defaultVal = 0);

/*
* 函数名： strToInt64
* 功能：   string 转换 INT64
* 参数：   int          
* 返回值： string
*/
std::string intToStr(int value);

/*
* 函数名： strToInt64
* 功能：   string 转换 INT64
* 参数：   int
* 返回值： string
*/
std::string intToStr(INT64 value);

/*
* 函数名： strToFloat
* 功能：   string 转换 double
* 参数：   std::string
* 返回值： double
*/
double strToFloat(const std::string& str, double defaultVal = 0);

/*
* 函数名： floatToStr
* 功能：   double 转换 string
* 参数：   double
* 返回值： string
*/
std::string floatToStr(double value, const char *format = "%f");

/*
* 函数名： strToBool
* 功能：   string 转换 bool
* 参数：   string
* 返回值： bool
*/
bool strToBool(const std::string& str, bool defaultVal = false);

/*
* 函数名： boolToStr
* 功能：   bool 转换 string
* 参数：   bool
* 返回值： string
*/
std::string boolToStr(bool value, bool useBoolStrs = false);

/*
* 函数名： sameText
* 功能：   比较两个字符串是否相等   (不区分大小写)
* 参数：   str
* 返回值： true 是  false 否
*/
bool sameText(const std::string& str1, const std::string& str2);

/*
* 函数名： compareText
* 功能：   比较两个字符串是否相等   (不区分大小写)
* 参数：   char*
* 返回值： true 是  false 否
*/
int compareText(const char* str1, const char* str2);

/*
* 函数名： trimString
* 功能：   去掉字符串头尾的空白字符 (ASCII <= 32)
* 参数：   str
* 返回值： str
*/
std::string trimString(const std::string& str);

/*
* 函数名： upperCase
* 功能：   字符串变大写
* 参数：   str
* 返回值： str
*/
std::string upperCase(const std::string& str);

/*
* 函数名： lowerCase
* 功能：   字符串变小写
* 参数：   str
* 返回值： str
*/
std::string lowerCase(const std::string& str);

/*
*  函数名  repalceString
*  描述: 字符串替换
*  参数:
*    sourceStr			- 源串
*    oldPattern			- 源串中将被替换的字符串
*    newPattern			- 取代 oldPattern 的字符串
*    replaceAll			- 是否替换源串中所有匹配的字符串(若为false，则只替换第一处)
*    caseSensitive		- 是否区分大小写
*  返回:
*   进行替换动作之后的字符串
*/
std::string repalceString(const std::string& sourceStr, const std::string& oldPattern,
	const std::string& newPattern, bool replaceAll = false, bool caseSensitive = true);

//-----------------------------------------------------------------------------
// 描述: 分割字符串
// 参数:
//   sourceStr   - 源串
//   splitter  - 分隔符
//   strList     - 存放分割之后的字符串列表
//   trimResult - 是否对分割后的结果进行 trim 处理
// 示例:
//   ""          -> []
//   " "         -> [" "]
//   ","         -> ["", ""]
//   "a,b,c"     -> ["a", "b", "c"]
//   ",a,,b,c,"  -> ["", "a", "", "b", "c", ""]
//-----------------------------------------------------------------------------
void splitString(const std::string& sourceStr, char splitter, StrList& strList,
	bool trimResult = false);

//-----------------------------------------------------------------------------
// 描述: 分割字符串并转换成整型数列表
// 参数:
//   sourceStr  - 源串
//   splitter - 分隔符
//   intList    - 存放分割之后的整型数列表
//-----------------------------------------------------------------------------
void splitStringToInt(const std::string& sourceStr, char splitter, IntegerArray& intList);

//-----------------------------------------------------------------------------
// 描述: 从源串中获取一个子串
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
// 描述: 在数字中间插入逗号进行数据分组
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
//-- 系统相关:

/*
* 函数名： getLastSysError
* 功能：   获取最后一次错误信息
* 参数：
* 返回值： int
*/
int getLastSysError();

/*
* 函数名： getCurThreadId
* 功能：   获取当前线程id
* 参数：
* 返回值： THREAD_ID
*/
THREAD_ID getCurThreadId();

/*
* 函数名： sysErrorMessage
* 功能：   获取系统错误信息
* 参数：   code
* 返回值： string
*/
std::string sysErrorMessage(int errorCode);

/*
* 函数名： getCurTicks
* 功能：   获取当前毫秒数
* 参数：   
* 返回值： UINT64
*/
UINT64 getCurTicks();


/*
* 函数名： getTickDiff
* 功能：   取得两个 Ticks 之差
* 参数：   oldTicks   newTicks
* 返回值： UINT64
*/
UINT64 getTickDiff(UINT64 oldTicks, UINT64 newTicks);

/*
* 函数名： sleepSeconds
* 功能：   睡眠 seconds 秒，可精确到纳秒。
* 参数：   seconds        - 睡眠的秒数，可为小数，可精确到纳秒 (实际精确度取决于操作系统)
           allowInterrupt - 是否允许信号中断
* 返回值： 
*/
void sleepSeconds(double seconds, bool allowInterrupt);

//-----------------------------------------------------------------------------
//-- 随机数:
/*
* 函数名： randomize
* 功能：   随机化 "随机数种子"
* 参数：  
* 返回值： 
*/
void randomize();


/*
* 函数名： randomize
* 功能：   返回 [min..max] 之间的一个随机数，包含边界
* 参数：
* 返回值：
*/
int getRandom(int min, int max);

//-----------------------------------------------------------------------------
//-- 文件和目录:

/*
* 函数名： fileExists
* 功能：   检查文件是否存在
* 参数：   fileName
* 返回值：
*/
bool fileExists(const std::string& fileName);

/*
* 函数名： directoryExists
* 功能：   检查目录是否存在
* 参数：   fileName
* 返回值：
*/
bool directoryExists(const std::string& dir);


/*
* 函数名： createDir
* 功能：   创建目录
			createDir("C:\\test");
  			createDir("/home/test");
*/
bool createDir(const std::string& dir);

/*
* 函数名： deleteDir
* 功能：   删除目录
* 参数：   dir       - 待删除的目录
           recursive - 是否递归删除
* 返回值： true   - 成功  false  - 失败
*/
bool deleteDir(const std::string& dir, bool recursive = false);

/*
* 函数名： extractFilePath
* 功能：   从文件名字符串中取出文件路径
* 参数：
* 返回值：
* 示例:
//   extractFilePath("C:\\MyDir\\test.c");         返回: "C:\\MyDir\\"
//   extractFilePath("C:");                        返回: "C:\\"
//   extractFilePath("/home/user1/data/test.c");   返回: "/home/user1/data/"
*/
std::string extractFilePath(const std::string& fileName);

/*
* 函数名： extractFileName
* 功能：   从文件名字符串中取出单独的文件名
* 参数：   fileName - 包含路径的文件名
* 返回值：
// 示例:
//   extractFileName("C:\\MyDir\\test.c");         返回: "test.c"
//   extractFilePath("/home/user1/data/test.c");   返回: "test.c"
*/
std::string extractFileName(const std::string& fileName);

/*
* 函数名： extractFileExt
* 功能：   从文件名字符串中取出文件扩展名
* 参数：   fileName - 文件名 (可包含路径)
* 返回值：
// 示例:
//   extractFileExt("C:\\MyDir\\test.txt");         返回:  ".txt"
//   extractFileExt("/home/user1/data/test.c");     返回:  ".c"
*/
std::string extractFileExt(const std::string& fileName);

/*
* 函数名： changeFileExt
* 功能：   改变文件名字符串中的文件扩展名
* 参数：   fileName - 原文件名 (可包含路径)
           ext      - 新的文件扩展名
* 返回值： 新的文件名
// 示例:
			changeFileExt("c:\\test.txt", ".new");        返回:  "c:\\test.new"
//			changeFileExt("test.txt", ".new");            返回:  "test.new"
//			changeFileExt("test", ".new");                返回:  "test.new"
//			changeFileExt("test.txt", "");                返回:  "test"
//			changeFileExt("test.txt", ".");               返回:  "test."
//			changeFileExt("/home/user1/test.c", ".new");  返回:  "/home/user1/test.new"
*/
std::string changeFileExt(const std::string& fileName, const std::string& ext);

/*
* 函数名： forceDirectories
* 功能：   强制创建目录
* 参数：    
* 返回值：  true   - 成功  false  - 失败
*/
bool forceDirectories(std::string dir);

/*
* 函数名： deleteFile
* 功能：   删除文件
* 参数：
* 返回值：  true   - 成功  false  - 失败
*/
bool deleteFile(const std::string& fileName);

/*
* 函数名： removeFile
* 功能：   删除文件
* 参数：
* 返回值： true   - 成功  false  - 失败
*/
bool removeFile(const std::string& fileName);

/*
* 函数名： renameFile
* 功能：   文件重命名
* 参数：
* 返回值： true   - 成功  false  - 失败
*/
bool renameFile(const std::string& oldFileName, const std::string& newFileName);

/*
* 函数名： getFileSize
* 功能：   取得文件的大小。若失败则返回-1
* 参数：
* 返回值： true   - 成功  false  - 失败
*/
INT64 getFileSize(const std::string& fileName);


/*
* 函数名： findFiles
* 功能：   在指定路径下查找符合条件的文件
* 参数：    path    - 指定在哪个路径下进行查找，并必须指定通配符
			attr      - 只查找符合此属性的文件
			findResult - 传回查找结果
* 返回值： 
// 示例:
//   findFiles("C:\\test\\*.*", FA_ANY_FILE & ~FA_HIDDEN, fr);
//   findFiles("/home/*.log", FA_ANY_FILE & ~FA_SYM_LINK, fr);
*/
void findFiles(const std::string& path, UINT attr, FileFindResult& findResult);

/*
* 函数名： pathWithSlash
* 功能：   补全路径字符串后面的 "\" 或 "/"
* 参数：
* 返回值：
*/
std::string pathWithSlash(const std::string& path);

/*
* 函数名： pathWithoutSlash
* 功能：   去掉路径字符串后面的 "\" 或 "/"
* 参数：
* 返回值：
*/
std::string pathWithoutSlash(const std::string& path);

/*
* 函数名： getAppExeName
* 功能：   取得可执行文件的名称
* 参数：
* 返回值：
*/
std::string getAppExeName(bool includePath = true);

/*
* 函数名： getAppPath
* 功能：   取得可执行文件所在的路径
* 参数：
* 返回值：
*/
std::string getAppPath();

/*
* 函数名： getAppSubPath
* 功能：   取得可执行文件所在的路径的子目录
* 参数：
* 返回值：
*/
std::string getAppSubPath(const std::string& subDir = "");

//取小
template <typename T>
const T& min(const T& a, const T& b) { return ((a < b) ? a : b); }

//取大
template <typename T>
const T& max(const T& a, const T& b) { return ((a < b) ? b : a); }

//范围判断
template <typename T>
const T& ensureRange(const T& value, const T& minVal, const T& maxVal)
{
	return (value > maxVal) ? maxVal : (value < minVal ? minVal : value);
}

//交换值
template <typename T>
void swap(T& a, T& b) { T temp(a); a = b; b = temp; }


//比较值
template <typename T>
int compare(const T& a, const T& b) { return (a < b) ? -1 : (a > b ? 1 : 0); }

#endif 
