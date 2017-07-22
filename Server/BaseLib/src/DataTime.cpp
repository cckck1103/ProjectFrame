#include "DataTime.h"
///////////////////////////////////////////////////////////////////////////////
// class Timestamp

//-----------------------------------------------------------------------------
// 描述: 返回当前时间戳
//-----------------------------------------------------------------------------
Timestamp Timestamp::now()
{
	Timestamp result;

#ifdef _COMPILER_WIN
	FILETIME ft;
	::GetSystemTimeAsFileTime(&ft);

	ULARGE_INTEGER epoch; // UNIX epoch (1970-01-01 00:00:00) expressed in Windows NT FILETIME
	epoch.LowPart = 0xD53E8000;
	epoch.HighPart = 0x019DB1DE;

	ULARGE_INTEGER ts;
	ts.LowPart = ft.dwLowDateTime;
	ts.HighPart = ft.dwHighDateTime;
	ts.QuadPart -= epoch.QuadPart;
	result.value_ = ts.QuadPart / 10000;
#endif
#ifdef _COMPILER_LINUX
	struct timeval tv;
	gettimeofday(&tv, NULL);
	result.value_ = TimeVal(tv.tv_sec) * MILLISECS_PER_SECOND + tv.tv_usec;
#endif

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 设置为当前时间
//-----------------------------------------------------------------------------
void Timestamp::update()
{
	value_ = Timestamp::now().value_;
}

//-----------------------------------------------------------------------------
// 描述: 将 time_t 赋给时间戳
//-----------------------------------------------------------------------------
void Timestamp::setEpochTime(time_t value)
{
	value_ =  value * MILLISECS_PER_SECOND;
}

//-----------------------------------------------------------------------------
// 描述: 以 time_t 形式返回 (since 1970-01-01 00:00:00, in seconds)
//-----------------------------------------------------------------------------
time_t Timestamp::epochTime() const
{
	return (time_t)(value_ / MILLISECS_PER_SECOND);
}

//-----------------------------------------------------------------------------
// 描述: 以毫秒精度返回 epoch 时间 (since 1970-01-01 00:00:00)
//-----------------------------------------------------------------------------
Timestamp::TimeVal Timestamp::epochMilliseconds() const
{
	return value_;
}

//-----------------------------------------------------------------------------
// 描述: 转换成字符串
//-----------------------------------------------------------------------------
std::string Timestamp::toString(const std::string& dateSep, const std::string& dateTimeSep,
	const std::string& timeSep, const std::string& microSecSep) const
{
	time_t seconds = static_cast<time_t>(value_ / MILLISECS_PER_SECOND);
	int milliseconds = static_cast<int>(value_ % MILLISECS_PER_SECOND);

	std::string result = DateTime(seconds).toDateTimeString(dateSep, dateTimeSep, timeSep);
	result += formatString("%s%03d", microSecSep.c_str(), milliseconds);

	return result;
}


///////////////////////////////////////////////////////////////////////////////
// class DateTime

//-----------------------------------------------------------------------------
// 描述: 返回当前时间 (从1970-01-01 00:00:00 算起的秒数, UTC时间)
//-----------------------------------------------------------------------------
DateTime DateTime::now()
{
	return DateTime(time(NULL));
}

//-----------------------------------------------------------------------------
// 描述: 将 Timestamp 转换成 DateTime
//-----------------------------------------------------------------------------
DateTime& DateTime::operator = (const Timestamp& rhs)
{
	time_ = rhs.epochTime();
	return *this;
}

//-----------------------------------------------------------------------------
// 描述: 将字符串转换成 DateTime
// 注意: dateTimeStr 的格式必须为 YYYY-MM-DD HH:MM:SS
//-----------------------------------------------------------------------------
DateTime& DateTime::operator = (const std::string& dateTimeStr)
{
	int year, month, day, hour, minute, second;

	if (dateTimeStr.length() == 19)
	{
		year = strToInt(dateTimeStr.substr(0, 4), 0);
		month = strToInt(dateTimeStr.substr(5, 2), 0);
		day = strToInt(dateTimeStr.substr(8, 2), 0);
		hour = strToInt(dateTimeStr.substr(11, 2), 0);
		minute = strToInt(dateTimeStr.substr(14, 2), 0);
		second = strToInt(dateTimeStr.substr(17, 2), 0);

		encodeDateTime(year, month, day, hour, minute, second);
		return *this;
	}
	else
	{
		ThrowException(SEM_INVALID_DATETIME_STR);
		return *this;
	}
}

//-----------------------------------------------------------------------------
// 描述: 日期时间编码，并存入 *this
//-----------------------------------------------------------------------------
void DateTime::encodeDateTime(int year, int month, int day,
	int hour, int minute, int second)
{
	struct tm tm;

	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;

	time_ = mktime(&tm);
}

//-----------------------------------------------------------------------------
// 描述: 日期时间解码，并存入各参数
//-----------------------------------------------------------------------------
void DateTime::decodeDateTime(int *year, int *month, int *day,
	int *hour, int *minute, int *second, int *weekDay, int *yearDay) const
{
	struct tm tm;

#ifdef _COMPILER_WIN
	localtime_s(&tm, &time_);
#endif

#ifdef _COMPILER_LINUX
	localtime_r(&time_, &tm);
#endif

	if (year) *year = tm.tm_year + 1900;
	if (month) *month = tm.tm_mon + 1;
	if (day) *day = tm.tm_mday;
	if (hour) *hour = tm.tm_hour;
	if (minute) *minute = tm.tm_min;
	if (second) *second = tm.tm_sec;
	if (weekDay) *weekDay = tm.tm_wday;
	if (yearDay) *yearDay = tm.tm_yday;
}

//-----------------------------------------------------------------------------
// 描述: 返回日期字符串
// 参数:
//   dateSep - 日期分隔符
// 格式:
//   YYYY-MM-DD
//-----------------------------------------------------------------------------
std::string DateTime::toDateString(const std::string& dateSep) const
{
	std::string result;
	int year, month, day;

	decodeDateTime(&year, &month, &day, NULL, NULL, NULL, NULL);
	result = formatString("%04d%s%02d%s%02d",
		year, dateSep.c_str(), month, dateSep.c_str(), day);

	return result;
}

//-----------------------------------------------------------------------------
// 描述: 返回日期时间字符串
// 参数:
//   dateSep     - 日期分隔符
//   dateTimeSep - 日期和时间之间的分隔符
//   timeSep     - 时间分隔符
// 格式:
//   YYYY-MM-DD HH:MM:SS
//-----------------------------------------------------------------------------
std::string DateTime::toDateTimeString(const std::string& dateSep,
	const std::string& dateTimeSep, const std::string& timeSep) const
{
	std::string result;
	int year, month, day, hour, minute, second;

	decodeDateTime(&year, &month, &day, &hour, &minute, &second, NULL);
	result = formatString("%04d%s%02d%s%02d%s%02d%s%02d%s%02d",
		year, dateSep.c_str(), month, dateSep.c_str(), day,
		dateTimeSep.c_str(),
		hour, timeSep.c_str(), minute, timeSep.c_str(), second);

	return result;
}
