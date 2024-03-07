/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_routeinf.cxx
 *生成日期：2012-01-01
 *作者：    yay
 *功能说明：实现与规约库的接口类SCN_RouteInf.该类为规约库提供所需的Route参数信息
 *其它说明：
 *修改记录：date, maintainer
 *          2012-01-01, yay, 建立代码
*******************************************************************************/

#include "scn_routeinf.h"
#include "plugin104_define.h"

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 初始化路由号
输入参数：
             uint32 routeno        路由号
输出参数：  
             无
            
返回值：     无        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void SCN_RouteInf::Init( uint32 routeno )
{
    //RouteNo = routeno;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取得本Route连接的Rtu号。
输入参数：
             FrmStatType type        为枚举类型，标识帧类别。
             sint32 rtuId        收发帧所属Rtu号
输出参数：  
             无
            
返回值：     >=0    Rtu号
         <0        数据库读取错误        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
QString SCN_RouteInf::GetRtuId( void )
{
    return m_rtuId;
}

/************************************************************************
作者：   yay     
创建日期：2010.10.2009   
函数功能说明： 取得本Route连接的Rtu类型。
输入参数：
            无
输出参数：  
             无
            
返回值：    >=0  Rtu类型
              -1  数据库读取错误        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
sint32 SCN_RouteInf::GetRtuType( void )
{
#if CFQ
    int routeidx;
    int rtuidx;

    /*routeidx = COM_DbRouteGetIdx( RouteNo+1 );*/
    routeidx = SLM_UTL_GetRouteIdx( RouteNo+1 );
    if( routeidx >= 0 )
    {
        rtuidx = COM_Route[routeidx].rtu_idx;
        if( rtuidx >= 0 )
            return COM_Rtu[rtuidx].rtu_type;
    }
#endif
    return -1;    
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取得本Route连接的Channel号。
输入参数：
             无
输出参数：  
             无
            
返回值：     >=0    Channel号
         <0        数据库读取错误        
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
QString SCN_RouteInf::GetChnId( void )
{
    return this->m_chnId;
}
/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取得本Route号。
输入参数：
             无
输出参数：  
             无
            
返回值：     本Route号    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
sint32 SCN_RouteInf::GetRouteNo( void )
{
    //return RouteNo;
    return -1;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 请求许可。
输入参数：
             PermissionRequest req     请求内容
输出参数：  
             无
            
返回值：      
        =1，许可；
        =0，不许可。    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8 SCN_RouteInf::ReqPermission( PermissionRequest req )
{
#if CFQ
    sint32 ChnId = GetChnId();
    if( ChnId < 0 )
        return 0;
    
    switch( req )
    {
    case Ordinarily_Send:    //普通数据发送
        if( Channel[ChnId].ChanLevel == COMMLEVEL_MAIN_1 ||
            Channel[ChnId].ChanLevel == COMMLEVEL_MAIN_2 ||    /*add for foshan 2002.12.9*/
            Channel[ChnId].ChanLevel == COMMLEVEL_SLAVE_1 ||
            GetHopFlag() )
            return 1;
        else return 0;
    case Command_Send:        //命令发送
        if( Channel[ChnId].ChanLevel == COMMLEVEL_MAIN_1 )
            return 1;
        else return 0;
    /*add for foshan 2002.12.9*/
    case Query_Send:
        if( Channel[ChnId].ChanLevel == COMMLEVEL_MAIN_2)
            return 1;
        else
            return 0;
    default:
        return 0;
    }
#else
    return 1;//@todo cfq
#endif
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取本Route的通讯重试次数
输入参数：
             无
输出参数：  
             无
            
返回值：      >0    重试次数    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
sint32 SCN_RouteInf::GetRetryTimes( void )
{
    if(m_protocolCfgParam.retryTimes <= 0)
    {
        return 3;
    }
    return m_protocolCfgParam.retryTimes;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取本Route的接收超时时限
输入参数：
             无
输出参数：  
             无
            
返回值：      >0    超时时限    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
sint32 SCN_RouteInf::GetRxTimeouts( void )
{
    return 10;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取本Route的调试标志，规约库据此判断是否调用Log_Event()记录事件。
输入参数：
             无
输出参数：  
             无
            
返回值：      1        记录
          0        不记录    
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8 SCN_RouteInf::GetDebugFlag( void )
{
    return 1;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取本Route的全数据扫描周期（秒）
输入参数：
             无
输出参数：  
             无
            
返回值：      >0    全数据扫描周期
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RouteInf::GetAllScan( void )
{
    if(m_protocolCfgParam.scanFullTime <= 0)
    {
        /*chenfuqing 20230914 modified 轨道需求：总召时间间隔（秒）为 0时 ，不要发总召命令*/
        return 0;//300;
    }
    return m_protocolCfgParam.scanFullTime;
}


uint32 SCN_RouteInf::GetScanPulseTime( void )
{
    if(m_protocolCfgParam.scanPulseTime <= 0)
    {
        /*chenfuqing 20230914 modified 轨道需求：脉冲召唤周期（秒）为 0时 ，不要脉冲召唤命令*/
        return 0;//300;
    }
    return m_protocolCfgParam.scanPulseTime;
}

/************************************************************************
作者：   lee     
创建日期：2009.5.20   
函数功能说明： 取本Route的命令重连标志
输入参数：
             无
输出参数：  
             无
            
返回值：     无
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void SCN_RouteInf::UpdateRouteDev()
{
    LOG_INFO(this->m_chnId, QString("SCN_RouteInf::UpdateRouteDev...try to reconnect net! chnId:%1").arg(GetChnId()))
    if(m_protoInterface)
    {
        emit m_protoInterface->reconnectDeviceSig();
    }
}

void SCN_RouteInf::SetRtuId(const QString &rtuId)
{
    this->m_rtuId = rtuId;
}

void SCN_RouteInf::SetRtuAddr(uint32 rtuAddr)
{
    this->m_rtuAddr = rtuAddr;
}


void SCN_RouteInf::SetProtocolcfgParam(const ProtocolCfgParam_S &protocolCfgParam)
{
    this->m_protocolCfgParam= protocolCfgParam;
}

uint32 SCN_RouteInf::GetRtuAddr()
{
    return this->m_rtuAddr;
}

void SCN_RouteInf::SetChnId(const QString &chnId)
{
    this->m_chnId = chnId;
}
