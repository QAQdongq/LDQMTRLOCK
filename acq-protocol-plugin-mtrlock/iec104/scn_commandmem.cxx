/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_commandmem.cxx
 *生成日期：2012-01-01
 *作者：    yay
 *功能说明：定义与规约库的接口类SCN_CommandMem
 *其它说明：
 *修改记录：date, maintainer
 *          2012-01-01, yay, 建立代码
*******************************************************************************/

#include "plugin104_define.h"
#include "scn_commandmem.h"
#include "sicd.h"
#include "utils/converter_util.h"
#include "acq_json.h"
#include <QDateTime>
#include "ieccomm.h"
#include "scn_tasklist.h"

uint8 commandmem_debug_flag = 1;

/************************************************************************
作者：   lee     
创建日期：2009.3.20   
函数功能说明：SCN_CommandMem类析构函数：清除所有命令.
输入参数：
             无

输出参数：  无
            
返回值：     
         无
            
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
SCN_CommandMem::~SCN_CommandMem()
{
    RemoveAllCommand();
    //m_logger = NULL;//释放日志引用
}


/************************************************************************
作者：   lee     
创建日期：2009.3.20   
函数功能说明：将命令cmd写入对应RtuId的命令队列.
输入参数：
             COMMAND *cmd    命令

输出参数：  无
            
返回值：     
         -1        失败
         1        成功
            
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
int SCN_CommandMem::PutACommand(COMMAND &cmd )
{
    std::lock_guard<std::mutex> locker(m_mutex);
    LOG_DEBUG(m_cchId, QString("cmdType:%1, taskId:%2").arg(cmd.cmdType).arg(cmd.taskId));
    std::shared_ptr<COMMAND> cmdPtr(new COMMAND(cmd));
    m_cmdQueue.push(cmdPtr);
    return 1;
}

/************************************************************************
作者：   lee     
创建日期：2009.3.20   
函数功能说明：取rtuId号rtu命令缓冲区中命令个数
输入参数：

输出参数：  无
            
返回值：     
         
         >=0        rtu命令缓冲区内命令个数
            
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
int SCN_CommandMem::GetCommandNum()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    return m_cmdQueue.size();//CmdQueue.GetNodeNum();
}

/************************************************************************
作者：   lee     
创建日期：2009.3.20   
函数功能说明：从对应RtuId的命令队列中读取1个命令.
输入参数：

输出参数：  COMMAND *cmd    获得的命令
            
返回值：          
         -1        失败
         1        成功
            
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
int SCN_CommandMem::GetACommand(std::shared_ptr<COMMAND> &cmd )
{
    std::lock_guard<std::mutex> locker(m_mutex);
    if( m_cmdQueue.size() == 0)
    {
        return -1;
    }
    cmd = m_cmdQueue.front();
    return 1;
}

/************************************************************************
作者：   lee     
创建日期：2009.3.20   
函数功能说明：从对应RtuId的命令队列中移除1个命令
输入参数：

输出参数：  无
            
返回值：          
         -1        失败
         1        成功
            
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
int SCN_CommandMem::RemoveACommand()
{
    std::lock_guard<std::mutex> locker(m_mutex);
    m_cmdQueue.pop();
    return 1;
}

/************************************************************************
作者：   lee     
创建日期：2009.3.20   
函数功能说明：清空Rtu的命令缓冲区。
输入参数：

输出参数：  无
            
返回值：          
         无
            
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void SCN_CommandMem::RemoveAllCommand()
{
    LOG_DEBUG(m_cchId, "");
    std::lock_guard<std::mutex> locker(m_mutex);
    while(!m_cmdQueue.empty())
    {
        m_cmdQueue.pop();
    }
}

//发送遥控[或遥调]结果响应到网关
int SCN_CommandMem::SendYkYtReply(int cmdType,const QString &rtuId, int pointNo, int ctrlType, int ctrlReply, std::shared_ptr<TaskInfo> taskInfoPtr)
{
    LOG_INFO(m_cchId, QString("cmdType:%1,chnId:%2,rtuId:%3,pointNo:%4,ctrlType:%5,ctrlReply:%6").arg(cmdType).arg(m_cchId).arg(rtuId).arg(pointNo).arg(ctrlType).arg(ctrlReply));
    if(nullptr == taskInfoPtr)
    {
        LOG_ERROR(m_cchId, QString("SendYkYtReply: Not found task! pointNo:%2").arg(pointNo));
        return -1;
    }

    if(CMD_TYPE_YK != cmdType && CMD_TYPE_YT != cmdType)
    {
        LOG_WARN(m_cchId, QString("Unsupport cmdType! ykCmd:%1").arg(cmdType));
        return -1;
    }
    int ykCmd = -1;
    uint32 ykNo = pointNo;
    int ykResult = YK_RESULT_FAIL; //result--结果状态(<=0-失败，1-成功）;
    QString reason ="";
    int value =0;//value--遥控类型：0-分，1-合;
    switch(ctrlType)
    {
        case CTRL_TYPE_LOWER://下调
        case CTRL_TYPE_TRIP://分闸（遥控）
        {
            value =0;
            break;
        }
        case CTRL_TYPE_CLOSE://合闸（遥控）
        case CTRL_TYPE_RAISE://上调
        {
            value =1;
            break;
        }
        default:
            LOG_ERROR(m_cchId,QString("Unknown ctrlType! ctrlType:%1").arg(ctrlType));
            break;
    }

    switch(ctrlReply)
    {
        case COMMAND_SELECT_FAIL:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_SELECT_FAIL ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_SELECT;
            ykResult = YK_RESULT_FAIL;
            reason = "预置-失败！"; //QObject::tr("Select failed!"); //预置失败！
            break;
        }
        case COMMAND_SELECT_SUCCESS:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_SELECT_SUCCESS ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_SELECT;
            ykResult = YK_RESULT_SUCCESS;
            reason = "预置-成功！"; //QObject::tr("Select success!"); //预置成功！
            break;
        }
        case COMMAND_LINK_BUSY:
        {
            LOG_WARN(m_cchId, "Link is busy!");
            ykResult = YK_RESULT_FAIL;
            reason ="链路忙！" ;//QObject::tr("Link is busy!"); //链路忙！
            break;
        }
        case COMMAND_UNKNOWN_YKNO:
        {
            LOG_WARN(m_cchId, "Unknown ykno!");
            ykResult = YK_RESULT_FAIL;
            reason = "未知控点！";//QObject::tr("Unknown ykno!"); //未知控点！
            break;
        }
        case COMMAND_YKCANCEL_SUCCESS://遥控撤销成功
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YKCANCEL_SUCCESS ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_CANCEL;
            ykResult = YK_RESULT_SUCCESS;
            reason = "撤销-成功！";//QObject::tr("Cancle success!"); //撤销成功！
            break;
        }
        case COMMAND_YKCANCEL_FAIL://遥控撤销失败
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YKCANCEL_FAIL ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_CANCEL;
            ykResult = YK_RESULT_FAIL;
            reason = "撤销-失败！";//QObject::tr("Cancle failed!"); //撤销失败！
            break;
        }
        case COMMAND_YKEXEC_FAIL:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YKEXEC_FAIL ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_EXECUTE;
            ykResult = YK_RESULT_FAIL;
            reason = "执行-失败！";//QObject::tr("Execute failed!"); //执行失败！
            break;
        }
        case COMMAND_YKEXEC_SUCCESS://遥控执行成功
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YKEXEC_SUCCESS ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_EXECUTE;
            ykResult = YK_RESULT_SUCCESS;
            reason = "执行-成功！";//QObject::tr("Execute success!"); //执行成功！
            break;
        }        
        case COMMAND_YKEXEC_TIMEOUT://执行超时
        {
            LOG_INFO(m_cchId, QString("COMMAND_YKEXEC_TIMEOUT  ctrlReply:%1").arg(ctrlReply));
            ykCmd = YK_CMD_EXECUTE;
            ykResult = YK_RESULT_TIMEOUT;
            reason = "执行-超时！";
            break;
        }
    #if 0
        case COMMAND_WF_SUCCESS:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_WF_SUCCESS ctrlReply:%1").arg(ctrlReply));
            ykCmd = WF_CMD_EXECUTE;
            ykResult = YK_RESULT_SUCCESS;
            reason = value == 1? "防误校验合-成功！":"防误模式校验分-成功！";
            break;
        }
        case COMMAND_WF_FAIL:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_WF_FAIL ctrlReply:%1").arg(ctrlReply));
            ykCmd = WF_CMD_EXECUTE;
            ykResult = YK_RESULT_FAIL;
            reason = value == 1? "防误校验合-失败！":"防误模式校验分-失败！";
            break;
        }
        case COMMAND_WF_TIMEOUT:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_WF_TIMEOUT ctrlReply:%1").arg(ctrlReply));
            ykCmd = WF_CMD_EXECUTE;
            ykResult = YK_RESULT_TIMEOUT;
            reason = value == 1? "防误校验合-超时！":"防误模式校验分-超时！";
            break;
        }
        case COMMAND_YJMS_FAIL:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YJMS_FAIL ctrlReply:%1").arg(ctrlReply));
            ykCmd = WF_CMD_YJMS;
            ykResult = YK_RESULT_FAIL;
            reason = value == 1? "应急模式校验合-失败！":"应急模式校验分-失败！";
            break;
        }
        case COMMAND_YJMS_SUCCESS:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YJMS_SUCCESS ctrlReply:%1").arg(ctrlReply));
            ykCmd = WF_CMD_EXECUTE;
            ykResult = YK_RESULT_SUCCESS;
            reason = value == 1? "应急模式校验合-成功！":"应急模式校验分-成功！";
            break;
        }
        case COMMAND_YJMS_TIMEOUT:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YJMS_TIMEOUT ctrlReply:%1").arg(ctrlReply));
            ykCmd = WF_CMD_EXECUTE;
            ykResult = YK_RESULT_TIMEOUT;
            reason = value == 1? "应急模式校验合-超时！":"应急模式校验分-超时！";
            break;
        }
     #endif
        default:
        {
            LOG_WARN(m_cchId, "Unknown reply!");
            ykResult = YK_RESULT_FAIL;
            reason = "未知回复！";//QObject::tr("Unknown reply!"); //未知回复！
            break;
        }
    }


    if(ykCmd < 0)
    {
        LOG_ERROR(m_cchId, QString("Unknown ykcmd! ykCmd:%1").arg(ykCmd));
        return -1;
    }

    QVariantHash dataHash;
    YKRspParam_S data;
    data.rtuId = rtuId;
    data.cchId = m_cchId;
    data.no = ykNo;
    data.type = ykCmd;
    data.value = value;
    data.result = ykResult;
    data.reason = reason;
    dataHash = ConverterUtil::toYKRspHash(cmdType, data, taskInfoPtr->taskId);
    if(m_protoInterface)
    {
        LOG_INFO(m_cchId, "<====YK/YT reply, dataHash="+ QString(AcqHashToJson(dataHash)));
        m_protoInterface->sendToGatewayByMq(dataHash);//通过MQ上送
    }
    return 1;
}

//发送设置点结果响应到网关
int SCN_CommandMem::SendSpReply(int cmdType, const QString &rtuId, int pointNo, int ctrlReply, std::shared_ptr<TaskInfo> taskInfoPtr)
{
    if(nullptr == taskInfoPtr)
    {
        LOG_ERROR(m_cchId, QString("SP reply: Not found task! no:%2").arg(pointNo));
        return -1;
    }

    int ykResult = YK_RESULT_FAIL; //result--结果状态(0-失败，1-成功）;
    QString reason ="";


    //LOG_INFO(m_cchId, QString("cmdType:%1,chnId:%2,rtuId:%3,pointNo:%4,type:%5,ctrlReply:%6,taskId:%7").arg(cmdType).arg(spReqData.cchId).arg(spReqData.rtuId).arg(spReqData.no).arg(spReqData.type).arg(ctrlReply).arg(taskId));
    int spType = taskInfoPtr->otherParam.value(KEY_TYPE).toInt();
    LOG_INFO(m_cchId, QString("cmdType:%1,chnId:%2,rtuId:%3,pointNo:%4,type:%5,ctrlReply:%6,taskId:%7").arg(cmdType).arg(m_cchId).arg(rtuId).arg(pointNo).arg(spType).arg(ctrlReply).arg(taskInfoPtr->taskId));
    switch(ctrlReply)
    {
        case COMMAND_SELECT_FAIL:
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_SELECT_FAIL  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_FAIL;
            reason = "预置-失败！";//QObject::tr("Select failed!"); //预置失败！
            break;
        }
        case COMMAND_SELECT_SUCCESS:
        {
            LOG_INFO(m_cchId, QString("COMMAND_SELECT_SUCCESS  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_SUCCESS;
            reason = "预置-成功！";//QObject::tr("Select success!"); //预置成功！
            break;
        }
        case COMMAND_LINK_BUSY:
        {
            LOG_WARN(m_cchId, "Link is busy!");
            ykResult = YK_RESULT_FAIL;
            reason = "链路忙！";//QObject::tr("Link is busy!"); //链路忙！
            //reason = QString::fromUtf8("链路忙！");
            break;
        }
        case COMMAND_UNKNOWN_YKNO:
        {
            LOG_WARN(m_cchId, "Unknown ykno!");
            ykResult = YK_RESULT_FAIL;
            reason = "未知控点！";//QObject::tr("Unknown ykno!"); //未知控点！
            //reason = QString::fromUtf8("未知控点！");
            break;
        }
        case COMMAND_YKCANCEL_SUCCESS://撤销成功
        {
            LOG_INFO(m_cchId, QString("COMMAND_YKCANCEL_SUCCESS  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_SUCCESS;
            reason = "撤销-成功！";//QObject::tr("Cancel success!"); //撤销成功！
            break;
        }        
        case COMMAND_YKCANCEL_FAIL://遥控撤销失败
        {
            LOG_DEBUG(m_cchId, QString("COMMAND_YKCANCEL_FAIL  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_FAIL;
            reason = "撤销-失败！";//QObject::tr("Cancle failed!"); //撤销失败！
            break;
        }
        case COMMAND_YKEXEC_FAIL://执行失败
        {
            LOG_INFO(m_cchId, QString("COMMAND_YKEXEC_FAIL  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_FAIL;
            reason = "执行-失败！";//QObject::tr("Execute failed!"); //执行失败！
            break;
        }
        case COMMAND_YKEXEC_SUCCESS://执行成功
        {
            LOG_INFO(m_cchId, QString("COMMAND_YKEXEC_SUCCESS  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_SUCCESS;
            reason = "执行-成功！";//QObject::tr("Execute success!"); //执行成功！
            break;
        }
        case COMMAND_YKEXEC_TIMEOUT://执行超时
        {
            LOG_INFO(m_cchId, QString("COMMAND_YKEXEC_TIMEOUT  ctrlReply:%1").arg(ctrlReply));
            ykResult = YK_RESULT_TIMEOUT;
            reason = "执行-超时！";
            break;
        }
        default:
        {
            LOG_WARN(m_cchId, "Unknown reply!");//未知回复！
            ykResult = YK_RESULT_FAIL;
            reason = QString::fromUtf8("未知回复！");//QObject::tr("Unknown reply!"); //未知回复！
            break;
        }
    }
    QVariantHash dataHash;
    SPRspParam_S data;
    data.rtuId = rtuId;
    data.cchId = m_cchId;
    data.no = pointNo;
    data.type = spType;
    switch(data.type)
    {
        case SP_TYPE_DECIMAL:
        {
            data.decValue = taskInfoPtr->otherParam.value(KEY_DECVALUE).toFloat();
            break;
        }
        case SP_TYPE_INT:
        {
            data.intValue = taskInfoPtr->otherParam.value(KEY_INTVALUE).toInt();
            break;
        }
        case SP_TYPE_FLOAT:
        {
            data.floatValue = taskInfoPtr->otherParam.value(KEY_FLOATVALUE).toFloat();
            break;
        }
        case SP_TYPE_DIGIT:
        {
            data.strValue =  taskInfoPtr->otherParam.value(KEY_STRVALUE).toString();
            break;
        }
        default:
        {
            LOG_ERROR(m_cchId, QString("Unknown type: %1 !").arg(data.type));
            break;
        }
    }
    data.result = ykResult;
    data.reason = reason;
    dataHash = ConverterUtil::toSPRspHash(data, taskInfoPtr->taskId);
    if(m_protoInterface)
    {
        LOG_INFO(m_cchId, "<====SP Reply, dataHash="+ QString(AcqHashToJson(dataHash)));
        m_protoInterface->sendToGatewayByMq(dataHash);//通过MQ上送
    }
    return 1;
}

int SCN_CommandMem::SendFWReply(int cmdType, const QString &rtuId, const QString &taskId, QList<FwRspParam_S> &fwRspList)
{
    LOG_INFO(m_cchId, QString("cmdType:%1,chnId:%2,rtuId:%3,taskId:%4").arg(cmdType).arg(m_cchId).arg(rtuId).arg(taskId));

    QVariantHash dataHash = ConverterUtil::toFwRspHash(fwRspList, taskId);
    if(m_protoInterface)
    {
        LOG_INFO(m_cchId, "<====FW reply, dataHash="+ QString(AcqHashToJson(dataHash)));
        m_protoInterface->sendToGatewayByMq(dataHash);//通过MQ上送
    }
    return 1;
}

void SCN_CommandMem::slotPutCmd(std::list<COMMAND> &cmdList)
{
    LOG_DEBUG(m_cchId,QString("cmdList.size:%1").arg(cmdList.size()));
    std::list<COMMAND>::iterator iter =  cmdList.begin();
    for(; iter != cmdList.end(); iter++)
    {
        PutACommand(*iter);
    }
}


/**
 * @brief 设置所连接的设备IP和端口(发送源头端)
 * @param source 所连接的设备IP和端口
 */
void SCN_CommandMem::SetSource(const std::string &source)
{
    //LOG_DEBUG(m_cchId,"m_source:"+source);
    this->m_source = source;
}
