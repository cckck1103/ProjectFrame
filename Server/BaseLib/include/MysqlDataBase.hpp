///////////////////////////////////////////////////////////////////////////////
// MysqlDataBase.h
// Classes:
//   * MySqlConnection       - 数据库连接类
//   * MySqlField            - 字段数据类
//   * MySqlDataSet          - 数据集类
//   * MySqlQuery            - 数据查询器类
//   * MySqlDatabase         - 数据库类
///////////////////////////////////////////////////////////////////////////////

#ifndef __DBI_MYSQL_H_
#define __DBI_MYSQL_H_

#include "CDataBase.h"
#include "LogManager.h"
#include "UtilClass.h"

#include <errmsg.h>
#include <mysql.h>

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class MySqlConnection;
class MySqlField;
class MySqlDataSet;
class MySqlQuery;
class MySqlDatabase;

///////////////////////////////////////////////////////////////////////////////
// 错误信息 (Error Message)

const char* const SEM_MYSQL_INIT_ERROR = "mysql init error.";
const char* const SEM_MYSQL_NUM_FIELDS_ERROR = "mysql_num_fields error.";
const char* const SEM_MYSQL_REAL_CONNECT_ERROR = "mysql_real_connect failed. Error: %s.";
const char* const SEM_MYSQL_STORE_RESULT_ERROR = "mysql_store_result failed. Error: %s.";
const char* const SEM_MYSQL_CONNECTING = "Try to connect MySQL server... (%p)";
const char* const SEM_MYSQL_CONNECTED = "Connected to MySQL server. (%p)";
const char* const SEM_MYSQL_LOST_CONNNECTION = "Lost connection to MySQL server.";

///////////////////////////////////////////////////////////////////////////////
// class MySqlConnection - 数据库连接类

class MySqlConnection : public DbConnection
{
public:
	MySqlConnection(Database *database) : DbConnection(database)
	{
		memset(&connObject_, 0, sizeof(connObject_));
	}
	virtual ~MySqlConnection()
	{

	}

    // 建立连接 (若失败则抛出异常)
	virtual void doConnect()
	{
		static Mutex s_mutex;
		AutoLocker locker(s_mutex);

		if (mysql_init(&connObject_) == NULL)
			ThrowDbException(SEM_MYSQL_INIT_ERROR);

		INFO_LOG(SEM_MYSQL_CONNECTING, &connObject_);

		int value = 0;
		mysql_options(&connObject_, MYSQL_OPT_RECONNECT, (char*)&value);

		if (mysql_real_connect(&connObject_,
			database_->getDbConnParams()->getHostName().c_str(),
			database_->getDbConnParams()->getUserName().c_str(),
			database_->getDbConnParams()->getPassword().c_str(),
			database_->getDbConnParams()->getDbName().c_str(),
			database_->getDbConnParams()->getPort(), NULL, 0) == NULL)
		{
			mysql_close(&connObject_);
			ThrowDbException(formatString(SEM_MYSQL_REAL_CONNECT_ERROR, mysql_error(&connObject_)).c_str());
		}

		// for MYSQL 5.0.7 or higher
		std::string strInitialCharSet = database_->getDbOptions()->getInitialCharSet();
		if (!strInitialCharSet.empty())
			mysql_set_character_set(&connObject_, strInitialCharSet.c_str());

		INFO_LOG(SEM_MYSQL_CONNECTED, &connObject_);
	}
    // 断开连接
    virtual void doDisconnect()
	{
		mysql_close(&connObject_);
	}

    // 取得MySQL连接对象
    MYSQL& getConnObject() { return connObject_; }

private:
    MYSQL connObject_;            // 连接对象
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlField - 字段数据类

class MySqlField : public DbField
{
public:
	MySqlField()
	{
		dataPtr_ = NULL;
		dataSize_ = 0;
	}

    virtual ~MySqlField() {}

	void setData(void *dataPtr, int dataSize)
	{
			dataPtr_ = (char*)dataPtr;
			dataSize_ = dataSize;
	}

    virtual bool isNull() const 
	{ 
		return (dataPtr_ == NULL); 
	}

	virtual std::string asString() const
	{
		std::string result;

		if (dataPtr_ && dataSize_ > 0)
			result.assign(dataPtr_, dataSize_);

		return result;
	}

private:
    char* dataPtr_;               // 指向字段数据
    int dataSize_;                // 字段数据的总字节数
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlDataSet - 数据集类

class MySqlDataSet : public DbDataSet
{
public:
	MySqlDataSet(DbQuery* dbQuery) :
		DbDataSet(dbQuery),
		res_(NULL),
		row_(NULL)
	{
		// nothing
	}

    virtual ~MySqlDataSet()
	{
		freeDataSet();
	}

    virtual bool rewind()
	{
		if (getRecordCount() > 0)
		{
			mysql_data_seek(res_, 0);
			return true;
		}
		else
			return false;
	}

    virtual bool next()
	{
		row_ = mysql_fetch_row(res_);
		if (row_)
		{
			MySqlField* field;
			int fieldCount;
			unsigned long *lengths;

			fieldCount = mysql_num_fields(res_);
			lengths = (unsigned long*)mysql_fetch_lengths(res_);

			for (int i = 0; i < fieldCount; i++)
			{
				if (i < dbFieldList_.getCount())
				{
					field = (MySqlField*)dbFieldList_[i];
				}
				else
				{
					field = new MySqlField();
					dbFieldList_.add(field);
				}

				field->setData(row_[i], lengths[i]);
			}
		}

		return (row_ != NULL);
	}

    virtual UINT64 getRecordCount()
	{
		if (res_)
			return (UINT64)mysql_num_rows(res_);
		else
			return 0;
	}


    virtual bool isEmpty()
	{
		return (getRecordCount() == 0);
	}


protected:
	//-----------------------------------------------------------------------------
	// 描述: 初始化数据集 (若初始化失败则抛出异常)
	//-----------------------------------------------------------------------------
	virtual void initDataSet()
	{
		// 从MySQL服务器一次性获取所有行
		res_ = mysql_store_result(&getConnObject());

		// 如果获取失败
		if (!res_)
		{
			ThrowDbException(formatString(SEM_MYSQL_STORE_RESULT_ERROR,
				mysql_error(&getConnObject())).c_str());
		}
	}

	//-----------------------------------------------------------------------------
	// 描述: 初始化数据集各字段的定义
	//-----------------------------------------------------------------------------
	virtual void initFieldDefs()
	{
		MYSQL_FIELD *mySqlFields;
		DbFieldDef* fieldDef;
		int fieldCount;

		dbFieldDefList_.clear();
		fieldCount = mysql_num_fields(res_);
		mySqlFields = mysql_fetch_fields(res_);

		if (fieldCount <= 0)
			ThrowDbException(SEM_MYSQL_NUM_FIELDS_ERROR);

		for (int i = 0; i < fieldCount; i++)
		{
			fieldDef = new DbFieldDef();
			fieldDef->setData(mySqlFields[i].name, mySqlFields[i].type);
			dbFieldDefList_.add(fieldDef);
		}
	}

private:
    MYSQL& getConnObject()
	{
		return ((MySqlConnection*)dbQuery_->getDbConnection())->getConnObject();
	}

    void freeDataSet()
	{
		if (res_)
			mysql_free_result(res_);
		res_ = NULL;
	}

private:
    MYSQL_RES* res_;      // MySQL查询结果集
    MYSQL_ROW row_;       // MySQL查询结果行
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlQuery - 查询器类

class MySqlQuery : public DbQuery
{
public:
	MySqlQuery(Database *database) :
		DbQuery(database)
	{
		// nothing
	}
	virtual ~MySqlQuery()
	{
		// nothing
	}

	//-----------------------------------------------------------------------------
	// 描述: 转换字符串使之在SQL中合法
	//-----------------------------------------------------------------------------
    virtual std::string escapeString(const std::string& str)
	{
		if (str.empty()) return "";

		int srcLen = (int)str.size();
		Buffer buffer(srcLen * 2 + 1);
		char *end;

		ensureConnected();

		end = (char*)buffer.data();
		end += mysql_real_escape_string(&getConnObject(), end, str.c_str(), srcLen);
		*end = '\0';

		return (char*)buffer.data();
	}

	//-----------------------------------------------------------------------------
	// 描述: 获取执行SQL后受影响的行数
	//-----------------------------------------------------------------------------
    virtual UINT getAffectedRowCount()
	{
		UINT result = 0;

		if (dbConnection_)
			result = (UINT)mysql_affected_rows(&getConnObject());

		return result;
	}

	//-----------------------------------------------------------------------------
	// 描述: 获取最后一条插入语句的自增ID的值
	//-----------------------------------------------------------------------------
	virtual UINT64 getLastInsertId()
	{
		UINT64 result = 0;

		if (dbConnection_)
			result = (UINT64)mysql_insert_id(&getConnObject());

		return result;
	}

protected:
	virtual void doExecute(DbDataSet *resultDataSet)
	{
		/*
		即：只要用 mysql_real_connect() 连接到数据库(而不是 mysql_connect())，那么
		在 mysql_real_query() 调用时即使连接丢失(比如TCP连接断开)，该调用也会尝试
		去重新连接数据库。该特性经测试被证明属实。

		注:
		1. 对于 <= 5.0.3 的版本，MySQL默认会重连；此后的版本需调用 mysql_options()
		明确指定 MYSQL_OPT_RECONNECT 为 true，才会重连。
		2. 为了在连接丢失并重连后，能执行“数据库刚建立连接时要执行的命令”，程序明确
		指定不让 mysql_real_query() 自动重连，而是由程序显式重连。
		*/

		for (int times = 0; times < 2; times++)
		{
			int r = mysql_real_query(&getConnObject(), sql_.c_str(), (int)sql_.length());

			// 如果执行SQL失败
			if (r != 0)
			{
				// 如果是首次，并且错误类型为连接丢失，则重试连接
				if (times == 0)
				{
					int errNum = mysql_errno(&getConnObject());
					if (errNum == CR_SERVER_GONE_ERROR || errNum == CR_SERVER_LOST)
					{
						INFO_LOG(SEM_MYSQL_LOST_CONNNECTION);

						// 强制重新连接
						getDbConnection()->activateConnection(true);
						continue;
					}
				}

				// 否则抛出异常
				std::string sql(sql_);
				if (sql.length() > 1024 * 2)
				{
					sql.resize(100);
					sql += "...";
				}

				std::string errMsg = formatString("%s; Error: %s", sql.c_str(), mysql_error(&getConnObject()));
				ThrowDbException(errMsg.c_str());
			}
			else break;
		}
	}

private:
    MYSQL& getConnObject()
	{
		return ((MySqlConnection*)dbConnection_)->getConnObject();
	}
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlDatabase

class MySqlDatabase : public Database
{
public:
    virtual DbConnection* createDbConnection() { return new MySqlConnection(this); }
    virtual DbDataSet* createDbDataSet(DbQuery* dbQuery) { return new MySqlDataSet(dbQuery); }
    virtual DbQuery* createDbQuery() { return new MySqlQuery(this); }
};

///////////////////////////////////////////////////////////////////////////////
#endif // __DBI_MYSQL_H_
