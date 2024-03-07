/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_rawdatadb.cxx
 *生成日期：2012-01-01
 *作者：    yay
 *功能说明：实现与规约库的接口类SCN_RawdataDb. 该类为规约库提供所需的系统数据存储接口
 *其它说明：数据库中的change_flag已改为有效标志之意,表示数据是否有效
 *修改记录：date, maintainer
 *          2012-01-01, yay, 建立代码
*******************************************************************************/

#include <string.h>
#include <math.h>
#include "scn_rawdatadb.h"
#include "plugin104_define.h"
#include "../acq-protocol-plugin-utils/string_util.h"
#include "utils/converter_util.h"
#include "acq_json.h"
#include <QDateTime>

static std::atomic_llong yx_counts(0);
static std::atomic_llong yc_counts(0);
static std::atomic_llong soe_counts(0);
static std::atomic_llong dd_counts(0);

static std::atomic_llong yx_send_times(0);
static std::atomic_llong yc_send_times(0);
static std::atomic_llong soe_send_times(0);
static std::atomic_llong dd_send_times(0);


/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 将收到的sci配置写入数据库中
输入参数：
             const QString &rtuId,        Rtu号
             const QString &ChnId,     通道号
             uint8 *buf        sci配置
输出参数：  
             无
            
返回值：     无        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void SCN_RawdataDb::PutSciConfig( const QString &rtuId, const QString &ChnId, uint8 *buf )
{
    LOG_DEBUG(this->m_chnId,"");
}

/**********************************************************************************************************
bh=”\0”情况下，bsInfo= SICD_M_WF_STS_ON_LINE且yxInfo= SICD_M_WF_STS_ON_LINE时为操作票的总解锁
bh=”\0”情况下，bsInfo=yxInfo=SICD_M_YKBS_ON_LINE | 0x10时，为遥控闭锁总解锁
bh=”\0”情况下，bsInfo=yxInfo=SICD_M_YKBS_ON_LINE 时，为遥控闭锁总闭锁

bh!=”\0”情况下,bsInfo与yxInfo值中有SICD_M_YKBS_ON_LINE掩码的为遥控闭锁的值，
               其中设备状态为yxInfo,1为合位，0为分位
               解锁闭锁信息存放在bsInfo中，0为解锁，1为闭锁

bh!=”\0”情况下,bsInfo与yxInfo值中有SICD_M_WF_ON_LINE掩码的为模拟屏传送的遥信与操作票闭锁的值，
               无此掩码的，则表示值不可用
**********************************************************************************************************/

void SCN_RawdataDb::PutAWf( const QString &rtuId, const QString &chnId, char *bh, sint16 yxVal, sint16 bsVal,int ykbs )
{
    QString msg = StringUtil::toQString("rtuId:%s, chnId:%s, bh:%s, yxVal:%d, bsVal:%d, ykbs:%d. ",rtuId.toStdString().c_str(),chnId.toStdString().c_str(),bh,yxVal,bsVal,ykbs);

    LOG_DEBUG(this->m_chnId,msg);
}


/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 向数据库写入指定Rtu的第KwhNo号电度值
输入参数：
             const QString &rtuId,        Rtu号
             int KwhNo,     电度号
             float32 KwhValue    电度值
             uint16 KwhStatus    电度状态
输出参数：  
             无
            
返回值：     无        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void SCN_RawdataDb::PutAKwh( const QString &rtuId, const QString &chnId, int KwhNo, float32 KwhValue, uint16 KwhStatus )
{
    QString msg = StringUtil::toQString("rtuId:%s, chnId:%s, KwhNo:%d, KwhValue:%f, KwhStatus:%d. ",rtuId.toStdString().c_str(),chnId.toStdString().c_str(),KwhNo,KwhValue,KwhStatus);
    LOG_DEBUG(this->m_chnId,msg);
}


/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 向数据库写入RtuId号厂站的一个事项soe
输入参数：
             const QString &rtuId        Rtu号
             int YxNo,        遥信号
             SOEDATA Soe,    soe事项数据
             uint16 SoeStatus,     状态
输出参数：  
             无
            
返回值：     无        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
#undef SCN_FUNC
#define SCN_FUNC "PutASoe" 
void SCN_RawdataDb::PutASoe(const QString &rtuId, const QString &chnId, int YxNo, SOEDATA Soe, uint16 SoeStatus )
{
    char timeStr[200]={0};
    sprintf(timeStr,"%d-%02d-%02d %02d:%02d:%02d.%03d", Soe.soetime.year,Soe.soetime.month,Soe.soetime.day,Soe.soetime.hour,Soe.soetime.minute,Soe.soetime.second,Soe.soetime.ms);
    long long timeStamp = QDateTime::fromString(QString::fromLocal8Bit(timeStr), "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
    QString msg = StringUtil::toQString("rtuId:%s, chnId:%s, YxNo:%d, Soe.YxValue:%d, SoeStatus:%d, timeStr:%s, timeStamp:%lld ",rtuId.toStdString().c_str(),chnId.toStdString().c_str(),YxNo,Soe.YxValue,SoeStatus, timeStr, timeStamp);

    LOG_DEBUG(this->m_chnId,msg);
    std::list<std::shared_ptr<BaseParam_S>> dataList;
    std::shared_ptr<YXParam_S> yxparam = std::make_shared<YXParam_S>();
    yxparam->rtuId = rtuId;
    yxparam->no = YxNo;
    yxparam->value = Soe.YxValue;
    yxparam->quality = SoeStatus;
    yxparam->time = timeStamp; //QDateTime::currentMSecsSinceEpoch();
    std::shared_ptr<BaseParam_S> param = yxparam;
    dataList.push_back(param);

    QVariantHash dataHash = ConverterUtil::toSOEHash(dataList);
    soe_counts += dataList.size();
    soe_send_times++;
    QString curtimeStr =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    std::string logMsg = QString("callRpc~~~SOE::[chn:%1,rtu:%2][%3],times:%4,CJ->[yx:%5,yc:%6,soe:%7]").arg(this->m_chnId).arg(this->m_rtuId).arg(curtimeStr).arg(yx_send_times.load()).arg(yx_counts.load()).arg(yc_counts.load()).arg(soe_counts.load()).toStdString();
    printf("%s\n", logMsg.c_str());

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByRpc(dataHash);//通过RPC上送【遥信】
    }
}

/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 取得Yx_Var_Out[ChnId}中的1个变化遥信的遥信号和状态值.
输入参数：
             const QString &rtuId        Rtu号
             const QString &ChnId        通道号

输出参数：  
             int *YxNo        遥信号
             uint8 *YxValue,     遥信值
             uint8 *Flag    变化标志
                         
返回值：         =1, 取得的遥信号和遥信值有效;
        =0, 无效.    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8 SCN_RawdataDb::GetAVarYx( const QString &rtuId, const QString &ChnId, int *YxNo, uint8 *YxValue, uint8 *Flag )
{
    LOG_DEBUG(this->m_chnId,"");
    //没有数据返回0
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 从数据库取得变化Soe数.
输入参数：
             const QString &rtuId        Rtu号
             const QString &ChnId        通道号
             

输出参数：  
             无
                         
返回值：         
        >=0,     Soe数    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RawdataDb::GetVarSoeNum( const QString &rtuId, const QString &ChnId )
{
    return 0;
}
/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 从数据库取得RtuId号厂站的一个变化Soe数据.
输入参数：
             const QString &rtuId        Rtu号
             const QString &ChnId        通道号
             
输出参数：  
             int *YxNo        遥信号
             uint8 *Flag    变化标志
                         
返回值：         =1, 取得的遥信值有效;
        =0, 无效.    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8 SCN_RawdataDb::GetAVarSoe( const QString &rtuId,const QString &ChnId, int *YxNo, SOEDATA *Soe )
{

    LOG_DEBUG(this->m_chnId,"");
  //没有数据返回0
    return 0;
}
/************************************************************************
作者：   lee     
创建日期：2009.5.27   
函数功能说明： 从数据库取得RtuId号厂站的一个Soe数据.
输入参数：
             const QString &rtuId        Rtu号
             
输出参数：  
             int *YxNo        遥信号
             SOEDATA *Soe    Soe数据
                         
返回值：         =1, 取得的遥信值有效;
        =0, 无效.    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8 SCN_RawdataDb::GetASoe( const QString &rtuId, int *YxNo, SOEDATA *Soe )
{
    LOG_DEBUG(this->m_chnId,"");
    return 0;
}


/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 取得Yx_Var_Out[ChnId-1}中的变化遥信的个数.
输入参数：
             const QString &rtuId        Rtu号
             const QString &ChnId        通道号
输出参数：

返回值：     >=0 变化遥信的个数.

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
uint32 SCN_RawdataDb::GetVarYxNum( const QString &rtuId, const QString &ChnId )
{
    return 0;
}

/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 找到hop_rtu_no是RtuId+1的RTU,如果该RTU的主通道在本地节点上,则返回TRUE.
输入参数：
             const QString &rtuId        Rtu号
输出参数：

返回值：     TRUE            有效
         FALSE            无效

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
#undef SCN_FUNC
#define SCN_FUNC "IsOutRtuDataValid"
bool SCN_RawdataDb::IsOutRtuDataValid( const QString &rtuId )
{
    LOG_DEBUG(this->m_chnId,"");
    return false;
}

/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 从数据库取得RtuId号KwhNo号电度值.
输入参数：
             const QString &rtuId        Rtu号
             int KwhNo        遥信号


输出参数：
             float32 *KwhValue,     电度值
             uint8 *Flag        变化标志

返回值：         =1, 取得的遥信值有效;
        =0, 无效.

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
uint8 SCN_RawdataDb::GetAKwh( const QString &rtuId, int KwhNo, float32 *KwhValue, uint8 *Flag )
{
    LOG_DEBUG(this->m_chnId,"");
    return 1;
}

/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 从数据库取得RtuId号厂站YcNo号遥测值
输入参数：
             const QString &rtuId        Rtu号
             int YcNo,        遥测号
             uint8 *Flag    遥测标志
输出参数：
             float32 *YcValue,    取得的遥测值

返回值：     =1            取得的遥测值有效
         =0            无效

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
uint8 SCN_RawdataDb::GetAYc( const QString &rtuId, int YcNo, float32 *YcValue, uint8 *Flag )
{
    LOG_DEBUG(this->m_chnId,"");
    return 1;
}

/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 从数据库取得RtuId号厂站YxNo号遥信状态值.
输入参数：
             const QString &rtuId        Rtu号
             int YxNo        遥信号

输出参数：
             uint8 *YxValue,     遥信值
             uint8 *Flag    变化标志

返回值：         =1, 取得的遥信值有效;
        =0, 无效.

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
uint8 SCN_RawdataDb::GetAYx( const QString &rtuId, int YxNo, uint8 *YxValue, uint8 *Flag )
{
    LOG_DEBUG(this->m_chnId,"");
    return 1;
}


/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 上送遥测
输入参数：
             const QString &rtuId,        Rtu号
             int YcNo,         遥测号
             float32 YcValue    遥测值
             uint32 ycbin    遥测整数值
             uint16 YcStatus    遥测状态
输出参数：
             无

返回值：     无

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
void SCN_RawdataDb::PutAYc( const QString &rtuId, int YcNo, float32 YcValue, uint32 YcBin, uint16 YcStatus )
{
    std::list<std::shared_ptr<BaseParam_S>> paramList;
    std::shared_ptr<YCParam_S> ycparam = std::make_shared<YCParam_S>();
    ycparam->rtuId = rtuId;
    ycparam->no = YcNo;
    ycparam->value = YcValue;
    ycparam->quality = YcStatus;
    ycparam->time = QDateTime::currentMSecsSinceEpoch();
    std::shared_ptr<BaseParam_S> param = ycparam;
    paramList.push_back(param);
    PutYcList(false, paramList);
}

/************************************************************************
作者：   lee
创建日期：2009.5.27
函数功能说明： 向数据库写入指定Rtu的第YxNo号遥信值
输入参数：
             const QString &rtuId,        Rtu号
             int YxNo,         遥信号
             float32 YxValue    遥信值
             uint16 YxStatus    遥信状态
输出参数：
             无

返回值：     无

函数扇出清单：
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：
*************************************************************************/
void SCN_RawdataDb::PutAYx( const QString &rtuId, int YxNo, uint8 YxValue, uint16 YxStatus )
{
    std::list<std::shared_ptr<BaseParam_S>> paramList;
    std::shared_ptr<YXParam_S> yxparam = std::make_shared<YXParam_S>();
    yxparam->rtuId = rtuId;
    yxparam->no = YxNo;
    yxparam->value = YxValue;
    yxparam->quality = YxStatus;
    yxparam->time = QDateTime::currentMSecsSinceEpoch();
    std::shared_ptr<BaseParam_S> param = yxparam;
    paramList.push_back(param);
    PutYxList(false, paramList);
}

/**
 * @brief 批量上送遥信状态
 * @param isCycCallAll 是否周期总召数据
 * @param dataList
 */
void SCN_RawdataDb::PutYxList(bool isCycCallAll,  std::list<std::shared_ptr<BaseParam_S>> &dataList)
{
    if(dataList.size() == 0)
    {
        return;
    }
    QVariantHash dataHash = ConverterUtil::toYXHash(isCycCallAll, dataList);
    yx_counts += dataList.size();
    yx_send_times++;
    QString timeStr =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    std::string logMsg = QString("callRpc~~~YX::[chn:%1,rtu:%2][%3],times:%4,CJ->[yx:%5,yc:%6,dd:%7]").arg(this->m_chnId).arg(this->m_rtuId).arg(timeStr).arg(yx_send_times.load()).arg(yx_counts.load()).arg(yc_counts.load()).arg(dd_counts.load()).toStdString();
    printf("%s\n", logMsg.c_str());

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByRpc(dataHash);//通过RPC上送【遥信】
    }
}


/**
 * @brief 批量上送遥测值
 * @param isCycCallAll 是否周期总召数据
 * @param dataList
 */
void SCN_RawdataDb::PutYcList(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S>> &dataList)
{
    if(dataList.size() == 0)
    {
        return;
    }
    QVariantHash dataHash = ConverterUtil::toYCHash(isCycCallAll, dataList);
    yc_counts += dataList.size();
    yc_send_times++;
    QString timeStr =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    std::string logMsg = QString("callRpc~~~YC::[chn:%1,rtu:%2][%3],times:%4,CJ->[yx:%5,yc:%6,dd:%7]").arg(this->m_chnId).arg(this->m_rtuId).arg(timeStr).arg(yc_send_times.load()).arg(yx_counts.load()).arg(yc_counts.load()).arg(dd_counts.load()).toStdString();
    printf("%s\n", logMsg.c_str());

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByRpc(dataHash);//通过RPC上送【遥测】
    }
}

void SCN_RawdataDb::PutKwhList(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S> > &dataList)
{
    if(dataList.size() == 0)
    {
        return;
    }
    QVariantHash dataHash = ConverterUtil::toDDHash(isCycCallAll, dataList);
    dd_counts += dataList.size();
    dd_send_times++;
    QString timeStr =  QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    std::string logMsg = QString("callRpc~~~DD::[chn:%1,rtu:%2][%3],times:%4,CJ->[yx:%5,yc:%6,dd:%7]").arg(this->m_chnId).arg(this->m_rtuId).arg(timeStr).arg(yc_send_times.load()).arg(yx_counts.load()).arg(yc_counts.load()).arg(dd_counts.load()).toStdString();
    printf("%s\n", logMsg.c_str());

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByRpc(dataHash);//通过RPC上送【遥测】
    }
}

/**
 * @brief 上送总召命令到上层应用(作为子站才会用到)
 */
void SCN_RawdataDb::SendCmdCallData()
{
    //std::string dataJson = "{\"cmdType\":\""+CMD_TYPE_STR_CALLDATA+"\",\"source\":\""+this->m_source+"\"}";
    //LOG_DEBUG(this->m_chnId,dataJson);

    CallReqParam_S data;
    QVariantHash dataHash = ConverterUtil::toCallDataHash(data);

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByMq(dataHash);//通过MQ上送
    }
}

/**
 * @brief 上送总召应答(结束)命令到上层应用
 */
void SCN_RawdataDb::SendCmdCallDataEnd(QString rtuId, QString cchId, int result, const QString &reason)
{
    CallRspParam_S data;
    data.rtuId=rtuId;
    data.cchId=cchId;
    data.result = result;
    data.reason = reason;
    QVariantHash dataHash = ConverterUtil::toCallDataEndHash(data);

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByMq(dataHash);//通过MQ上送
    }
}

/**
 * @brief 上送遥控命令到上层应用(作为子站才会用到)
 */
void SCN_RawdataDb::SendCmdYk(const YKReqParam_S &data)
{
    //std::string dataListStr = StringUtil::toString("{\"rtuAddr\":%d,\"pointAddr\":%d,\"ykCmd\":%d,\"value\":\"%s\"}",data.rtuAddr,data.pointAddr,data.ykCmd,data.value.c_str());//rtuAddr--RTU地址(公共地址码)；pointAddr--遥控点地址(序号+遥控基地址（不确定是否要+基地址？）)；value--RTU遥控状态(0-分，1-合）
    //std::string dataJson = MakeSendDataJson(CMD_TYPE_STR_YK, this->m_source, dataListStr);
    //LOG_DEBUG(this->m_chnId,dataJson);

    QVariantHash dataHash = ConverterUtil::toYKReqHash(data);

    if(m_protoInterface)
    {
        m_protoInterface->sendToGatewayByMq(dataHash);//通过MQ上送
    }
}

/**
 * @brief 设置所连接的设备IP和端口(发送源头端)
 * @param source 所连接的设备IP和端口
 */
void SCN_RawdataDb::SetSource(const std::string &source)
{
    //LOG_DEBUG(this->m_chnId,"m_source:"+source);
    this->m_source = source;
}

/**
 * @brief 生成发送JSON命令
 * @param cmdType 命令类型
 * @param source 源端
 * @param dataListStr 数据列表
 * @return
 */
/*std::string SCN_RawdataDb::MakeSendDataJson(const std::string &cmdType, const std::string &source, const std::string &dataListStr)
{
    std::string dataJson = "{\"cmdType\":\"";
    dataJson.append(cmdType);
    dataJson.append("\",\"source\":\"");
    dataJson.append(source);
    dataJson.append("\",\"dataList\":[");
    dataJson.append(dataListStr);
    dataJson.append("]}");
    return dataJson;
}*/
