///////////////////////////////////////////////////////////////////////////////
// 文件名称: GlobalDefs.h
// 功能描述: 全局定义
///////////////////////////////////////////////////////////////////////////////


#ifndef _GLOBAL_DEFS_H_
#define _GLOBAL_DEFS_H_

#include "Options.h"

#include <stdio.h>
#include <basetsd.h>
#include <stdint.h>
#include <stdlib.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>

#ifdef _COMPILER_WIN
#include <basetsd.h>
#else
#include <stdint.h>
#endif

#ifdef _COMPILER_WIN
typedef INT8      int8_t;
typedef INT16     int16_t;
typedef INT32     int32_t;
typedef INT64     int64_t;
typedef UINT8     uint8_t;
typedef UINT16    uint16_t;
typedef UINT32    uint32_t;
typedef UINT64    uint64_t;
#endif

#ifdef _COMPILER_LINUX
typedef int8_t    INT8;
typedef int16_t   INT16;
typedef int32_t   INT32;
typedef int64_t   INT64;

typedef uint8_t   UINT8;
typedef uint16_t  UINT16;
typedef uint32_t  UINT32;
typedef uint64_t  UINT64;

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint32_t  UINT;

typedef int       HANDLE;
typedef void*     PVOID;
typedef UINT8*    PBYTE;
typedef UINT16*   PWORD;
typedef UINT32*   PDWORD;
#endif

typedef void* POINTER;

typedef std::vector<std::string> StringArray;
typedef std::vector<int> IntegerArray;
typedef std::vector<bool> BooleanArray;
typedef std::set<int> IntegerSet;

typedef std::function<void(void)> Functor;

typedef std::function<void(void)> TimerCallback;
typedef INT64	TimerId;

#ifdef _COMPILER_WIN
typedef UINT32 THREAD_ID;
#endif
#ifdef _COMPILER_LINUX
typedef pid_t THREAD_ID;
#endif

#ifdef _COMPILER_WIN
const char PATH_DELIM = '\\';
const char DRIVER_DELIM = ':';
const char FILE_EXT_DELIM = '.';
const char* const S_CRLF = "\r\n";
#endif

#ifdef _COMPILER_LINUX
const char PATH_DELIM = '/';
const char FILE_EXT_DELIM = '.';
const char* const S_CRLF = "\n";
#endif

// 文件属性
const unsigned int FA_READ_ONLY = 0x00000001;
const unsigned int FA_HIDDEN = 0x00000002;
const unsigned int FA_SYS_FILE = 0x00000004;
const unsigned int FA_VOLUME_ID = 0x00000008;    // Windows Only
const unsigned int FA_DIRECTORY = 0x00000010;
const unsigned int FA_ARCHIVE = 0x00000020;
const unsigned int FA_SYM_LINK = 0x00000040;    // Linux Only
const unsigned int FA_ANY_FILE = 0x0000007F;

// 时间相关
const int MILLISECS_PER_SECOND = 1000;
const int SECONDS_PER_MINUTE = 60;
const int MINUTES_PER_HOUR = 60;
const int HOURS_PER_DAY = 24;
const int SECONDS_PER_HOUR = 3600;
const int SECONDS_PER_DAY = 86400;


// 范围值
#define MAXCHAR     0x7f
#define MAXSHORT    0x7fff
#define MAXINT      0x7fffffff
#define MAXBYTE     0xff
#define MAXWORD     0xffff
#define MAXDWORD    0xffffffff

// Strings 相关常量定义
#define DEFAULT_DELIMITER            ','
#define DEFAULT_QUOTE_CHAR           '\"'
#define DEFAULT_NAME_VALUE_SEP       '='
#define DEFAULT_LINE_BREAK           S_CRLF


#ifdef _COMPILER_LINUX
#define INVALID_HANDLE_VALUE        ((HANDLE)-1)
#endif

#define TRUE_STR                    "true"
#define FALSE_STR                   "false"


const int TIMEOUT_INFINITE = -1;

///////////////////////////////////////////////////////////////////////////////
// 宏定义
extern void internalAssert(const char *condition, const char *fileName, int lineNumber);


#ifdef _DEBUG
#define ASSERT_X(c)   ((c) ? (void)0 : internalAssert(#c, __FILE__, __LINE__))
#else
#define ASSERT_X(c)   ((void)0)
#endif

#define SAFE_DELETE(pData)          { try { delete pData; } catch (...) { ASSERT_X(FALSE); } pData=NULL; }
#define SAFE_DELETE_ARRAY(pData)    { try { delete [] pData; } catch (...) { ASSERT_X(FALSE); } pData=NULL; }
#define CATCH_ALL_EXCEPTION(x)  try { x; } catch(...) {ASSERT_X(FALSE);}



#endif