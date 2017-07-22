///////////////////////////////////////////////////////////////////////////////
// �ļ�����: BaseApplication.cpp
// ��������: ������Ӧ������Ԫ
///////////////////////////////////////////////////////////////////////////////

#include "BaseApplication.h"
#include "ErrMsgs.h"
#include "LogManager.h"


///////////////////////////////////////////////////////////////////////////////
// ȫ�ֱ�������

#ifdef _COMPILER_LINUX
// ���ڽ����˳�ʱ����ת
static sigjmp_buf procExitJmpBuf;
#endif

// �����ڴ治�������³����˳�
static char *reservedMemoryForExit;

///////////////////////////////////////////////////////////////////////////////
// �źŴ�����

#ifdef _COMPILER_LINUX
//-----------------------------------------------------------------------------
// ����: �����˳����� �źŴ�����
//-----------------------------------------------------------------------------
void exitProgramSignalHandler(BaseApplication* baseApp,int sigNo)
{
	ASSERT_X(baseApp != NULL);

    static bool isInHandler = false;
    if (isInHandler) return;
	if (!baseApp) return;
    isInHandler = true;

	// ֹͣ���߳�ѭ��
	baseApp->setTerminated(true);
	
	INFO_LOG(SEM_SIG_TERM, sigNo);

    siglongjmp(procExitJmpBuf, 1);
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ����� �źŴ�����
//-----------------------------------------------------------------------------
void fatalErrorSignalHandler(BaseApplication* baseApp, int sigNo)
{
	ASSERT_X(baseApp != NULL);

    static bool isInHandler = false;
    if (isInHandler) return;
	if (!baseApp) return;
    isInHandler = true;
	
    // ֹͣ���߳�ѭ��
	baseApp->setTerminated(true);

	INFO_LOG(SEM_SIG_FATAL_ERROR, sigNo);
    abort();
}
#endif

//-----------------------------------------------------------------------------
// ����: �û��źŴ�����
//-----------------------------------------------------------------------------
void userSignalHandler(BaseApplication* baseApp, int sigNo)
{
	ASSERT_X(baseApp != NULL);
	if (!baseApp) return;

    const CallbackList<UserSignalHandlerCallback>& callbackList = baseApp->onUserSignal_;

    for (int i = 0; i < callbackList.getCount(); i++)
    {
        const UserSignalHandlerCallback& callback = callbackList.getItem(i);
        if (callback)
            callback(sigNo);
    }
}

//-----------------------------------------------------------------------------
// ����: �ڴ治���������
// ��ע:
//   ��δ��װ��������(set_new_handler)���� new ����ʧ��ʱ�׳� bad_alloc �쳣��
//   ����װ����������new �����������׳��쳣�����ǵ��ô�����������
//-----------------------------------------------------------------------------
void outOfMemoryHandler()
{
    static bool isInHandler = false;
    if (isInHandler) return;
    isInHandler = true;

    // �ͷű����ڴ棬��������˳��������ٴγ����ڴ治��
    delete[] reservedMemoryForExit;
    reservedMemoryForExit = NULL;

	ERROR_LOG(SEM_OUT_OF_MEMORY);
    abort();
}

///////////////////////////////////////////////////////////////////////////////
// class BaseApplication

BaseApplication::BaseApplication() :
    appStartTime_(time(NULL)),
    initialized_(false),
    terminated_(false)
{
  
}

BaseApplication::~BaseApplication()
{
    Finalize();
}

//-----------------------------------------------------------------------------
// ����: ���������в���
// ����:
//   true  - ��������ִ��
//   false - ����Ӧ�˳� (���������в�������ȷ)
//-----------------------------------------------------------------------------
bool BaseApplication::parseArguments(int argc, char *argv[])
{
    // �ȼ�¼�����в���
    argList_.clear();
    for (int i = 1; i < argc; i++)
        argList_.add(argv[i]);

	return true;
}

//-----------------------------------------------------------------------------
// ����: Ӧ�ó����ʼ�� (����ʼ��ʧ�����׳��쳣)
//-----------------------------------------------------------------------------
void BaseApplication::Initialize()
{
    try
    {
#ifdef _COMPILER_LINUX
        // �ڳ�ʼ���׶�Ҫ�����˳��ź�
        SignalMasker sigMasker(true);
        sigMasker.setSignals(1, SIGTERM);
        sigMasker.block();
#endif

        networkInitialize();
        initExeName();
        checkMultiInstance();
        initSignals();
        initNewOperHandler();
		OnInitialize();
        initialized_ = true;
    }
    catch (Exception& e)
    {
        openTerminal();
        doFinalize();
		ThrowException(e.makeLogStr().c_str());
    }
}

//-----------------------------------------------------------------------------
// ����: Ӧ�ó��������
//-----------------------------------------------------------------------------
void BaseApplication::Finalize()
{
    if (initialized_)
    {
        openTerminal();
		doFinalize();
        initialized_ = false;
    }
}

//-----------------------------------------------------------------------------
// ����: ��ʼ����Ӧ�ó���
//-----------------------------------------------------------------------------
void BaseApplication::run()
{
#ifdef _COMPILER_LINUX
    // ���̱���ֹʱ����ת���˴�����������
    if (sigsetjmp(procExitJmpBuf, 0)) return;
#endif

	OnRun();
}

//-----------------------------------------------------------------------------
// ����: ȡ�ÿ�ִ���ļ����ڵ�·��
//-----------------------------------------------------------------------------
std::string BaseApplication::getExePath()
{
    return extractFilePath(exeName_);
}

//-----------------------------------------------------------------------------
// ����: ȡ�������в����ַ��� (index: 0-based)
//-----------------------------------------------------------------------------
std::string BaseApplication::getArgString(int index)
{
    if (index >= 0 && index < (int)argList_.getCount())
        return argList_[index];
    else
        return "";
}

//-----------------------------------------------------------------------------
// ����: ע���û��źŴ�����
//-----------------------------------------------------------------------------
void BaseApplication::registerUserSignalHandler(const UserSignalHandlerCallback& callback)
{
    onUserSignal_.registerCallback(callback);
}

//-----------------------------------------------------------------------------
// ����: ����Ƿ������˶������ʵ��
//-----------------------------------------------------------------------------
void BaseApplication::checkMultiInstance()
{
    if (multInstance_) return;

#ifdef _COMPILER_WIN
    CreateMutexA(NULL, false, getExeName().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        ThrowException(SEM_ALREADY_RUNNING);
#endif
#ifdef _COMPILER_LINUX
    umask(0);
    int fd = open(getExeName().c_str(), O_RDONLY, 0666);
    if (fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) != 0)
        ThrowException(SEM_ALREADY_RUNNING);
#endif
}

//-----------------------------------------------------------------------------
// ����: ȡ�������ļ���ȫ��������ʼ�� exeName_
//-----------------------------------------------------------------------------
void BaseApplication::initExeName()
{
    exeName_ = getAppExeName();
}


//-----------------------------------------------------------------------------
// ����: ��ʼ���ź� (�źŵİ�װ�����Ե�)
//
//  �ź�����    ֵ                         �ź�˵��
// ---------  ----  -----------------------------------------------------------
// # SIGHUP    1    ���ź����û��ն�����(�����������)����ʱ������ͨ�������ն˵Ŀ��ƽ���
//                  ����ʱ��֪ͨͬһ session �ڵĸ�����ҵ����ʱ����������ն˲��ٹ�����
// # SIGINT    2    ������ֹ(interrupt)�źţ����û�����INTR�ַ�(ͨ����Ctrl-C)ʱ������
// # SIGQUIT   3    �� SIGINT ���ƣ�����QUIT�ַ� (ͨ���� Ctrl-\) �����ơ����������յ�
//                  ���ź��˳�ʱ�����core�ļ��������������������һ����������źš�
// # SIGILL    4    ִ���˷Ƿ�ָ�ͨ������Ϊ��ִ���ļ�������ִ��󣬻�����ͼִ�����ݶΡ�
//                  ��ջ���ʱҲ�п��ܲ�������źš�
// # SIGTRAP   5    �ɶϵ�ָ������� trap ָ��������� debugger ʹ�á�
// # SIGABRT   6    �����Լ����ִ��󲢵��� abort ʱ������
// # SIGIOT    6    ��PDP-11����iotָ������������������Ϻ� SIGABRT һ����
// # SIGBUS    7    �Ƿ���ַ�������ڴ��ַ����(alignment)����eg: ����һ���ĸ��ֳ���
//                  �����������ַ���� 4 �ı�����
// # SIGFPE    8    �ڷ��������������������ʱ������������������������󣬻������������
//                  ��Ϊ 0 ���������е������Ĵ���
// # SIGKILL   9    ��������������������С����źŲ��ܱ�����������ͺ��ԡ�
// # SIGUSR1   10   �����û�ʹ�á�
// # SIGSEGV   11   ��ͼ����δ������Լ����ڴ棬����ͼ��û��дȨ�޵��ڴ��ַд���ݡ�
// # SIGUSR2   12   �����û�ʹ�á�
// # SIGPIPE   13   �ܵ�����(broken pipe)��дһ��û�ж��˿ڵĹܵ���
// # SIGALRM   14   ʱ�Ӷ�ʱ�źţ��������ʵ�ʵ�ʱ���ʱ��ʱ�䡣alarm ����ʹ�ø��źš�
// # SIGTERM   15   �������(terminate)�źţ��� SIGKILL ��ͬ���Ǹ��źſ��Ա������ʹ���
//                  ͨ������Ҫ������Լ������˳���shell ���� kill ȱʡ��������źš�
// # SIGSTKFLT 16   Э��������ջ����(stack fault)��
// # SIGCHLD   17   �ӽ��̽���ʱ�������̻��յ�����źš�
// # SIGCONT   18   ��һ��ֹͣ(stopped)�Ľ��̼���ִ�С����źŲ��ܱ�������������һ��
//                  handler ���ó������� stopped ״̬��Ϊ����ִ��ʱ����ض��Ĺ���������
//                  ������ʾ��ʾ����
// # SIGSTOP   19   ֹͣ(stopped)���̵�ִ�С�ע������terminate�Լ�interrupt������:
//                  �ý��̻�δ������ֻ����ִͣ�С����źŲ��ܱ��������������ԡ�
// # SIGTSTP   20   ֹͣ���̵����У������źſ��Ա�����ͺ��ԡ��û�����SUSP�ַ�ʱ(ͨ����^Z)
//                  ��������źš�
// # SIGTTIN   21   ����̨��ҵҪ���û��ն˶�����ʱ������ҵ�е����н��̻��յ����źš�ȱʡʱ
//                  ��Щ���̻�ִֹͣ�С�
// # SIGTTOU   22   ������SIGTTIN������д�ն�(���޸��ն�ģʽ)ʱ�յ���
// # SIGURG    23   �� "����" ���ݻ����(out-of-band) ���ݵ��� socket ʱ������
// # SIGXCPU   24   ����CPUʱ����Դ���ơ�������ƿ�����getrlimit/setrlimit����ȡ�͸ı䡣
// # SIGXFSZ   25   �����ļ���С��Դ���ơ�
// # SIGVTALRM 26   ����ʱ���źš������� SIGALRM�����Ǽ�����Ǹý���ռ�õ�CPUʱ�䡣
// # SIGPROF   27   ������SIGALRM/SIGVTALRM���������ý����õ�CPUʱ���Լ�ϵͳ���õ�ʱ�䡣
// # SIGWINCH  28   �ն��Ӵ��ĸı�ʱ������
// # SIGIO     29   �ļ�������׼�����������Կ�ʼ��������/���������
// # SIGPWR    30   Power failure.
// # SIGSYS    31   �Ƿ���ϵͳ���á�
//-----------------------------------------------------------------------------
void BaseApplication::initSignals()
{
#ifdef _COMPILER_WIN
#endif
#ifdef _COMPILER_LINUX
    UINT i;

    // ����ĳЩ�ź�
    int ignoreSignals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTSTP, SIGTTIN,
        SIGTTOU, SIGXCPU, SIGCHLD, SIGPWR, SIGALRM, SIGVTALRM, SIGIO};
    for (i = 0; i < sizeof(ignoreSignals)/sizeof(int); i++)
        signal(ignoreSignals[i], SIG_IGN);

    // ��װ�����Ƿ������źŴ�����
    int fatalSignals[] = {SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGPROF, SIGSYS};
    for (i = 0; i < sizeof(fatalSignals)/sizeof(int); i++)
        signal(fatalSignals[i], fatalErrorSignalHandler);

    // ��װ�����˳��źŴ�����
    int exitSignals[] = {SIGTERM/*, SIGABRT*/};
    for (i = 0; i < sizeof(exitSignals)/sizeof(int); i++)
        signal(exitSignals[i], exitProgramSignalHandler);

    // ��װ�û��źŴ�����
    int userSignals[] = {SIGUSR1, SIGUSR2};
    for (i = 0; i < sizeof(userSignals)/sizeof(int); i++)
        signal(userSignals[i], userSignalHandler);
#endif
}

//-----------------------------------------------------------------------------
// ����: ��ʼ�� new �������Ĵ�������
//-----------------------------------------------------------------------------
void BaseApplication::initNewOperHandler()
{
    const int RESERVED_MEM_SIZE = 1024*1024*2;     // 2M

    std::set_new_handler(outOfMemoryHandler);

    // �����ڴ治�������³����˳�
    reservedMemoryForExit = new char[RESERVED_MEM_SIZE];
}

//-----------------------------------------------------------------------------

#ifdef _COMPILER_LINUX
static int s_oldStdInFd = -1;
static int s_oldStdOutFd = -1;
static int s_oldStdErrFd = -1;
#endif

//-----------------------------------------------------------------------------
// ����: ���ն�
//-----------------------------------------------------------------------------
void BaseApplication::openTerminal()
{
#ifdef _COMPILER_LINUX
    if (s_oldStdInFd != -1)
        dup2(s_oldStdInFd, 0);

    if (s_oldStdOutFd != -1)
        dup2(s_oldStdOutFd, 1);

    if (s_oldStdErrFd != -1)
        dup2(s_oldStdErrFd, 2);
#endif
}

//-----------------------------------------------------------------------------
// ����: �ر��ն�
//-----------------------------------------------------------------------------
void BaseApplication::closeTerminal()
{
#ifdef _COMPILER_LINUX
    s_oldStdInFd = dup(0);
    s_oldStdOutFd = dup(1);
    s_oldStdErrFd = dup(2);

    int fd = open("/dev/null", O_RDWR);
    if (fd != -1)
    {
        dup2(fd, 0);  // stdin
        dup2(fd, 1);  // stdout
        dup2(fd, 2);  // stderr

        if (fd > 2)
            close(fd);
    }
#endif
}

//-----------------------------------------------------------------------------
// ����: Ӧ�ó�������� (����� initialized_ ��־)
//-----------------------------------------------------------------------------
void BaseApplication::doFinalize()
{
	try { OnRelease(); } catch (...) {}
    try { networkFinalize(); } catch (...) {}
}
