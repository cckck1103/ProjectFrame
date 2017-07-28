///////////////////////////////////////////////////////////////////////////////
// MysqlDataBase.h
// Classes:
//   * MySqlConnection       - ���ݿ�������
//   * MySqlField            - �ֶ�������
//   * MySqlDataSet          - ���ݼ���
//   * MySqlQuery            - ���ݲ�ѯ����
//   * MySqlDatabase         - ���ݿ���
///////////////////////////////////////////////////////////////////////////////

#ifndef __DBI_MYSQL_H_
#define __DBI_MYSQL_H_

#include "CDataBase.h"
#include "LogManager.h"
#include "UtilClass.h"

#include <errmsg.h>
#include <mysql.h>

///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class MySqlConnection;
class MySqlField;
class MySqlDataSet;
class MySqlQuery;
class MySqlDatabase;

///////////////////////////////////////////////////////////////////////////////
// ������Ϣ (Error Message)

const char* const SEM_MYSQL_INIT_ERROR = "mysql init error.";
const char* const SEM_MYSQL_NUM_FIELDS_ERROR = "mysql_num_fields error.";
const char* const SEM_MYSQL_REAL_CONNECT_ERROR = "mysql_real_connect failed. Error: %s.";
const char* const SEM_MYSQL_STORE_RESULT_ERROR = "mysql_store_result failed. Error: %s.";
const char* const SEM_MYSQL_CONNECTING = "Try to connect MySQL server... (%p)";
const char* const SEM_MYSQL_CONNECTED = "Connected to MySQL server. (%p)";
const char* const SEM_MYSQL_LOST_CONNNECTION = "Lost connection to MySQL server.";

///////////////////////////////////////////////////////////////////////////////
// class MySqlConnection - ���ݿ�������

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

    // �������� (��ʧ�����׳��쳣)
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
    // �Ͽ�����
    virtual void doDisconnect()
	{
		mysql_close(&connObject_);
	}

    // ȡ��MySQL���Ӷ���
    MYSQL& getConnObject() { return connObject_; }

private:
    MYSQL connObject_;            // ���Ӷ���
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlField - �ֶ�������

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
    char* dataPtr_;               // ָ���ֶ�����
    int dataSize_;                // �ֶ����ݵ����ֽ���
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlDataSet - ���ݼ���

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
	// ����: ��ʼ�����ݼ� (����ʼ��ʧ�����׳��쳣)
	//-----------------------------------------------------------------------------
	virtual void initDataSet()
	{
		// ��MySQL������һ���Ի�ȡ������
		res_ = mysql_store_result(&getConnObject());

		// �����ȡʧ��
		if (!res_)
		{
			ThrowDbException(formatString(SEM_MYSQL_STORE_RESULT_ERROR,
				mysql_error(&getConnObject())).c_str());
		}
	}

	//-----------------------------------------------------------------------------
	// ����: ��ʼ�����ݼ����ֶεĶ���
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
    MYSQL_RES* res_;      // MySQL��ѯ�����
    MYSQL_ROW row_;       // MySQL��ѯ�����
};

///////////////////////////////////////////////////////////////////////////////
// class MySqlQuery - ��ѯ����

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
	// ����: ת���ַ���ʹ֮��SQL�кϷ�
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
	// ����: ��ȡִ��SQL����Ӱ�������
	//-----------------------------------------------------------------------------
    virtual UINT getAffectedRowCount()
	{
		UINT result = 0;

		if (dbConnection_)
			result = (UINT)mysql_affected_rows(&getConnObject());

		return result;
	}

	//-----------------------------------------------------------------------------
	// ����: ��ȡ���һ��������������ID��ֵ
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
		����ֻҪ�� mysql_real_connect() ���ӵ����ݿ�(������ mysql_connect())����ô
		�� mysql_real_query() ����ʱ��ʹ���Ӷ�ʧ(����TCP���ӶϿ�)���õ���Ҳ�᳢��
		ȥ�����������ݿ⡣�����Ծ����Ա�֤����ʵ��

		ע:
		1. ���� <= 5.0.3 �İ汾��MySQLĬ�ϻ��������˺�İ汾����� mysql_options()
		��ȷָ�� MYSQL_OPT_RECONNECT Ϊ true���Ż�������
		2. Ϊ�������Ӷ�ʧ����������ִ�С����ݿ�ս�������ʱҪִ�е������������ȷ
		ָ������ mysql_real_query() �Զ������������ɳ�����ʽ������
		*/

		for (int times = 0; times < 2; times++)
		{
			int r = mysql_real_query(&getConnObject(), sql_.c_str(), (int)sql_.length());

			// ���ִ��SQLʧ��
			if (r != 0)
			{
				// ������״Σ����Ҵ�������Ϊ���Ӷ�ʧ������������
				if (times == 0)
				{
					int errNum = mysql_errno(&getConnObject());
					if (errNum == CR_SERVER_GONE_ERROR || errNum == CR_SERVER_LOST)
					{
						INFO_LOG(SEM_MYSQL_LOST_CONNNECTION);

						// ǿ����������
						getDbConnection()->activateConnection(true);
						continue;
					}
				}

				// �����׳��쳣
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
