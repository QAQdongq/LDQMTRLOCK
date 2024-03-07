
#include "task_thread.h"
#include "iec104/scn_tasklist.h"

//任务处理线程睡眠时间间隔（毫秒）
#define TASK_THREAD_SLEEPTIME    (1000)

TaskThread::TaskThread(SCN_TaskList *pTaskList, QObject *parent): QThread(parent)
{
    this->m_exitFlag.store(false);
    m_pTaskList = pTaskList;
}

TaskThread::~TaskThread()
{
    setExitFlag();
    //LOG_DEBUG(0, "quit end");
}

void TaskThread::run()
{
    while(!this->m_exitFlag.load())
    {
        if(m_pTaskList)
        {
            m_pTaskList->checkTaskTimeout();
        }
        QThread::msleep(TASK_THREAD_SLEEPTIME);
        //std::this_thread::sleep_for(std::chrono::milliseconds(PROC_THREAD_SLEEPTIME));
    }
}

void TaskThread::setExitFlag()
{
    this->m_exitFlag.store(true);
}
