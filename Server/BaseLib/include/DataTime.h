#ifndef _DATA_TIME_H_
#define _DATA_TIME_H_

#include "Options.h"
#include "SysUtils.h"
#include "UtilClass.h"
#include "GlobalDefs.h"

///////////////////////////////////////////////////////////////////////////////
// class DateTime - 日期时间类
class Timestamp;

class DateTime
{
public:
	DateTime() { time_ = 0; }
	DateTime(const DateTime& src) { time_ = src.time_; }
	explicit DateTime(time_t src) { time_ = src; }
	explicit DateTime(const std::string& src) { *this = src; }

	static DateTime now();

	DateTime& operator = (const DateTime& rhs) { time_ = rhs.time_; return *this; }
	DateTime& operator = (const time_t rhs) { time_ = rhs; return *this; }
	DateTime& operator = (const Timestamp& rhs);
	DateTime& operator = (const std::string& dateTimeStr);

	DateTime operator + (const DateTime& rhs) const { return DateTime(time_ + rhs.time_); }
	DateTime operator + (time_t rhs) const { return DateTime(time_ + rhs); }
	DateTime operator - (const DateTime& rhs) const { return DateTime(time_ - rhs.time_); }
	DateTime operator - (time_t rhs) const { return DateTime(time_ - rhs); }

	bool operator == (const DateTime& rhs) const { return time_ == rhs.time_; }
	bool operator != (const DateTime& rhs) const { return time_ != rhs.time_; }
	bool operator > (const DateTime& rhs) const { return time_ > rhs.time_; }
	bool operator < (const DateTime& rhs) const { return time_ < rhs.time_; }
	bool operator >= (const DateTime& rhs) const { return time_ >= rhs.time_; }
	bool operator <= (const DateTime& rhs) const { return time_ <= rhs.time_; }

	time_t epochTime() const { return time_; }

	void encodeDateTime(int year, int month, int day,
		int hour = 0, int minute = 0, int second = 0);
	void decodeDateTime(int *year, int *month, int *day,
		int *hour, int *minute, int *second,
		int *weekDay = NULL, int *yearDay = NULL) const;

	std::string toDateString(const std::string& dateSep = "-") const;
	std::string toDateTimeString(const std::string& dateSep = "-",
		const std::string& dateTimeSep = " ", const std::string& timeSep = ":") const;

private:
	time_t time_;     // (从1970-01-01 00:00:00 算起的秒数，UTC时间)
};


///////////////////////////////////////////////////////////////////////////////
// class Timestamp - 时间戳类 (毫秒精度)

class Timestamp
{
public:
	typedef INT64 TimeVal;   // UTC time value in millisecond resolution
	typedef INT64 TimeDiff;  // difference between two timestamps in milliseconds

public:
	Timestamp() { value_ = 0; }
	explicit Timestamp(TimeVal value) { value_ = value; }
	Timestamp(const Timestamp& src) { value_ = src.value_; }

	static Timestamp now();

	Timestamp& operator = (const Timestamp& rhs) { value_ = rhs.value_; return *this; }
	Timestamp& operator = (TimeVal rhs) { value_ = rhs; return *this; }
	Timestamp& operator = (const DateTime& rhs) { setEpochTime(rhs.epochTime()); return *this; }

	bool operator == (const Timestamp& rhs) const { return value_ == rhs.value_; }
	bool operator != (const Timestamp& rhs) const { return value_ != rhs.value_; }
	bool operator > (const Timestamp& rhs) const { return value_ > rhs.value_; }
	bool operator < (const Timestamp& rhs) const { return value_ < rhs.value_; }
	bool operator >= (const Timestamp& rhs) const { return value_ >= rhs.value_; }
	bool operator <= (const Timestamp& rhs) const { return value_ <= rhs.value_; }

	Timestamp  operator +  (TimeDiff d) const { return Timestamp(value_ + d); }
	Timestamp  operator -  (TimeDiff d) const { return Timestamp(value_ - d); }
	TimeDiff   operator -  (const Timestamp& rhs) const { return value_ - rhs.value_; }
	Timestamp& operator += (TimeDiff d) { value_ += d; return *this; }
	Timestamp& operator -= (TimeDiff d) { value_ -= d; return *this; }

	void update();
	void setEpochTime(time_t value);
	time_t epochTime() const;
	TimeVal epochMilliseconds() const;

	std::string toString(const std::string& dateSep = "-", const std::string& dateTimeSep = " ",
		const std::string& timeSep = ":", const std::string& microSecSep = ".") const;

private:
	TimeVal value_;
};

#endif