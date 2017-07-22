#ifndef _STREAM_CLASS_H_
#define _STREAM_CLASS_H_

#include "GlobalDefs.h"
#include "Options.h"
#include "UtilClass.h"
#include "SysUtils.h"
#include "NonCopyable.h"


///////////////////////////////////////////////////////////////////////////////
// class Stream - 流 基类

enum SEEK_ORIGIN
{
	SO_BEGINNING = 0,
	SO_CURRENT = 1,
	SO_END = 2
};

class Stream
{
public:
	virtual ~Stream() {}

	virtual int read(void *buffer, int count) = 0;
	virtual int write(const void *buffer, int count) = 0;
	virtual INT64 seek(INT64 offset, SEEK_ORIGIN seekOrigin) = 0;

	void readBuffer(void *buffer, int count);
	void writeBuffer(const void *buffer, int count);

	INT64 getPosition() { return seek(0, SO_CURRENT); }
	void setPosition(INT64 pos) { seek(pos, SO_BEGINNING); }

	virtual INT64 getSize();
	virtual void setSize(INT64 size) {}
};


///////////////////////////////////////////////////////////////////////////////
// class FileStream - 文件流类

// 文件流打开方式 (UINT openMode)
#ifdef _COMPILER_WIN
enum
{
	FM_CREATE = 0xFFFF,
	FM_OPEN_READ = 0x0000,
	FM_OPEN_WRITE = 0x0001,
	FM_OPEN_READ_WRITE = 0x0002,

	FM_SHARE_EXCLUSIVE = 0x0010,
	FM_SHARE_DENY_WRITE = 0x0020,
	FM_SHARE_DENY_NONE = 0x0040
};
#endif
#ifdef _COMPILER_LINUX
enum
{
	FM_CREATE = 0xFFFF,
	FM_OPEN_READ = O_RDONLY,  // 0
	FM_OPEN_WRITE = O_WRONLY,  // 1
	FM_OPEN_READ_WRITE = O_RDWR,    // 2

	FM_SHARE_EXCLUSIVE = 0x0010,
	FM_SHARE_DENY_WRITE = 0x0020,
	FM_SHARE_DENY_NONE = 0x0030
};
#endif

// 缺省文件存取权限 (rights)
#ifdef _COMPILER_WIN
enum { DEFAULT_FILE_ACCESS_RIGHTS = 0 };
#endif
#ifdef _COMPILER_LINUX
enum { DEFAULT_FILE_ACCESS_RIGHTS = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH };
#endif

class FileStream :
	public Stream,
	noncopyable
{
public:
	FileStream();
	FileStream(const std::string& fileName, UINT openMode, UINT rights = DEFAULT_FILE_ACCESS_RIGHTS);
	virtual ~FileStream();

	bool open(const std::string& fileName, UINT openMode,
		UINT rights = DEFAULT_FILE_ACCESS_RIGHTS, FileException* exception = NULL);
	void close();

	virtual int read(void *buffer, int count);
	virtual int write(const void *buffer, int count);
	virtual INT64 seek(INT64 offset, SEEK_ORIGIN seekOrigin);
	virtual void setSize(INT64 size);

	std::string getFileName() const { return fileName_; }
	HANDLE getHandle() const { return handle_; }
	bool isOpen() const;

private:
	void init();
	HANDLE fileCreate(const std::string& fileName, UINT rights);
	HANDLE fileOpen(const std::string& fileName, UINT openMode);
	void fileClose(HANDLE handle);
	int fileRead(HANDLE handle, void *buffer, int count);
	int fileWrite(HANDLE handle, const void *buffer, int count);
	INT64 fileSeek(HANDLE handle, INT64 offset, SEEK_ORIGIN seekOrigin);

private:
	std::string fileName_;
	HANDLE handle_;
};


///////////////////////////////////////////////////////////////////////////////
// class MemoryStream - 内存流类

class MemoryStream : public Stream
{
public:
	enum { DEFAULT_MEMORY_DELTA = 1024 };    // 缺省内存增长步长 (字节数，必须是 2 的 N 次方)
	enum { MIN_MEMORY_DELTA = 256 };         // 最小内存增长步长

public:
	explicit MemoryStream(int memoryDelta = DEFAULT_MEMORY_DELTA);
	MemoryStream(const MemoryStream& src);
	virtual ~MemoryStream();

	MemoryStream& operator = (const MemoryStream& rhs);

	virtual int read(void *buffer, int count);
	virtual int write(const void *buffer, int count);
	virtual INT64 seek(INT64 offset, SEEK_ORIGIN seekOrigin);
	virtual void setSize(INT64 size);
	bool loadFromStream(Stream& stream);
	bool loadFromFile(const std::string& fileName);
	bool saveToStream(Stream& stream);
	bool saveToFile(const std::string& fileName);
	void clear();
	char* getMemory() { return memory_; }

private:
	void init();
	void assign(const MemoryStream& src);
	void setMemoryDelta(int newMemoryDelta);
	void setPointer(char *memory, int size);
	void setCapacity(int newCapacity);
	char* realloc(int& newCapacity);

private:
	char *memory_;
	int capacity_;
	int size_;
	int position_;
	int memoryDelta_;
};



///////////////////////////////////////////////////////////////////////////////
// class Buffer - 缓存类

class Buffer
{
public:
	Buffer();
	Buffer(const Buffer& src);
	explicit Buffer(int size);
	Buffer(const void *buffer, int size);
	virtual ~Buffer();

	Buffer& operator = (const Buffer& rhs);
	const char& operator[] (int index) const { return (static_cast<char*>(buffer_))[index]; }
	char& operator[] (int index) { return const_cast<char&>((static_cast<const Buffer&>(*this))[index]); }
	operator char*() const { return static_cast<char*>(buffer_); }
	char* data() const { return static_cast<char*>(buffer_); }
	char* c_str() const;
	void assign(const void *buffer, int size);
	void clear() { setSize(0); }
	void setSize(int size, bool initZero = false);
	int getSize() const { return size_; }
	void ensureSize(int size) { if (getSize() < size) setSize(size); }
	void setPosition(int position);
	int getPosition() const { return position_; }

	bool loadFromStream(Stream& stream);
	bool loadFromFile(const std::string& fileName);
	bool saveToStream(Stream& stream);
	bool saveToFile(const std::string& fileName);
private:
	inline void init() { buffer_ = NULL; size_ = 0; position_ = 0; }
	void assign(const Buffer& src);
	void verifyPosition();
protected:
	void *buffer_;
	int size_;
	int position_;
};



#endif


