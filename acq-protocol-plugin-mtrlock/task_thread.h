#ifndef __TASK_THREAD_H
#define __TASK_THREAD_H
//#include <thread>
#include <QThread>

/**
 * @brief 任务超时监控线程
 */
class SCN_TaskList;
class TaskThread:public QThread
{
    Q_OBJECT
public:
    explicit TaskThread(SCN_TaskList *pTaskList, QObject *parent = nullptr);
    ~TaskThread();

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
    SCN_TaskList *m_pTaskList;
};

#endif // __TASK_THREAD_H
