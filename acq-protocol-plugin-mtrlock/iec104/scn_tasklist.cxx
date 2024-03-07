/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_tasklist.cxx
 *生成日期：2011-09-22
 *作者：    cfq
 *功能说明：任务（遥控、设点、总召等命令）队列
 *其它说明：
 *修改记录：date, maintainer
 *
*******************************************************************************/

#include "plugin104_define.h"
#include "scn_tasklist.h"
#include <iostream>
SCN_TaskList::SCN_TaskList()
{
    clear();
}
SCN_TaskList::~SCN_TaskList()
{
    clear();
}

int SCN_TaskList::getTaskCount()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_taskList.size();
}


void SCN_TaskList::addTask(TaskInfo &taskInfo)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    //LOG_DEBUG(m_cchId, taskInfo.toString());
    std::cout << "add a task:"<< taskInfo.toString().toStdString() << std::endl;
    std::shared_ptr<TaskInfo> taskInfoPtr(new TaskInfo(taskInfo));
    m_taskList.append(taskInfoPtr);
}

void SCN_TaskList::clear()
{
    //LOG_DEBUG(m_cchId, "");
    std::cout << "clear all task"<< std::endl;
    std::lock_guard<std::mutex> locker(m_mutex);
    m_taskList.clear();
}

bool SCN_TaskList::exist(const QString &code, int pointNo)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    if( m_taskList.size() == 0)
    {
        return false;
    }
    for(int i=0; i<m_taskList.size();i++)
    {
        if(code == m_taskList.at(i)->code &&
           pointNo == m_taskList.at(i)->pointNo)
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<TaskInfo> SCN_TaskList::getATask(const QString &code, int pointNo)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    if( m_taskList.size() == 0)
    {
        return nullptr;
    }
    for(int i=0; i<m_taskList.size();i++)
    {
        if(code == m_taskList.at(i)->code &&
           pointNo == m_taskList.at(i)->pointNo)
        {
            return m_taskList.at(i);
        }
    }
    return nullptr;
}

void SCN_TaskList::removeATask(const QString &code, int pointNo)
{
    std::lock_guard<std::mutex> locker(m_mutex);
    if( m_taskList.size() == 0)
    {
        return;
    }
    for(int i=0; i<m_taskList.size();i++)
    {
        if(code == m_taskList.at(i)->code &&
           pointNo == m_taskList.at(i)->pointNo)
        {
            m_taskList.removeAt(i);
            std::cout << "remove a task!  code:" << code.toStdString() << ", pointNo:" << pointNo << std::endl;
            return;
        }
    }
}

void SCN_TaskList::checkTaskTimeout()
{
    //std::cout << "SCN_TaskList::checkTaskTimeout "<< std::endl;
    std::lock_guard<std::mutex> locker(m_mutex);
    if( m_taskList.size() == 0)
    {
        return;
    }
    qint64 curTime = QDateTime::currentMSecsSinceEpoch();//ms
    //std::cout << "SCN_TaskList::checkTaskTimeout, curTime:" << curTime << std::endl;
    QList<std::shared_ptr<TaskInfo>>::iterator iter = m_taskList.begin();

    while(iter != m_taskList.end())
    {
        //5分钟超时就移除:5*60*1000=300000
        qint64 diffTime = (curTime - iter->get()->createTime);
        //std::cout << "SCN_TaskList::checkTaskTimeout, createTime:" << iter->get()->createTime << ", diffTime:" << diffTime << std::endl;
        if(diffTime >= 300000)
        {
            iter = m_taskList.erase(iter);
            std::cout << "SCN_TaskList::checkTaskTimeout, reach timeout ! erase  a task! " << std::endl;
            continue;
        }
        iter++;
    }
}
