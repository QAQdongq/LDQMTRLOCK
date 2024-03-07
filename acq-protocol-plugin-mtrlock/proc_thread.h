#ifndef __PROCTHREAD_H
#define __PROCTHREAD_H
//#include <thread>
#include <QThread>

class Protocol;
/**
 * @brief 规约处理线程,循环处理缓存中的数据
 */
class ProcThread:public QThread
{
    Q_OBJECT
public:
    explicit ProcThread(Protocol *protocolObj, QObject *parent = nullptr);
    ~ProcThread();

    /**
     * @brief 设置退出标志
     */
    void setExitFlag();
protected:
    /**
     * @brief 线程处理
     */
    virtual void run();


private:
    std::atomic<bool> m_exitFlag; //退出线程标志
    Protocol *m_protocolObj = NULL;

protected:
    //std::thread *m_thread = NULL;
};

#endif // PROCTHREAD_H
