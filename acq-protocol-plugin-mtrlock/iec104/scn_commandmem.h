/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_commandmem.h
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：定义与规约库的接口类SCN_CommandMem. 
 *其它说明：
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#ifndef __SCN_COMMANDMEM_H
#define __SCN_COMMANDMEM_H

#include <queue>
#include <mutex>
#include "proto_cmdmem.h"
//#include "scn_queue_tpl.h"
//#include "scn_def.h"
class TaskInfo;
class SCN_CommandMem : public PROTO_CommandMem
{
private:
    //SCN_LinkedQueue<COMMAND> CmdQueue;//CmdQueue[SCN_MAX_RTUNUMS];
    std::queue<std::shared_ptr<COMMAND>> m_cmdQueue;
    std::mutex m_mutex;
public:
    virtual ~SCN_CommandMem();

    virtual int GetCommandNum();
    virtual int PutACommand(COMMAND &cmd );
    virtual void RemoveAllCommand();
    //读取命令return=1 有效, =-1 无效
    virtual int GetACommand(std::shared_ptr<COMMAND> &cmd );    //读指针不移动
    virtual int RemoveACommand();    //移动读指针

    //发送遥控[或遥调]结果响应到网关
    virtual int SendYkYtReply(int cmdType, const QString &rtuId, int pointNo, int ctrlType, int ctrlReply, std::shared_ptr<TaskInfo> taskInfoPtr);

    //发送设置点结果响应到网关
    virtual int SendSpReply(int cmdType, const QString &rtuId, int pointNo, int ctrlReply, std::shared_ptr<TaskInfo> taskInfoPtr);

    //发送防误结果响应到网关
    virtual int SendFWReply(int cmdType, const QString &rtuId, const QString &taskId, QList<FwRspParam_S> &fwRspList);

    /**
     * @brief 设置所连接的设备IP和端口(发送源头端)
     * @param source 所连接的设备IP和端口
     */
    virtual void SetSource(const std::string &source);

    virtual void slotPutCmd(std::list<COMMAND> &cmdList);
private:
    std::string m_source;//所连接的设备IP和端口(发送源头端)
};

#endif //SCN_COMMANDMEM_H
