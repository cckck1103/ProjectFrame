#ifndef _STRING_LIST_H_
#define _STRING_LIST_H_

#include "Options.h"
#include "GlobalDefs.h"
#include "StreamClass.h"
///////////////////////////////////////////////////////////////////////////////
// class Strings - 字符串列表抽象类

class Strings
{
private:
	enum STRINGS_DEFINED
	{
		SD_DELIMITER = 0x0001,
		SD_QUOTE_CHAR = 0x0002,
		SD_NAME_VALUE_SEP = 0x0004,
		SD_LINE_BREAK = 0x0008
	};

public:
	// Calls beginUpdate() and endUpdate() automatically in a scope.
	class AutoUpdater
	{
	private:
		Strings& strings;
	public:
		AutoUpdater(Strings& _strings) : strings(_strings)
		{
			strings.beginUpdate();
		}
		~AutoUpdater()
		{
			strings.endUpdate();
		}
	};

public:
	Strings();
	virtual ~Strings() {}

	virtual int add(const char* str);
	virtual int add(const char* str, POINTER data);
	virtual void addStrings(const Strings& strings);
	virtual void insert(int index, const char* str) = 0;
	virtual void insert(int index, const char* str, POINTER data);
	virtual void clear() = 0;
	virtual void del(int index) = 0;
	virtual bool equals(const Strings& strings);
	virtual void exchange(int index1, int index2);
	virtual void move(int curIndex, int newIndex);
	virtual bool exists(const char* str) const;
	virtual int indexOf(const char* str) const;
	virtual int indexOfName(const char* name) const;
	virtual int indexOfData(POINTER data) const;

	virtual bool loadFromStream(Stream& stream);
	virtual bool loadFromFile(const char* fileName);
	virtual bool saveToStream(Stream& stream) const;
	virtual bool saveToFile(const char* fileName) const;

	virtual int getCapacity() const { return getCount(); }
	virtual void setCapacity(int value) {}
	virtual int getCount() const = 0;
	bool isEmpty() const { return (getCount() <= 0); }
	char getDelimiter() const;
	void setDelimiter(char value);
	std::string getLineBreak() const;
	void setLineBreak(const char* value);
	char getQuoteChar() const;
	void setQuoteChar(char value);
	char getNameValueSeparator() const;
	void setNameValueSeparator(char value);
	std::string combineNameValue(const char* name, const char* value) const;
	std::string getName(int index) const;
	std::string getValue(const char* name) const;
	std::string getValue(int index) const;
	void setValue(const char* name, const char* value);
	void setValue(int index, const char* value);
	virtual POINTER getData(int index) const { return NULL; }
	virtual void setData(int index, POINTER data) {}
	virtual std::string getText() const;
	virtual void setText(const char* value);
	std::string getCommaText() const;
	void setCommaText(const char* value);
	std::string getDelimitedText() const;
	void setDelimitedText(const char* value);
	virtual const std::string& getString(int index) const = 0;
	virtual void setString(int index, const char* value);

	void beginUpdate();
	void endUpdate();

	Strings& operator = (const Strings& rhs);
	const std::string& operator[] (int index) const { return getString(index); }

protected:
	virtual void setUpdateState(bool isUpdating) {}
	virtual int compareStrings(const char* str1, const char* str2) const;

protected:
	void init();
	void error(const char* msg, int data) const;
	int getUpdateCount() const { return updateCount_; }
	std::string extractName(const char* str) const;

private:
	void assign(const Strings& src);

protected:
	UINT defined_;
	char delimiter_;
	std::string lineBreak_;
	char quoteChar_;
	char nameValueSeparator_;
	int updateCount_;
};

///////////////////////////////////////////////////////////////////////////////
// class StrList - 字符串列表类

class StrList : public Strings
{
public:
	friend int stringListCompareProc(const StrList& stringList, int index1, int index2);

public:
	/// The comparison function prototype that used by Sort().
	typedef int(*StringListCompareProc)(const StrList& stringList, int index1, int index2);

	/// Indicates the response when an application attempts to add a duplicate entry to a list.
	enum DUPLICATE_MODE
	{
		DM_IGNORE,      // Ignore attempts to add duplicate entries (do not add the duplicate).
		DM_ACCEPT,      // Allow the list to contain duplicate entries (add the duplicate).
		DM_ERROR        // Throw an exception when the application tries to add a duplicate.
	};

public:
	StrList();
	StrList(const StrList& src);
	virtual ~StrList();

	virtual int add(const char* str);
	virtual int add(const char* str, POINTER data);
	int add(const std::string& str) { return add(str.c_str()); }
	virtual void clear();
	virtual void del(int index);
	virtual void exchange(int index1, int index2);
	virtual int indexOf(const char* str) const;
	virtual void insert(int index, const char* str);
	virtual void insert(int index, const char* str, POINTER data);

	virtual int getCapacity() const { return capacity_; }
	virtual void setCapacity(int value);
	virtual int getCount() const { return count_; }
	virtual POINTER getData(int index) const;
	virtual void setData(int index, POINTER data);
	virtual const std::string& getString(int index) const;
	virtual void setString(int index, const char* value);

	virtual bool find(const char* str, int& index) const;
	virtual void sort();
	virtual void sort(StringListCompareProc compareProc);

	DUPLICATE_MODE getDupMode() const { return dupMode_; }
	void setDupMode(DUPLICATE_MODE value) { dupMode_ = value; }
	bool getSorted() const { return isSorted_; }
	virtual void setSorted(bool value);
	bool getCaseSensitive() const { return isCaseSensitive_; }
	virtual void setCaseSensitive(bool value);

	StrList& operator = (const StrList& rhs);

protected: // override
	virtual void setUpdateState(bool isUpdating);
	virtual int compareStrings(const char* str1, const char* str2) const;

protected: // virtual
		   /// Occurs immediately before the list of strings changes.
	virtual void onChanging() {}
	/// Occurs immediately after the list of strings changes.
	virtual void onChanged() {}
	/// Internal method used to insert a std::string to the list.
	virtual void insertItem(int index, const char* str, POINTER data);

private:
	void init();
	void assign(const StrList& src);
	void internalClear();
	std::string& stringObjectNeeded(int index) const;
	void exchangeItems(int index1, int index2);
	void grow();
	void quickSort(int l, int r, StringListCompareProc compareProc);

private:
	struct StringItem
	{
		std::string *str;
		POINTER data;
	};

private:
	StringItem *list_;
	int count_;
	int capacity_;
	DUPLICATE_MODE dupMode_;
	bool isSorted_;
	bool isCaseSensitive_;
};


#endif