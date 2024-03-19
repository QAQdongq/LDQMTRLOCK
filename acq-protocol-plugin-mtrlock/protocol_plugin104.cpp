#include <QDateTime>
#include <QDir>
#include <QtGlobal>
#include "protocol_plugin104.h"
#include "acq_json.h"
#include "iec104/iec104zf.h"
#include "iec104/iec104.h"
#include "iec104/scn_routeinf.h"
#include "iec104/scn_rawdatadb.h"
#include "iec104/scn_rtuinf.h"
#include "iec104/scn_commandmem.h"
#include "utils/converter_util.h"
#include "utils/command_util.h"
#include "../acq-protocol-plugin-utils/string_util.h"

QSharedPointer<AcqProtocolInterface> Protocol104Factory::createOneDataProtocolInstance(const QVariantHash &channelHash)
{
    return QSharedPointer<AcqProtocolInterface>(new Protocol104Plugin(channelHash));
}

Protocol104Plugin::Protocol104Plugin(const QVariantHash &channelHash)
    :AcqProtocolInterface(channelHash)
{

    m_isInitSuccess=false;
    QString channelHashJson = AcqHashToJson(channelHash);//Hash转成JSON
    LOG_INFO(m_cchId, "channelHashJson="+ channelHashJson);

    //读取穿入参数
    if(!readParameter(channelHash))
    {
        m_isInitSuccess=false;
        LOG_INFO(m_cchId,"104plugin init failed!");
        return;
    }

    //创建会话
    //createSession();

    LOG_INFO(m_cchId,"104plugin init ok");
    m_isInitSuccess = true;
}

Protocol104Plugin::~Protocol104Plugin()
{
    LOG_DEBUG(m_cchId,"finalize ok");
}

std::shared_ptr<AcqProtocolSession> Protocol104Plugin::createSession()
{
    LOG_DEBUG(m_cchId,"");
    //std::shared_ptr<AcqProtocolInterface> protoInterface = std::shared_ptr<AcqProtocolInterface>(this);
    m_session = std::shared_ptr<AcqProtocolSession>(new AcqProtocolSession(&m_initData));
    //m_session->setIsConnected(true);
    m_session->init(this);
    return m_session;
}

void Protocol104Plugin::clearSession()
{
    if(m_session)
    {
        m_session->setProtocolInterface(NULL);
        m_session->finalize();
        m_session = NULL;
    }
}

void Protocol104Plugin::resetSession()
{
    LOG_INFO(m_cchId,"");

    //清除当前会话
    clearSession();

    //创建新会话
    createSession();
}

/**
 * @brief 读取输入参数
 * @param channelHash 当前通道参数
 * @param parameter 所有输入参数
 */
bool Protocol104Plugin::readParameter(const QVariantHash &channelHash)
{
    /*
    channelHash 通道配置表结构如下：
          ┠─cchId    int    是    1    通道号
          ┠─cchName    string    是    channel1    通道名称
          ┠─cchDesc    string    是    测试通道1    通道描述
          ┠─type    int    是    0    通道类型：        0:TCP客户端；        1: TCP服务端；        2: UDP；        3: 串口
          ┠─node    string    是    192.168.80.100    所属采集节点Ip地址
          ┠─localIP    string    是    192.168.80.120    本机IP
          ┠─localPort    int    是    8001    本机监听端口
          ┠─remoteIp    string    是    192.168.80.10    远端设备IP
          ┠─remotePort    int    是    8000    远端设备端口
          ┠─useFlag    int    是    1    启用标志:0-不启用；1-启用
          ┠─extend    string    否        通道通讯参数预留扩展信息
          ┠─rtuId    string    否        所属RTU
          ┠─comParam    Object    否        串口参数
             ┠─comName    string    否    com4    串口名称
             ┠─baud    int    否    9600    串口波特率
             ┠─dataBit    int    否    5    串口数据位
             ┠─stopBit    int    否    1    串口停止位
             ┠─parity    int    否    1    串口校验方式：        0:无校验(none);        1:奇校验(odd);        2:偶校验(even);        3:1校验(Mark);        4:0校验(Space)
          ┠─protocolInfo    Object    是        规约信息
             ┠─protocolId    int    是    1    规约ID
             ┠─protocolName    string    是    iec104    规约名称
             ┠─protocolDesc    string    是    104规约    规约描述
             ┠─protocolPath    string     是    /tmp/iec104.so    规约插件路径
          ┠─protocolParam    Object    是        规约配置参数
             ┠─scanTime    int    是    60    扫描周期
             ┠─scanFullTime    int    是    300    全数据召唤周期
             ┠─scanPulseTime    int    是    60    脉冲召唤周期
             ┠─syncTime    int    是    300    校时周期
             ┠─retryTimes    int    是    3    重试次数
             ┠─ykTimeout    int    是    15    遥控命令(选择或执行)超时
             ┠─spTimeout    int    否    15    设置点命令超时
             ┠─linkTimeout    int    是    60    链路应答超时
             ┠─ykCmdMode    int    是    2    遥控命令模式:        1:单点命令模式，        2:双点命令模式(默认)
             ┠─spCmdMode    int    是    1    设点命令模式:        1:单个设点模式(默认)        2:连续设点模式
             ┠─soeTransMode    int    是    1    SOE传输模式:        1:一次传输,要根据SOE生成YX(默认)        2:二次传输
             ┠─cotNum    int    是    2    ASDU传送原因字节数(默认为2)
             ┠─comAddrNum    int    是    2    ASDU公共地址字节数(默认为2)
             ┠─infAddrNum    int    是    3    ASDU信息体地址字节数(默认为3)
             ┠─yxBaseAddr    int    是    1    ASDU状态量(YX)起始地址
             ┠─ycBaseAddr    int    是    16385    ASDU模拟量(YC)起始地址
             ┠─ykBaseAddr    int    是    24577    ASDU控制量(YK)起始地址
             ┠─spBaseAddr    int    是    25089    ASDU设置点(SP)起始地址
             ┠─kwhBaseAddr    int    是    25601    ASDU电度量起始地址
             ┠─extend    string    否        规约预留扩展信息
    */
    m_initData.chnCfg.id = channelHash.value(KEY_CHNID).toString();
    m_cchId = m_initData.chnCfg.id;
    m_initData.chnCfg.name = channelHash.value(KEY_CHNNAME).toString();
    m_initData.chnCfg.type = channelHash.value(KEY_TYPE).toInt();

    //初始化日志
    PluginLogHelper::instance()->init(m_cchId, "104", "");

    QVariantHash protocolParam = channelHash.value(KEY_PROTOCOLPARAM).toHash();
    m_initData.protocolCfgParam.scanTime = protocolParam.value("scanTime", 60).toInt();//扫描周期
    m_initData.protocolCfgParam.scanFullTime = protocolParam.value("scanFullTime", 900).toInt();//全数据召唤周期(15min)
    m_initData.protocolCfgParam.scanPulseTime = protocolParam.value("scanPulseTime", 60).toInt();//脉冲召唤周期
    m_initData.protocolCfgParam.syncTime = protocolParam.value("syncTime", 300).toInt();//校时周期
    m_initData.protocolCfgParam.retryTimes = protocolParam.value("retryTimes", 3).toInt();//重试次数

    m_initData.protocolCfgParam.linkTimeout = protocolParam.value("linkTimeout",60).toInt();//链路应答超时
    m_initData.protocolCfgParam.ykTimeout = protocolParam.value("ykTimeout", 15).toInt();//遥控超时时间（秒）
    m_initData.protocolCfgParam.spTimeout = protocolParam.value("spTimeout", 15).toInt();//设点超时时间（秒）
    m_initData.protocolCfgParam.fwTimeout = protocolParam.value("fwTimeout", 15).toInt();//防误超时时间（秒）

    m_initData.protocolCfgParam.ykCmdMode = protocolParam.value("ykCmdMode", 1).toInt(); //遥控命令模式(1:单点命令模式，2:双点命令模式)(默认为1)
    m_initData.protocolCfgParam.spCmdMode= protocolParam.value("spCmdMode", 1).toInt(); //设点命令模式(1:单个设点模式,2:连续设点模式)(默认为1)
    m_initData.protocolCfgParam.soeTransMode= protocolParam.value("soeTransMode", 1).toInt(); //SOE传输模式(1:一次传输,要根据SOE生成YX, 2:二次传输)(默认为1)
    m_initData.protocolCfgParam.cotNum= protocolParam.value("cotNum", 2).toInt();       //ASDU传送原因字节数(默认为2)
    m_initData.protocolCfgParam.comAddrNum= protocolParam.value("comAddrNum", 2).toInt();   //ASDU公共地址字节数(默认为2)
    m_initData.protocolCfgParam.infAddrNum= protocolParam.value("infAddrNum", 3).toInt();   //ASDU信息体地址字节数(默认为3)

    m_initData.protocolCfgParam.yxBaseAddr= protocolParam.value("yxBaseAddr", 1).toInt();   //ASDU状态量(YX)起始地址
    m_initData.protocolCfgParam.ycBaseAddr= protocolParam.value("ycBaseAddr", 16385).toInt();   //ASDU模拟量(YC)起始地址
    m_initData.protocolCfgParam.ykBaseAddr= protocolParam.value("ykBaseAddr", 24577).toInt();   //ASDU控制量(YK)起始地址
    m_initData.protocolCfgParam.spBaseAddr= protocolParam.value("spBaseAddr", 25089).toInt();   //ASDU设置点(SP)起始地址
    m_initData.protocolCfgParam.ytBaseAddr= m_initData.protocolCfgParam.ykBaseAddr;//protocolParam.value("ytBaseAddr").toInt();   //ASDU调节量(YT-步操作)起始地址 --目前遥控，遥调公用遥控点表，起始地址也一样。
    m_initData.protocolCfgParam.ddBaseAddr= protocolParam.value("ddBaseAddr", 25601).toInt();  //ASDU电度量起始地址
    m_initData.protocolCfgParam.fwBaseAddr= protocolParam.value("fwBaseAddr", 24577).toInt();  //防误起始地址

    m_initData.protocolCfgParam.isCheckSeqNum = (1==protocolParam.value("isCheckSeqNum","0").toInt()?true:false);  //是否检查接受发送序列号
    m_initData.protocolCfgParam.isYkWaitExecFinish = (1==protocolParam.value("isYkWaitExecFinish","1").toInt()?true:false);  //遥控执行是否需要等待执行结束（Finish）
    m_initData.protocolCfgParam.isYtWaitExecFinish = (1==protocolParam.value("isYtWaitExecFinish","1").toInt()?true:false);  //遥调执行是否需要等待执行结束（Finish）

     m_initData.protocolCfgParam.spFullCodeVal= protocolParam.value("spFullCodeVal", 65536).toInt();   //设点归一化满码值(需要通讯双方约定，一般为：2<<15 即 65536)
    QVariantList rtuParams = channelHash.value(KEY_RTUPARAMS).toList();

    m_initData.curUsedRtu.rtuId = channelHash.value(KEY_RTUID).toString();
    m_initData.curUsedRtu.rtuAddr = -1;
    m_initData.rtuList.clear();
    for(int i=0; i<rtuParams.size(); i++)
    {
        QVariantHash rtuHash = rtuParams.at(i).toHash();
        QString rtuId = rtuHash.value(KEY_RTUID).toString();
        if(m_initData.curUsedRtu.rtuAddr < 0)
        {
            if(m_initData.curUsedRtu.rtuId == rtuId)
            {
                m_initData.curUsedRtu.rtuAddr = rtuHash.value(KEY_RTUADDR).toInt();
            }
        }
        Rtu_S rtu;
        rtu.rtuId = rtuId;
        rtu.rtuName = rtuHash.value(KEY_RTUNAME).toString();
        rtu.rtuDesc = rtuHash.value(KEY_RTUDESC).toString();
        rtu.rtuAddr = rtuHash.value(KEY_RTUADDR).toInt();
        rtu.yxCounts = rtuHash.value(KEY_YXCOUNTS).toInt();
        rtu.ycCounts = rtuHash.value(KEY_YCCOUNTS).toInt();
        rtu.ykCounts = rtuHash.value(KEY_YKCOUNTS).toInt();
        rtu.ddCounts = rtuHash.value(KEY_DDCOUNTS).toInt();
        rtu.setCounts = rtuHash.value(KEY_SETCOUNTS).toInt();
        rtu.extend = rtuHash.value(KEY_EXTEND).toString();

        LOG_DEBUG(m_cchId,"rtu.rtuId=" + rtu.rtuId+ ", rtu.rtuAddr=" + QString::number(rtu.rtuAddr));
        m_initData.rtuList.push_back(rtu);
    }
    if(m_initData.curUsedRtu.rtuId.isEmpty())
    {
        LOG_ERROR(m_cchId,"No matched rtuId!!! cchId:" + m_cchId);
        return false;
    }
    if(m_initData.curUsedRtu.rtuAddr < 0)
    {
        LOG_ERROR(m_cchId,"Wrong rtuAddr:" + QString::number(m_initData.curUsedRtu.rtuAddr)+"!!! rtuId:"+ m_initData.curUsedRtu.rtuId +", cchId:" + m_cchId);
        return false;
    }

    LOG_DEBUG(m_cchId,"curUsedRtu.rtuId ="+ m_initData.curUsedRtu.rtuId+", m_initData.curUsedRtu.rtuAddr="+ QString::number(m_initData.curUsedRtu.rtuAddr));
    return true;
}


/**
 * @brief 初始化
 * @return true->成功，false->失败
 */
bool Protocol104Plugin::init()
{
    return m_isInitSuccess;
}

/**
 * @brief 处理从装置（RTU）获取的数据，放到规约接收缓存中
 */
void Protocol104Plugin::recieveFromDeviceSlot(const QByteArray &data)
{
    std::string frame = std::string(data.data(), data.size());
    LOG_INFO(m_cchId, "["+ m_initData.chnCfg.id+"]\n<Recv> " + QString::fromStdString(StringUtil::toHexString(frame, " ")));
    if(m_session)
    {
        m_session->slotRead(frame);
    }
}

/**
 * @brief 处理从采集网关机获取的数据，具体命令类型处理后，下发到装置（RTU）
 */
void Protocol104Plugin::recieveFromGatewaySlot(const QVariantHash &data)
{
    LOG_INFO(m_cchId, "====>data="+ QString(AcqHashToJson(data)));
    if(m_session)
    {
        m_session->sendCmdData(data);
    }
}

/**
 * @brief 接收装置连接状态变化
 * @param state
 */
void Protocol104Plugin::recieveNetStateChangedSlot(int state)
{
    LOG_INFO(m_cchId, "state=" + QString::number(state));
    bool isConnected = (1==state)?true:false;
    if(isConnected)
    {
        if(m_session)
        {
            if(!m_session->isConnected())
            {
                m_session->setIsConnected(isConnected);
            }
        }
        else
        {
            //创建新会话
            createSession();
            m_session->setIsConnected(isConnected);
        }
    }
    else//连接断开
    {
        if(m_session && m_session->isConnected())
        {
            m_session->setIsConnected(isConnected);
            //m_session->finalize();
            clearSession();//清除当前会话
        }
    }
    //LOG_DEBUG(m_cchId,"END Protocol104Plugin::recieveNetStateChangedSlot::state="<<state);
}
