///////////////////////////////////////////////////////////////////////////////
// 文件名称: SysUtils.cpp
// 功能描述: 系统杂项功能
///////////////////////////////////////////////////////////////////////////////

#include "SysUtils.h"
#include "Exceptions.h"
#include "StreamClass.h"
#include "LogManager.h"
#include "StringList.h"

//断言处理
void internalAssert(const char *condition, const char *fileName, int lineNumber)
{
	SimpleException e(formatString("Assertion failed: %s", condition).c_str(), fileName, lineNumber);
	ERROR_LOG(e.makeLogStr().c_str());
	throw e;
}

void formatStringV(std::string& result, const char *format, va_list argList)
{
#if defined(_COMPILER_WIN)

	int size = _vscprintf(format, argList);
	char *buffer = (char *)malloc(size + 1);
	if (buffer)
	{
		vsprintf(buffer, format, argList);
		result = buffer;
		free(buffer);
	}
	else
		result = "";

#else

	int size = 100;
	char *buffer = static_cast<char*>(malloc(size));
	va_list args;

	while (buffer)
	{
		int charCount;

		va_copy(args, argList);
		charCount = vsnprintf(buffer, size, format, args);
		va_end(args);

		if (charCount > -1 && charCount < size)
			break;
		if (charCount > -1)
			size = charCount + 1;
		else
			size *= 2;
		buffer = static_cast<char*>(realloc(buffer, size));
	}

	if (buffer)
	{
		result = buffer;
		free(buffer);
	}
	else
		result = "";

#endif
}

std::string formatString(const char *format, ...)
{
	std::string result;

	va_list argList;
	va_start(argList, format);
	formatStringV(result, format, argList);
	va_end(argList);

	return result;
}

bool isIntStr(const std::string& str)
{
	bool result;
	size_t len = str.size();
	const char *strPtr = str.c_str();

	result = (len > 0) && !isspace(strPtr[0]);

	if (result)
	{
		char *endPtr;
		strtol(strPtr, &endPtr, 10);
		result = (endPtr - strPtr == static_cast<int>(len));
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成整型(若转换失败，则返回 defaultVal)
//-----------------------------------------------------------------------------
int strToInt(const std::string& str, int defaultVal)
{
    if (isIntStr(str))
        return strtol(str.c_str(), NULL, 10);
    else
        return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成64位整型(若转换失败，则返回 defaultVal)
//-----------------------------------------------------------------------------
INT64 strToInt64(const std::string& str, INT64 defaultVal)
{
	if (isInt64Str(str))
#ifdef _COMPILER_WIN
		return _strtoi64(str.c_str(), NULL, 10);
#endif
#ifdef _COMPILER_LINUX
	return strtol(str.c_str(), NULL, 10);
#endif
	else
		return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 整型转换成字符串
//-----------------------------------------------------------------------------
std::string intToStr(int value)
{
	char temp[64];
	sprintf(temp, "%d", value);
	return &temp[0];
}

//-----------------------------------------------------------------------------
// 描述: 64位整型转换成字符串
//-----------------------------------------------------------------------------
std::string intToStr(INT64 value)
{
	char temp[64];
#ifdef _COMPILER_WIN
	sprintf(temp, "%I64d", value);
#endif
#ifdef _COMPILER_LINUX
	sprintf(temp, "%lld", static_cast<long long int>(value));
#endif
	return &temp[0];
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个整数
//-----------------------------------------------------------------------------
bool isInt64Str(const std::string& str)
{
	bool result;
	size_t len = str.size();
	const char *strPtr = str.c_str();

	result = (len > 0) && !isspace(strPtr[0]);

	if (result)
	{
		char *endPtr;
#ifdef _COMPILER_WIN
		_strtoi64(strPtr, &endPtr, 10);
#endif
#ifdef _COMPILER_LINUX
		strtoll(strPtr, &endPtr, 10);
#endif
		result = (endPtr - strPtr == static_cast<int>(len));
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串是不是一个浮点数
//-----------------------------------------------------------------------------
bool isFloatStr(const std::string& str)
{
	bool result;
	size_t len = str.size();
	const char *strPtr = str.c_str();

	result = (len > 0) && !isspace(strPtr[0]);

	if (result)
	{
		char *endp;
		strtod(strPtr, &endp);
		result = (endp - strPtr == static_cast<int>(len));
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 判断一个字符串可否转换成布尔型
//-----------------------------------------------------------------------------
bool isBoolStr(const std::string& str)
{
	return sameText(str, TRUE_STR) || sameText(str, FALSE_STR) || isFloatStr(str);
}



//-----------------------------------------------------------------------------
// 描述: 字符串转换成浮点型(若转换失败，则返回 defaultVal)
//-----------------------------------------------------------------------------
double strToFloat(const std::string& str, double defaultVal)
{
	if (isFloatStr(str))
		return strtod(str.c_str(), NULL);
	else
		return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 浮点型转换成字符串
//-----------------------------------------------------------------------------
std::string floatToStr(double value, const char *format)
{
	char temp[256];
	sprintf(temp, format, value);
	return &temp[0];
}

//-----------------------------------------------------------------------------
// 描述: 字符串转换成布尔型
//-----------------------------------------------------------------------------
bool strToBool(const std::string& str, bool defaultVal)
{
	if (isBoolStr(str))
	{
		if (sameText(str, TRUE_STR))
			return true;
		else if (sameText(str, FALSE_STR))
			return false;
		else
			return (strToFloat(str, 0) != 0);
	}
	else
		return defaultVal;
}

//-----------------------------------------------------------------------------
// 描述: 布尔型转换成字符串
//-----------------------------------------------------------------------------
std::string boolToStr(bool value, bool useBoolStrs)
{
	if (useBoolStrs)
		return (value ? TRUE_STR : FALSE_STR);
	else
		return (value ? "1" : "0");
}

//-----------------------------------------------------------------------------
// 描述: 比较两个字符串 (不区分大小写)
//-----------------------------------------------------------------------------
int compareText(const char* str1, const char* str2)
{
#ifdef _COMPILER_WIN
	return _stricmp(str1, str2);
#endif
#ifdef _COMPILER_LINUX
	return strcasecmp(str1, str2);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 判断两个字符串是否相同 (不区分大小写)
//-----------------------------------------------------------------------------
bool sameText(const std::string& str1, const std::string& str2)
{
	return compareText(str1.c_str(), str2.c_str()) == 0;
}

//-----------------------------------------------------------------------------
// 描述: 去掉字符串头尾的空白字符 (ASCII <= 32)
//-----------------------------------------------------------------------------
std::string trimString(const std::string& str)
{
	std::string result;
	int i, len;

	len = static_cast<int>(str.size());
	i = 0;
	while (i < len && (BYTE)str[i] <= 32) i++;
	if (i < len)
	{
		while ((BYTE)str[len - 1] <= 32) len--;
		result = str.substr(i, len - i);
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串变大写
//-----------------------------------------------------------------------------
std::string  upperCase(const std::string & str)
{
	std::string  result = str;
	int len = static_cast<int>(result.size());
	char c;

	for (int i = 0; i < len; i++)
	{
		c = result[i];
		if (c >= 'a' && c <= 'z')
			result[i] = static_cast<char>(c - 32);
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串变小写
//-----------------------------------------------------------------------------
std::string  lowerCase(const std::string & str)
{
	std::string  result = str;
	int len = static_cast<int>(result.size());
	char c;

	for (int i = 0; i < len; i++)
	{
		c = result[i];
		if (c >= 'A' && c <= 'Z')
			result[i] = static_cast<char>(c + 32);
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 字符串替换
// 参数:
//   sourceStr        - 源串
//   oldPattern    - 源串中将被替换的字符串
//   newPattern    - 取代 oldPattern 的字符串
//   replaceAll      - 是否替换源串中所有匹配的字符串(若为false，则只替换第一处)
//   caseSensitive   - 是否区分大小写
// 返回:
//   进行替换动作之后的字符串
//-----------------------------------------------------------------------------
std::string repalceString(const std::string& sourceStr, const std::string& oldPattern,
	const std::string& newPattern, bool replaceAll, bool caseSensitive)
{
	std::string result = sourceStr;
	std::string searchStr, patternStr;
	std::string::size_type offset, index;
	int oldPattLen, newPattLen;

	if (!caseSensitive)
	{
		searchStr = upperCase(sourceStr);
		patternStr = upperCase(oldPattern);
	}
	else
	{
		searchStr = sourceStr;
		patternStr = oldPattern;
	}

	oldPattLen = (int)oldPattern.size();
	newPattLen = (int)newPattern.size();
	index = 0;

	while (index < searchStr.size())
	{
		offset = searchStr.find(patternStr, index);
		if (offset == std::string::npos) break;  // 若没找到

		searchStr.replace(offset, oldPattLen, newPattern);
		result.replace(offset, oldPattLen, newPattern);
		index = (offset + newPattLen);

		if (!replaceAll) break;
	}

	return result;
}


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
	bool trimResult)
{
	std::string::size_type offset, index = 0;

	strList.clear();
	if (sourceStr.empty()) return;

	while (true)
	{
		offset = sourceStr.find(splitter, index);
		if (offset == std::string::npos)   // 若没找到
		{
			strList.add(sourceStr.substr(index).c_str());
			break;
		}
		else
		{
			strList.add(sourceStr.substr(index, offset - index).c_str());
			index = offset + 1;
		}
	}

	if (trimResult)
	{
		for (int i = 0; i < strList.getCount(); i++)
			strList.setString(i, trimString(strList[i]).c_str());
	}
}

//-----------------------------------------------------------------------------
// 描述: 分割字符串并转换成整型数列表
// 参数:
//   sourceStr  - 源串
//   splitter - 分隔符
//   intList    - 存放分割之后的整型数列表
//-----------------------------------------------------------------------------
void splitStringToInt(const std::string& sourceStr, char splitter, IntegerArray& intList)
{
	StrList strList;
	splitString(sourceStr, splitter, strList, true);

	intList.clear();
	for (int i = 0; i < strList.getCount(); i++)
		intList.push_back(atoi(strList[i].c_str()));
}


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
std::string fetchStr(std::string& inputStr, char delimiter, bool del)
{
	std::string result;

	std::string::size_type pos = inputStr.find(delimiter, 0);
	if (pos == std::string::npos)
	{
		result = inputStr;
		if (del)
			inputStr.clear();
	}
	else
	{
		result = inputStr.substr(0, pos);
		if (del)
			inputStr = inputStr.substr(pos + 1);
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 在数字中间插入逗号进行数据分组
//-----------------------------------------------------------------------------
std::string addThousandSep(const INT64& number)
{
	std::string result = intToStr(number);
	for (int i = (int)result.length() - 3; i > 0; i -= 3)
		result.insert(i, ",");
	return result;
}

//-----------------------------------------------------------------------------
// Converts a string to a quoted string.
// For example:
//    abc         ->     "abc"
//    ab'c        ->     "ab'c"
//    ab"c        ->     "ab""c"
//    (empty)     ->     ""
//-----------------------------------------------------------------------------
std::string getQuotedStr(const char* str, char quoteChar)
{
	std::string result;
	std::string srcStr(str);

	result = quoteChar;

	std::string::size_type start = 0;
	while (true)
	{
		std::string::size_type pos = srcStr.find(quoteChar, start);
		if (pos != std::string::npos)
		{
			result += srcStr.substr(start, pos - start) + quoteChar + quoteChar;
			start = pos + 1;
		}
		else
		{
			result += srcStr.substr(start);
			break;
		}
	}

	result += quoteChar;

	return result;
}
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
std::string extractQuotedStr(const char*& str, char quoteChar)
{
	std::string result;
	const char* startPtr = str;

	if (str == NULL || *str != quoteChar)
		return "";

	// Calc the character count after converting.

	int size = 0;
	str++;
	while (*str != '\0')
	{
		if (str[0] == quoteChar)
		{
			if (str[1] == quoteChar)
			{
				size++;
				str += 2;
			}
			else
			{
				str++;
				break;
			}
		}
		else
		{
			const char* p = str;
			str++;
			size += (int)(str - p);
		}
	}

	// Begin to retrieve the characters.

	result.resize(size);
	char* resultPtr = (char*)result.c_str();
	str = startPtr;
	str++;
	while (*str != '\0')
	{
		if (str[0] == quoteChar)
		{
			if (str[1] == quoteChar)
			{
				*resultPtr++ = *str;
				str += 2;
			}
			else
			{
				str++;
				break;
			}
		}
		else
		{
			const char* p = str;
			str++;
			while (p < str)
				*resultPtr++ = *p++;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// Converts a quoted string to an unquoted string.
//
// getDequotedStr removes the quote characters from the beginning and end of a quoted string, and
// reduces pairs of quote characters within the string to a single quote character. The quoteChar
// parameter defines what character to use as a quote character. If the @a str parameter does
// not begin and end with the quote character, getDequotedStr returns @a str unchanged.
//
// For example:
//    "abc"     ->     abc
//    "ab""c"   ->     ab"c
//    "abc      ->     "abc
//    abc"      ->     abc"
//    (empty)   ->    (empty)
//-----------------------------------------------------------------------------
std::string getDequotedStr(const char* str, char quoteChar)
{
	const char* startPtr = str;
	int strLen = (int)strlen(str);

	std::string result = extractQuotedStr(str, quoteChar);

	if ((result.empty() || *str == '\0') &&
		strLen > 0 && (startPtr[0] != quoteChar || startPtr[strLen - 1] != quoteChar))
		result = startPtr;

	return result;
}


//-----------------------------------------------------------------------------
// 描述: 返回操作系统错误代码对应的错误信息
//-----------------------------------------------------------------------------
std::string sysErrorMessage(int errorCode)
{
#ifdef _COMPILER_WIN
	char *errorMsg;

	errorMsg = strerror(errorCode);
	return errorMsg;
#endif
#ifdef _COMPILER_LINUX
	const int ERROR_MSG_SIZE = 256;
	char errorMsg[ERROR_MSG_SIZE];
	std::string result;

	errorMsg[0] = 0;
	strerror_r(errorCode, errorMsg, ERROR_MSG_SIZE);
	if (errorMsg[0] == 0)
		result = formatString("System error: %d", errorCode);
	else
		result = errorMsg;

	return result;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 返回操作系统错误代码
//-----------------------------------------------------------------------------
int getLastSysError()
{
#ifdef _COMPILER_WIN
	return ::GetLastError();
#endif
#ifdef _COMPILER_LINUX
	return errno;
#endif
}


//-----------------------------------------------------------------------------
// 描述: 取得当前线程的ID
//-----------------------------------------------------------------------------
THREAD_ID getCurThreadId()
{
#ifdef _COMPILER_WIN
	static __declspec (thread) THREAD_ID t_tid = 0;
	if (t_tid == 0)
		t_tid = ::GetCurrentThreadId();
	return t_tid;
#endif
#ifdef _COMPILER_LINUX
	static __thread THREAD_ID t_tid = 0;
	if (t_tid == 0)
		t_tid = syscall(SYS_gettid);
	return t_tid;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 睡眠 seconds 秒，可精确到纳秒。
// 参数:
//   seconds        - 睡眠的秒数，可为小数，可精确到纳秒 (实际精确度取决于操作系统)
//   allowInterrupt - 是否允许信号中断
//-----------------------------------------------------------------------------
void sleepSeconds(double seconds, bool allowInterrupt)
{
#ifdef _COMPILER_WIN
	::Sleep((UINT)(seconds * 1000));
#endif
#ifdef _COMPILER_LINUX
	const UINT NANO_PER_SEC = 1000000000;  // 一秒等于多少纳秒
	struct timespec req, remain;
	int r;

	req.tv_sec = (UINT)seconds;
	req.tv_nsec = (UINT)((seconds - req.tv_sec) * NANO_PER_SEC);

	while (true)
	{
		r = nanosleep(&req, &remain);
		if (r == -1 && errno == EINTR && !allowInterrupt)
			req = remain;
		else
			break;
	}
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得当前 Ticks，单位:毫秒
//-----------------------------------------------------------------------------
UINT64 getCurTicks()
{
#ifdef _COMPILER_WIN
	return static_cast<UINT64>(GetTickCount());
#endif
#ifdef _COMPILER_LINUX
	timeval tv;
	gettimeofday(&tv, NULL);
	return static_cast<UINT64>(static_cast<UINT64>(tv.tv_sec) * 1000 + tv.tv_usec / 1000);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得两个 Ticks 之差
//-----------------------------------------------------------------------------
UINT64 getTickDiff(UINT64 oldTicks, UINT64 newTicks)
{
	if (newTicks >= oldTicks)
		return (newTicks - oldTicks);
	else
		return (UINT64(-1) - oldTicks + newTicks);
}

//-----------------------------------------------------------------------------
// 描述: 随机化 "随机数种子"
//-----------------------------------------------------------------------------
void randomize()
{
	srand((unsigned int)time(NULL));
}


//-----------------------------------------------------------------------------
// 描述: 返回 [min..max] 之间的一个随机数，包含边界
//-----------------------------------------------------------------------------
int getRandom(int min, int max)
{
	ASSERT_X((max - min) < MAXINT);
	return min + (int)(((double)rand() / ((double)RAND_MAX + 0.1)) * (max - min + 1));
}

//-----------------------------------------------------------------------------

#ifdef _COMPILER_WIN
static bool GetFileFindData(const std::string& fileName, WIN32_FIND_DATAA& findData)
{
	HANDLE findHandle = ::FindFirstFileA(fileName.c_str(), &findData);
	bool result = (findHandle != INVALID_HANDLE_VALUE);
	if (result) ::FindClose(findHandle);
	return result;
}
#endif
//-----------------------------------------------------------------------------
// 描述: 检查文件是否存在
//-----------------------------------------------------------------------------
bool fileExists(const std::string& fileName)
{
#ifdef _COMPILER_WIN
	DWORD fileAttr = ::GetFileAttributesA(fileName.c_str());
	if (fileAttr != (DWORD)(-1))
		return ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) == 0);
	else
	{
		WIN32_FIND_DATAA findData;
		DWORD lastError = ::GetLastError();
		return
			(lastError != ERROR_FILE_NOT_FOUND) &&
			(lastError != ERROR_PATH_NOT_FOUND) &&
			(lastError != ERROR_INVALID_NAME) &&
			GetFileFindData(fileName, findData) &&
			(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}
#endif
#ifdef _COMPILER_LINUX
	return (euidaccess(fileName.c_str(), F_OK) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 检查目录是否存在
//-----------------------------------------------------------------------------
bool directoryExists(const std::string& dir)
{
#ifdef _COMPILER_WIN
	int code;
	code = GetFileAttributesA(dir.c_str());
	return (code != -1) && ((FILE_ATTRIBUTE_DIRECTORY & code) != 0);
#endif
#ifdef _COMPILER_LINUX
	std::string path = pathWithSlash(dir);
	struct stat st;
	bool result;

	if (stat(path.c_str(), &st) == 0)
		result = ((st.st_mode & S_IFDIR) == S_IFDIR);
	else
		result = false;

	return result;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 创建目录
// 示例:
//   createDir("C:\\test");
//   createDir("/home/test");
//-----------------------------------------------------------------------------
bool createDir(const std::string& dir)
{
#ifdef _COMPILER_WIN
	return CreateDirectoryA(dir.c_str(), NULL) != 0;
#endif
#ifdef _COMPILER_LINUX
	return mkdir(dir.c_str(), (mode_t)(-1)) == 0;
#endif
}


//-----------------------------------------------------------------------------
// 描述: 删除目录
// 参数:
//   dir     - 待删除的目录
//   recursive - 是否递归删除
// 返回:
//   true   - 成功
//   false  - 失败
//-----------------------------------------------------------------------------
bool deleteDir(const std::string& dir, bool recursive)
{
	if (!recursive)
	{
#ifdef _COMPILER_WIN
		return RemoveDirectoryA(dir.c_str()) != 0;
#endif
#ifdef _COMPILER_LINUX
		return rmdir(dir.c_str()) == 0;
#endif
	}

#ifdef _COMPILER_WIN
	const char* const ALL_FILE_WILDCHAR = "*.*";
#endif
#ifdef _COMPILER_LINUX
	const char* const ALL_FILE_WILDCHAR = "*";
#endif

	bool result = true;
	std::string path = pathWithSlash(dir);
	FileFindResult fr;
	findFiles(path + ALL_FILE_WILDCHAR, FA_ANY_FILE, fr);

	for (int i = 0; i < (int)fr.size() && result; i++)
	{
		std::string fullName = path + fr[i].fileName;
		if (fr[i].attr & FA_DIRECTORY)
			result = deleteDir(fullName, true);
		else
			removeFile(fullName);
	}

	result = deleteDir(path, false);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 取得文件名中最后一个分隔符的位置(0-based)。若没有，则返回-1
//-----------------------------------------------------------------------------
int getLastDelimPos(const std::string& fileName, const std::string& delims)
{
	int result = (int)fileName.size() - 1;

	for (; result >= 0; result--)
		if (delims.find(fileName[result], 0) != std::string::npos)
			break;

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出文件路径
// 参数:
//   fileName - 包含路径的文件名
// 返回:
//   文件的路径
// 示例:
//   extractFilePath("C:\\MyDir\\test.c");         返回: "C:\\MyDir\\"
//   extractFilePath("C:");                        返回: "C:\\"
//   extractFilePath("/home/user1/data/test.c");   返回: "/home/user1/data/"
//-----------------------------------------------------------------------------
std::string extractFilePath(const std::string& fileName)
{
	std::string delims;
	delims += PATH_DELIM;
#ifdef _COMPILER_WIN
	delims += DRIVER_DELIM;
#endif

	int pos = getLastDelimPos(fileName, delims);
	return pathWithSlash(fileName.substr(0, pos + 1));
}

//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出单独的文件名
// 参数:
//   fileName - 包含路径的文件名
// 返回:
//   文件名
// 示例:
//   extractFileName("C:\\MyDir\\test.c");         返回: "test.c"
//   extractFilePath("/home/user1/data/test.c");   返回: "test.c"
//-----------------------------------------------------------------------------
std::string extractFileName(const std::string& fileName)
{
	std::string delims;
	delims += PATH_DELIM;
#ifdef _COMPILER_WIN
	delims += DRIVER_DELIM;
#endif

	int pos = getLastDelimPos(fileName, delims);
	return fileName.substr(pos + 1, fileName.size() - pos - 1);
}


//-----------------------------------------------------------------------------
// 描述: 从文件名字符串中取出文件扩展名
// 参数:
//   fileName - 文件名 (可包含路径)
// 返回:
//   文件扩展名
// 示例:
//   extractFileExt("C:\\MyDir\\test.txt");         返回:  ".txt"
//   extractFileExt("/home/user1/data/test.c");     返回:  ".c"
//-----------------------------------------------------------------------------
std::string extractFileExt(const std::string& fileName)
{
	std::string delims;
	delims += PATH_DELIM;
#ifdef _COMPILER_WIN
	delims += DRIVER_DELIM;
#endif
	delims += FILE_EXT_DELIM;

	int pos = getLastDelimPos(fileName, delims);
	if (pos >= 0 && fileName[pos] == FILE_EXT_DELIM)
		return fileName.substr(pos, fileName.length());
	else
		return "";
}

//-----------------------------------------------------------------------------
// 描述: 改变文件名字符串中的文件扩展名
// 参数:
//   fileName - 原文件名 (可包含路径)
//   ext      - 新的文件扩展名
// 返回:
//   新的文件名
// 示例:
//   changeFileExt("c:\\test.txt", ".new");        返回:  "c:\\test.new"
//   changeFileExt("test.txt", ".new");            返回:  "test.new"
//   changeFileExt("test", ".new");                返回:  "test.new"
//   changeFileExt("test.txt", "");                返回:  "test"
//   changeFileExt("test.txt", ".");               返回:  "test."
//   changeFileExt("/home/user1/test.c", ".new");  返回:  "/home/user1/test.new"
//-----------------------------------------------------------------------------
std::string changeFileExt(const std::string& fileName, const std::string& ext)
{
	std::string result(fileName);
	std::string newExt(ext);

	if (!result.empty())
	{
		if (!newExt.empty() && newExt[0] != FILE_EXT_DELIM)
			newExt = FILE_EXT_DELIM + newExt;

		std::string oldExt = extractFileExt(result);
		if (!oldExt.empty())
			result.erase(result.length() - oldExt.length());
		result += newExt;
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 强制创建目录
// 参数:
//   dir - 待创建的目录 (可以是多级目录)
// 返回:
//   true   - 成功
//   false  - 失败
// 示例:
//   forceDirectories("C:\\MyDir\\Test");
//   forceDirectories("/home/user1/data");
//-----------------------------------------------------------------------------
bool forceDirectories(std::string dir)
{
	int len = (int)dir.length();

	if (dir.empty()) return false;
	if (dir[len - 1] == PATH_DELIM)
		dir.resize(len - 1);

#ifdef _COMPILER_WIN
	if (dir.length() < 3 || directoryExists(dir) ||
		extractFilePath(dir) == dir) return true;    // avoid 'xyz:\' problem.
#endif
#ifdef _COMPILER_LINUX
	if (dir.empty() || directoryExists(dir)) return true;
#endif
	return forceDirectories(extractFilePath(dir)) && createDir(dir);
}

//-----------------------------------------------------------------------------
// 描述: 删除文件
//-----------------------------------------------------------------------------
bool deleteFile(const std::string& fileName)
{
#ifdef _COMPILER_WIN
	DWORD fileAttr = ::GetFileAttributesA(fileName.c_str());
	if (fileAttr != (DWORD)(-1) && (fileAttr & FILE_ATTRIBUTE_READONLY) != 0)
	{
		fileAttr &= ~FILE_ATTRIBUTE_READONLY;
		::SetFileAttributesA(fileName.c_str(), fileAttr);
	}

	return ::DeleteFileA(fileName.c_str()) != 0;
#endif
#ifdef _COMPILER_LINUX
	return (unlink(fileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 同 deleteFile()
//-----------------------------------------------------------------------------
bool removeFile(const std::string& fileName)
{
	return deleteFile(fileName);
}

//-----------------------------------------------------------------------------
// 描述: 文件重命名
//-----------------------------------------------------------------------------
bool renameFile(const std::string& oldFileName, const std::string& newFileName)
{
#ifdef _COMPILER_WIN
	return ::MoveFileA(oldFileName.c_str(), newFileName.c_str()) != 0;
#endif
#ifdef _COMPILER_LINUX
	return (rename(oldFileName.c_str(), newFileName.c_str()) == 0);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得文件的大小。若失败则返回-1
//-----------------------------------------------------------------------------
INT64 getFileSize(const std::string& fileName)
{
	INT64 result;

	try
	{
		FileStream fileStream(fileName, FM_OPEN_READ | FM_SHARE_DENY_NONE);
		result = fileStream.getSize();
	}
	catch (Exception&)
	{
		result = -1;
	}

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 在指定路径下查找符合条件的文件
// 参数:
//   path    - 指定在哪个路径下进行查找，并必须指定通配符
//   attr      - 只查找符合此属性的文件
//   findResult - 传回查找结果
// 示例:
//   findFiles("C:\\test\\*.*", FA_ANY_FILE & ~FA_HIDDEN, fr);
//   findFiles("/home/*.log", FA_ANY_FILE & ~FA_SYM_LINK, fr);
//-----------------------------------------------------------------------------
void findFiles(const std::string& path, UINT attr, FileFindResult& findResult)
{
	const UINT FA_SPECIAL = FA_HIDDEN | FA_SYS_FILE | FA_VOLUME_ID | FA_DIRECTORY;
	UINT excludeAttr = ~attr & FA_SPECIAL;
	findResult.clear();

#ifdef _COMPILER_WIN
	HANDLE findHandle;
	WIN32_FIND_DATAA findData;

	findHandle = FindFirstFileA(path.c_str(), &findData);
	if (findHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			DWORD attr = findData.dwFileAttributes;
			std::string name = findData.cFileName;
			bool isSpecDir = (attr & FA_DIRECTORY) && (name == "." || name == "..");

			if ((attr & excludeAttr) == 0 && !isSpecDir)
			{
				FileFindItem item;
				item.fileSize = findData.nFileSizeHigh;
				item.fileSize = (item.fileSize << 32) | findData.nFileSizeLow;
				item.fileName = name;
				item.attr = attr;

				findResult.push_back(item);
			}
		} while (FindNextFileA(findHandle, &findData));

		FindClose(findHandle);
	}
#endif

#ifdef _COMPILER_LINUX
	std::string pathOnlyStr = extractFilePath(path);
	std::string patternStr = extractFileName(path);
	std::string fullName, name;
	DIR *dirPtr;
	struct dirent dirEnt, *dirEntPtr = NULL;
	struct stat statBuf, linkStatBuf;
	UINT fileAttr, fileMode;

	if (pathOnlyStr.empty()) pathOnlyStr = "/";

	dirPtr = opendir(pathOnlyStr.c_str());
	if (dirPtr)
	{
		while ((readdir_r(dirPtr, &dirEnt, &dirEntPtr) == 0) && dirEntPtr)
		{
			if (!fnmatch(patternStr.c_str(), dirEntPtr->d_name, 0) == 0) continue;

			name = dirEntPtr->d_name;
			fullName = pathOnlyStr + name;

			if (lstat(fullName.c_str(), &statBuf) == 0)
			{
				fileAttr = 0;
				fileMode = statBuf.st_mode;

				if (S_ISDIR(fileMode))
					fileAttr |= FA_DIRECTORY;
				else if (!S_ISREG(fileMode))
				{
					if (S_ISLNK(fileMode))
					{
						fileAttr |= FA_SYM_LINK;
						if ((stat(fullName.c_str(), &linkStatBuf) == 0) &&
							(S_ISDIR(linkStatBuf.st_mode)))
							fileAttr |= FA_DIRECTORY;
					}
					fileAttr |= FA_SYS_FILE;
				}

				if (dirEntPtr->d_name[0] == '.' && dirEntPtr->d_name[1])
					if (!(dirEntPtr->d_name[1] == '.' && !dirEntPtr->d_name[2]))
						fileAttr |= FA_HIDDEN;

				if (euidaccess(fullName.c_str(), W_OK) != 0)
					fileAttr |= FA_READ_ONLY;

				bool isSpecDir = (fileAttr & FA_DIRECTORY) && (name == "." || name == "..");

				if ((fileAttr & excludeAttr) == 0 && !isSpecDir)
				{
					FileFindItem item;
					item.fileSize = statBuf.st_size;
					item.fileName = name;
					item.attr = fileAttr;

					findResult.push_back(item);
				}
			}
		} // while

		closedir(dirPtr);
	}
#endif
}

//-----------------------------------------------------------------------------
// 描述: 补全路径字符串后面的 "\" 或 "/"
//-----------------------------------------------------------------------------
std::string pathWithSlash(const std::string& path)
{
	std::string result = trimString(path);
	int len = (int)result.size();
	if (len > 0 && result[len - 1] != PATH_DELIM)
		result += PATH_DELIM;
	return result;
}

//-----------------------------------------------------------------------------
// 描述: 去掉路径字符串后面的 "\" 或 "/"
//-----------------------------------------------------------------------------
std::string pathWithoutSlash(const std::string& path)
{
	std::string result = trimString(path);
	int len = (int)result.size();
	if (len > 0 && result[len - 1] == PATH_DELIM)
		result.resize(len - 1);
	return result;
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件的名称
//-----------------------------------------------------------------------------
std::string getAppExeName(bool includePath)
{
#ifdef _COMPILER_WIN
	char buffer[MAX_PATH];
	::GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string result = std::string(buffer);
	if (!includePath)
		result = extractFileName(result);
	return result;
#endif
#ifdef _COMPILER_LINUX
	const int BUFFER_SIZE = 1024;

	int r;
	char buffer[BUFFER_SIZE];
	std::string result;

	r = readlink("/proc/self/exe", buffer, BUFFER_SIZE);
	if (r != -1)
	{
		if (r >= BUFFER_SIZE)
			r = BUFFER_SIZE - 1;
		buffer[r] = 0;
		result = buffer;
		if (!includePath)
			result = extractFileName(result);
	}
	else
	{
		ThrowException(SEM_NO_PERM_READ_PROCSELFEXE);
	}

	return result;
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径
//-----------------------------------------------------------------------------
std::string getAppPath()
{
	return extractFilePath(getAppExeName(true));
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径的子目录
//-----------------------------------------------------------------------------
std::string getAppSubPath(const std::string& subDir)
{
	return pathWithSlash(getAppPath() + subDir);
}