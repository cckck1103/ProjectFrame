

#ifndef _OBJ_ARRAY_H_
#define _OBJ_ARRAY_H_

#include "GlobalDefs.h"
#include "BaseMutex.h"

/************************************************************************/
/*
PointerList - 列表类
说明:
1. 此类的实现原理与 Delphi::TList 完全相同；
2. 此列表类有如下优点:
a. 公有方法简单明确 (STL虽无比强大但稍显晦涩)；
b. 支持下标随机存取各个元素 (STL::list不支持)；
c. 支持快速获取列表长度 (STL::list不支持)；
d. 支持尾部快速增删元素；
3. 此列表类有如下缺点:
a. 不支持头部和中部的快速增删元素；
b. 只支持单一类型元素(Pointer类型)；
/************************************************************************/
class PointerList
{
public:
	PointerList();
	virtual ~PointerList();

	/*
	* 函数名： add
	* 功能：   向列表中添加元素
	* 参数：   POINTER
	* 返回值： list 剩余个数
	*/
	int				add(POINTER item);

	/*
	* 函数名： insert
	* 功能：   向列表中插入元素
	* 参数：   index - 插入位置下标号(0-based)
	* 返回值： void
	*/
	void insert(int index, POINTER item);


	/*
	* 函数名： del
	* 功能：   从列表中删除元素
	* 参数：   index - 下标号(0-based)
	* 返回值： void
	*/
	void del(int index);

	/*
	* 函数名： remove
	* 功能：   从列表中删除元素
	* 参数：   POINTER
	* 返回值： 被删除元素在列表中的下标号(0-based)，若未找到，则返回 -1.
	*/
	int remove(POINTER item);

	/*
	* 函数名： extract
	* 功能：   从列表中删除元素
	* 参数：   POINTER
	* 返回值： 被删除的元素值，若未找到，则返回 NULL.
	*/
	POINTER extract(POINTER item);

	/*
	* 函数名： move
	* 功能：   移动一个元素到新的位置
	* 参数：   curIndex   
	           newIndex
	* 返回值： void
	*/
	void move(int curIndex, int newIndex);

	/*
	* 函数名： resize
	* 功能：   改变列表的大小
	* 参数：   count
	* 返回值： void
	*/
	void resize(int count);

	/*
	* 函数名： clear
	* 功能：   清空列表
	* 参数：   
	* 返回值： void
	*/
	void clear();

	/*
	* 函数名： first
	* 功能：   返回列表中的首个元素 (若列表为空则抛出异常)
	* 参数：
	* 返回值： POINTER
	*/
	POINTER first() const;

	/*
	* 函数名： first
	* 功能：   返回列表中的首个元素 (若列表为空则抛出异常)
	* 参数：
	* 返回值： POINTER
	*/
	POINTER last() const;

	/*
	* 函数名： indexOf
	* 功能：   返回元素在列表中的下标号 (若未找到则返回 -1)
	* 参数：   POINTER
	* 返回值： int
	*/
	int indexOf(POINTER item) const;

	/*
	* 函数名： getCount
	* 功能：   返回列表中元素总数
	* 参数：   
	* 返回值： int
	*/
	int getCount() const;


	bool isEmpty() const { return (getCount() <= 0); }
	PointerList& operator = (const PointerList& rhs);
	const POINTER& operator[] (int index) const;
	POINTER& operator[] (int index);

protected:
	virtual void grow();
	POINTER get(int index) const;
	void put(int index, POINTER item);
	void setCapacity(int newCapacity);
	void setCount(int newCount);

private:
	POINTER *list_;
	int count_;
	int capacity_;
};


///////////////////////////////////////////////////////////////////////////////
// class CustomObjectList - 对象列表基类

template<typename ObjectType>
class CustomObjectList
{
public:
	class InternalMutex : public BaseMutex
	{
	public:
		InternalMutex(bool active) : mutex_(NULL) { if (active) mutex_ = new Mutex(); }
		virtual ~InternalMutex() { delete mutex_; }
	public:
		virtual void lock() { if (mutex_) mutex_->lock(); }
		virtual void unlock() { if (mutex_) mutex_->unlock(); }
	private:
		Mutex *mutex_;
	};

	typedef ObjectType* ObjectPtr;

public:
	CustomObjectList() :
		mutex_(false), isOwnsObjects_(true) {}

	CustomObjectList(bool isThreadSafe, bool isOwnsObjects) :
		mutex_(isThreadSafe), isOwnsObjects_(isOwnsObjects) {}

	virtual ~CustomObjectList() { clear(); }

protected:
	virtual void notifyDelete(int index)
	{
		if (isOwnsObjects_)
		{
			ObjectPtr item = static_cast<ObjectPtr>(items_[index]);
			items_[index] = NULL;
			delete item;
		}
	}

protected:
	int add(ObjectPtr item, bool allowDuplicate = true)
	{
		ASSERT_X(item);
		AutoLocker locker(mutex_);

		if (allowDuplicate || items_.indexOf(item) == -1)
			return items_.add(item);
		else
			return -1;
	}

	int remove(ObjectPtr item)
	{
		AutoLocker locker(mutex_);

		int result = items_.indexOf(item);
		if (result >= 0)
		{
			notifyDelete(result);
			items_.del(result);
		}
		return result;
	}

	ObjectPtr extract(ObjectPtr item)
	{
		AutoLocker locker(mutex_);

		ObjectPtr result = NULL;
		int i = items_.remove(item);
		if (i >= 0)
			result = item;
		return result;
	}

	ObjectPtr extract(int index)
	{
		AutoLocker locker(mutex_);

		ObjectPtr result = NULL;
		if (index >= 0 && index < items_.getCount())
		{
			result = static_cast<ObjectPtr>(items_[index]);
			items_.del(index);
		}
		return result;
	}

	void del(int index)
	{
		AutoLocker locker(mutex_);

		if (index >= 0 && index < items_.getCount())
		{
			notifyDelete(index);
			items_.del(index);
		}
	}

	void insert(int index, ObjectPtr item)
	{
		ASSERT_X(item);
		AutoLocker locker(mutex_);
		items_.insert(index, item);
	}

	int indexOf(ObjectPtr item) const
	{
		AutoLocker locker(mutex_);
		return items_.indexOf(item);
	}

	bool exists(ObjectPtr item) const
	{
		AutoLocker locker(mutex_);
		return items_.indexOf(item) >= 0;
	}

	ObjectPtr first() const
	{
		AutoLocker locker(mutex_);
		return static_cast<ObjectPtr>(items_.first());
	}

	ObjectPtr last() const
	{
		AutoLocker locker(mutex_);
		return static_cast<ObjectPtr>(items_.last());
	}

	void clear()
	{
		AutoLocker locker(mutex_);

		for (int i = items_.getCount() - 1; i >= 0; i--)
			notifyDelete(i);
		items_.clear();
	}

	void freeObjects()
	{
		AutoLocker locker(mutex_);

		for (int i = items_.getCount() - 1; i >= 0; i--)
		{
			ObjectPtr item = static_cast<ObjectPtr>(items_[i]);
			items_[i] = NULL;
			delete item;
		}
	}

	int getCount() const { return items_.getCount(); }
	ObjectPtr& getItem(int index) const { return (ObjectPtr&)items_[index]; }
	CustomObjectList& operator = (const CustomObjectList& rhs) { items_ = rhs.items_; return *this; }
	ObjectPtr& operator[] (int index) const { return getItem(index); }
	bool isEmpty() const { return (getCount() <= 0); }
	void setOwnsObjects(bool value) { isOwnsObjects_ = value; }

protected:
	PointerList items_;       // 对象列表
	mutable InternalMutex mutex_;
	bool isOwnsObjects_;      // 元素被删除时，是否自动释放元素对象
};


///////////////////////////////////////////////////////////////////////////////
// class ObjectList - 对象列表类

template<typename ObjectType>
class ObjectList : public CustomObjectList<ObjectType>
{
public:
	ObjectList() :
		CustomObjectList<ObjectType>(false, true) {}
	ObjectList(bool isThreadSafe, bool isOwnsObjects) :
		CustomObjectList<ObjectType>(isThreadSafe, isOwnsObjects) {}
	virtual ~ObjectList() {}

	using CustomObjectList<ObjectType>::add;
	using CustomObjectList<ObjectType>::remove;
	using CustomObjectList<ObjectType>::extract;
	using CustomObjectList<ObjectType>::del;
	using CustomObjectList<ObjectType>::insert;
	using CustomObjectList<ObjectType>::indexOf;
	using CustomObjectList<ObjectType>::exists;
	using CustomObjectList<ObjectType>::first;
	using CustomObjectList<ObjectType>::last;
	using CustomObjectList<ObjectType>::clear;
	using CustomObjectList<ObjectType>::freeObjects;
	using CustomObjectList<ObjectType>::getCount;
	using CustomObjectList<ObjectType>::getItem;
	using CustomObjectList<ObjectType>::operator=;
	using CustomObjectList<ObjectType>::operator[];
	using CustomObjectList<ObjectType>::isEmpty;
	using CustomObjectList<ObjectType>::setOwnsObjects;
};


///////////////////////////////////////////////////////////////////////////////
// class PropertyList - 属性列表类
//
// 说明:
// 1. 属性列表中的每个项目由属性名(Name)和属性值(Value)组成。
// 2. 属性名不可重复，不区分大小写，且其中不可含有等号"="。属性值可为任意值。

class PropertyList
{
public:
	enum { NAME_VALUE_SEP = '=' };        // Name 和 Value 之间的分隔符
	enum { PROP_ITEM_SEP = ',' };        // 属性项目之间的分隔符
	enum { PROP_ITEM_QUOT = '"' };

	struct PropertyItem
	{
		std::string name, value;
	public:
		PropertyItem(const PropertyItem& src) :
			name(src.name), value(src.value) {}
		PropertyItem(const std::string& _name, const std::string& _value) :
			name(_name), value(_value) {}
	};

public:
	PropertyList();
	PropertyList(const PropertyList& src);
	virtual ~PropertyList();

	void add(const std::string& name, const std::string& value);
	bool remove(const std::string& name);
	void clear();
	int indexOf(const std::string& name) const;
	bool nameExists(const std::string& name) const;
	bool getValue(const std::string& name, std::string& value) const;
	int getCount() const { return items_.getCount(); }
	bool isEmpty() const { return (getCount() <= 0); }
	const PropertyItem& getItem(int index) const;
	std::string getPropString() const;
	void setPropString(const std::string& propString);

	PropertyList& operator = (const PropertyList& rhs);
	std::string& operator[] (const std::string& name);

private:
	void assign(const PropertyList& src);
	PropertyItem* find(const std::string& name);
	static bool isReservedChar(char ch);
	static bool hasReservedChar(const std::string& str);
	static char* scanStr(char *str, char ch);
	static std::string makeQuotedStr(const std::string& str);
	static std::string extractQuotedStr(char*& strPtr);

private:
	PointerList items_;                        // (PropertyItem* [])
};

#endif