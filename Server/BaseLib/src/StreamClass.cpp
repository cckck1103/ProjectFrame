#include "StreamClass.h"

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���

union Int64Rec
{
	INT64 value;
	struct {
		INT32 lo;
		INT32 hi;
	} ints;
};

///////////////////////////////////////////////////////////////////////////////
// class Stream

void Stream::readBuffer(void *buffer, int count)
{
	if (count != 0 && read(buffer, count) != count)
		ThrowStreamException(SEM_STREAM_READ_ERROR);
}

//-----------------------------------------------------------------------------

void Stream::writeBuffer(const void *buffer, int count)
{
	if (count != 0 && write(buffer, count) != count)
		ThrowStreamException(SEM_STREAM_WRITE_ERROR);
}

//-----------------------------------------------------------------------------

INT64 Stream::getSize()
{
	INT64 pos, result;

	pos = seek(0, SO_CURRENT);
	result = seek(0, SO_END);
	seek(pos, SO_BEGINNING);

	return result;
}



///////////////////////////////////////////////////////////////////////////////
// class FileStream

//-----------------------------------------------------------------------------
// ����: ȱʡ���캯��
//-----------------------------------------------------------------------------
FileStream::FileStream()
{
	init();
}

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   fileName   - �ļ���
//   openMode   - �ļ����򿪷�ʽ
//   rights     - �ļ���ȡȨ��
//-----------------------------------------------------------------------------
FileStream::FileStream(const std::string& fileName, UINT openMode, UINT rights)
{
	init();

	FileException e;
	if (!open(fileName, openMode, rights, &e))
		throw FileException(e);
}

//-----------------------------------------------------------------------------
// ����: ��������
//-----------------------------------------------------------------------------
FileStream::~FileStream()
{
	close();
}

//-----------------------------------------------------------------------------
// ����: ���ļ�
// ����:
//   fileName - �ļ���
//   openMode   - �ļ����򿪷�ʽ
//   rights     - �ļ���ȡȨ��
//   exception  - ��������쳣���򴫻ظ��쳣
//-----------------------------------------------------------------------------
bool FileStream::open(const std::string& fileName, UINT openMode, UINT rights,
	FileException* exception)
{
	close();

	if (openMode == FM_CREATE)
		handle_ = fileCreate(fileName, rights);
	else
		handle_ = fileOpen(fileName, openMode);

	bool result = (handle_ != INVALID_HANDLE_VALUE);

	if (!result && exception != NULL)
	{
		if (openMode == FM_CREATE)
			*exception = FileException(fileName.c_str(), getLastSysError(),
				formatString(SEM_CANNOT_CREATE_FILE, fileName.c_str(),
					sysErrorMessage(getLastSysError()).c_str()).c_str());
		else
			*exception = FileException(fileName.c_str(), getLastSysError(),
				formatString(SEM_CANNOT_OPEN_FILE, fileName.c_str(),
					sysErrorMessage(getLastSysError()).c_str()).c_str());
	}

	fileName_ = fileName;
	return result;
}

//-----------------------------------------------------------------------------
// ����: �ر��ļ�
//-----------------------------------------------------------------------------
void FileStream::close()
{
	if (handle_ != INVALID_HANDLE_VALUE)
	{
		fileClose(handle_);
		handle_ = INVALID_HANDLE_VALUE;
	}
	fileName_.clear();
}

//-----------------------------------------------------------------------------
// ����: ���ļ���
//-----------------------------------------------------------------------------
int FileStream::read(void *buffer, int count)
{
	int result;

	result = fileRead(handle_, buffer, count);
	if (result == -1) result = 0;

	return result;
}

//-----------------------------------------------------------------------------
// ����: д�ļ���
//-----------------------------------------------------------------------------
int FileStream::write(const void *buffer, int count)
{
	int result;

	result = fileWrite(handle_, buffer, count);
	if (result == -1) result = 0;

	return result;
}

//-----------------------------------------------------------------------------
// ����: �ļ���ָ�붨λ
//-----------------------------------------------------------------------------
INT64 FileStream::seek(INT64 offset, SEEK_ORIGIN seekOrigin)
{
	return fileSeek(handle_, offset, seekOrigin);
}

//-----------------------------------------------------------------------------
// ����: �����ļ�����С
//-----------------------------------------------------------------------------
void FileStream::setSize(INT64 size)
{
	bool success;
	seek(size, SO_BEGINNING);

#ifdef _COMPILER_WIN
	success = ::SetEndOfFile(handle_) != 0;
#endif
#ifdef _COMPILER_LINUX
	success = (::ftruncate(handle_, static_cast<__off_t>(getPosition())) == 0);
#endif

	if (!success)
		ThrowFileException(fileName_.c_str(), getLastSysError(), SEM_SET_FILE_STREAM_SIZE_ERR);
}

//-----------------------------------------------------------------------------
// ����: �ж��ļ�����ǰ�Ƿ��״̬
//-----------------------------------------------------------------------------
bool FileStream::isOpen() const
{
	return (handle_ != INVALID_HANDLE_VALUE);
}

//-----------------------------------------------------------------------------
// ����: ��ʼ��
//-----------------------------------------------------------------------------
void FileStream::init()
{
	fileName_.clear();
	handle_ = INVALID_HANDLE_VALUE;
}

//-----------------------------------------------------------------------------
// ����: �����ļ�
//-----------------------------------------------------------------------------
HANDLE FileStream::fileCreate(const std::string& fileName, UINT rights)
{
#ifdef _COMPILER_WIN
	return ::CreateFileA(fileName.c_str(), GENERIC_READ | GENERIC_WRITE,
		0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
#endif
#ifdef _COMPILER_LINUX
	umask(0);  // ��ֹ rights �� umask ֵ ����
	return ::open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, rights);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ļ�
//-----------------------------------------------------------------------------
HANDLE FileStream::fileOpen(const std::string& fileName, UINT openMode)
{
#ifdef _COMPILER_WIN
	UINT accessModes[3] = {
		GENERIC_READ,
		GENERIC_WRITE,
		GENERIC_READ | GENERIC_WRITE
	};
	UINT shareModes[5] = {
		0,
		0,
		FILE_SHARE_READ,
		FILE_SHARE_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE
	};

	HANDLE fileHandle = INVALID_HANDLE_VALUE;

	if ((openMode & 3) <= FM_OPEN_READ_WRITE &&
		(openMode & 0xF0) <= FM_SHARE_DENY_NONE)
		fileHandle = ::CreateFileA(fileName.c_str(), accessModes[openMode & 3],
			shareModes[(openMode & 0xF0) >> 4], NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	return fileHandle;
#endif
#ifdef _COMPILER_LINUX
	BYTE shareModes[4] = {
		0,          // none
		F_WRLCK,    // FM_SHARE_EXCLUSIVE
		F_RDLCK,    // FM_SHARE_DENY_WRITE
		0           // FM_SHARE_DENY_NONE
	};

	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	BYTE shareMode;

	if (fileExists(fileName) &&
		(openMode & 0x03) <= FM_OPEN_READ_WRITE &&
		(openMode & 0xF0) <= FM_SHARE_DENY_NONE)
	{
		umask(0);  // ��ֹ openMode �� umask ֵ����
		fileHandle = ::open(fileName.c_str(), (openMode & 0x03), DEFAULT_FILE_ACCESS_RIGHTS);
		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			shareMode = static_cast<BYTE>((openMode & 0xF0) >> 4);
			if (shareModes[shareMode] != 0)
			{
				struct flock flk;

				flk.l_type = shareModes[shareMode];
				flk.l_whence = SEEK_SET;
				flk.l_start = 0;
				flk.l_len = 0;

				if (fcntl(fileHandle, F_SETLK, &flk) < 0)
				{
					fileClose(fileHandle);
					fileHandle = INVALID_HANDLE_VALUE;
				}
			}
		}
	}

	return fileHandle;
#endif
}

//-----------------------------------------------------------------------------
// ����: �ر��ļ�
//-----------------------------------------------------------------------------
void FileStream::fileClose(HANDLE handle)
{
#ifdef _COMPILER_WIN
	::CloseHandle(handle);
#endif
#ifdef _COMPILER_LINUX
	::close(handle);
#endif
}

//-----------------------------------------------------------------------------
// ����: ���ļ�
//-----------------------------------------------------------------------------
int FileStream::fileRead(HANDLE handle, void *buffer, int count)
{
#ifdef _COMPILER_WIN
	unsigned long result;
	if (!::ReadFile(handle, buffer, count, &result, NULL))
		result = -1;
	return result;
#endif
#ifdef _COMPILER_LINUX
	return ::read(handle, buffer, count);
#endif
}

//-----------------------------------------------------------------------------
// ����: д�ļ�
//-----------------------------------------------------------------------------
int FileStream::fileWrite(HANDLE handle, const void *buffer, int count)
{
#ifdef _COMPILER_WIN
	unsigned long result;
	if (!::WriteFile(handle, buffer, count, &result, NULL))
		result = -1;
	return result;
#endif
#ifdef _COMPILER_LINUX
	return ::write(handle, buffer, count);
#endif
}

//-----------------------------------------------------------------------------
// ����: �ļ�ָ�붨λ
//-----------------------------------------------------------------------------
INT64 FileStream::fileSeek(HANDLE handle, INT64 offset, SEEK_ORIGIN seekOrigin)
{
#ifdef _COMPILER_WIN
	INT64 result = offset;
	((Int64Rec*)&result)->ints.lo = ::SetFilePointer(
		handle, ((Int64Rec*)&result)->ints.lo,
		(PLONG)&(((Int64Rec*)&result)->ints.hi), seekOrigin);
	if (((Int64Rec*)&result)->ints.lo == -1 && GetLastError() != 0)
		((Int64Rec*)&result)->ints.hi = -1;
	return result;
#endif
#ifdef _COMPILER_LINUX
	INT64 result = ::lseek(handle, static_cast<__off_t>(offset), seekOrigin);
	return result;
#endif
}



///////////////////////////////////////////////////////////////////////////////
// class MemoryStream

//-----------------------------------------------------------------------------
// ����: ���캯��
// ����:
//   memoryDelta - �ڴ��������� (�ֽ����������� 2 �� N �η�)
//-----------------------------------------------------------------------------
MemoryStream::MemoryStream(int memoryDelta)
{
	init();
}

//-----------------------------------------------------------------------------

MemoryStream::MemoryStream(const MemoryStream& src)
{
	init();
	assign(src);
}

//-----------------------------------------------------------------------------

MemoryStream::~MemoryStream()
{
	clear();
}

//-----------------------------------------------------------------------------

void MemoryStream::init()
{
	memory_ = NULL;
	capacity_ = 0;
	size_ = 0;
	position_ = 0;
	setMemoryDelta(DEFAULT_MEMORY_DELTA);
}

//-----------------------------------------------------------------------------

MemoryStream& MemoryStream::operator = (const MemoryStream& rhs)
{
	if (this != &rhs)
		assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------
// ����: ���ڴ���
//-----------------------------------------------------------------------------
int MemoryStream::read(void *buffer, int count)
{
	int result = 0;

	if (position_ >= 0 && count >= 0)
	{
		result = size_ - position_;
		if (result > 0)
		{
			if (result > count) result = count;
			memmove(buffer, memory_ + (UINT)position_, result);
			position_ += result;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// ����: д�ڴ���
//-----------------------------------------------------------------------------
int MemoryStream::write(const void *buffer, int count)
{
	int result = 0;
	int pos;

	if (position_ >= 0 && count >= 0)
	{
		pos = position_ + count;
		if (pos > 0)
		{
			if (pos > size_)
			{
				if (pos > capacity_)
					setCapacity(pos);
				size_ = pos;
			}
			memmove(memory_ + (UINT)position_, buffer, count);
			position_ = pos;
			result = count;
		}
	}

	return result;
}

//-----------------------------------------------------------------------------
// ����: �ڴ���ָ�붨λ
//-----------------------------------------------------------------------------
INT64 MemoryStream::seek(INT64 offset, SEEK_ORIGIN seekOrigin)
{
	switch (seekOrigin)
	{
	case SO_BEGINNING:
		position_ = (int)offset;
		break;
	case SO_CURRENT:
		position_ += (int)offset;
		break;
	case SO_END:
		position_ = size_ + (int)offset;
		break;
	}

	return position_;
}

//-----------------------------------------------------------------------------
// ����: �����ڴ�����С
//-----------------------------------------------------------------------------
void MemoryStream::setSize(INT64 size)
{
	ASSERT_X(size <= MAXINT);

	int oldPos = position_;

	setCapacity((int)size);
	size_ = (int)size;
	if (oldPos > size) seek(0, SO_END);
}

//-----------------------------------------------------------------------------
// ����: �����������뵽�ڴ�����
//-----------------------------------------------------------------------------
bool MemoryStream::loadFromStream(Stream& stream)
{
	try
	{
		INT64 count = stream.getSize();
		ASSERT_X(count <= MAXINT);

		stream.setPosition(0);
		setSize(count);
		if (count != 0)
			stream.readBuffer(memory_, (int)count);
		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���ļ����뵽�ڴ�����
//-----------------------------------------------------------------------------
bool MemoryStream::loadFromFile(const std::string& fileName)
{
	FileStream fs;
	bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
	if (result)
		result = loadFromStream(fs);
	return result;
}

//-----------------------------------------------------------------------------
// ����: ���ڴ������浽��������
//-----------------------------------------------------------------------------
bool MemoryStream::saveToStream(Stream& stream)
{
	try
	{
		if (size_ != 0)
			stream.writeBuffer(memory_, size_);
		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���ڴ������浽�ļ���
//-----------------------------------------------------------------------------
bool MemoryStream::saveToFile(const std::string& fileName)
{
	FileStream fs;
	bool result = fs.open(fileName, FM_CREATE);
	if (result)
		result = saveToStream(fs);
	return result;
}

//-----------------------------------------------------------------------------
// ����: ����ڴ���
//-----------------------------------------------------------------------------
void MemoryStream::clear()
{
	setCapacity(0);
	size_ = 0;
	position_ = 0;
}

//-----------------------------------------------------------------------------

void MemoryStream::assign(const MemoryStream& src)
{
	setMemoryDelta(src.memoryDelta_);
	setSize(src.size_);
	setPosition(src.position_);
	if (size_ > 0)
		memmove(memory_, src.memory_, size_);
}

//-----------------------------------------------------------------------------

void MemoryStream::setMemoryDelta(int newMemoryDelta)
{
	if (newMemoryDelta != DEFAULT_MEMORY_DELTA)
	{
		if (newMemoryDelta < MIN_MEMORY_DELTA)
			newMemoryDelta = MIN_MEMORY_DELTA;

		// ��֤ newMemoryDelta ��2��N�η�
		for (int i = sizeof(int) * 8 - 1; i >= 0; i--)
			if (((1 << i) & newMemoryDelta) != 0)
			{
				newMemoryDelta &= (1 << i);
				break;
			}
	}

	memoryDelta_ = newMemoryDelta;
}

//-----------------------------------------------------------------------------

void MemoryStream::setPointer(char *memory, int size)
{
	memory_ = memory;
	size_ = size;
}

//-----------------------------------------------------------------------------

void MemoryStream::setCapacity(int newCapacity)
{
	setPointer(realloc(newCapacity), size_);
	capacity_ = newCapacity;
}

//-----------------------------------------------------------------------------

char* MemoryStream::realloc(int& newCapacity)
{
	char* result;

	if (newCapacity > 0 && newCapacity != size_)
		newCapacity = (newCapacity + (memoryDelta_ - 1)) & ~(memoryDelta_ - 1);

	result = memory_;
	if (newCapacity != capacity_)
	{
		if (newCapacity == 0)
		{
			free(memory_);
			result = NULL;
		}
		else
		{
			if (capacity_ == 0)
				result = (char*)malloc(newCapacity);
			else
				result = (char*)::realloc(memory_, newCapacity);

			if (!result)
				ThrowMemoryException();
		}
	}

	return result;
}



///////////////////////////////////////////////////////////////////////////////
// class Buffer

Buffer::Buffer()
{
	init();
}

//-----------------------------------------------------------------------------

Buffer::Buffer(const Buffer& src)
{
	init();
	assign(src);
}

//-----------------------------------------------------------------------------

Buffer::Buffer(int size)
{
	init();
	setSize(size);
}

//-----------------------------------------------------------------------------

Buffer::Buffer(const void *buffer, int size)
{
	init();
	assign(buffer, size);
}

//-----------------------------------------------------------------------------

Buffer::~Buffer()
{
	if (buffer_)
		free(buffer_);
}

//-----------------------------------------------------------------------------

Buffer& Buffer::operator = (const Buffer& rhs)
{
	if (this != &rhs)
		assign(rhs);
	return *this;
}

//-----------------------------------------------------------------------------
// ����: �� buffer �е� size ���ֽڸ��Ƶ� *this �У�������С����Ϊ size
//-----------------------------------------------------------------------------
void Buffer::assign(const void *buffer, int size)
{
	setSize(size);
	if (size_ > 0)
		memmove(buffer_, buffer, size_);
	verifyPosition();
}

//-----------------------------------------------------------------------------
// ����: ���û����С
// ����:
//   size     - �»�������С
//   initZero - ���»������Ⱦɻ��������Ƿ񽫶���Ŀռ���'\0'���
// ��ע:
//   �µĻ���ᱣ��ԭ������
//-----------------------------------------------------------------------------
void Buffer::setSize(int size, bool initZero)
{
	if (size <= 0)
	{
		if (buffer_) free(buffer_);
		buffer_ = NULL;
		size_ = 0;
		position_ = 0;
	}
	else if (size != size_)
	{
		void *newBuf;

		// ��� buffer_ == NULL���� realloc �൱�� malloc��
		newBuf = realloc(buffer_, size + 1);  // �����һ���ֽ����� c_str()!

		if (newBuf)
		{
			if (initZero && (size > size_))
				memset(((char*)newBuf) + size_, 0, size - size_);
			buffer_ = newBuf;
			size_ = size;
			verifyPosition();
		}
		else
		{
			ThrowMemoryException();
		}
	}
}

//-----------------------------------------------------------------------------
// ����: ���� Position
//-----------------------------------------------------------------------------
void Buffer::setPosition(int position)
{
	position_ = position;
	verifyPosition();
}

//-----------------------------------------------------------------------------
// ����: ���� C �����ַ��� (ĩ�˸��ӽ����� '\0')
//-----------------------------------------------------------------------------
char* Buffer::c_str() const
{
	if (size_ <= 0 || !buffer_)
		return (char*)"";
	else
	{
		((char*)buffer_)[size_] = 0;
		return (char*)buffer_;
	}
}

//-----------------------------------------------------------------------------
// ����: �����������뵽������
//-----------------------------------------------------------------------------
bool Buffer::loadFromStream(Stream& stream)
{
	try
	{
		INT64 size64 = stream.getSize() - stream.getPosition();
		ASSERT_X(size64 <= MAXINT);
		int size = (int)size64;

		setPosition(0);
		setSize(size);
		stream.readBuffer(data(), size);
		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: ���ļ����뵽������
//-----------------------------------------------------------------------------
bool Buffer::loadFromFile(const std::string& fileName)
{
	FileStream fs;
	bool result = fs.open(fileName, FM_OPEN_READ | FM_SHARE_DENY_WRITE);
	if (result)
		result = loadFromStream(fs);
	return result;
}

//-----------------------------------------------------------------------------
// ����: �����汣�浽��������
//-----------------------------------------------------------------------------
bool Buffer::saveToStream(Stream& stream)
{
	try
	{
		if (getSize() > 0)
			stream.writeBuffer(data(), getSize());
		return true;
	}
	catch (Exception&)
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// ����: �����汣�浽�ļ���
//-----------------------------------------------------------------------------
bool Buffer::saveToFile(const std::string& fileName)
{
	FileStream fs;
	bool result = fs.open(fileName, FM_CREATE);
	if (result)
		result = saveToStream(fs);
	return result;
}

//-----------------------------------------------------------------------------

void Buffer::assign(const Buffer& src)
{
	setSize(src.getSize());
	setPosition(src.getPosition());
	if (size_ > 0)
		memmove(buffer_, src.buffer_, size_);
}

//-----------------------------------------------------------------------------

void Buffer::verifyPosition()
{
	if (position_ < 0) position_ = 0;
	if (position_ > size_) position_ = size_;
}
