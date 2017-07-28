///////////////////////////////////////////////////////////////////////////////
// CDataBase.h
// Classes:
//   * DbConnParams          - ���ݿ����Ӳ�����
//   * DbOptions             - ���ݿ����ò�����
//   * DbConnection          - ���ݿ����ӻ���
//   * DbConnectionPool      - ���ݿ����ӳػ���
//   * DbFieldDef            - �ֶζ�����
//   * DbFieldDefList        - �ֶζ����б���
//   * DbField               - �ֶ�������
//   * DbFieldList           - �ֶ������б���
//   * DbParam               - SQL������
//   * DbParamList           - SQL�����б���
//   * DbDataSet             - ���ݼ���
//   * DbQuery               - ���ݲ�ѯ����
//   * DbQueryWrapper        - ���ݲ�ѯ����װ��
//   * DbDataSetWrapper      - ���ݼ���װ��
//   * Database              - ���ݿ���
///////////////////////////////////////////////////////////////////////////////

#ifndef __DATABASE_H_
#define __DATABASE_H_

#include "Options.h"
#include "GlobalDefs.h"
#include "UtilClass.h"
#include "BaseSocket.h"
#include "Exceptions.h"


///////////////////////////////////////////////////////////////////////////////
// ��ǰ����

class DbConnParams;
class DbOptions;
class DbConnection;
class DbConnectionPool;
class DbFieldDef;
class DbFieldDefList;
class DbField;
class DbFieldList;
class DbParam;
class DbParamList;
class DbDataSet;
class DbQuery;
class DbQueryWrapper;
class DbDataSetWrapper;
class Database;

///////////////////////////////////////////////////////////////////////////////
// class DbConnParams - ���ݿ����Ӳ�����

class DbConnParams
{
public:
    DbConnParams();
    DbConnParams(const DbConnParams& src);
    DbConnParams(const std::string& hostName, const std::string& userName,
        const std::string& password, const std::string& dbName, const int port);

    std::string getHostName() const { return hostName_; }
    std::string getUserName() const { return userName_; }
    std::string getPassword() const { return password_; }
    std::string getDbName() const { return dbName_; }
    int getPort() const { return port_; }

    void setHostName(const std::string& value) { hostName_ = value; }
    void setUserName(const std::string& value) { userName_ = value; }
    void setPassword(const std::string& value) { password_ = value; }
    void setDbName(const std::string& value) { dbName_ = value; }
    void setPort(const int value) { port_ = value; }

private:
    std::string hostName_;      // ������ַ
    std::string userName_;      // �û���
    std::string password_;      // �û�����
    std::string dbName_;        // ���ݿ���
    int port_;             // ���Ӷ˿ں�
};

///////////////////////////////////////////////////////////////////////////////
// class DbOptions - ���ݿ����ò�����

class DbOptions
{
public:
    enum { DEF_MAX_DB_CONNECTIONS = 100 };      // ���ӳ����������ȱʡֵ

public:
    DbOptions();

    int getMaxDbConnections() const { return maxDbConnections_; }
    StrList& initialSqlCmdList() { return initialSqlCmdList_; }
    std::string getInitialCharSet() { return initialCharSet_; }

    void setMaxDbConnections(int value);
    void setInitialSqlCmd(const std::string& value);
    void setInitialCharSet(const std::string& value);

private:
    int maxDbConnections_;                 // ���ӳ�����������������
    StrList initialSqlCmdList_;            // ���ݿ�ս�������ʱҪִ�е�����
    std::string initialCharSet_;                // ���ݿ�ս�������ʱҪ���õ��ַ���
};

///////////////////////////////////////////////////////////////////////////////
// class DbConnection - ���ݿ����ӻ���

class DbConnection : noncopyable
{
public:
    friend class DbConnectionPool;

public:
    DbConnection(Database *database);
    virtual ~DbConnection();

    // �������ݿ�����
    void activateConnection(bool force = false);

protected:
    // �����ݿ⽨������(��ʧ�����׳��쳣)
    virtual void doConnect() {}
    // �����ݿ�Ͽ�����
    virtual void doDisconnect() {}

protected:
    // �������ݿ����Ӳ������������ (��ʧ�����׳��쳣)
    void connect();
    // �Ͽ����ݿ����Ӳ������������
    void disconnect();
    // �ս�������ʱִ������
    void execCmdOnConnected();

    // ConnectionPool ���������к������������ӵ�ʹ�����
    bool getDbConnection();         // ��������
    void returnDbConnection();      // �黹����
    bool isBusy();                  // �����Ƿ񱻽���

protected:
    Database *database_;            // Database ���������
    bool isConnected_;              // �Ƿ��ѽ�������
    bool isBusy_;                   // �����ӵ�ǰ�Ƿ�����ռ��
};

///////////////////////////////////////////////////////////////////////////////
// class DbConnectionPool - ���ݿ����ӳػ���
//
// ����ԭ��:
// 1. ����ά��һ�������б�����ǰ����������(�������ӡ�æ����)����ʼΪ�գ�����һ���������ޡ�
// 2. ��������:
//    ���ȳ��Դ������б����ҳ�һ���������ӣ����ҵ������ɹ���ͬʱ��������Ϊæ״̬����û�ҵ�
//    ��: (1)��������δ�ﵽ���ޣ��򴴽�һ���µĿ������ӣ�(2)���������Ѵﵽ���ޣ������ʧ�ܡ�
//    ������ʧ��(�������������޷����������ӵ�)�����׳��쳣��
// 3. �黹����:
//    ֻ�轫������Ϊ����״̬���ɣ�����(Ҳ����)�Ͽ����ݿ����ӡ�

class DbConnectionPool : noncopyable
{
public:
    DbConnectionPool(Database *database);
    virtual ~DbConnectionPool();

    // ����һ�����õĿ������� (��ʧ�����׳��쳣)
    virtual DbConnection* getConnection();
    // �黹���ݿ�����
    virtual void returnConnection(DbConnection *dbConnection);

protected:
    void clearPool();

protected:
    Database *database_;            // ���� Database ����
    PointerList dbConnectionList_;  // ��ǰ�����б� (DbConnection*[])�������������Ӻ�æ����
    Mutex mutex_;                   // ������
};

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDef - �ֶζ�����

class DbFieldDef
{
public:
    DbFieldDef() {}
    DbFieldDef(const std::string& name, int type);
    DbFieldDef(const DbFieldDef& src);
    virtual ~DbFieldDef() {}

    void setData(char *name, int type) { name_ = name; type_ = type; }

    std::string getName() const { return name_; }
    int getType() const { return type_; }

protected:
    std::string name_;          // �ֶ�����
    int type_;             // �ֶ�����(���������ඨ��)
};

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDefList - �ֶζ����б���

class DbFieldDefList
{
public:
    DbFieldDefList();
    virtual ~DbFieldDefList();

    // ���һ���ֶζ������
    void add(DbFieldDef *fieldDef);
    // �ͷŲ���������ֶζ������
    void clear();
    // �����ֶ�����Ӧ���ֶ����(0-based)
    int indexOfName(const std::string& name);
    // ����ȫ���ֶ���
    void getFieldNameList(StrList& list);

    DbFieldDef* operator[] (int index);
    int getCount() const { return items_.getCount(); }

private:
    PointerList items_;                  // (DbFieldDef* [])
};

///////////////////////////////////////////////////////////////////////////////
// class DbField - �ֶ�������

class DbField
{
public:
    DbField();
    virtual ~DbField() {}

    virtual bool isNull() const { return false; }
    virtual int asInteger(int defaultVal = 0) const;
    virtual INT64 asInt64(INT64 defaultVal = 0) const;
    virtual double asFloat(double defaultVal = 0) const;
    virtual bool asBoolean(bool defaultVal = false) const;
    virtual std::string asString() const { return ""; };
};

///////////////////////////////////////////////////////////////////////////////
// class DbFieldList - �ֶ������б���

class DbFieldList
{
public:
    DbFieldList();
    virtual ~DbFieldList();

    // ���һ���ֶ����ݶ���
    void add(DbField *field);
    // �ͷŲ���������ֶ����ݶ���
    void clear();

    DbField* operator[] (int index);
    int getCount() const { return items_.getCount(); }

private:
    PointerList items_;       // (DbField* [])
};

///////////////////////////////////////////////////////////////////////////////
// class DbParam - SQL������

class DbParam
{
public:
    friend class DbParamList;

public:
    DbParam() : dbQuery_(NULL), number_(0) {}
    virtual ~DbParam() {}

    virtual void setInt(int value) {}
    virtual void setFloat(double value) {}
    virtual void setString(const std::string& value) {}

protected:
    DbQuery *dbQuery_;   // DbQuery ��������
    std::string name_;        // ��������
    int number_;         // �������(1-based)
};

///////////////////////////////////////////////////////////////////////////////
// class DbParamList - SQL�����б���

class DbParamList
{
public:
    DbParamList(DbQuery *dbQuery);
    virtual ~DbParamList();

    virtual DbParam* paramByName(const std::string& name);
    virtual DbParam* paramByNumber(int number);

    void clear();

protected:
    virtual DbParam* findParam(const std::string& name);
    virtual DbParam* findParam(int number);
    virtual DbParam* createParam(const std::string& name, int number);

protected:
    DbQuery *dbQuery_;       // DbQuery ��������
    PointerList items_;      // (DbParam* [])
};

///////////////////////////////////////////////////////////////////////////////
// class DbDataSet - ���ݼ���
//
// ˵��:
// 1. ����ֻ�ṩ����������ݼ��Ĺ��ܡ�
// 2. ���ݼ���ʼ��(InitDataSet)���α�ָ���һ����¼֮ǰ������� Next() ��ָ���һ����¼��
//    ���͵����ݼ���������Ϊ: while(DataSet.Next()) { ... }
//
// Լ��:
// 1. DbQuery.Query() ����һ���µ� DbDataSet ����֮����������� DbDataSet ����
//    �������� DbQuery ����
// 2. DbQuery ��ִ�в�ѯA�󴴽���һ�����ݼ�A��֮����ִ�в�ѯBǰӦ�ر����ݼ�A��

class DbDataSet : noncopyable
{
public:
    friend class DbQuery;

public:
    DbDataSet(DbQuery *dbQuery);
    virtual ~DbDataSet();

    // ���α�ָ����ʼλ��(��һ����¼֮ǰ)
    virtual bool rewind() = 0;
    // ���α�ָ����һ����¼
    virtual bool next() = 0;

    // ȡ�õ�ǰ���ݼ��еļ�¼����
    virtual UINT64 getRecordCount();
    // �������ݼ��Ƿ�Ϊ��
    virtual bool isEmpty();

    // ȡ�õ�ǰ��¼�е��ֶ�����
    int getFieldCount();
    // ȡ�õ�ǰ��¼��ĳ���ֶεĶ��� (index: 0-based)
    DbFieldDef* getFieldDefs(int index);
    // ȡ�õ�ǰ��¼��ĳ���ֶε����� (index: 0-based)
    DbField* getFields(int index);
    DbField* getFields(const std::string& name);

protected:
    // ��ʼ�����ݼ� (��ʧ�����׳��쳣)
    virtual void initDataSet() = 0;
    // ��ʼ�����ݼ����ֶεĶ���
    virtual void initFieldDefs() = 0;

protected:
    DbQuery *dbQuery_;              // DbQuery ��������
    DbFieldDefList dbFieldDefList_; // �ֶζ�������б�
    DbFieldList dbFieldList_;       // �ֶ����ݶ����б�
};

///////////////////////////////////////////////////////////////////////////////
// class DbQuery - ���ݲ�ѯ����
//
// ����ԭ��:
// 1. ִ��SQL: �����ӳ�ȡ��һ���������ӣ�Ȼ�����ô�����ִ��SQL�����黹���ӡ�

class DbQuery : noncopyable
{
public:
    friend class DbConnection;

public:
    DbQuery(Database *database);
    virtual ~DbQuery();

    // ����SQL���
    void setSql(const std::string& sql);

    // ��������ȡ�ò�������
    virtual DbParam* paramByName(const std::string& name);
    // �������(1-based)ȡ�ò�������
    virtual DbParam* paramByNumber(int number);

    // ִ��SQL (�޷��ؽ��, ��ʧ�����׳��쳣)
    void execute();
    // ִ��SQL (�������ݼ�, ��ʧ�����׳��쳣)
    DbDataSet* query();

    // ת���ַ���ʹ֮��SQL�кϷ� (str �пɺ� '\0' �ַ�)
    virtual std::string escapeString(const std::string& str);
    // ȡ��ִ��SQL����Ӱ�������
    virtual UINT getAffectedRowCount();
    // ȡ�����һ��������������ID��ֵ
    virtual UINT64 getLastInsertId();

    // ȡ�ò�ѯ�����õ����ݿ�����
    DbConnection* getDbConnection();
    // ȡ�� Database ����
    Database* getDatabase() { return database_; }

protected:
    // ����SQL���
    virtual void doSetSql(const std::string& sql) {}
    // ִ��SQL (�� resultDataSet Ϊ NULL�����ʾ�����ݼ����ء���ʧ�����׳��쳣)
    virtual void doExecute(DbDataSet *resultDataSet) {}

protected:
    void ensureConnected();

protected:
    Database *database_;           // Database ��������
    DbConnection *dbConnection_;   // DbConnection ��������
    DbParamList *dbParamList_;     // SQL �����б�
    std::string sql_;                   // ��ִ�е�SQL���
};

///////////////////////////////////////////////////////////////////////////////
// class DbQueryWrapper - ��ѯ����װ��
//
// ˵��: �������ڰ�װ DbQuery �����Զ��ͷű���װ�Ķ��󣬷�ֹ��Դй©��
// ʾ��:
//      int main()
//      {
//          DbQueryWrapper query( MyDatabase.createDbQuery() );
//          query->setSql("select * from users");
//          /* ... */
//          // ջ���� query ���Զ����٣����ͬʱ����װ�ĶѶ���Ҳ�Զ��ͷš�
//      }

class DbQueryWrapper
{
public:
    DbQueryWrapper(DbQuery *dbQuery) : dbQuery_(dbQuery) {}
    virtual ~DbQueryWrapper() { delete dbQuery_; }

    DbQuery* operator-> () { return dbQuery_; }

private:
    DbQuery *dbQuery_;
};

///////////////////////////////////////////////////////////////////////////////
// class DbDataSetWrapper - ���ݼ���װ��
//
// ˵��:
// 1. �������ڰ�װ DbDataSet �����Զ��ͷű���װ�Ķ��󣬷�ֹ��Դй©��
// 2. ÿ�θ���װ����ֵ(Wrapper = DataSet)���ϴα���װ�Ķ����Զ��ͷš�
// ʾ��:
//      int main()
//      {
//          DbQueryWrapper query( MyDatabase.CreateDbQuery() );
//          DbDataSetWrapper ds;
//
//          query->setSql("select * from users");
//          ds = query->query();
//          /* ... */
//
//          // ջ���� query �� ds ���Զ����٣����ͬʱ����װ�ĶѶ���Ҳ�Զ��ͷš�
//      }

class DbDataSetWrapper
{
public:
    DbDataSetWrapper() : dbDataSet_(NULL) {}
    virtual ~DbDataSetWrapper() { delete dbDataSet_; }

    DbDataSetWrapper& operator = (DbDataSet *dataSet)
    {
        delete dbDataSet_;
        dbDataSet_ = dataSet;
        return *this;
    }

    DbDataSet* operator-> () { return dbDataSet_; }

private:
    DbDataSet *dbDataSet_;
};

///////////////////////////////////////////////////////////////////////////////
// class Database - ���ݿ���

class Database : noncopyable
{
public:
    Database();
    virtual ~Database();

    // �๤������:
    virtual DbConnParams* createDbConnParams() { return new DbConnParams(); }
    virtual DbOptions* createDbOptions() { return new DbOptions(); }
    virtual DbConnection* createDbConnection() = 0;
    virtual DbConnectionPool* createDbConnectionPool() { return new DbConnectionPool(this); }
    virtual DbParam* createDbParam() { return new DbParam(); }
    virtual DbParamList* createDbParamList(DbQuery* dbQuery) { return new DbParamList(dbQuery); }
    virtual DbDataSet* createDbDataSet(DbQuery* dbQuery) = 0;
    virtual DbQuery* createDbQuery() = 0;

    DbConnParams* getDbConnParams();
    DbOptions* getDbOptions();
    DbConnectionPool* getDbConnectionPool();

private:
    void ensureInited();

protected:
    DbConnParams *dbConnParams_;             // ���ݿ����Ӳ���
    DbOptions *dbOptions_;                   // ���ݿ����ò���
    DbConnectionPool *dbConnectionPool_;     // ���ݿ����ӳ�
};

#endif // _DATABASE_H_
