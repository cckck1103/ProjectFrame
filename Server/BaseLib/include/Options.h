///////////////////////////////////////////////////////////////////////////////
// 文件名称: Options.h
// 功能描述: 全局配置
///////////////////////////////////////////////////////////////////////////////

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

///////////////////////////////////////////////////////////////////////////////
// 平台定义

#if defined(_MSC_VER)
#define _COMPILER_WIN
#endif

#if defined(__GNUC__)
#define _COMPILER_LINUX
#endif

///////////////////////////////////////////////////////////////////////////////
// 编译器相关设置

#ifdef _COMPILER_WIN
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#define _CRT_SECURE_NO_WARNINGS

#ifndef _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#endif

#define NOMINMAX
#pragma warning(disable: 4355)
#pragma warning(disable: 4244)
#pragma warning(disable: 4311)
#pragma warning(disable: 4312)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif


///////////////////////////////////////////////////////////////////////////////
// 其它

// 防止 winsock2.h 和 winsock.h 同时包含引起冲突
#ifdef _COMPILER_WIN
#define _WINSOCKAPI_
#endif

///////////////////////////////////////////////////////////////////////////////
//stl
#ifdef _COMPILER_WIN
#include <windows.h>
#include <hash_map>
#include <hash_set>
#endif
#ifdef __GNUC__
#include <ext/hash_map>
#include <ext/hash_set>
#include <semaphore.h>
#endif

#ifdef _COMPILER_WIN
#define EXT_STL_NAMESPACE   stdext
#endif

#ifdef __GNUC__
#define EXT_STL_NAMESPACE   __gnu_cxx
#endif

#endif // _OPTIONS_H_
