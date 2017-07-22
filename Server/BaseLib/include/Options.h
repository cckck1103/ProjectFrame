///////////////////////////////////////////////////////////////////////////////
// �ļ�����: Options.h
// ��������: ȫ������
///////////////////////////////////////////////////////////////////////////////

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

///////////////////////////////////////////////////////////////////////////////
// ƽ̨����

#if defined(_MSC_VER)
#define _COMPILER_WIN
#endif

#if defined(__GNUC__)
#define _COMPILER_LINUX
#endif

///////////////////////////////////////////////////////////////////////////////
// �������������

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
// ����

// ��ֹ winsock2.h �� winsock.h ͬʱ���������ͻ
#ifdef _COMPILER_WIN
#define _WINSOCKAPI_
#endif

///////////////////////////////////////////////////////////////////////////////
//stl
#ifdef _COMPILER_WIN
#include <hash_map>
#include <hash_set>
#endif
#ifdef __GNUC__
#include <ext/hash_map>
#include <ext/hash_set>
#endif

#ifdef _COMPILER_WIN
#define EXT_STL_NAMESPACE   stdext
#endif

#ifdef __GNUC__
#define EXT_STL_NAMESPACE   __gnu_cxx
#endif

#ifdef _COMPILER_LINUX
namespace EXT_STL_NAMESPACE
{
	template <class _CharT, class _Traits, class _Alloc>
	inline size_t stl_string_hash(const basic_string<_CharT, _Traits, _Alloc>& s)
	{
		unsigned long h = 0;
		size_t len = s.size();
		const _CharT* data = s.data();
		for (size_t i = 0; i < len; ++i)
			h = 5 * h + data[i];
		return size_t(h);
	}

	template <> struct hash<string> {
		size_t operator()(const string& s) const { return stl_string_hash(s); }
	};

	template <> struct hash<wstring> {
		size_t operator()(const wstring& s) const { return stl_string_hash(s); }
	};
}
#endif
#endif // _OPTIONS_H_
