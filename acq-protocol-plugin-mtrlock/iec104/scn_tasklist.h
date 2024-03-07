/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_tasklist.h
 *生成日期：2011-09-22
 *作者：    cfq
 *功能说明：任务（遥控、设点、总召等命令）队列
 *其它说明：
 *修改记录：date, maintainer
 *
*******************************************************************************/

#ifndef __SCN_TASKLIST_H
#define __SCN_TASKLIST_H

#include <QList>
#include <mutex>
#include <memory>
#include <QString>
#include <QDateTime>
#include <QVariantHash>
#include "plugin104_define.h"
//任务（遥控、设点、总召等命令）信息
class TaskInfo
{
public:
    QString code;//功能码
    QString taskId;//任务号
    int pointNo;//点序号
    qint64 createTime;//创建时间
    QVariantHash otherParam;//其他参数
    QList<std::shared_ptr<BaseParam_S>> dataList;

    TaskInfo(const QString &code, const QString &taskId, int pointNo)
    {
        this->code = code;
        this->taskId = taskId;
        this->pointNo = pointNo;
        this->createTime = QDateTime::currentMSecsSinceEpoch();
    }

    TaskInfo(const QString &code, const QString &taskId, int pointNo, const QVariantHash &otherParam)
    {
        this->code = code;
        this->taskId = taskId;
        this->pointNo = pointNo;
        this->createTime = QDateTime::currentMSecsSinceEpoch();
        this->otherParam = otherParam;
    }
    TaskInfo(const QString &code, const QString &taskId, int pointNo, const QVariantHash &otherParam, QList<std::shared_ptr<BaseParam_S>> &dataList)
    {
        this->code = code;
        this->taskId = taskId;
        this->pointNo = pointNo;
        this->createTime = QDateTime::currentMSecsSinceEpoch();
        this->otherParam = otherParam;
        this->dataList = dataList;
    }


    TaskInfo(const TaskInfo &taskInfo)
    {
        this->code = taskInfo.code;
        this->taskId = taskInfo.taskId;
        this->pointNo = taskInfo.pointNo;
        this->createTime = taskInfo.createTime;
        this->otherParam = taskInfo.otherParam;
        this->dataList = taskInfo.dataList;
    }

    TaskInfo& operator=(const TaskInfo &taskInfo)
    {
        if (this != &taskInfo)
        {
            this->code = taskInfo.code;
            this->taskId = taskInfo.taskId;
            this->pointNo = taskInfo.pointNo;
            this->createTime = taskInfo.createTime;
            this->otherParam = taskInfo.otherParam;
            this->dataList = taskInfo.dataList;
        }
        return *this;
    }

    QString toString()
    {

        return "code:"+code+",pointNo:"+QString::number(pointNo)+",taskId:"+taskId+",createTime:"+ QString::number(createTime);
    }
};

class SCN_TaskList
{
public:
    SCN_TaskList();
    ~SCN_TaskList();

    int getTaskCount();//获取任务数
    void addTask(TaskInfo &taskInfo);//
    void clear();//清空任务队列
    bool exist(const QString &code, int pointNo);//判断是否已存在
    std::shared_ptr<TaskInfo> getATask(const QString &code, int pointNo);//获取一个任务
    void removeATask(const QString &code, int pointNo);    //移除一个任务

    void checkTaskTimeout();//检测任务是否过期，过期则移除掉
private:
    QList<std::shared_ptr<TaskInfo>> m_taskList;
    std::mutex m_mutex;
};

#endif //__SCN_TASKLIST_H
