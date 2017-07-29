///////////////////////////////////////////////////////////////////////////////
// CDataBase.h
// Classes:
//   * DbConnParams          - 数据库连接参数类
//   * DbOptions             - 数据库配置参数类
//   * DbConnection          - 数据库连接基类
//   * DbConnectionPool      - 数据库连接池基类
//   * DbFieldDef            - 字段定义类
//   * DbFieldDefList        - 字段定义列表类
//   * DbField               - 字段数据类
//   * DbFieldList           - 字段数据列表类
//   * DbParam               - SQL参数类
//   * DbParamList           - SQL参数列表类
//   * DbDataSet             - 数据集类
//   * DbQuery               - 数据查询器类
//   * DbQueryWrapper        - 数据查询器包装类
//   * DbDataSetWrapper      - 数据集包装类
//   * Database              - 数据库类
///////////////////////////////////////////////////////////////////////////////

#ifndef __DATABASE_H_
#define __DATABASE_H_

#include "Options.h"
#include "GlobalDefs.h"
#include "UtilClass.h"
#include "BaseSocket.h"
#include "Exceptions.h"


///////////////////////////////////////////////////////////////////////////////
// 提前声明

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
// class DbConnParams - 数据库连接参数类

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
    std::string hostName_;      // 主机地址
    std::string userName_;      // 用户名
    std::string password_;      // 用户口令
    std::string dbName_;        // 数据库名
    int port_;             // 连接端口号
};

///////////////////////////////////////////////////////////////////////////////
// class DbOptions - 数据库配置参数类

class DbOptions
{
public:
    enum { DEF_MAX_DB_CONNECTIONS = 100 };      // 连接池最大连接数缺省值

public:
    DbOptions();

    int getMaxDbConnections() const { return maxDbConnections_; }
    StrList& initialSqlCmdList() { return initialSqlCmdList_; }
    std::string getInitialCharSet() { return initialCharSet_; }

    void setMaxDbConnections(int value);
    void setInitialSqlCmd(const std::string& value);
    void setInitialCharSet(const std::string& value);

private:
    int maxDbConnections_;                 // 连接池所允许的最大连接数
    StrList initialSqlCmdList_;            // 数据库刚建立连接时要执行的命令
    std::string initialCharSet_;                // 数据库刚建立连接时要设置的字符集
};

///////////////////////////////////////////////////////////////////////////////
// class DbConnection - 数据库连接基类

class DbConnection : noncopyable
{
public:
    friend class DbConnectionPool;

public:
    DbConnection(Database *database);
    virtual ~DbConnection();

    // 激活数据库连接
    void activateConnection(bool force = false);

protected:
    // 和数据库建立连接(若失败则抛出异常)
    virtual void doConnect() {}
    // 和数据库断开连接
    virtual void doDisconnect() {}

protected:
    // 建立数据库连接并进行相关设置 (若失败则抛出异常)
    void connect();
    // 断开数据库连接并进行相关设置
    void disconnect();
    // 刚建立连接时执行命令
    void execCmdOnConnected();

    // ConnectionPool 将调用下列函数，控制连接的使用情况
    bool getDbConnection();         // 借用连接
    void returnDbConnection();      // 归还连接
    bool isBusy();                  // 连接是否被借用

protected:
    Database *database_;            // Database 对象的引用
    bool isConnected_;              // 是否已建立连接
    bool isBusy_;                   // 此连接当前是否正被占用
};

///////////////////////////////////////////////////////////////////////////////
// class DbConnectionPool - 数据库连接池基类
//
// 工作原理:
// 1. 类中维护一个连接列表，管理当前的所有连接(空闲连接、忙连接)，初始为空，且有一个数量上限。
// 2. 分配连接:
//    首先尝试从连接列表中找出一个空闲连接，若找到则分配成功，同时将连接置为忙状态；若没找到
//    则: (1)若连接数未达到上限，则创建一个新的空闲连接；(2)若连接数已达到上限，则分配失败。
//    若分配失败(连接数已满、无法建立新连接等)，则抛出异常。
// 3. 归还连接:
//    只需将连接置为空闲状态即可，无需(也不可)断开数据库连接。

class DbConnectionPool : noncopyable
{
public:
    DbConnectionPool(Database *database);
    virtual ~DbConnectionPool();

    // 分配一个可用的空闲连接 (若失败则抛出异常)
    virtual DbConnection* getConnection();
    // 归还数据库连接
    virtual void returnConnection(DbConnection *dbConnection);

protected:
    void clearPool();

protected:
    Database *database_;            // 所属 Database 引用
    PointerList dbConnectionList_;  // 当前连接列表 (DbConnection*[])，包含空闲连接和忙连接
    Mutex mutex_;                   // 互斥锁
};

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDef - 字段定义类

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
    std::string name_;          // 字段名称
    int type_;             // 字段类型(含义由子类定义)
};

///////////////////////////////////////////////////////////////////////////////
// class DbFieldDefList - 字段定义列表类

class DbFieldDefList
{
public:
    DbFieldDefList();
    virtual ~DbFieldDefList();

    // 添加一个字段定义对象
    void add(DbFieldDef *fieldDef);
    // 释放并清空所有字段定义对象
    void clear();
    // 返回字段名对应的字段序号(0-based)
    int indexOfName(const std::string& name);
    // 返回全部字段名
    void getFieldNameList(StrList& list);

    DbFieldDef* operator[] (int index);
    int getCount() const { return items_.getCount(); }

private:
    PointerList items_;                  // (DbFieldDef* [])
};

///////////////////////////////////////////////////////////////////////////////
// class DbField - 字段数据类

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
// class DbFieldList - 字段数据列表类

class DbFieldList
{
public:
    DbFieldList();
    virtual ~DbFieldList();

    // 添加一个字段数据对象
    void add(DbField *field);
    // 释放并清空所有字段数据对象
    void clear();

    DbField* operator[] (int index);
    int getCount() const { return items_.getCount(); }

private:
    PointerList items_;       // (DbField* [])
};

///////////////////////////////////////////////////////////////////////////////
// class DbParam - SQL参数类

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
    DbQuery *dbQuery_;   // DbQuery 对象引用
    std::string name_;        // 参数名称
    int number_;         // 参数序号(1-based)
};

///////////////////////////////////////////////////////////////////////////////
// class DbParamList - SQL参数列表类

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
    DbQuery *dbQuery_;       // DbQuery 对象引用
    PointerList items_;      // (DbParam* [])
};

///////////////////////////////////////////////////////////////////////////////
// class DbDataSet - 数据集类
//
// 说明:
// 1. 此类只提供单向遍历数据集的功能。
// 2. 数据集初始化(InitDataSet)后，游标指向第一条记录之前，需调用 Next() 才指向第一条记录。
//    典型的数据集遍历方法为: while(DataSet.Next()) { ... }
//
// 约定:
// 1. DbQuery.Query() 创建一个新的 DbDataSet 对象，之后必须先销毁 DbDataSet 对象，
//    才能销毁 DbQuery 对象。
// 2. DbQuery 在执行查询A后创建了一个数据集A，之后在执行查询B前应关闭数据集A。

class DbDataSet : noncopyable
{
public:
    friend class DbQuery;

public:
    DbDataSet(DbQuery *dbQuery);
    virtual ~DbDataSet();

    // 将游标指向起始位置(第一条记录之前)
    virtual bool rewind() = 0;
    // 将游标指向下一条记录
    virtual bool next() = 0;

    // 取得当前数据集中的记录总数
    virtual UINT64 getRecordCount();
    // 返回数据集是否为空
    virtual bool isEmpty();

    // 取得当前记录中的字段总数
    int getFieldCount();
    // 取得当前记录中某个字段的定义 (index: 0-based)
    DbFieldDef* getFieldDefs(int index);
    // 取得当前记录中某个字段的数据 (index: 0-based)
    DbField* getFields(int index);
    DbField* getFields(const std::string& name);

protected:
    // 初始化数据集 (若失败则抛出异常)
    virtual void initDataSet() = 0;
    // 初始化数据集各字段的定义
    virtual void initFieldDefs() = 0;

protected:
    DbQuery *dbQuery_;              // DbQuery 对象引用
    DbFieldDefList dbFieldDefList_; // 字段定义对象列表
    DbFieldList dbFieldList_;       // 字段数据对象列表
};

///////////////////////////////////////////////////////////////////////////////
// class DbQuery - 数据查询器类
//
// 工作原理:
// 1. 执行SQL: 从连接池取得一个空闲连接，然后利用此连接执行SQL，最后归还连接。

class DbQuery : noncopyable
{
public:
    friend class DbConnection;

public:
    DbQuery(Database *database);
    virtual ~DbQuery();

    // 设置SQL语句
    void setSql(const std::string& sql);

    // 根据名称取得参数对象
    virtual DbParam* paramByName(const std::string& name);
    // 根据序号(1-based)取得参数对象
    virtual DbParam* paramByNumber(int number);

    // 执行SQL (无返回结果, 若失败则抛出异常)
    void execute();
    // 执行SQL (返回数据集, 若失败则抛出异常)
    DbDataSet* query();

    // 转换字符串使之在SQL中合法 (str 中可含 '\0' 字符)
    virtual std::string escapeString(const std::string& str);
    // 取得执行SQL后受影响的行数
    virtual UINT getAffectedRowCount();
    // 取得最后一条插入语句的自增ID的值
    virtual UINT64 getLastInsertId();

    // 取得查询器所用的数据库连接
    DbConnection* getDbConnection();
    // 取得 Database 对象
    Database* getDatabase() { return database_; }

protected:
    // 设置SQL语句
    virtual void doSetSql(const std::string& sql) {}
    // 执行SQL (若 resultDataSet 为 NULL，则表示无数据集返回。若失败则抛出异常)
    virtual void doExecute(DbDataSet *resultDataSet) {}

protected:
    void ensureConnected();

protected:
    Database *database_;           // Database 对象引用
    DbConnection *dbConnection_;   // DbConnection 对象引用
    DbParamList *dbParamList_;     // SQL 参数列表
    std::string sql_;                   // 待执行的SQL语句
};

///////////////////////////////////////////////////////////////////////////////
// class DbQueryWrapper - 查询器包装类
//
// 说明: 此类用于包装 DbQuery 对象，自动释放被包装的对象，防止资源泄漏。
// 示例:
//      int main()
//      {
//          DbQueryWrapper query( MyDatabase.createDbQuery() );
//          query->setSql("select * from users");
//          /* ... */
//          // 栈对象 query 会自动销毁，与此同时被包装的堆对象也自动释放。
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
// class DbDataSetWrapper - 数据集包装类
//
// 说明:
// 1. 此类用于包装 DbDataSet 对象，自动释放被包装的对象，防止资源泄漏。
// 2. 每次给包装器赋值(Wrapper = DataSet)，上次被包装的对象自动释放。
// 示例:
//      int main()
//      {
//          DbQueryWrapper query( MyDatabase.CreateDbQuery() );
//          DbDataSetWrapper ds;
//
//          query->setSql("select * from users");
//          ds = query->query();
//          /* ... */
//
//          // 栈对象 query 和 ds 会自动销毁，与此同时被包装的堆对象也自动释放。
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
// class Database - 数据库类

class Database : noncopyable
{
public:
    Database();
    virtual ~Database();

    // 类工厂方法:
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
    DbConnParams *dbConnParams_;             // 数据库连接参数
    DbOptions *dbOptions_;                   // 数据库配置参数
    DbConnectionPool *dbConnectionPool_;     // 数据库连接池
};

#endif // _DATABASE_H_
