#include "iec104/iec104zf.h"
#include "iec104/iec104.h"
#include "iec104/scn_routeinf.h"
#include "iec104/scn_rawdatadb.h"
#include "iec104/scn_rtuinf.h"
#include "iec104/scn_commandmem.h"
#include "utils/converter_util.h"
#include "utils/command_util.h"
#include "../acq-protocol-plugin-utils/string_util.h"
#include "../acq-protocol-plugin-utils/date_util.h"
#include "protocol_session.h"
#include "iec104/scn_tasklist.h"

AcqProtocolSession::AcqProtocolSession(InitData_S *initData)
{
    m_cchId = "";
    this->m_isConnected = false;
    this->m_isInit = false;
    m_protoconfig.pRouteInf = NULL;
    m_protoconfig.pCommandMem = NULL;
    m_protoconfig.pRawDb = NULL;
    m_protoconfig.pRtuInf = NULL;

    m_procThread = NULL;
    //m_sendThread = NULL;
    if(NULL == initData)
    {
        LOG_ERROR(m_cchId, "initData is null! wrong input parameter!");
        return;
    }
    m_cchId = initData->chnCfg.id;
    m_rtuId = initData->curUsedRtu.rtuId;
    this->m_initData = initData;
    this->m_chnIdStr = this->m_initData->chnCfg.id.toStdString();
}

AcqProtocolSession::~AcqProtocolSession()
{
    LOG_DEBUG(m_cchId, "");

    //释放空间
    finalize();
}



/**
 * @brief 初始化
 */
void AcqProtocolSession::init(AcqProtocolInterface *protoInterface)
{
    if(!this->m_isInit)
    {
        setProtocolInterface(protoInterface);

        //初始化规约
        if(!initProtocol())
        {
            return;
        }

        //开启规约处理线程
        m_procThread = new ProcThread(this->m_protocolObj);
        m_procThread->start();

        //开启任务处理线程
        m_taskThread = new TaskThread(m_protoconfig.pTaskList);
        m_taskThread->start();

        this->m_isInit = true;
    }
    LOG_DEBUG(m_cchId, "init ok");
}

/**
 * @brief 释放
 */
void AcqProtocolSession::finalize()
{
    if(this->m_isInit)
    {
        exitProcThread();
        exitTaskThread();

        finalizeProtocol();
        this->m_isInit = false;
    }
    LOG_DEBUG(m_cchId, "finalize ok");
}

/**
 * @brief 初始化规约
 */
bool AcqProtocolSession::initProtocol()
{

    m_protoconfig.pRouteInf = new SCN_RouteInf();
    m_protoconfig.pRouteInf->SetProtocolcfgParam(this->m_initData->protocolCfgParam);
    m_protoconfig.pRouteInf->SetRtuAddr(this->m_initData->curUsedRtu.rtuAddr);
    m_protoconfig.pRouteInf->SetRtuId(this->m_initData->curUsedRtu.rtuId);
    m_protoconfig.pRouteInf->SetChnId(this->m_initData->chnCfg.id);

    m_protoconfig.pRawDb = new SCN_RawdataDb();
    m_protoconfig.pRawDb->SetSource("");
    m_protoconfig.pRawDb->SetRtuId(this->m_initData->curUsedRtu.rtuId);
    m_protoconfig.pRawDb->SetChnId(this->m_initData->chnCfg.id);

    //初始化Rtu
    m_protoconfig.pRtuInf = new SCN_RtuInf();
    m_protoconfig.pRtuInf->SetProtocolcfgParam(this->m_initData->protocolCfgParam);
    m_protoconfig.pRtuInf->SetRtuList(this->m_initData->rtuList);

    //命令缓冲区对象指针
    m_protoconfig.pCommandMem = new SCN_CommandMem();
    m_protoconfig.pCommandMem->SetSource("");
    m_protoconfig.pCommandMem->setCchId(this->m_initData->chnCfg.id);

    //任务队列
    m_protoconfig.pTaskList = new SCN_TaskList();


    if(CHN_TYPE_SERVER == this->m_initData->chnCfg.type)//服务端模式（即子站模式）
    {
        //this->m_protocolObj = new Iec104Zf();//子站（或转发站）
        LOG_ERROR(m_cchId, "Wrong channel type! It must be 'TCP Client'!");
        return false;
    }
    else
    {
        this->m_protocolObj = new Iec104();//主站
    }
    this->m_protocolObj ->SetProtocolConfig(m_protoconfig);
    this->m_protocolObj ->SetConfigParam(this->m_initData->protocolCfgParam);


    m_protoconfig.pRawDb->setProtocolInterface(m_protoInterface);
    m_protoconfig.pRouteInf->setProtocolInterface(m_protoInterface);
    m_protoconfig.pCommandMem->setProtocolInterface(m_protoInterface);
    this->m_protocolObj ->setProtocolInterface(m_protoInterface);
    return true;

}

/**
 * @brief 释放规约
 */
void AcqProtocolSession::finalizeProtocol()
{
    if(NULL != this->m_protocolObj)
    {
        delete this->m_protocolObj;
        this->m_protocolObj = NULL;
    }

    if(NULL != m_protoconfig.pRouteInf)
    {
        delete m_protoconfig.pRouteInf;
        m_protoconfig.pRouteInf = NULL;
    }
    if(NULL != m_protoconfig.pRawDb)
    {
        delete m_protoconfig.pRawDb;
        m_protoconfig.pRawDb = NULL;
    }
    if(NULL != m_protoconfig.pRtuInf)
    {
        delete m_protoconfig.pRtuInf;
        m_protoconfig.pRtuInf = NULL;
    }
    if(NULL != m_protoconfig.pCommandMem)
    {
        delete m_protoconfig.pCommandMem;
        m_protoconfig.pCommandMem = NULL;
    }
    if(NULL != m_protoconfig.pTaskList)
    {
        delete m_protoconfig.pTaskList;
        m_protoconfig.pTaskList = NULL;
    }
}

/**
 * @brief 退出规约处理线程
 */
void AcqProtocolSession::exitProcThread()
{
    LOG_DEBUG(m_cchId, "");
    if(NULL != this->m_procThread)
    {
        this->m_procThread->setExitFlag();
        //start: cfq 20220627 解决可能出现的致命异常：QThread: Destroyed while thread is still running
        this->m_procThread->quit();
        this->m_procThread->wait();
        //end: cfq 20220627

        delete this->m_procThread;
        this->m_procThread = NULL;
    }
}


/**
 * @brief 退出任务处理线程
 */
void AcqProtocolSession::exitTaskThread()
{
    LOG_DEBUG(m_cchId, "");
    if(NULL != this->m_taskThread)
    {
        this->m_taskThread->setExitFlag();
        //start: cfq 20220627 解决可能出现的致命异常：QThread: Destroyed while thread is still running
        this->m_taskThread->quit();
        this->m_taskThread->wait();
        //end: cfq 20220627

        delete this->m_taskThread;
        this->m_taskThread = NULL;
    }
}


/**
 * @brief 获取当前规约对象
 * @return
 */
Protocol *AcqProtocolSession::getProtocolObj()
{
    return this->m_protocolObj;
}

/**
 * @brief 通过回调上送遥信变位数据到智能分析网关机
 * @param data
 */
void AcqProtocolSession::slotPutData(std::string data)
{

}


/**
 * @brief 处理从socket取的数据，放到规约接收缓存中
 */
void AcqProtocolSession::slotRead(const std::string &data)
{
    if(NULL != m_protocolObj)
    {
        this->m_protocolObj->FeedProtocolBytes(data);
    }
}

/**
 * @brief 下发送命令数据
 * @param data 待发送数据
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 * @note liujie note 下发命令
 */
void AcqProtocolSession::sendCmdData(const QVariantHash &data)
{
    if(!data.contains("code"))
    {
        if(data.contains(CFG_LOG_LEVEL))//设置日志级别
        {
            int logLevel = data.value(CFG_LOG_LEVEL).toInt();
            PluginLogHelper::instance()->setLogLevel(m_cchId, logLevel);
            return;
        }
    }
    std::string codeStr = data.value("code").toString().toStdString();
    QString errMsg;
    if( NULL == this->m_protocolObj )
    {
        errMsg ="m_protocolObj is null!";
        LOG_ERROR(m_cchId, errMsg);
        return;
    }
    int cmdType = ConverterUtil::cmdTypeStrToInt(codeStr);
    if(INVALID_CMDTYPE == cmdType)
    {
        errMsg =QString("Invalid cmdType! cmdType:%1").arg(cmdType);
        LOG_DEBUG(m_cchId, errMsg);
        return;
    }
    std::list<COMMAND> cmdList;
    bool ret = false;

    //LOG_ERROR(m_cchId, cmdType);
    switch(cmdType)
    {
        case CMD_TYPE_YX://遥信
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(codeStr), data);
            ret = CommandUtil::makeYxCmd(cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_YC://遥测
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(codeStr), data);
            ret = CommandUtil::makeYcCmd(cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_YK://遥控
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_YK), data);
            ret = CommandUtil::makeYkCmd(this->m_protoconfig.pTaskList, cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_YT://遥调
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_YT), data);
            ret = CommandUtil::makeYTCmd(this->m_protoconfig.pTaskList, cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_WF://五防操作 liujie add 20230525
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_WF), data);
            ret = CommandUtil::makeWFCmd(this->m_protoconfig.pTaskList, cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_YKRESULT://遥控（选择或执行）结果
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_YKRESULT), data);
            ret = CommandUtil::makeYkResultCmd(cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_YTRESULT://遥调(预置或执行)结果
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_YTRESULT), data);
            ret = CommandUtil::makeYTResultCmd(cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_CALLALLDATA://总召
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_CALLDATA), data);
            ret = CommandUtil::makeCallDataCmd(cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_CALLALLDATAEND://总召结束
        {
            ret = CommandUtil::makeCallDataEndCmd(cmdList, errMsg);
            break;
        }
        case CMD_TYPE_SP://设置值
        {
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_SP), data);
            ret = CommandUtil::makeSPCmd(this->m_protoconfig.pTaskList, cmdData, cmdList, errMsg);
            break;
        }
        case CMD_TYPE_SetPasswordReg://如果json中标志为SetPasswordReg
        {

            //1、将json结构体转成预定义的结构体
            std::shared_ptr<BaseCmdData_S> cmdData = ConverterUtil::toCmdData(QString::fromStdString(CMD_TYPE_STR_SetPasswordReg), data);
            std::size_t line  = cmdData->param.size();

            //2、将结构体加入到队列中
            ret = CommandUtil::makeSetPasswordRegCmd(cmdData, cmdList, errMsg);
            break;
        }
        default:
        {
            errMsg =QString("Unhandle cmdType! cmdType:%1").arg(cmdType);
            break;
        }
    }
    if(!ret)
    {
        errMsg ="Failed to make command data!" + errMsg;
        LOG_ERROR(m_cchId, errMsg);
        return;
    }
    if(!errMsg.isEmpty())
    {
        LOG_WARN(m_cchId, errMsg);
    }
    if(0 == cmdList.size())
    {
        errMsg ="Failed to make command data! No command need to be sent!";
        LOG_ERROR(m_cchId, errMsg);
        return;
    }
    if(NULL != m_protoconfig.pCommandMem)
    {
        m_protoconfig.pCommandMem->slotPutCmd(cmdList);//发送命令
    }
    else
    {
        errMsg ="pCommandMem is null!";
        LOG_ERROR(m_cchId, errMsg);
    }
}

/**
 * @brief 获取通道号字符串格式
 * @return
 */
std::string AcqProtocolSession::getChnIdStr()
{
    return m_chnIdStr;
}

/**
 * @brief 断开连接
 */
#if 0
void AcqProtocolSession::disconnect()
{
    LOG_INFO(m_cchId, "disconnect start!");
    /*if(NULL != m_sockio)
    {
        hio_close(m_sockio);
        m_sockio = NULL;
    }*/
    setIsConnected(false);

    LOG_INFO(m_cchId, "disconnect end!");
}
#endif

/**
 * @brief 重连
 */
void AcqProtocolSession::reconnect()
{
    LOG_INFO(m_cchId, "reconnect start!");
    setIsConnected(false);
    if(m_protoInterface)
    {
        emit m_protoInterface->reconnectDeviceSig();
    }
}

/**
 * @brief 是否已连接
 * @return true-已连接, false-未连接
 */
bool AcqProtocolSession::isConnected()
{
    return this->m_isConnected;
}

/**
 * @brief 设置连接标志
 * @param isConnected 是否已连接
 */
void AcqProtocolSession::setIsConnected(bool isConnected)
{
    this->m_isConnected = isConnected;
    if(!this->m_isConnected)
    {
        if(NULL != m_protocolObj)
        {
            this->m_protocolObj->Restore();
        }

        //网络断开时，设置RTU的链路状态为：0-未连接
        if(m_protoInterface)
        {
            emit m_protoInterface->setRTUStatus(m_rtuId, 0);
        }
    }
}

/**
 * @brief 将数据通过socket发送
 * @param data 待发送数据
 * @return true-成功, false-失败
 */
bool AcqProtocolSession::slotWrite(const std::string &data)
{
    if(!this->isConnected())
    {
        LOG_WARN(m_cchId, "Connection is closed!!! Waiting reconnect!!");
        return false;
    }
    /*if(NULL != m_sockio)
    {
       LOG_INFO(m_cchId, "["+ this->getSource() +"] <Send>:\n" +StringUtil::toHexString(data, " "));
       int writeLen = hio_write(m_sockio, (char *)(data.c_str()), data.length());
       if(writeLen == data.length())
       {
         //LOG_DEBUG(m_cchId, "Write successfully!!!");
         return true;
       }
    }*/
    LOG_ERROR(m_cchId, "Write failed!!!");
    return false;
}

void AcqProtocolSession::setProtocolInterface(AcqProtocolInterface *protoInterface)
{
    m_protoInterface = protoInterface;
    /*if(m_protoconfig.pRouteInf)
    {
        m_protoconfig.pRouteInf->setProtoInterface(m_protoInterface);
    }
    if(m_protoconfig.pRawDb)
    {
        m_protoconfig.pRawDb->setProtoInterface(m_protoInterface);
    }
    if(m_protoconfig.pCommandMem)
    {
        m_protoconfig.pCommandMem->setProtoInterface(m_protoInterface);
    }
    if(this->m_protocolObj)
    {
        this->m_protocolObj->setProtoInterface(m_protoInterface);
    }*/
}

