///////////////////////////////////////////////////////////////////////////////
// Application.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _BASE_APPLICATION_H_
#define _BASE_APPLICATION_H_

#include "Options.h"
#include "UtilClass.h"
#include "ServiceThread.h"
#include "SysUtils.h"
#include "BaseSocket.h"
#include "Exceptions.h"
#include "TCPServer.h"
#include "Timers.h"
#include "NonCopyable.h"

#ifdef _COMPILER_WIN
#include <stdarg.h>
#include <windows.h>
#endif

#ifdef _COMPILER_LINUX
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// ���Ͷ���


// �û��źŴ���ص�
typedef std::function<void (int signalNumber)> UserSignalHandlerCallback;

///////////////////////////////////////////////////////////////////////////////
// class BaseApplication - Ӧ�ó��������
//
// ˵��:
// 1. ����������������������ܣ�ȫ�ֵ�������(Application)�ڳ�������ʱ����������
// 2. һ����˵��������������ⲿ��������(kill)���˳��ġ������յ�kill�˳������
//    (��ʱ��ǰִ�е�һ���� App().run() ��)���ᴥ�� exitProgramSignalHandler
//    �źŴ��������������� longjmp ����ʹִ�е�ģ��� run() ���˳����̶�ִ�� finalize��
// 3. �������������ķǷ��������󣬻��ȴ��� fatalErrorSignalHandler �źŴ�������
//    Ȼ��ͬ����������������˳���˳���
// 4. �������ڲ��������˳������Ƽ�ʹ��exit���������� setTeraminted(true).
//    ���������ó���������������˳���˳���

class BaseApplication : noncopyable
{
public:
	BaseApplication();
    virtual ~BaseApplication();
  
	virtual void OnInitialize()  = 0;
	virtual void OnRelease() = 0;
	virtual void OnRun() = 0;

	bool parseArguments(int argc, char *argv[]);
	void Initialize();
	void Finalize();
	//��ѭ��
	void run();

    void setTerminated(bool value) { terminated_ = value; }
    bool isTerminated() { return terminated_; }

	void setMultiInstance(bool value) { multInstance_ = value; };

    // ȡ�ÿ�ִ���ļ���ȫ��(������·��)
    std::string getExeName() { return exeName_; }
    // ȡ�ÿ�ִ���ļ����ڵ�·��
	std::string getExePath();
    // ȡ�������в�������(�׸�����Ϊ����·���ļ���)
    int getArgCount() { return argList_.getCount(); }
    // ȡ�������в����ַ��� (index: 0-based)
	std::string getArgString(int index);
    // ȡ�ó�������ʱ��ʱ��
    time_t getAppStartTime() { return appStartTime_; }

    // ע���û��źŴ�����
    void registerUserSignalHandler(const UserSignalHandlerCallback& callback);

private:
    void checkMultiInstance();
    void initExeName();
    void initSignals();
    void initNewOperHandler();
    void openTerminal();
    void closeTerminal();
    void doFinalize();

private:
    StrList argList_;										// �����в��� (������������ argv[0])
    std::string exeName_;									// ��ִ���ļ���ȫ��(������·��)
    time_t appStartTime_;									// ��������ʱ��ʱ��
    bool initialized_;										// �Ƿ�ɹ���ʼ��
    bool terminated_;										// �Ƿ�Ӧ�˳��ı�־
	bool multInstance_;										// �Ƿ�������ʵ������
    CallbackList<UserSignalHandlerCallback> onUserSignal_;  // �û��źŴ���ص�
	
    friend void userSignalHandler(BaseApplication* baseApp, int sigNo);
};

#endif // _APPLICATION_H_
