#include "ObjectArray.h"
#include "Exceptions.h"
#include "SysUtils.h"


///////////////////////////////////////////////////////////////////////////////
// class PointerList

PointerList::PointerList() :
	list_(NULL),
	count_(0),
	capacity_(0)
{
	// nothing
}

//-----------------------------------------------------------------------------

PointerList::~PointerList()
{
	clear();
}

//-----------------------------------------------------------------------------
// ����: ���б������Ԫ��
//-----------------------------------------------------------------------------
int PointerList::add(POINTER item)
{
	if (count_ == capacity_) grow();
	list_[count_] = item;
	count_++;

	return count_ - 1;
}

//-----------------------------------------------------------------------------
// ����: ���б��в���Ԫ��
// ����:
//   index - ����λ���±��(0-based)
//-----------------------------------------------------------------------------
void PointerList::insert(int index, POINTER item)
{
	if (index < 0 || index > count_)
		ThrowException(SEM_INDEX_ERROR);

	if (count_ == capacity_) grow();
	if (index < count_)
		memmove(&list_[index + 1], &list_[index], (count_ - index) * sizeof(POINTER));
	list_[index] = item;
	count_++;
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����:
//   index - �±��(0-based)
//-----------------------------------------------------------------------------
void PointerList::del(int index)
{
	if (index < 0 || index >= count_)
		ThrowException(SEM_INDEX_ERROR);

	count_--;
	if (index < count_)
		memmove(&list_[index], &list_[index + 1], (count_ - index) * sizeof(POINTER));
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����: ��ɾ��Ԫ�����б��е��±��(0-based)����δ�ҵ����򷵻� -1.
//-----------------------------------------------------------------------------
int PointerList::remove(POINTER item)
{
	int result;

	result = indexOf(item);
	if (result >= 0)
		del(result);

	return result;
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����: ��ɾ����Ԫ��ֵ����δ�ҵ����򷵻� NULL.
//-----------------------------------------------------------------------------
POINTER PointerList::extract(POINTER item)
{
	int i;
	POINTER result = NULL;

	i = indexOf(item);
	if (i >= 0)
	{
		result = item;
		list_[i] = NULL;
		del(i);
	}

	return result;
}

//-----------------------------------------------------------------------------
// ����: �ƶ�һ��Ԫ�ص��µ�λ��
//-----------------------------------------------------------------------------
void PointerList::move(int curIndex, int newIndex)
{
	POINTER item;

	if (curIndex != newIndex)
	{
		if (newIndex < 0 || newIndex >= count_)
			ThrowException(SEM_INDEX_ERROR);

		item = get(curIndex);
		list_[curIndex] = NULL;
		del(curIndex);
		insert(newIndex, NULL);
		list_[newIndex] = item;
	}
}

//-----------------------------------------------------------------------------
// ����: �ı��б�Ĵ�С
//-----------------------------------------------------------------------------
void PointerList::resize(int count)
{
	setCount(count);
}

//-----------------------------------------------------------------------------
// ����: ����б�
//-----------------------------------------------------------------------------
void PointerList::clear()
{
	setCount(0);
	setCapacity(0);
}

//-----------------------------------------------------------------------------
// ����: �����б��е��׸�Ԫ�� (���б�Ϊ�����׳��쳣)
//-----------------------------------------------------------------------------
POINTER PointerList::first() const
{
	return get(0);
}

//-----------------------------------------------------------------------------
// ����: �����б��е����Ԫ�� (���б�Ϊ�����׳��쳣)
//-----------------------------------------------------------------------------
POINTER PointerList::last() const
{
	return get(count_ - 1);
}

//-----------------------------------------------------------------------------
// ����: ����Ԫ�����б��е��±�� (��δ�ҵ��򷵻� -1)
//-----------------------------------------------------------------------------
int PointerList::indexOf(POINTER item) const
{
	int result = 0;

	while (result < count_ && list_[result] != item) result++;
	if (result == count_)
		result = -1;

	return result;
}

//-----------------------------------------------------------------------------
// ����: �����б���Ԫ������
//-----------------------------------------------------------------------------
int PointerList::getCount() const
{
	return count_;
}

//-----------------------------------------------------------------------------
// ����: ��ֵ������
//-----------------------------------------------------------------------------
PointerList& PointerList::operator = (const PointerList& rhs)
{
	if (this == &rhs) return *this;

	clear();
	setCapacity(rhs.capacity_);
	for (int i = 0; i < rhs.getCount(); i++)
		add(rhs[i]);
	return *this;
}

//-----------------------------------------------------------------------------
// ����: ��ȡ�б�������Ԫ��
//-----------------------------------------------------------------------------
const POINTER& PointerList::operator[] (int index) const
{
	if (index < 0 || index >= count_)
		ThrowException(SEM_INDEX_ERROR);

	return list_[index];
}

//-----------------------------------------------------------------------------
// ����: ��ȡ�б�������Ԫ��
//-----------------------------------------------------------------------------
POINTER& PointerList::operator[] (int index)
{
	return
		const_cast<POINTER&>(
		((const PointerList&)(*this))[index]
			);
}

//-----------------------------------------------------------------------------

void PointerList::grow()
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

POINTER PointerList::get(int index) const
{
	if (index < 0 || index >= count_)
		ThrowException(SEM_INDEX_ERROR);

	return list_[index];
}

//-----------------------------------------------------------------------------

void PointerList::put(int index, POINTER item)
{
	if (index < 0 || index >= count_)
		ThrowException(SEM_INDEX_ERROR);

	list_[index] = item;
}

//-----------------------------------------------------------------------------

void PointerList::setCapacity(int newCapacity)
{
	if (newCapacity < count_)
		ThrowException(SEM_LIST_CAPACITY_ERROR);

	if (newCapacity != capacity_)
	{
		if (newCapacity > 0)
		{
			POINTER *p = (POINTER*)realloc(list_, newCapacity * sizeof(PVOID));
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

		capacity_ = newCapacity;
	}
}

//-----------------------------------------------------------------------------

void PointerList::setCount(int newCount)
{
	if (newCount < 0)
		ThrowException(SEM_LIST_COUNT_ERROR);

	if (newCount > capacity_)
		setCapacity(newCount);
	if (newCount > count_)
		memset(&list_[count_], 0, (newCount - count_) * sizeof(POINTER));
	else
		for (int i = count_ - 1; i >= newCount; i--) del(i);

	count_ = newCount;
}


///////////////////////////////////////////////////////////////////////////////
// class PropertyList

PropertyList::PropertyList()
{
	// nothing
}

PropertyList::PropertyList(const PropertyList& src)
{
	assign(src);
}

PropertyList::~PropertyList()
{
	clear();
}

//-----------------------------------------------------------------------------
// ����: ���б������Ԫ��
//-----------------------------------------------------------------------------
void PropertyList::add(const std::string& name, const std::string& value)
{
	if (name.find(NAME_VALUE_SEP, 0) != std::string::npos)
		ThrowException(SEM_PROPLIST_NAME_ERROR);

	PropertyItem *item = find(name);
	if (!item)
	{
		item = new PropertyItem(name, value);
		items_.add(item);
	}
	else
		item->value = value;
}

//-----------------------------------------------------------------------------
// ����: ���б���ɾ��Ԫ��
// ����:
//   true  - �ɹ�
//   false - ʧ��(δ�ҵ�)
//-----------------------------------------------------------------------------
bool PropertyList::remove(const std::string& name)
{
	int i = indexOf(name);
	bool result = (i >= 0);

	if (result)
	{
		delete (PropertyItem*)items_[i];
		items_.del(i);
	}

	return result;
}

//-----------------------------------------------------------------------------
// ����: ����б�
//-----------------------------------------------------------------------------
void PropertyList::clear()
{
	for (int i = 0; i < items_.getCount(); i++)
		delete (PropertyItem*)items_[i];
	items_.clear();
}

//-----------------------------------------------------------------------------
// ����: ����ĳ���������б��е�λ��(0-based)
//-----------------------------------------------------------------------------
int PropertyList::indexOf(const std::string& name) const
{
	int result = -1;

	for (int i = 0; i < items_.getCount(); i++)
		if (sameText(name, ((PropertyItem*)items_[i])->name))
		{
			result = i;
			break;
		}

	return result;
}

//-----------------------------------------------------------------------------
// ����: �ж�ĳ�������Ƿ����
//-----------------------------------------------------------------------------
bool PropertyList::nameExists(const std::string& name) const
{
	return (indexOf(name) >= 0);
}

//-----------------------------------------------------------------------------
// ����: ���� Name ���б���ȡ�� Value
// ����: ���б��в����ڸ������ԣ��򷵻� False��
//-----------------------------------------------------------------------------
bool PropertyList::getValue(const std::string& name, std::string& value) const
{
	int i = indexOf(name);
	bool result = (i >= 0);

	if (result)
		value = ((PropertyItem*)items_[i])->value;

	return result;
}

//-----------------------------------------------------------------------------
// ����: �����±��(0-based)ȡ��������Ŀ
//-----------------------------------------------------------------------------
const PropertyList::PropertyItem& PropertyList::getItem(int index) const
{
	if (index < 0 || index >= getCount())
		ThrowException(SEM_INDEX_ERROR);

	return *((PropertyItem*)items_[index]);
}

//-----------------------------------------------------------------------------
// ����: �������б�ת���� PropString ������
// ʾ��:
//   [<abc,123>, <def,456>]   ->  abc=123,def=456
//   [<abc,123>, <,456>]      ->  abc=123,=456
//   [<abc,123>, <",456>]     ->  abc=123,"""=456"
//   [<abc,123>, <',456>]     ->  abc=123,'=456
//   [<abc,123>, <def,">]     ->  abc=123,"def="""
//   [<abc,123>, < def,456>]  ->  abc=123," def=456"
//   [<abc,123>, <def,=>]     ->  abc=123,def==
//   [<abc,123>, <=,456>]     ->  �׳��쳣(Name�в�������ڵȺ�"=")
//-----------------------------------------------------------------------------
std::string PropertyList::getPropString() const
{
	std::string result;
	std::string itemStr;

	for (int index = 0; index < getCount(); index++)
	{
		const PropertyItem& item = getItem(index);

		itemStr = item.name + (char)NAME_VALUE_SEP + item.value;
		if (hasReservedChar(itemStr))
			itemStr = makeQuotedStr(itemStr);

		result += itemStr;
		if (index < getCount() - 1) result += (char)PROP_ITEM_SEP;
	}

	return result;
}

//-----------------------------------------------------------------------------
// ����: �� PropString ת���������б�
//-----------------------------------------------------------------------------
void PropertyList::setPropString(const std::string& propString)
{
	char *strPtr = const_cast<char*>(propString.c_str());
	std::string itemStr;

	clear();
	while (*strPtr)
	{
		if (*strPtr == PROP_ITEM_QUOT)
			itemStr = extractQuotedStr(strPtr);
		else
		{
			char *p = strPtr;
			strPtr = scanStr(strPtr, PROP_ITEM_SEP);
			itemStr.assign(p, strPtr - p);
			if (*strPtr == PROP_ITEM_SEP) strPtr++;
		}

		std::string::size_type i = itemStr.find(NAME_VALUE_SEP, 0);
		if (i != std::string::npos)
			add(itemStr.substr(0, i), itemStr.substr(i + 1));
	}
}

//-----------------------------------------------------------------------------
// ����: ��ֵ������
//-----------------------------------------------------------------------------
PropertyList& PropertyList::operator = (const PropertyList& rhs)
{
	if (this != &rhs)
		assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------
// ����: ��ȡ�б�������Ԫ��
// ע��: ���ô˺���ʱ���� name �����ڣ���������ӵ��б��У�
//-----------------------------------------------------------------------------
std::string& PropertyList::operator[] (const std::string& name)
{
	int i = indexOf(name);
	if (i < 0)
	{
		add(name, "");
		i = getCount() - 1;
	}

	return ((PropertyItem*)items_[i])->value;
}

//-----------------------------------------------------------------------------

void PropertyList::assign(const PropertyList& src)
{
	clear();
	for (int i = 0; i < src.getCount(); i++)
		add(src.getItem(i).name, src.getItem(i).value);
}

//-----------------------------------------------------------------------------
// ����: ����ĳ��������Ŀ��û�ҵ��򷵻� NULL
//-----------------------------------------------------------------------------
PropertyList::PropertyItem* PropertyList::find(const std::string& name)
{
	int i = indexOf(name);
	PropertyItem *result = (i >= 0 ? (PropertyItem*)items_[i] : NULL);
	return result;
}

//-----------------------------------------------------------------------------

bool PropertyList::isReservedChar(char ch)
{
	return
		((ch >= 0) && (ch <= 32)) ||
		(ch == (char)PROP_ITEM_SEP) ||
		(ch == (char)PROP_ITEM_QUOT);
}

//-----------------------------------------------------------------------------

bool PropertyList::hasReservedChar(const std::string& str)
{
	for (UINT i = 0; i < str.length(); i++)
		if (isReservedChar(str[i])) return true;
	return false;
}

//-----------------------------------------------------------------------------

char* PropertyList::scanStr(char *str, char ch)
{
	char *result = str;
	while ((*result != ch) && (*result != 0)) result++;
	return result;
}

//-----------------------------------------------------------------------------
// ����: ���ַ��� str ������(")������
// ʾ��:
//   abcdefg   ->  "abcdefg"
//   abc"efg   ->  "abc""efg"
//   abc""fg   ->  "abc""""fg"
//-----------------------------------------------------------------------------
std::string PropertyList::makeQuotedStr(const std::string& str)
{
	const char QUOT_CHAR = (char)PROP_ITEM_QUOT;
	std::string result;

	for (UINT i = 0; i < str.length(); i++)
	{
		if (str[i] == QUOT_CHAR)
			result += std::string(2, QUOT_CHAR);
		else
			result += str[i];
	}
	result = "\"" + result + "\"";

	return result;
}

//-----------------------------------------------------------------------------
// ����: �� MakeQuotedStr ���ɵ��ַ�����ԭ
// ����:
//   strPtr - ��������ַ���ָ�룬������󴫻ش����ֹλ��
// ����:
//   ��ԭ����ַ���
// ʾ��:
//   "abcde"xyz   ->  abcde       ��������ʱ strPtr ָ�� x
//   "ab""cd"     ->  ab"cd       ��������ʱ strPtr ָ�� '\0'
//   "ab""""cd"   ->  ab""cd      ��������ʱ strPtr ָ�� '\0'
//-----------------------------------------------------------------------------
std::string PropertyList::extractQuotedStr(char*& strPtr)
{
	const char QUOT_CHAR = (char)PROP_ITEM_QUOT;
	std::string result;
	int dropCount;
	char *p;

	if (strPtr == NULL || *strPtr != QUOT_CHAR) return result;

	strPtr++;
	dropCount = 1;
	p = strPtr;
	strPtr = scanStr(strPtr, QUOT_CHAR);
	while (*strPtr)
	{
		strPtr++;
		if (*strPtr != QUOT_CHAR) break;
		strPtr++;
		dropCount++;
		strPtr = scanStr(strPtr, QUOT_CHAR);
	}

	if (((strPtr - p) <= 1) || ((strPtr - p - dropCount) == 0)) return result;

	if (dropCount == 1)
		result.assign(p, strPtr - p - 1);
	else
	{
		result.resize(strPtr - p - dropCount);
		char *dest = const_cast<char*>(result.c_str());
		while (p < strPtr)
		{
			if ((*p == QUOT_CHAR) && (p < strPtr - 1) && (*(p + 1) == QUOT_CHAR)) p++;
			*dest++ = *p++;
		}
	}

	return result;
}