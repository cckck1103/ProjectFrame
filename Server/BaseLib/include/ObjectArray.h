

#ifndef _OBJ_ARRAY_H_
#define _OBJ_ARRAY_H_

#include "GlobalDefs.h"
#include "BaseMutex.h"

/************************************************************************/
/*
PointerList - �б���
˵��:
1. �����ʵ��ԭ���� Delphi::TList ��ȫ��ͬ��
2. ���б����������ŵ�:
a. ���з�������ȷ (STL���ޱ�ǿ�����Ի�ɬ)��
b. ֧���±������ȡ����Ԫ�� (STL::list��֧��)��
c. ֧�ֿ��ٻ�ȡ�б��� (STL::list��֧��)��
d. ֧��β��������ɾԪ�أ�
3. ���б���������ȱ��:
a. ��֧��ͷ�����в��Ŀ�����ɾԪ�أ�
b. ֻ֧�ֵ�һ����Ԫ��(Pointer����)��
/************************************************************************/
class PointerList
{
public:
	PointerList();
	virtual ~PointerList();

	/*
	* �������� add
	* ���ܣ�   ���б������Ԫ��
	* ������   POINTER
	* ����ֵ�� list ʣ�����
	*/
	int				add(POINTER item);

	/*
	* �������� insert
	* ���ܣ�   ���б��в���Ԫ��
	* ������   index - ����λ���±��(0-based)
	* ����ֵ�� void
	*/
	void insert(int index, POINTER item);


	/*
	* �������� del
	* ���ܣ�   ���б���ɾ��Ԫ��
	* ������   index - �±��(0-based)
	* ����ֵ�� void
	*/
	void del(int index);

	/*
	* �������� remove
	* ���ܣ�   ���б���ɾ��Ԫ��
	* ������   POINTER
	* ����ֵ�� ��ɾ��Ԫ�����б��е��±��(0-based)����δ�ҵ����򷵻� -1.
	*/
	int remove(POINTER item);

	/*
	* �������� extract
	* ���ܣ�   ���б���ɾ��Ԫ��
	* ������   POINTER
	* ����ֵ�� ��ɾ����Ԫ��ֵ����δ�ҵ����򷵻� NULL.
	*/
	POINTER extract(POINTER item);

	/*
	* �������� move
	* ���ܣ�   �ƶ�һ��Ԫ�ص��µ�λ��
	* ������   curIndex   
	           newIndex
	* ����ֵ�� void
	*/
	void move(int curIndex, int newIndex);

	/*
	* �������� resize
	* ���ܣ�   �ı��б�Ĵ�С
	* ������   count
	* ����ֵ�� void
	*/
	void resize(int count);

	/*
	* �������� clear
	* ���ܣ�   ����б�
	* ������   
	* ����ֵ�� void
	*/
	void clear();

	/*
	* �������� first
	* ���ܣ�   �����б��е��׸�Ԫ�� (���б�Ϊ�����׳��쳣)
	* ������
	* ����ֵ�� POINTER
	*/
	POINTER first() const;

	/*
	* �������� first
	* ���ܣ�   �����б��е��׸�Ԫ�� (���б�Ϊ�����׳��쳣)
	* ������
	* ����ֵ�� POINTER
	*/
	POINTER last() const;

	/*
	* �������� indexOf
	* ���ܣ�   ����Ԫ�����б��е��±�� (��δ�ҵ��򷵻� -1)
	* ������   POINTER
	* ����ֵ�� int
	*/
	int indexOf(POINTER item) const;

	/*
	* �������� getCount
	* ���ܣ�   �����б���Ԫ������
	* ������   
	* ����ֵ�� int
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
// class CustomObjectList - �����б����

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
	PointerList items_;       // �����б�
	mutable InternalMutex mutex_;
	bool isOwnsObjects_;      // Ԫ�ر�ɾ��ʱ���Ƿ��Զ��ͷ�Ԫ�ض���
};


///////////////////////////////////////////////////////////////////////////////
// class ObjectList - �����б���

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
// class PropertyList - �����б���
//
// ˵��:
// 1. �����б��е�ÿ����Ŀ��������(Name)������ֵ(Value)��ɡ�
// 2. �����������ظ��������ִ�Сд�������в��ɺ��еȺ�"="������ֵ��Ϊ����ֵ��

class PropertyList
{
public:
	enum { NAME_VALUE_SEP = '=' };        // Name �� Value ֮��ķָ���
	enum { PROP_ITEM_SEP = ',' };        // ������Ŀ֮��ķָ���
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