#include "StringList.h"

#include "Exceptions.h"
///////////////////////////////////////////////////////////////////////////////
// class Strings

Strings::Strings()
{
	init();
}

//-----------------------------------------------------------------------------

int Strings::add(const char* str)
{
	int result = getCount();
	insert(result, str);
	return result;
}

//-----------------------------------------------------------------------------

int Strings::add(const char* str, POINTER data)
{
	int result = add(str);
	setData(result, data);
	return result;
}

//-----------------------------------------------------------------------------

void Strings::addStrings(const Strings& strings)
{
	AutoUpdater autoUpdater(*this);

	for (int i = 0; i < strings.getCount(); i++)
		add(strings.getString(i).c_str(), strings.getData(i));
}

//-----------------------------------------------------------------------------

void Strings::insert(int index, const char* str, POINTER data)
{
	insert(index, str);
	setData(index, data);
}

//-----------------------------------------------------------------------------

bool Strings::equals(const Strings& strings)
{
	bool result;
	int count = getCount();

	result = (count == strings.getCount());
	if (result)
	{
		for (int i = 0; i < count; i++)
			if (getString(i) != strings.getString(i))
			{
				result = false;
				break;
			}
	}

	return result;
}

//-----------------------------------------------------------------------------

void Strings::exchange(int index1, int index2)
{
	AutoUpdater autoUpdater(*this);

	POINTER tempData;
	std::string tempStr;

	tempStr = getString(index1);
	tempData = getData(index1);
	setString(index1, getString(index2).c_str());
	setData(index1, getData(index2));
	setString(index2, tempStr.c_str());
	setData(index2, tempData);
}

//-----------------------------------------------------------------------------

void Strings::move(int curIndex, int newIndex)
{
	if (curIndex != newIndex)
	{
		AutoUpdater autoUpdater(*this);

		POINTER tempData;
		std::string tempStr;

		tempStr = getString(curIndex);
		tempData = getData(curIndex);
		del(curIndex);
		insert(newIndex, tempStr.c_str(), tempData);
	}
}

//-----------------------------------------------------------------------------

bool Strings::exists(const char* str) const
{
	return (indexOf(str) >= 0);
}

//-----------------------------------------------------------------------------

int Strings::indexOf(const char* str) const
{
	int result = -1;

	for (int i = 0; i < getCount(); i++)
		if (compareStrings(getString(i).c_str(), str) == 0)
		{
			result = i;
			break;
		}

	return result;
}

//-----------------------------------------------------------------------------

int Strings::indexOfName(const char* name) const
{
	int result = -1;

	for (int i = 0; i < getCount(); i++)
		if (compareStrings((extractName(getString(i).c_str()).c_str()), name) == 0)
		{
			result = i;
			break;
		}

	return result;
}

//-----------------------------------------------------------------------------

int Strings::indexOfData(POINTER data) const
{
	int result = -1;

	for (int i = 0; i < getCount(); i++)
		if (getData(i) == data)
		{
			result = i;
			break;
		}

	return result;
}

//-----------------------------------------------------------------------------

bool Strings::loadFromStream(Stream& stream)
{
	try
	{
		AutoUpdater autoUpdater(*this);

		const int BLOCK_SIZE = 1024 * 64;
		Buffer block(BLOCK_SIZE);
		MemoryStream memStream;

		while (true)
		{
			int bytesRead = stream.read(block.data(), block.getSize());
			if (bytesRead > 0)
				memStream.write(block.data(), bytesRead);

			if (bytesRead < block.getSize())
				break;
		}

		char endChar = 0;
		memStream.write(&endChar, sizeof(char));

		setText(memStream.getMemory());
		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------

bool Strings::loadFromFile(const char* fileName)
{
	FileStream fs;
	bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
	if (result)
		result = loadFromStream(fs);
	return result;
}

//-----------------------------------------------------------------------------

bool Strings::saveToStream(Stream& stream) const
{
	try
	{
		std::string text = getText();
		int len = (int)text.length();
		stream.writeBuffer((char*)text.c_str(), len * sizeof(char));
		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------

bool Strings::saveToFile(const char* fileName) const
{
	FileStream fs;
	bool result = fs.open(fileName, FM_CREATE);
	if (result)
		result = saveToStream(fs);
	return result;
}

//-----------------------------------------------------------------------------

char Strings::getDelimiter() const
{
	if ((defined_ & SD_DELIMITER) == 0)
		const_cast<Strings&>(*this).setDelimiter(DEFAULT_DELIMITER);
	return delimiter_;
}

//-----------------------------------------------------------------------------

void Strings::setDelimiter(char value)
{
	if (delimiter_ != value || (defined_ & SD_DELIMITER) == 0)
	{
		defined_ |= SD_DELIMITER;
		delimiter_ = value;
	}
}

//-----------------------------------------------------------------------------

std::string Strings::getLineBreak() const
{
	if ((defined_ & SD_LINE_BREAK) == 0)
		const_cast<Strings&>(*this).setLineBreak(DEFAULT_LINE_BREAK);
	return lineBreak_;
}

//-----------------------------------------------------------------------------

void Strings::setLineBreak(const char* value)
{
	if (lineBreak_ != std::string(value) || (defined_ & SD_LINE_BREAK) == 0)
	{
		defined_ |= SD_LINE_BREAK;
		lineBreak_ = value;
	}
}

//-----------------------------------------------------------------------------

char Strings::getQuoteChar() const
{
	if ((defined_ & SD_QUOTE_CHAR) == 0)
		const_cast<Strings&>(*this).setQuoteChar(DEFAULT_QUOTE_CHAR);
	return quoteChar_;
}

//-----------------------------------------------------------------------------

void Strings::setQuoteChar(char value)
{
	if (quoteChar_ != value || (defined_ & SD_QUOTE_CHAR) == 0)
	{
		defined_ |= SD_QUOTE_CHAR;
		quoteChar_ = value;
	}
}

//-----------------------------------------------------------------------------

char Strings::getNameValueSeparator() const
{
	if ((defined_ & SD_NAME_VALUE_SEP) == 0)
		const_cast<Strings&>(*this).setNameValueSeparator(DEFAULT_NAME_VALUE_SEP);
	return nameValueSeparator_;
}

//-----------------------------------------------------------------------------

void Strings::setNameValueSeparator(char value)
{
	if (nameValueSeparator_ != value || (defined_ & SD_NAME_VALUE_SEP) == 0)
	{
		defined_ |= SD_NAME_VALUE_SEP;
		nameValueSeparator_ = value;
	}
}

//-----------------------------------------------------------------------------

std::string Strings::combineNameValue(const char* name, const char* value) const
{
	std::string nameStr(name);
	char nameValueSep = getNameValueSeparator();

	if (nameStr.find(nameValueSep) != std::string::npos)
		error(SEM_STRINGS_NAME_ERROR, 0);

	return nameStr + nameValueSep + value;
}

//-----------------------------------------------------------------------------

std::string Strings::getName(int index) const
{
	return extractName(getString(index).c_str());
}

//-----------------------------------------------------------------------------

std::string Strings::getValue(const char* name) const
{
	std::string nameStr(name);
	int i = indexOfName(nameStr.c_str());
	if (i >= 0)
		return getString(i).substr(nameStr.length() + 1);
	else
		return "";
}

//-----------------------------------------------------------------------------

std::string Strings::getValue(int index) const
{
	if (index >= 0)
	{
		std::string name = getName(index);
		if (!name.empty())
			return getString(index).substr(name.length() + 1);
		else
		{
			std::string strItem = getString(index);
			if (!strItem.empty() && strItem[0] == getNameValueSeparator())
				return strItem.substr(1);
			else
				return "";
		}
	}
	else
		return "";
}

//-----------------------------------------------------------------------------

void Strings::setValue(const char* name, const char* value)
{
	std::string nameStr(name);
	std::string valueStr(value);

	int i = indexOfName(nameStr.c_str());
	if (valueStr.empty())
	{
		if (i >= 0) del(i);
	}
	else
	{
		if (i < 0)
			i = add("");
		setString(i, (nameStr + getNameValueSeparator() + valueStr).c_str());
	}
}

//-----------------------------------------------------------------------------

void Strings::setValue(int index, const char* value)
{
	std::string valueStr(value);

	if (valueStr.empty())
	{
		if (index >= 0) del(index);
	}
	else
	{
		if (index < 0)
			index = add("");
		setString(index, (getName(index) + getNameValueSeparator() + valueStr).c_str());
	}
}

//-----------------------------------------------------------------------------

std::string Strings::getText() const
{
	std::string result;
	int count = getCount();
	std::string lineBreak = getLineBreak();

	for (int i = 0; i < count; i++)
	{
		result += getString(i);
		result += lineBreak;
	}

	return result;
}

//-----------------------------------------------------------------------------

void Strings::setText(const char* value)
{
	AutoUpdater autoUpdater(*this);

	std::string valueStr(value);
	std::string lineBreak = getLineBreak();
	int start = 0;

	clear();
	while (true)
	{
		std::string::size_type pos = valueStr.find(lineBreak, start);
		if (pos != std::string::npos)
		{
			add(valueStr.substr(start, pos - start).c_str());
			start = (int)(pos + lineBreak.length());
		}
		else
		{
			std::string str = valueStr.substr(start);
			if (!str.empty())
				add(str.c_str());
			break;
		}
	}
}

//-----------------------------------------------------------------------------

std::string Strings::getCommaText() const
{
	UINT bakDefined = defined_;
	char bakDelimiter = delimiter_;
	char bakQuoteChar = quoteChar_;

	const_cast<Strings&>(*this).setDelimiter(DEFAULT_DELIMITER);
	const_cast<Strings&>(*this).setQuoteChar(DEFAULT_QUOTE_CHAR);

	std::string result = getDelimitedText();  // 不可以抛出异常

	const_cast<Strings&>(*this).defined_ = bakDefined;
	const_cast<Strings&>(*this).delimiter_ = bakDelimiter;
	const_cast<Strings&>(*this).quoteChar_ = bakQuoteChar;

	return result;
}

//-----------------------------------------------------------------------------

void Strings::setCommaText(const char* value)
{
	setDelimiter(DEFAULT_DELIMITER);
	setQuoteChar(DEFAULT_QUOTE_CHAR);

	setDelimitedText(value);
}

//-----------------------------------------------------------------------------

std::string Strings::getDelimitedText() const
{
	std::string result;
	std::string line;
	int count = getCount();
	char quoteChar = getQuoteChar();
	char delimiter = getDelimiter();

	if (count == 1 && getString(0).empty())
		return std::string(getQuoteChar(), 2);

	std::string delimiters;
	for (int i = 1; i <= 32; i++)
		delimiters += (char)i;
	delimiters += quoteChar;
	delimiters += delimiter;

	for (int i = 0; i < count; i++)
	{
		line = getString(i);
		if (line.find_first_of(delimiters) != std::string::npos)
			line = getQuotedStr((char*)line.c_str(), quoteChar);

		if (i > 0) result += delimiter;
		result += line;
	}

	return result;
}

//-----------------------------------------------------------------------------

void Strings::setDelimitedText(const char* value)
{
	AutoUpdater autoUpdater(*this);

	char quoteChar = getQuoteChar();
	char delimiter = getDelimiter();
	const char* curPtr = value;
	std::string line;

	clear();
	while (*curPtr >= 1 && *curPtr <= 32)
		curPtr++;

	while (*curPtr != '\0')
	{
		if (*curPtr == quoteChar)
			line = extractQuotedStr(curPtr, quoteChar);
		else
		{
			const char* p = curPtr;
			while (*curPtr > 32 && *curPtr != delimiter)
				curPtr++;
			line.assign(p, curPtr - p);
		}

		add(line.c_str());

		while (*curPtr >= 1 && *curPtr <= 32)
			curPtr++;

		if (*curPtr == delimiter)
		{
			const char* p = curPtr;
			p++;
			if (*p == '\0')
				add("");

			do curPtr++; while (*curPtr >= 1 && *curPtr <= 32);
		}
	}
}

//-----------------------------------------------------------------------------

void Strings::setString(int index, const char* value)
{
	POINTER tempData = getData(index);
	del(index);
	insert(index, value, tempData);
}

//-----------------------------------------------------------------------------

void Strings::beginUpdate()
{
	if (updateCount_ == 0)
		setUpdateState(true);
	updateCount_++;
}

//-----------------------------------------------------------------------------

void Strings::endUpdate()
{
	updateCount_--;
	if (updateCount_ == 0)
		setUpdateState(false);
}

//-----------------------------------------------------------------------------

Strings& Strings::operator = (const Strings& rhs)
{
	if (this != &rhs)
		assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------

int Strings::compareStrings(const char* str1, const char* str2) const
{
	int r = compareText(str1, str2);

	if (r > 0) r = 1;
	else if (r < 0) r = -1;

	return r;
}

//-----------------------------------------------------------------------------

void Strings::init()
{
	defined_ = 0;
	delimiter_ = 0;
	quoteChar_ = 0;
	nameValueSeparator_ = 0;
	updateCount_ = 0;
}

//-----------------------------------------------------------------------------

void Strings::error(const char* msg, int data) const
{
	ThrowException(formatString(msg, data).c_str());
}

//-----------------------------------------------------------------------------

std::string Strings::extractName(const char* str) const
{
	std::string result(str);

	std::string::size_type i = result.find(getNameValueSeparator());
	if (i != std::string::npos)
		result = result.substr(0, i);
	else
		result.clear();

	return result;
}

//-----------------------------------------------------------------------------

void Strings::assign(const Strings& src)
{
	AutoUpdater autoUpdater(*this);
	clear();

	defined_ = src.defined_;
	delimiter_ = src.delimiter_;
	lineBreak_ = src.lineBreak_;
	quoteChar_ = src.quoteChar_;
	nameValueSeparator_ = src.nameValueSeparator_;

	addStrings(src);
}

///////////////////////////////////////////////////////////////////////////////
// class StrList

//-----------------------------------------------------------------------------

StrList::StrList()
{
	init();
}

//-----------------------------------------------------------------------------

StrList::StrList(const StrList& src)
{
	init();
	assign(src);
}

//-----------------------------------------------------------------------------

StrList::~StrList()
{
	internalClear();
}

//-----------------------------------------------------------------------------

void StrList::assign(const StrList& src)
{
	Strings::operator=(src);
}

//-----------------------------------------------------------------------------

int StrList::add(const char* str)
{
	return add(str, NULL);
}

//-----------------------------------------------------------------------------

int StrList::add(const char* str, POINTER data)
{
	int result;

	if (!isSorted_)
		result = count_;
	else
	{
		if (find(str, result))
			switch (dupMode_)
			{
			case DM_IGNORE:
				return result;
			case DM_ERROR:
				error(SEM_DUPLICATE_STRING, 0);
			default:
				break;
			}
	}

	insertItem(result, str, data);
	return result;
}

//-----------------------------------------------------------------------------

void StrList::clear()
{
	internalClear();
}

//-----------------------------------------------------------------------------

void StrList::del(int index)
{
	if (index < 0 || index >= count_)
		error(SEM_LIST_INDEX_ERROR, index);

	onChanging();

	delete list_[index].str;
	list_[index].str = NULL;

	count_--;
	if (index < count_)
	{
		memmove(list_ + index, list_ + index + 1, (count_ - index) * sizeof(StringItem));
		list_[count_].str = NULL;
	}

	onChanged();
}

//-----------------------------------------------------------------------------

void StrList::exchange(int index1, int index2)
{
	if (index1 < 0 || index1 >= count_)
		error(SEM_LIST_INDEX_ERROR, index1);
	if (index2 < 0 || index2 >= count_)
		error(SEM_LIST_INDEX_ERROR, index2);

	onChanging();
	exchangeItems(index1, index2);
	onChanged();
}

//-----------------------------------------------------------------------------

int StrList::indexOf(const char* str) const
{
	int result;

	if (!isSorted_)
		result = Strings::indexOf(str);
	else if (!find(str, result))
		result = -1;

	return result;
}

//-----------------------------------------------------------------------------

void StrList::insert(int index, const char* str)
{
	insert(index, str, NULL);
}

//-----------------------------------------------------------------------------

void StrList::insert(int index, const char* str, POINTER data)
{
	if (isSorted_)
		error(SEM_SORTED_LIST_ERROR, 0);
	if (index < 0 || index > count_)
		error(SEM_LIST_INDEX_ERROR, index);

	insertItem(index, str, data);
}

//-----------------------------------------------------------------------------

void StrList::setCapacity(int value)
{
	if (value < 0) value = 0;

	for (int i = value; i < capacity_; i++)
		delete list_[i].str;

	if (value > 0)
	{
		StringItem *p = (StringItem*)realloc(list_, value * sizeof(StringItem));
		if (p)
			list_ = p;
		else
			ThrowMemoryException();
	}
	else
	{
		if (list_)
		{
			free(list_);
			list_ = NULL;
		}
	}

	if (list_ != NULL)
	{
		for (int i = capacity_; i < value; i++)
		{
			list_[i].str = NULL;   // new std::string object when needed
			list_[i].data = NULL;
		}
	}

	capacity_ = value;
	if (count_ > capacity_)
		count_ = capacity_;
}

//-----------------------------------------------------------------------------

POINTER StrList::getData(int index) const
{
	if (index < 0 || index >= count_)
		error(SEM_LIST_INDEX_ERROR, index);

	return list_[index].data;
}

//-----------------------------------------------------------------------------

void StrList::setData(int index, POINTER data)
{
	if (index < 0 || index >= count_)
		error(SEM_LIST_INDEX_ERROR, index);

	onChanging();
	list_[index].data = data;
	onChanged();
}

//-----------------------------------------------------------------------------

const std::string& StrList::getString(int index) const
{
	if (index < 0 || index >= count_)
		error(SEM_LIST_INDEX_ERROR, index);

	return stringObjectNeeded(index);
}

//-----------------------------------------------------------------------------

void StrList::setString(int index, const char* value)
{
	if (isSorted_)
		error(SEM_SORTED_LIST_ERROR, 0);
	if (index < 0 || index >= count_)
		error(SEM_LIST_INDEX_ERROR, index);

	onChanging();
	stringObjectNeeded(index) = value;
	onChanged();
}

//-----------------------------------------------------------------------------

bool StrList::find(const char* str, int& index) const
{
	if (!isSorted_)
	{
		index = indexOf(str);
		return (index >= 0);
	}

	bool result = false;
	int l, h, i, c;

	l = 0;
	h = count_ - 1;
	while (l <= h)
	{
		i = (l + h) >> 1;
		c = compareStrings(stringObjectNeeded(i).c_str(), str);
		if (c < 0)
			l = i + 1;
		else
		{
			h = i - 1;
			if (c == 0)
			{
				result = true;
				if (dupMode_ != DM_ACCEPT)
					l = i;
			}
		}
	}

	index = l;
	return result;
}

//-----------------------------------------------------------------------------

int stringListCompareProc(const StrList& stringList, int index1, int index2)
{
	return stringList.compareStrings(
		stringList.stringObjectNeeded(index1).c_str(),
		stringList.stringObjectNeeded(index2).c_str());
}

//-----------------------------------------------------------------------------

void StrList::sort()
{
	sort(stringListCompareProc);
}

//-----------------------------------------------------------------------------

void StrList::sort(StringListCompareProc compareProc)
{
	if (!isSorted_ && count_ > 1)
	{
		onChanging();
		quickSort(0, count_ - 1, compareProc);
		onChanged();
	}
}

//-----------------------------------------------------------------------------

void StrList::setSorted(bool value)
{
	if (value != isSorted_)
	{
		if (value) sort();
		isSorted_ = value;
	}
}

//-----------------------------------------------------------------------------

void StrList::setCaseSensitive(bool value)
{
	if (value != isCaseSensitive_)
	{
		isCaseSensitive_ = value;
		if (isSorted_) sort();
	}
}

//-----------------------------------------------------------------------------

StrList& StrList::operator = (const StrList& rhs)
{
	if (this != &rhs)
		assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------

void StrList::setUpdateState(bool isUpdating)
{
	if (isUpdating)
		onChanging();
	else
		onChanged();
}

//-----------------------------------------------------------------------------

int StrList::compareStrings(const char* str1, const char* str2) const
{
	if (isCaseSensitive_)
		return strcmp(str1, str2);
	else
		return compareText(str1, str2);
}

//-----------------------------------------------------------------------------

void StrList::insertItem(int index, const char* str, POINTER data)
{
	onChanging();
	if (count_ == capacity_) grow();
	if (index < count_)
	{
		memmove(list_ + index + 1, list_ + index, (count_ - index) * sizeof(StringItem));
		list_[index].str = NULL;
	}

	stringObjectNeeded(index) = str;
	list_[index].data = data;

	count_++;
	onChanged();
}

//-----------------------------------------------------------------------------

void StrList::init()
{
	list_ = NULL;
	count_ = 0;
	capacity_ = 0;
	dupMode_ = DM_IGNORE;
	isSorted_ = false;
	isCaseSensitive_ = false;
}

//-----------------------------------------------------------------------------

void StrList::internalClear()
{
	setCapacity(0);
}

//-----------------------------------------------------------------------------

std::string& StrList::stringObjectNeeded(int index) const
{
	if (list_[index].str == NULL)
		list_[index].str = new std::string();
	return *(list_[index].str);
}

//-----------------------------------------------------------------------------

void StrList::exchangeItems(int index1, int index2)
{
	StringItem temp;

	temp = list_[index1];
	list_[index1] = list_[index2];
	list_[index2] = temp;
}

//-----------------------------------------------------------------------------

void StrList::grow()
{
	int delta;

	if (capacity_ > 64)
		delta = capacity_ / 4;
	else if (capacity_ > 8)
		delta = 16;
	else
		delta = 4;

	setCapacity(capacity_ + delta);
}

//-----------------------------------------------------------------------------

void StrList::quickSort(int l, int r, StringListCompareProc compareProc)
{
	int i, j, p;

	do
	{
		i = l;
		j = r;
		p = (l + r) >> 1;
		do
		{
			while (compareProc(*this, i, p) < 0) i++;
			while (compareProc(*this, j, p) > 0) j--;
			if (i <= j)
			{
				exchangeItems(i, j);
				if (p == i)
					p = j;
				else if (p == j)
					p = i;
				i++;
				j--;
			}
		} while (i <= j);

		if (l < j)
			quickSort(l, j, compareProc);
		l = i;
	} while (i < r);
}
