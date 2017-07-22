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
// 描述: 向列表中添加元素
//-----------------------------------------------------------------------------
int PointerList::add(POINTER item)
{
	if (count_ == capacity_) grow();
	list_[count_] = item;
	count_++;

	return count_ - 1;
}

//-----------------------------------------------------------------------------
// 描述: 向列表中插入元素
// 参数:
//   index - 插入位置下标号(0-based)
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
// 描述: 从列表中删除元素
// 参数:
//   index - 下标号(0-based)
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
// 描述: 从列表中删除元素
// 返回: 被删除元素在列表中的下标号(0-based)，若未找到，则返回 -1.
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
// 描述: 从列表中删除元素
// 返回: 被删除的元素值，若未找到，则返回 NULL.
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
// 描述: 移动一个元素到新的位置
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
// 描述: 改变列表的大小
//-----------------------------------------------------------------------------
void PointerList::resize(int count)
{
	setCount(count);
}

//-----------------------------------------------------------------------------
// 描述: 清空列表
//-----------------------------------------------------------------------------
void PointerList::clear()
{
	setCount(0);
	setCapacity(0);
}

//-----------------------------------------------------------------------------
// 描述: 返回列表中的首个元素 (若列表为空则抛出异常)
//-----------------------------------------------------------------------------
POINTER PointerList::first() const
{
	return get(0);
}

//-----------------------------------------------------------------------------
// 描述: 返回列表中的最后元素 (若列表为空则抛出异常)
//-----------------------------------------------------------------------------
POINTER PointerList::last() const
{
	return get(count_ - 1);
}

//-----------------------------------------------------------------------------
// 描述: 返回元素在列表中的下标号 (若未找到则返回 -1)
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
// 描述: 返回列表中元素总数
//-----------------------------------------------------------------------------
int PointerList::getCount() const
{
	return count_;
}

//-----------------------------------------------------------------------------
// 描述: 赋值操作符
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
// 描述: 存取列表中任意元素
//-----------------------------------------------------------------------------
const POINTER& PointerList::operator[] (int index) const
{
	if (index < 0 || index >= count_)
		ThrowException(SEM_INDEX_ERROR);

	return list_[index];
}

//-----------------------------------------------------------------------------
// 描述: 存取列表中任意元素
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
// 描述: 向列表中添加元素
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
// 描述: 从列表中删除元素
// 返回:
//   true  - 成功
//   false - 失败(未找到)
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
// 描述: 清空列表
//-----------------------------------------------------------------------------
void PropertyList::clear()
{
	for (int i = 0; i < items_.getCount(); i++)
		delete (PropertyItem*)items_[i];
	items_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 返回某项属性在列表中的位置(0-based)
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
// 描述: 判断某项属性是否存在
//-----------------------------------------------------------------------------
bool PropertyList::nameExists(const std::string& name) const
{
	return (indexOf(name) >= 0);
}

//-----------------------------------------------------------------------------
// 描述: 根据 Name 从列表中取出 Value
// 返回: 若列表中不存在该项属性，则返回 False。
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
// 描述: 根据下标号(0-based)取得属性项目
//-----------------------------------------------------------------------------
const PropertyList::PropertyItem& PropertyList::getItem(int index) const
{
	if (index < 0 || index >= getCount())
		ThrowException(SEM_INDEX_ERROR);

	return *((PropertyItem*)items_[index]);
}

//-----------------------------------------------------------------------------
// 描述: 将整个列表转换成 PropString 并返回
// 示例:
//   [<abc,123>, <def,456>]   ->  abc=123,def=456
//   [<abc,123>, <,456>]      ->  abc=123,=456
//   [<abc,123>, <",456>]     ->  abc=123,"""=456"
//   [<abc,123>, <',456>]     ->  abc=123,'=456
//   [<abc,123>, <def,">]     ->  abc=123,"def="""
//   [<abc,123>, < def,456>]  ->  abc=123," def=456"
//   [<abc,123>, <def,=>]     ->  abc=123,def==
//   [<abc,123>, <=,456>]     ->  抛出异常(Name中不允许存在等号"=")
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
// 描述: 将 PropString 转换成属性列表
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
// 描述: 赋值操作符
//-----------------------------------------------------------------------------
PropertyList& PropertyList::operator = (const PropertyList& rhs)
{
	if (this != &rhs)
		assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------
// 描述: 存取列表中任意元素
// 注意: 调用此函数时，若 name 不存在，则立即添加到列表中！
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
// 描述: 查找某个属性项目，没找到则返回 NULL
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
// 描述: 将字符串 str 用引号(")括起来
// 示例:
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
// 描述: 将 MakeQuotedStr 生成的字符串还原
// 参数:
//   strPtr - 待处理的字符串指针，处理完后传回处理截止位置
// 返回:
//   还原后的字符串
// 示例:
//   "abcde"xyz   ->  abcde       函数返回时 strPtr 指向 x
//   "ab""cd"     ->  ab"cd       函数返回时 strPtr 指向 '\0'
//   "ab""""cd"   ->  ab""cd      函数返回时 strPtr 指向 '\0'
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