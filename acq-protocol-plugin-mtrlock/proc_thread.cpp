
#include "proc_thread.h"
#include "iec104/protocol.h"

//规约处理线程睡眠时间间隔（毫秒）
#define PROC_THREAD_SLEEPTIME    (10)

ProcThread::ProcThread(Protocol *protocolObj, QObject *parent):m_protocolObj(protocolObj),QThread(parent)
{
    this->m_exitFlag.store(false);
    //this->m_thread = new std::thread(&ProcThread::run, this);
    //this->m_thread->detach();
}

ProcThread::~ProcThread()
{
    setExitFlag();
    /*if(NULL != this->m_thread)
    {
        delete this->m_thread;
        this->m_thread = NULL;
    }*/
    LOG_DEBUG(0, "quit end");
}

void ProcThread::run()
{
    if(NULL == this->m_protocolObj)
    {
        LOG_ERROR(0, "protocol object is null! skip run proc thread!");
        return;
    }
    while(!this->m_exitFlag.load())
    {
        m_protocolObj->RxProc();
        m_protocolObj->TxProc();
        QThread::msleep(PROC_THREAD_SLEEPTIME);
        //std::this_thread::sleep_for(std::chrono::milliseconds(PROC_THREAD_SLEEPTIME));
    }
}

void ProcThread::setExitFlag()
{
    this->m_exitFlag.store(true);
}

