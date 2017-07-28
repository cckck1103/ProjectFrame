///////////////////////////////////////////////////////////////////////////////
// �ļ�����: ise_database.cpp
// ��������: ���ݿ�ӿ�(DBI: Database Interface)
///////////////////////////////////////////////////////////////////////////////

#include "CDataBase.h"
#include "SysUtils.h"
#include "ErrMsgs.h"

///////////////////////////////////////////////////////////////////////////////
// class DbConnParams

DbConnParams::DbConnParams() :
    port_(0)
{
    // nothing
}

DbConnParams::DbConnParams(const DbConnParams& src)
{
    hostName_ = src.hostName_;
    userName_ = src.userName_;
    password_ = src.password_;
    dbName_ = src.dbName_;
    port_ = src.port_;
}

DbConnParams::DbConnParams(const std::string& hostName, const std::string& userName,
    const std::string& password, const std::string& dbName, int port)
{
    hostName_ = hostName;
    userName_ = userName;
    password_ = password;
    dbName_ = dbName;
    port_ = port;
}

///////////////////////////////////////////////////////////////////////////////
// class DbOptions

DbOptions::DbOptions()
{
    setMaxDbConnections(DEF_MAX_DB_CONNECTIONS);
}

void DbOptions::setMaxDbConnections(int value)
{
    if (value < 1) value = 1;
    maxDbConnections_ = value;
}

void DbOptions::setInitialSqlCmd(const std::string& value)
{
    initialSqlCmdList_.clear();
    initialSqlCmdList_.add(value.c_str());
}

void DbOptions::setInitialCharSet(const std::string& value)
{
    initialCharSet_ = value;
}

///////////////////////////////////////////////////////////////////////////////
// class DbConnection

DbConnection::DbConnection(Database *database)
{
    database_ = database;
    isConnected_ = false;
    isBusy_ = false;
}

DbConnection::~DbConnection()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: �������ݿ�����
// ����:
//   force - �Ƿ�ǿ�Ƽ���
//-----------------------------------------------------------------------------
void DbConnection::activateConnection(bool force)
{
    // û���������ݿ���������
    if (!isConnected_ || force)
    {
        disconnect();
        connect();
        return;
    }
}

//-----------------------------------------------------------------------------
// ����: �������ݿ����Ӳ������������ (��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void DbConnection::connect()
{
    if (!isConnected_)
    {
        doConnect();
        execCmdOnConnected();
        isConnected_ = true;
    }
}

//-----------------------------------------------------------------------------
// ����: �Ͽ����ݿ����Ӳ������������
//-----------------------------------------------------------------------------
void DbConnection::disconnect()
{
    if (isConnected_)
    {
        doDisconnect();
        isConnected_ = false;
    }
}

//-----------------------------------------------------------------------------
// ����: �ս�������ʱִ������
//-----------------------------------------------------------------------------
void DbConnection::execCmdOnConnected()
{
    try
    {
        StrList& cmdList = database_->getDbOptions()->initialSqlCmdList();
        if (!cmdList.isEmpty())
        {
            DbQueryWrapper query(database_->createDbQuery());
            query->dbConnection_ = this;

            for (int i = 0; i < cmdList.getCount(); ++i)
            {
                try
                {
                    query->setSql(cmdList[i]);
                    query->execute();
                }
                catch (...)
                {}
            }

            query->dbConnection_ = NULL;
        }
    }
    catch (...)
    {}
}

//-----------------------------------------------------------------------------
// ����: �������� (�� ConnectionPool ����)
//-----------------------------------------------------------------------------
bool DbConnection::getDbConnection()
{
    if (!isBusy_)
    {
        activateConnection();
        isBusy_ = true;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// ����: �黹���� (�� ConnectionPool ����)
//-----------------------------------------------------------------------------
void DbConnection::returnDbConnection()
{
    isBusy_ = false;
}

//-----------------------------------------------------------------------------
// ����: ���������Ƿ񱻽��� (�� ConnectionPool ����)
//-----------------------------------------------------------------------------
bool DbConnection::isBusy()
{
    return isBusy_;
}

///////////////////////////////////////////////////////////////////////////////
// class DbConnectionPool

DbConnectionPool::DbConnectionPool(Database *database) :
    database_(database)
{
    // nothing
}

DbConnectionPool::~DbConnectionPool()
{
    clearPool();
}

//-----------------------------------------------------------------------------
// ����: ����һ�����õĿ������� (��ʧ�����׳��쳣)
// ����: ���Ӷ���ָ��
//-----------------------------------------------------------------------------
DbConnection* DbConnectionPool::getConnection()
{
    DbConnection *dbConnection = NULL;
    bool result = false;

    {
        AutoLocker locker(mutex_);

        // ������е������Ƿ�����
        for (int i = 0; i < dbConnectionList_.getCount(); i++)
        {
            dbConnection = (DbConnection*)dbConnectionList_[i];
            result = dbConnection->getDbConnection();  // �������
            if (result) break;
        }

        // ������ʧ�ܣ��������µ����ݿ����ӵ����ӳ�
        if (!result && (dbConnectionList_.getCount() < database_->getDbOptions()->getMaxDbConnections()))
        {
            dbConnection = database_->createDbConnection();
            dbConnectionList_.add(dbConnection);
            result = dbConnection->getDbConnection();
        }
    }

    if (!result)
        ThrowDbException(SEM_GET_CONN_FROM_POOL_ERROR);

    return dbConnection;
}

//-----------------------------------------------------------------------------
// ����: �黹���ݿ�����
//-----------------------------------------------------------------------------
void DbConnectionPool::returnConnection(DbConnection *dbConnection)
{
    AutoLocker locker(mutex_);
    dbConnection->returnDbConnection();
}

//-----------------------------------------------------------------------------
// ����: ������ӳ�
//-----------------------------------------------------------------------------
void DbConnectionPool::clearPool()
{
    AutoLocker locker(mutex_);

    for (int i = 0; i < dbConnectionList_.getCount(); i++)
    {
        DbConnection *dbConnection;
        dbConnection = (DbConnection*)dbConnectionList_[i];
        dbConnection->doDisconnect();
        delete dbConnection;
    }

    dbConnectionList_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDef

DbFieldDef::DbFieldDef(const std::string& name, int type)
{
    name_ = name;
    type_ = type;
}

DbFieldDef::DbFieldDef(const DbFieldDef& src)
{
    name_ = src.name_;
    type_ = src.type_;
}

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDefList

DbFieldDefList::DbFieldDefList()
{
    // nothing
}

DbFieldDefList::~DbFieldDefList()
{
    clear();
}

//-----------------------------------------------------------------------------
// ����: ���һ���ֶζ������
//-----------------------------------------------------------------------------
void DbFieldDefList::add(DbFieldDef *fieldDef)
{
    if (fieldDef != NULL)
        items_.add(fieldDef);
}

//-----------------------------------------------------------------------------
// ����: �ͷŲ���������ֶζ������
//-----------------------------------------------------------------------------
void DbFieldDefList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (DbFieldDef*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// ����: �����ֶ�����Ӧ���ֶ����(0-based)����δ�ҵ��򷵻�-1.
//-----------------------------------------------------------------------------
int DbFieldDefList::indexOfName(const std::string& name)
{
    int index = -1;

    for (int i = 0; i < items_.getCount(); i++)
    {
        if (sameText(((DbFieldDef*)items_[i])->getName(), name))
        {
            index = i;
            break;
        }
    }

    return index;
}

//-----------------------------------------------------------------------------
// ����: ����ȫ���ֶ���
//-----------------------------------------------------------------------------
void DbFieldDefList::getFieldNameList(StrList& list)
{
    list.clear();
    for (int i = 0; i < items_.getCount(); i++)
        list.add(((DbFieldDef*)items_[i])->getName().c_str());
}

//-----------------------------------------------------------------------------
// ����: �����±�ŷ����ֶζ������ (index: 0-based)
//-----------------------------------------------------------------------------
DbFieldDef* DbFieldDefList::operator[] (int index)
{
    if (index >= 0 && index < items_.getCount())
        return (DbFieldDef*)items_[index];
    else
        return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class DbField

DbField::DbField()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: �����ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
int DbField::asInteger(int defaultVal) const
{
    return strToInt(asString(), defaultVal);
}

//-----------------------------------------------------------------------------
// ����: ��64λ���ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
INT64 DbField::asInt64(INT64 defaultVal) const
{
    return strToInt64(asString(), defaultVal);
}

//-----------------------------------------------------------------------------
// ����: �Ը����ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
double DbField::asFloat(double defaultVal) const
{
    return strToFloat(asString(), defaultVal);
}

//-----------------------------------------------------------------------------
// ����: �Բ����ͷ����ֶ�ֵ (��ת��ʧ���򷵻�ȱʡֵ)
//-----------------------------------------------------------------------------
bool DbField::asBoolean(bool defaultVal) const
{
    return asInteger(defaultVal? 1 : 0) != 0;
}

///////////////////////////////////////////////////////////////////////////////
// class DbFieldList

DbFieldList::DbFieldList()
{
    // nothing
}

DbFieldList::~DbFieldList()
{
    clear();
}

//-----------------------------------------------------------------------------
// ����: ���һ���ֶ����ݶ���
//-----------------------------------------------------------------------------
void DbFieldList::add(DbField *field)
{
    items_.add(field);
}

//-----------------------------------------------------------------------------
// ����: �ͷŲ���������ֶ����ݶ���
//-----------------------------------------------------------------------------
void DbFieldList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (DbField*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// ����: �����±�ŷ����ֶ����ݶ��� (index: 0-based)
//-----------------------------------------------------------------------------
DbField* DbFieldList::operator[] (int index)
{
    if (index >= 0 && index < items_.getCount())
        return (DbField*)items_[index];
    else
        return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class DbParamList

DbParamList::DbParamList(DbQuery *dbQuery) :
    dbQuery_(dbQuery)
{
    // nothing
}

DbParamList::~DbParamList()
{
    clear();
}

//-----------------------------------------------------------------------------
// ����: �������Ʒ��ض�Ӧ�Ĳ������������򷵻�NULL
//-----------------------------------------------------------------------------
DbParam* DbParamList::paramByName(const std::string& name)
{
    DbParam *result = findParam(name);
    if (!result)
    {
        result = createParam(name, 0);
        items_.add(result);
    }

    return result;
}

//-----------------------------------------------------------------------------
// ����: �������(1-based)���ض�Ӧ�Ĳ������������򷵻�NULL
//-----------------------------------------------------------------------------
DbParam* DbParamList::paramByNumber(int number)
{
    DbParam *result = findParam(number);
    if (!result)
    {
        result = createParam("", number);
        items_.add(result);
    }

    return result;
}

//-----------------------------------------------------------------------------
// ����: �ͷŲ���������ֶ����ݶ���
//-----------------------------------------------------------------------------
void DbParamList::clear()
{
    for (int i = 0; i < items_.getCount(); i++)
        delete (DbParam*)items_[i];
    items_.clear();
}

//-----------------------------------------------------------------------------
// ����: ���ݲ����������б��в��Ҳ�������
//-----------------------------------------------------------------------------
DbParam* DbParamList::findParam(const std::string& name)
{
    DbParam *result = NULL;

    for (int i = 0; i < items_.getCount(); i++)
    {
        if (sameText(((DbParam*)items_[i])->name_, name))
        {
            result = (DbParam*)items_[i];
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// ����: ���ݲ���������б��в��Ҳ�������
//-----------------------------------------------------------------------------
DbParam* DbParamList::findParam(int number)
{
    DbParam *result = NULL;

    for (int i = 0; i < items_.getCount(); i++)
    {
        if (((DbParam*)items_[i])->number_ == number)
        {
            result = (DbParam*)items_[i];
            break;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// ����: ����һ���������󲢷���
//-----------------------------------------------------------------------------
DbParam* DbParamList::createParam(const std::string& name, int number)
{
    DbParam *result = dbQuery_->getDatabase()->createDbParam();

    result->dbQuery_ = dbQuery_;
    result->name_ = name;
    result->number_ = number;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class DbDataSet

DbDataSet::DbDataSet(DbQuery *dbQuery) :
    dbQuery_(dbQuery)
{
    // nothing
}

DbDataSet::~DbDataSet()
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ���ݼ��еļ�¼����
//-----------------------------------------------------------------------------
UINT64 DbDataSet::getRecordCount()
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return 0;
}

//-----------------------------------------------------------------------------
// ����: �������ݼ��Ƿ�Ϊ��
//-----------------------------------------------------------------------------
bool DbDataSet::isEmpty()
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return true;
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼�е��ֶ�����
//-----------------------------------------------------------------------------
int DbDataSet::getFieldCount()
{
    return dbFieldDefList_.getCount();
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼��ĳ���ֶεĶ��� (index: 0-based)
//-----------------------------------------------------------------------------
DbFieldDef* DbDataSet::getFieldDefs(int index)
{
    if (index >= 0 && index < dbFieldDefList_.getCount())
        return dbFieldDefList_[index];
    else
        ThrowDbException(SEM_INDEX_ERROR);

    return NULL;
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼��ĳ���ֶε����� (index: 0-based)
//-----------------------------------------------------------------------------
DbField* DbDataSet::getFields(int index)
{
    if (index >= 0 && index < dbFieldList_.getCount())
        return dbFieldList_[index];
    else
        ThrowDbException(SEM_INDEX_ERROR);

    return NULL;
}

//-----------------------------------------------------------------------------
// ����: ȡ�õ�ǰ��¼��ĳ���ֶε�����
// ����:
//   name - �ֶ���
//-----------------------------------------------------------------------------
DbField* DbDataSet::getFields(const std::string& name)
{
    int index = dbFieldDefList_.indexOfName(name);

    if (index >= 0)
        return getFields(index);
    else
    {
        StrList fieldNames;
        dbFieldDefList_.getFieldNameList(fieldNames);
        std::string fieldNameList = fieldNames.getCommaText();

        std::string errMsg = formatString(SEM_FIELD_NAME_ERROR, name.c_str(), fieldNameList.c_str());
        ThrowDbException(errMsg.c_str());
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// class DbQuery

DbQuery::DbQuery(Database *database) :
    database_(database),
    dbConnection_(NULL),
    dbParamList_(NULL)
{
    dbParamList_ = database->createDbParamList(this);
}

DbQuery::~DbQuery()
{
    delete dbParamList_;

    if (dbConnection_)
        database_->getDbConnectionPool()->returnConnection(dbConnection_);
}

//-----------------------------------------------------------------------------
// ����: ����SQL���
//-----------------------------------------------------------------------------
void DbQuery::setSql(const std::string& sql)
{
    sql_ = sql;
    dbParamList_->clear();

    doSetSql(sql);
}

//-----------------------------------------------------------------------------
// ����: ��������ȡ�ò�������
// ��ע:
//   ȱʡ����´˹��ܲ����ã�������Ҫ���ô˹��ܣ��ɵ��ã�
//   return dbParamList_->ParamByName(name);
//-----------------------------------------------------------------------------
DbParam* DbQuery::paramByName(const std::string& name)
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return NULL;
}

//-----------------------------------------------------------------------------
// ����: �������(1-based)ȡ�ò�������
// ��ע:
//   ȱʡ����´˹��ܲ����ã�������Ҫ���ô˹��ܣ��ɵ��ã�
//   return dbParamList_->ParamByNumber(number);
//-----------------------------------------------------------------------------
DbParam* DbQuery::paramByNumber(int number)
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return NULL;
}

//-----------------------------------------------------------------------------
// ����: ִ��SQL (�޷��ؽ��)
//-----------------------------------------------------------------------------
void DbQuery::execute()
{
    ensureConnected();
    doExecute(NULL);
}

//-----------------------------------------------------------------------------
// ����: ִ��SQL (�������ݼ�)
//-----------------------------------------------------------------------------
DbDataSet* DbQuery::query()
{
    ensureConnected();

    DbDataSet *dataSet = database_->createDbDataSet(this);
    try
    {
        // ִ�в�ѯ
        doExecute(dataSet);
        // ��ʼ�����ݼ�
        dataSet->initDataSet();
        // ��ʼ�����ݼ����ֶεĶ���
        dataSet->initFieldDefs();
    }
    catch (Exception&)
    {
        delete dataSet;
        dataSet = NULL;
        throw;
    }

    return dataSet;
}

//-----------------------------------------------------------------------------
// ����: ת���ַ���ʹ֮��SQL�кϷ� (str �пɺ� '\0' �ַ�)
//-----------------------------------------------------------------------------
std::string DbQuery::escapeString(const std::string& str)
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return "";
}

//-----------------------------------------------------------------------------
// ����: ȡ��ִ��SQL����Ӱ�������
//-----------------------------------------------------------------------------
UINT DbQuery::getAffectedRowCount()
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return 0;
}

//-----------------------------------------------------------------------------
// ����: ȡ�����һ��������������ID��ֵ
//-----------------------------------------------------------------------------
UINT64 DbQuery::getLastInsertId()
{
    ThrowDbException(SEM_FEATURE_NOT_SUPPORTED);
    return 0;
}

//-----------------------------------------------------------------------------
// ����: ȡ�ò�ѯ�����õ����ݿ�����
//-----------------------------------------------------------------------------
DbConnection* DbQuery::getDbConnection()
{
    ensureConnected();
    return dbConnection_;
}

//-----------------------------------------------------------------------------

void DbQuery::ensureConnected()
{
    if (!dbConnection_)
        dbConnection_ = database_->getDbConnectionPool()->getConnection();
}

///////////////////////////////////////////////////////////////////////////////
// class Database

Database::Database()
{
    dbConnParams_ = NULL;
    dbOptions_ = NULL;
    dbConnectionPool_ = NULL;
}

Database::~Database()
{
    delete dbConnParams_;
    delete dbOptions_;
    delete dbConnectionPool_;
}

DbConnParams* Database::getDbConnParams()
{
    ensureInited();
    return dbConnParams_;
}

DbOptions* Database::getDbOptions()
{
    ensureInited();
    return dbOptions_;
}

DbConnectionPool* Database::getDbConnectionPool()
{
    ensureInited();
    return dbConnectionPool_;
}

void Database::ensureInited()
{
    if (!dbConnParams_)
    {
        dbConnParams_ = createDbConnParams();
        dbOptions_ = createDbOptions();
        dbConnectionPool_ = createDbConnectionPool();
    }
}