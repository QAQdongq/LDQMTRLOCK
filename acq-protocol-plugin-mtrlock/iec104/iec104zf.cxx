/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  iec104zf.cxx
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：实现规约类Iec104Zf
 *其它说明：description
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/
#if 0//cfq 转发规约（子站）才用
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <math.h>

#if defined(__linux) || defined(_AIX)
#include <unistd.h>
#endif

#include "iec104zf.h"
#include "../acq-protocol-plugin-utils/string_util.h"

void Iec104Zf::Restore()
{
    //Restore 清空接受缓冲区是必须的,已免处理上次连接的残留字节流yay
    pRxBuf->clear();
    //pTxBuf->clear();

    InitData( );

//  有无必要把变化数据清掉?
//    App_Layer.ChangeYxBuf.Clear( );
//    App_Layer.SoeBuf.Clear( );
//  App_Layer.ChangeYcBuf.Clear( );

    restore_flag = false;

    LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("\nIec104Zf::Restore channo=%s rtuaddr=%d\n",pRouteInf->GetChnId().toStdString().c_str(),m_rtuAddr));
}

/********************************************************************************
*
*    描述: 规约参数设置函数,负责初始化规约接口对象和规约对象内部数据.
*    参数: protocol_config: 规约配置参数(输入)；
*    返回: 无.
*
********************************************************************************/

void Iec104Zf::SetProtocolConfig( PROTOCOL_CONFIG protocol_config )
{
    Poll::SetProtocolConfig( protocol_config );
    
    m_rtuId = pRouteInf->GetRtuId();
    m_rtuAddr = pRouteInf->GetRtuAddr();
    //GetDebugConfig();
    InitData( );
    App_Layer.ChangeYxBuf.Create( 128, sizeof(IEC104_ChangeYx) );
    App_Layer.SoeBuf.Create( 128, sizeof(IEC104_Soe) );
    App_Layer.ChangeYcBuf.Create( 512, 2 );
}


/********************************************************************************
*
*    描述: 初始化规约数据.
*    参数: 无.
*    返回: 无.
*
********************************************************************************/

void Iec104Zf::InitData( void )
{
    TIME curtime;
    rxStep = 0;

    App_Layer.RxWindowNum = 0;
    App_Layer.Inited_Flag = 0;
    App_Layer.SendACKFlag = 0;
    App_Layer.WaitACKFlag = 0;
    App_Layer.State = IEC104_APP_STATE_UNRESET;
    App_Layer.Snd_seqnum = 0;
    App_Layer.Rec_seqnum = 0;
    App_Layer.Ack_num = 0;
    GetCurSec( &curtime );
    App_Layer.LastTxTime = curtime.Sec;
    App_Layer.AckStep = 0;
    App_Layer.rxSize = 0;
    App_Layer.AckYcIdx = 0;
    App_Layer.AckYxIdx = 0;
    App_Layer.AckKwhIdx = 0;

  strcpy(delete_name_saved,"initial_name");

    IfCheckLinkOk = 1;
    IfAndPnBit = 1;
    StartCharErr = 0;
    AppTypeErr = 0;
    CotErr = 0;
    ComAddrErr = 0;
    IfCheckComAddr = 1;

    //ReadConfigParam( );
}

/********************************************************************************
*
*    描述: 普通遥控，一键顺控104协议，智能防误主机模式单点遥控解锁
*    参数: 无.
*    返回: 0  本轮问答过程未结束
*          1  本轮问答过程结束
*  
********************************************************************************/
int Iec104Zf::EditLockFrame(COMMAND *cmd)
{
    return 1;
}

int Iec104Zf::TxProc( void )
{
    if(restore_flag)
        Restore();

    if( pRouteInf->ReqPermission( Ordinarily_Send ) == 0 )
    {
        InitData( );
        return 1;
    }

#if CFQ    
    COMMAND *command;
    command = (COMMAND*)malloc( sizeof(COMMAND));
    if( 0 == command )
    {
            LOG_DEBUG(pRouteInf->GetChnId(), "Iec104Zf::TxProc command malloc error!\n");
            return 1;
    }
    memset( command, 0, sizeof(COMMAND) );
    if( pCommandMem->GetACommand( rtu_no, command) == 1 )
    {
        if( command->CmdType == CMD_TYPE_WFDATA_INFO )
        {
            SICD_D_WFDATA_INFO *pWfDataInfo = (SICD_D_WFDATA_INFO *)command->CmdData;
            ProcessOneCommand(*pWfDataInfo);
        }
        else if( command->CmdType == CMD_TYPE_LOCK )/* wyh 2019-12-05 add*/
        {
            EditLockFrame(command);
        }
        pCommandMem->RemoveACommand( rtu_no );
    }
    free(command);
#else

    std::shared_ptr<COMMAND> command;
    if( pCommandMem->GetACommand(command) == 1 )
    {
        if( App_SendCommand(command.get() ) == 1 )
        {
            pCommandMem->RemoveACommand();
        }
    }
#endif

    if( 1 == App_Layer.Inited_Flag )
        App_SearchChangeYc();

#if CFQ
  /******************************如果是整站转发*****yay*****************************/
  if(pRtuInf->GetRtuType( rtu_no ) == 7)//整站转发
  {
      bool isvalid = pRawDb->IsOutRtuDataValid(rtu_no);
      //如果该站的数据没有效,就不给对方回答任何数据
      if(!isvalid)
      {
            ClearYxSoeVarOut();
          return 1;
      }
  }
  /*******************************************************************************/
#endif
    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_ACK:        
        //LOG_DEBUG(pRouteInf->GetChnId(), "App_Layer.State=IEC104_APP_STATE_ACK");
        if( App_Ack() == 1 )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
        }
        break;
    case IEC104_APP_STATE_IDLE:
        if( 1 == App_Layer.Inited_Flag )
        {
            if( App_Layer.ChangeYxBuf.GetDataNum() > 0 )
            {
                App_Ack_ChangeYx( ACK_UNSOLICITED );    
                break;
            }
            if( App_Layer.SoeBuf.GetDataNum() > 0 )
            {    
                App_Ack_Soe();
                break;
            }
            if( App_Layer.ChangeYcBuf.GetDataNum() > 0 )
            {    
                App_Ack_ChangeYc();
                break;
            }
        }
        if( App_SendAppUFormat( APP_UFORMAT_TESTACT )==1 )    //Send U(TESTFR act) format
        {
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Send Test Act Frame when idle===\n");

            App_Layer.State = IEC104_APP_STATE_WAITTESTCONF;
            return 1;
        }
        break;
        
    case IEC104_APP_STATE_WAITTESTCONF:
        {    
            TIME curtime;
            
            GetCurSec( &curtime );
            if( curtime.Sec-App_Layer.LastTxTime > Config_Param.OverTime1 )//t1推荐15秒
            {
                if( pRouteInf->GetDebugFlag() )
                {
                    LOG_WARN(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Wait TestConfirm out of time !!!  ---\n");
                    LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("IEC104ZF:IEC104zf Wait TestConfirm App_Lyer.State %d , time out %d s ,Curtime_Sec: %lu s\n",App_Layer.State,Config_Param.OverTime1, curtime.Sec));
                }
//                pRouteInf->RecAFrame( PROTO_RXTIMEOUTS );
                App_Layer.State = IEC104_APP_STATE_IDLE;

                InitData( );         //重新初始化

                pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay
                AckFinished = 1;    //接收结束    
            }
        }
        break;
    case IEC104_APP_STATE_UNRESET:
        if( App_SendAppUFormat( APP_UFORMAT_TESTACT )==1 )    //Send U(TESTFR act) format
        {
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Send Test Act Frame when unreset===\n");

            App_Layer.State = IEC104_APP_STATE_WAITTESTCONF;
            return 1;
        }
        break;
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf unknow App_Layer.State ---\n");

        break;
    }
    
    if( App_Layer.SendACKFlag == 1 )                //Send S format
    {
        if( App_SendAppSFormat() == 1 )
        {
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104ZF:IEC104zf Send S(%d) Frame ===\n",App_Layer.Rec_seqnum));
        }
    }
    
    return AckFinished;
}

/********************************************************************************
*
*    描述: 响应主站的请求函数.
*    参数: 无.
*    返回： 0  发送失败
*           1  发送成功
*
********************************************************************************/
int Iec104Zf::App_Ack( void )
{
    int ret;
    switch( App_Layer.rxReq.type )
    {
    case APPTYPE_CALLDATA:        //100
        if( App_Layer.rxReq.detail == 0x14 )    //总召唤
            ret = App_Ack_AllData();
        else 
            ret = App_Ack_GroupData( App_Layer.rxReq.detail-0x14 ); 
        break;
    case APPTYPE_CALLKWH:        //101
        if( (App_Layer.rxReq.detail&0x3f) == 5 )
            ret = App_Ack_AllKwh();
        else 
            ret = App_Ack_GroupKwh( App_Layer.rxReq.detail );
        break;
    case APPTYPE_TIMESYNC:        //103
        ret = App_Ack_SyncTime();
        break;
    case APPTYPE_RSTPROC:
        ret = App_Ack_Rst();
        break;
    default:
        ret = App_Ack_OtherType(App_Layer.rxReq.type,App_Layer.rxData+6,App_Layer.rxSize-6);
        break;
    }

    if( ret == 1 )
    {
        AckFinished = 1;
        return 1;
    }
    else return 0;
}

int Iec104Zf::App_Ack_OtherType(int apptype, uint8 *appdata, int datalen)
{
    return 1;
}

/*******************************************************************************
*
*    描述：发送某一组电能脉冲计数量
*    参数：组号group,取值要求主站和子站应一致（分成4组，每组32个量）
*    返回：1    发送成功
*          0 发送失败            
*
********************************************************************************/
int Iec104Zf::App_Ack_GroupKwh( int group )
{
#if CFQ
    uint8 buf[256], writept, kwhnum,i;
    float32 kwhfval;
    uint32 kwhval;
    uint16 kwhtotal, kwhidx;

    kwhtotal = pRtuInf->GetKwhNum( rtu_no );
    if( kwhtotal > 128 )
        kwhtotal = 128;
 
    kwhidx = (group-1)*IEC104_KWHNUM_PERGROUP;
    if( kwhidx >= kwhtotal )
    {
        //AckFinished = 1;
        return 1;
    }

    i = 2;

    buf[0] = 15;
    //buf[1] = 

    buf[i++] = (37+group)%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = (37+group)/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = (kwhidx+Config_Param.DdBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (kwhidx+Config_Param.DdBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (kwhidx+Config_Param.DdBaseAddr)/256%256;
        buf[i++] = (kwhidx+Config_Param.DdBaseAddr)/256/256;
    }
    
    kwhnum = 0;
    writept = i;

    while( kwhnum <= IEC104_KWHNUM_PERGROUP )
    {
        if( kwhidx >= kwhtotal )
            break;
        
        pRawDb->GetAKwh( rtu_no, kwhidx, &kwhfval );
        kwhval = kwhfval;
        buf[writept++] = kwhval%256;
        buf[writept++] = (kwhval/256)%256;
        buf[writept++] = (kwhval/256/256)%256;
        buf[writept++] = kwhval/256/256/256;
        buf[writept++] = kwhnum;

        kwhnum++;
        kwhidx++;
    }

    buf[1] = kwhnum|0x80;            
    if( App_SendAppIFormat( buf, writept ) == 1 )
        return 1;
    else
        return 0;
#else
    //cfq 20200605 暂不支持，但还是要进行回复，以便完成整个请求与响应，否则会出现接收与发送序号不匹配情况
    uint8 buf[256], writept, kwhnum,i;
    float32 kwhfval;
    uint32 kwhval;
    uint16 kwhtotal, kwhidx;

    //cfq 20200605 无数据可发送
    kwhtotal = 0;
    kwhidx=0;

    i = 2;

    buf[0] = 15;
    //buf[1] =

    buf[i++] = (37+group)%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = (37+group)/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = (kwhidx+Config_Param.DdBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (kwhidx+Config_Param.DdBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (kwhidx+Config_Param.DdBaseAddr)/256%256;
        buf[i++] = (kwhidx+Config_Param.DdBaseAddr)/256/256;
    }

    kwhnum = 0;
    writept = i;

    while( kwhnum <= IEC104_KWHNUM_PERGROUP )
    {
        if( kwhidx >= kwhtotal )
            break;

        pRawDb->GetAKwh( 0, kwhidx, &kwhfval );//pRawDb->GetAKwh( rtu_no, kwhidx, &kwhfval );
        kwhval = kwhfval;
        buf[writept++] = kwhval%256;
        buf[writept++] = (kwhval/256)%256;
        buf[writept++] = (kwhval/256/256)%256;
        buf[writept++] = kwhval/256/256/256;
        buf[writept++] = kwhnum;

        kwhnum++;
        kwhidx++;
    }

    buf[1] = kwhnum|0x80;
    if( App_SendAppIFormat( buf, writept ) == 1 )
        return 1;
    else
        return 0;
#endif
}

/*******************************************************************************
*
*    描述：发送遥控响应帧
*    参数：无
*    返回：1                
*
********************************************************************************/
int Iec104Zf::App_Ack_Yk(COMMAND *command)
{
    if(NULL == command)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "command is null!");
        return -1;
    }
    if(command->dataList.size() != 1) //一次只能发送一个遥控确认命令；所以下发时只能一个一个下发
    {
        LOG_ERROR(pRouteInf->GetChnId(), "Only can send one yk ack command at one time!");
        return -1;
    }

    std::shared_ptr<YKRspParam> data = std::static_pointer_cast<YKRspParam>(command->dataList.front());
    int ykCmd = data->type; //ykCmd--遥控命令: 0-遥控选择，1-遥控执行，2-遥控撤销，3-遥调选择，4-遥调执行，5-遥调撤销，6-设置点值
    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("rtuId:%s, no:%d, type:%d,value:%d",data->rtuId.toStdString().c_str(),data->no,ykCmd, data->value));

    uint8 buf[256];
    int i = 2;

    if(1==Config_Param.YKCmdMode)//(1:单点命令)还是(2:双点命令)
    {
        buf[0] = APPTYPE_YK45;    //1
    }
    else
    {
        buf[0] = APPTYPE_YK46;    //1
    }
    buf[1] = 0x01;

    int appCot; //传输原因
    if(YK_RESULT_SUCCESS == data->result)//确认
    {
        appCot = APP_COT_ACT_CON;//激活确认
    }
    else//否定应答
    {
        appCot = APP_COT_ACT_CON_NO;//否定激活确认
    }

    buf[i++] = appCot%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = appCot/256;

    int rtuAddr = pRouteInf->GetRtuAddr();
    buf[i++] = ( rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = ( rtuAddr)/256;

    buf[i++] = (data->no+Config_Param.YkBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (data->no+Config_Param.YkBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (data->no+Config_Param.YkBaseAddr)/256%256;
        buf[i++] = (data->no+Config_Param.YkBaseAddr)/256/256;
    }

    if(1==Config_Param.YKCmdMode)//(1:单点命令)还是(2:双点命令)
    {
        LOG_DEBUG(pRouteInf->GetChnId(), "1==Config_Param.YKCmdMode");
        int val = (1 == data->value)?1:0; //遥控类型：0-分，1-合;
        switch(ykCmd)
        {
            case YK_CMD_SELECT://遥控选择
                buf[i++] = 0x80|val;
                break;
            case YK_CMD_EXECUTE://遥控执行
                buf[i++] = 0x00|val;
                break;
            default:
                LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Unsupport ykcmd! ykCmd:%d", ykCmd));
                return -1;
                break;
        }
    }
    else
    {
        int val = (1 == data->value)?2:1; //遥控类型：0-分，1-合;
        switch(ykCmd)
        {
            case YK_CMD_SELECT://遥控选择
                buf[i++] = 0x80|val;
                break;
            case YK_CMD_EXECUTE://遥控执行
                buf[i++] = 0x00|val;
                break;
            default:
                LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Unsupport ykcmd! ykCmd:%d",ykCmd));
                return -1;
                break;
        }
    }

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
        return 1;
    }
    return 0;
}

/*******************************************************************************
*
*    描述：发送复位响应帧
*    参数：无
*    返回：1    发送成功
*          0 发送失败                
*
********************************************************************************/
int Iec104Zf::App_Ack_Rst( void )
{
    App_Layer.rxApdu[2] = 7;
    if( App_SendAppIFormat( App_Layer.rxApdu, App_Layer.rxSize ) == 1 )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Ack_Rst!  ---\n");

        return 1;
    }
    else
        return 0;
}


/*******************************************************************************
*
*    描述：发送同步时钟响应帧
*    参数：无
*    返回：1    发送成功
*          0 发送失败            
*
********************************************************************************/
int Iec104Zf::App_Ack_SyncTime( void )
{
    //暂无对钟响应帧!
    App_Layer.rxApdu[2] = 7;
    if( App_SendAppIFormat( App_Layer.rxApdu, App_Layer.rxSize ) == 1 )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Ack_SyncTime!  ---\n");

        return 1;
    }
    else
        return 0;
}

/********************************************************************************
*
*    描述:发送全电度响应
*    参数：无
*    返回： 0  发送失败
*           1  发送成功    
*    注：暂不发送确认和完成帧
*********************************************************************************/
int Iec104Zf::App_Ack_AllKwh( void )
{
    switch( App_Layer.AckStep )
    {
    case 0:
        if( App_Kwh_Conf() == 1 )
        {
            App_Layer.AckStep = 1;
        }
        return 0;
    case 1:
        if( App_Ack_Kwh() == 1 )
        {
            App_Layer.AckStep = 2;
        }
        return 0;
    case 2:
        if( App_AckKwh_Finish() == 1 )
        {
            App_Layer.AckStep = 0;
            return 1;
        }
        else
        {
            return 0;
        }
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Ack_AllKwh AckStep value error\n");

        return 1;
    }
}

/********************************************************************************
*
*    描述：发送全电度响应确认帧
*    参数：无
*    返回： 0  发送失败
*           1  发送成功
*
*********************************************************************************/
int Iec104Zf::App_Kwh_Conf( void )
{
    uint8 buf[16],i;

    i = 2;

    buf[0] = 101;
    buf[1] = 1;

    buf[i++] = 7%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = 7/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    buf[i++] = App_Layer.rxReq.detail;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf 发送KWH确认\n");

        return 1;
    }
    else return 0;
}

/********************************************************************************
*
*    描述：发送全电度响应结束帧
*    参数：无
*    返回： 0  发送失败
*           1  发送成功
*
*********************************************************************************/
int Iec104Zf::App_AckKwh_Finish( void )
{
    uint8 buf[16],i;
    
    i = 2;
    
    buf[0] = 101;
    buf[1] = 1;

    buf[i++] = 10%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = 10/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = (m_rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    buf[i++] = App_Layer.rxReq.detail;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf 发送Kwh结束帧\n");

        return 1;
    }
    else return 0;
}

/********************************************************************************
*
*    描述：发送全电度响应数据帧
*    参数：无
*    返回： 0  发送未结束，还有电度值待发送
*           1  发送结束
*
*********************************************************************************/
int Iec104Zf::App_Ack_Kwh( void )
{
#if CFQ
    uint8 buf[256], writept, kwhnum, ret,i;
    float32 kwhfval;
    sint32  kwhval;
    uint16 kwhtotal, oldkwhidx;

    kwhtotal = pRtuInf->GetKwhNum( rtu_no );
    if( kwhtotal == 0 )
        return 1;
    if( kwhtotal > 128 )
        kwhtotal = 128;

     i = 2;

    buf[0] = 15;
    //buf[1] = 

    buf[i++] = 5%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = 5/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = (m_rtuAddr)/256;

    buf[i++] = (App_Layer.AckKwhIdx+Config_Param.DdBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (App_Layer.AckKwhIdx+Config_Param.DdBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (App_Layer.AckKwhIdx+Config_Param.DdBaseAddr)/256%256;
        buf[i++] = (App_Layer.AckKwhIdx+Config_Param.DdBaseAddr)/256/256;
    }
    
    kwhnum = 0;
    writept = i;
    oldkwhidx = App_Layer.AckKwhIdx;

    while( App_Layer.AckKwhIdx < kwhtotal )
    {
        if( kwhnum >= 32 )
            break;
        
        ret = pRawDb->GetAKwh( rtu_no, App_Layer.AckKwhIdx, &kwhfval );
        kwhval = kwhfval;
        buf[writept++] = kwhval%256;
        buf[writept++] = (kwhval/256)%256;
        buf[writept++] = (kwhval/256/256)%256;
        buf[writept++] = kwhval/256/256/256;
        buf[writept++] = kwhnum;

        kwhnum++;
        App_Layer.AckKwhIdx++;
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:%d kwhval = %d, kwhtotal=%d, ret=%d\n", App_Layer.AckKwhIdx, kwhval, kwhtotal, ret );
    }

    buf[1] = kwhnum|0x80;                    
    if( App_SendAppIFormat( buf, writept ) == 1 )
    {
        if( App_Layer.AckKwhIdx >= kwhtotal )
        {
            App_Layer.AckKwhIdx = 0;
            return 1;
        }
        else 
        {
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf 全电度数据未发送完！===\n");

            return 0;
        }
    }
    else
    { 
        App_Layer.AckKwhIdx = oldkwhidx;
        return 0;
    }
#else
    return 1; //cfq 20200605 目前暂不支持，直接返回正常即可，否则 将无法完成完整的电度交互过程： 收到电度请求-》电度确认-》上电度送数据-》电度结束
#endif
}

/********************************************************************************
*
*    描述：发送全数据响应
*    参数：无
*    返回： 0  发送失败
*           1  发送成功
*    注：暂不发送确认和结束帧
*********************************************************************************/
int Iec104Zf::App_Ack_AllData( void )
{
#if CFQ
    switch( App_Layer.AckStep )
    {
    case 0:
        if( App_AllData_Conf() == 1 )
            App_Layer.AckStep = 1;
        return 0;
    case 1:
        if( App_Ack_Yx() == 1 )
            App_Layer.AckStep = 2;
        return 0;
    case 2:
        if( App_Ack_Yc() == 1 )
        {
            App_Layer.AckStep = 3;
            //return 1;
        }
        else return 0;
    case 3:
        if( App_All_Finish() == 1 )
        {
            App_Layer.AckStep = 0;
            return 1;
        }
        else return 0;
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Ack_AllData AckStep value error!  \n");

        break;
    }
    return 1;
#else
    //总召过程： 收到总召请求-》总召确认-》上送遥信，遥测状态-》总召结束
    switch( App_Layer.AckStep )
    {
    case 0://总召唤确认
        if( App_AllData_Conf() == 1 )
            App_Layer.AckStep = 1;
        return 0;//不能返回1，因为整个过程还没有结束
    case 1://上送到总召命令到智能分析应用，由上层应用进行遥信遥测回应
        this->pRawDb->SendCmdCallData();
        App_Layer.AckStep = 3;//直接跳到最后，因为智能分析应用会发一起发遥信遥测过来，可跳过第二步遥测
        return 0; //不能返回1，因为整个过程还没有结束，需要等待 总召结束命令才算完成
    case 2://遥测回应
    case 3://总召结束
    {
        TIME curtime;
        GetCurSec( &curtime );
        if( curtime.Sec-App_Layer.LastRxTime > Config_Param.ackCalldataTimeout )//总召超时
        {
            LOG_WARN(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Wait Ack_AllData out of time !!!  ---\n");
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("IEC104ZF:IEC104zf Wait Ack_AllData App_Lyer.State %d , time out %d s ,Curtime_Sec: %lu s\n",App_Layer.State,Config_Param.ackCalldataTimeout, curtime.Sec));
//            pRouteInf->RecAFrame( PROTO_RXTIMEOUTS );
            App_Layer.State = IEC104_APP_STATE_IDLE;
            InitData( );         //重新初始化
            pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay
            AckFinished = 1;    //接收结束
        }
        return 0;//第三步需要等待智能分析应用下发总召结束命令才触发，所以这里直接不处理返回即可
    }
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_ERROR(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Ack_AllData AckStep value error!  \n");

        break;
    }
    return 1;
#endif
}

/********************************************************************************
*
*    描述：发送全数据响应确认帧
*    参数：无
*    返回： 0  发送失败
*           1  发送成功
*
*********************************************************************************/
int Iec104Zf::App_AllData_Conf( void )
{
    uint8 buf[16],i;

    i = 2;
    
    buf[0] = APPTYPE_CALLDATA;
    buf[1] = 0x01;

    buf[i++] = APP_COT_ACT_CON%256;    //7
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = APP_COT_ACT_CON/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = (m_rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    buf[i++] = 0x14; 

    if( App_SendAppIFormat( buf, i ) == 1 )
        return 1;
    else return 0;
}

/********************************************************************************
*
*    描述：发送全数据响应结束帧
*    参数：无
*    返回： 0  发送失败
*           1  发送成功
*
*********************************************************************************/
int Iec104Zf::App_All_Finish( void )
{
    uint8 buf[16],i;

    i = 2;

    buf[0] = 0x64;
    buf[1] = 0x01;

    buf[i++] = 10%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = 10/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = (m_rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    buf[i++] = 20;
    
    if( App_SendAppIFormat( buf, i ) == 1 )
        return 1;
    else return 0;
}

/********************************************************************************
*
*    描述：发送全数据响应遥信帧
*    参数：无
*    返回： 0  发送未结束，还有遥信值待发送
*           1  发送结束
*
*********************************************************************************/
int Iec104Zf::App_Ack_Yx(COMMAND *command)
{
#if CFQ
    uint8 buf[256], yxnum = 0, ret, yxval, writept = 0,i;
    uint16 yxtotal, oldyxidx;

    yxtotal = pRtuInf->GetYxNum( rtu_no );
    if( yxtotal == 0 )
        return 1;

    if( yxtotal > 4096 )
        yxtotal = 4096;

    i = 2;    

    buf[0] = APPTYPE_SP_NT;    //1
    buf[i++] = 20%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = 20/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = (App_Layer.AckYxIdx+Config_Param.YxBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (App_Layer.AckYxIdx+Config_Param.YxBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (App_Layer.AckYxIdx+Config_Param.YxBaseAddr)/256%256;
        buf[i++] = (App_Layer.AckYxIdx+Config_Param.YxBaseAddr)/256/256;
    }

    yxnum = 0;
    oldyxidx = App_Layer.AckYxIdx;
    writept = i;

    while( App_Layer.AckYxIdx < yxtotal )
    {
        //if( writept >= 249 )
        //    break;

        ret = pRawDb->GetAYx( rtu_no, App_Layer.AckYxIdx, &yxval );
    if( 0 == ret )/* 值无效 */
            buf[writept] = 0x80;
        else
            buf[writept] = 0x00;

        if( yxval&0x01 )
            buf[writept++] |= 0x01;
        else buf[writept++] |= 0;

        yxnum++;
        App_Layer.AckYxIdx++;
        
        if( yxnum >= 127 )    //add 11.17
            break;
    }

    buf[1] = yxnum|0x80;                                
    if( App_SendAppIFormat( buf, writept ) == 1 )
    {
        if( App_Layer.AckYxIdx >= yxtotal )
        {
            App_Layer.AckYxIdx = 0;
            return 1;
        }
        else 
        {
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf 遥信全数据未发送完！YxIdx = %d ,SendNum = %d, RecNum = %d ===\n",App_Layer.AckYxIdx,App_Layer.Snd_seqnum,App_Layer.Rec_seqnum);

            return 0;
        }
    }
    else
    { 
        App_Layer.AckYxIdx = oldyxidx;
        return 0;
    }
#else

    if(command->dataList.size() == 0)
    {
        LOG_WARN(pRouteInf->GetChnId(), "No yx data need to be sent!");
        return 1;
    }

    uint8 buf[256], yxnum = 0, writept = 0,i;
    uint16 yxtotal;
    yxtotal = command->dataList.size();
    if( yxtotal > 4096 )
        yxtotal = 4096;
    i = 2;

    buf[0] = APPTYPE_SP_NT;    //1
    buf[i++] = 20%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = 20/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = ( m_rtuAddr)/256;
#if 0
    buf[i++] = (Config_Param.YxBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (Config_Param.YxBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (Config_Param.YxBaseAddr)/256%256;
        buf[i++] = (Config_Param.YxBaseAddr)/256/256;
    }

    yxnum = 0;
    writept = i;

    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command->dataList.begin();
    for(; iter != command->dataList.end(); iter++)
    {
        std::shared_ptr<YXParam_S> data = std::static_pointer_cast<YXParam_S>(*iter);
        //int rtuAddr = data.rtuAddr;
        //int ptAddress = data.pointAddr;
        int yxVal = data->value;//std::stoi(data->value);//data.value.toInt();
#if CFQ
        //ret = pRawDb->GetAYx( rtu_no, App_Layer.AckYxIdx, &yxval );
        if( 0 == ret )/* 值无效 */
            buf[writept] = 0x80;
        else
            buf[writept] = 0x00;
#else
        buf[writept] = 0x00;
#endif

        if( yxVal&0x01 )
            buf[writept++] |= 0x01;
        else
            buf[writept++] |= 0;

        yxnum++;
        if( yxnum >= 127 )    //add 11.17
            break;
    }

    buf[1] = yxnum|0x80;
#else
    yxnum = 0;
    writept = i;

    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command->dataList.begin();
    for(; iter != command->dataList.end(); iter++)
    {
        if( writept >= 246 )                    //保证ASDU长度不超过249
            break;

        std::shared_ptr<YXParam_S> data = std::static_pointer_cast<YXParam_S>(*iter);
        //QString rtuId = data->rtuId;
        int ptAddress = data->no;
        int yxVal = data->value;//std::stoi(data->value);//data.value.toInt();
        buf[writept++] = (ptAddress+Config_Param.YxBaseAddr)%256%256;
        if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
            buf[writept++] = (ptAddress+Config_Param.YxBaseAddr)/256%256;
        if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
        {
            buf[writept++] = (ptAddress+Config_Param.YxBaseAddr)/256%256;
            buf[writept++] = (ptAddress+Config_Param.YxBaseAddr)/256/256;
        }

        if( yxVal&0x01 )
            buf[writept++] = 0x01;
        else
            buf[writept++] = 0;

        yxnum++;
    }

    buf[1] = yxnum;
#endif
    if( App_SendAppIFormat( buf, writept ) == 1 )
    {
        return 1;
    }
    return 0;

#endif
    return 0;
}

/********************************************************************************
*
*    描述：发送全数据响应遥测帧
*    参数：无
*    返回： 0  发送未结束，还有遥测值待发送
*           1  发送结束
*
*********************************************************************************/
int Iec104Zf::App_Ack_Yc(COMMAND *command)
{
#if CFQ
    uint8     buf[256], writept = 0, ycnum,i;
    uint16     oldycidx, yctotal;
    float32 ycfval;

    yctotal = pRtuInf->GetYcNum( rtu_no );

    if( yctotal == 0 )
        return 1;    
    if( yctotal > 512 )
        yctotal = 512;
        
    i = 2;    

    buf[0] = APPTYPE_ME_FLOAT;            //13

    buf[i++] = APP_COT_RESP_CALLALL%256;    //20
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = APP_COT_RESP_CALLALL/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = (App_Layer.AckYcIdx+Config_Param.YcBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (App_Layer.AckYcIdx+Config_Param.YcBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (App_Layer.AckYcIdx+Config_Param.YcBaseAddr)/256%256;
        buf[i++] = (App_Layer.AckYcIdx+Config_Param.YcBaseAddr)/256/256;
    }

    writept = i;
    ycnum = 0;
    oldycidx = App_Layer.AckYcIdx;

    while( App_Layer.AckYcIdx < yctotal )
    {
        if( writept >= 128 )
            break;
        pRawDb->GetAYc( rtu_no, App_Layer.AckYcIdx, &ycfval );
        App_Layer.LastYcVal[App_Layer.AckYcIdx] = ycfval;

        float_to_char(ycfval,(char *)(&buf[writept]));
        writept += 4;
        buf[writept++] = 0;

        App_Layer.AckYcIdx++;
        ycnum++;
    }

    buf[1] = ycnum|0x80;                                

    if( App_SendAppIFormat( buf, writept ) == 1 )
    {
        if( App_Layer.AckYcIdx >= yctotal )
        {
            App_Layer.AckYcIdx = 0;
            return 1;
        }
        else 
        {    
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf 遥测全数据未发送完！YcIdx = %d ,SendNum = %d, RecNum = %d ===\n",App_Layer.AckYcIdx,App_Layer.Snd_seqnum,App_Layer.Rec_seqnum);

            return 0;
        }
    }
    else
    { 
        App_Layer.AckYcIdx = oldycidx;
        return 0;
    }
#else

    if(command->dataList.size() == 0)
    {
        LOG_WARN(pRouteInf->GetChnId(), "No yc data need to be sent!");
        return 1;
    }
    uint8     buf[256], writept = 0, ycnum,i;
    uint16     yctotal;
    //float32 ycfval;

    yctotal = command->dataList.size();//pRtuInf->GetYcNum( rtu_no );
    if( yctotal > 512 )
        yctotal = 512;

    i = 2;

    buf[0] = APPTYPE_ME_FLOAT;            //13

    buf[i++] = APP_COT_RESP_CALLALL%256;    //20
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = APP_COT_RESP_CALLALL/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )
        buf[i++] = ( m_rtuAddr)/256;
#if 0
    buf[i++] = (Config_Param.YcBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (Config_Param.YcBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (Config_Param.YcBaseAddr)/256%256;
        buf[i++] = (Config_Param.YcBaseAddr)/256/256;
    }

    writept = i;
    ycnum = 0;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command->dataList.begin();
    for(; iter != command->dataList.end(); iter++)
    {
        std::shared_ptr<YCParam_S> data = std::static_pointer_cast<YCParam_S>(*iter);
        //int rtuAddr = data.rtuAddr;
        //int ptAddress = data.pointAddr;
        float32 ycfval = data->value;//std::stof(data.value);//.toFloat();

        if( writept >= 128 )
            break;
        //pRawDb->GetAYc( rtu_no, App_Layer.AckYcIdx, &ycfval );
        //App_Layer.LastYcVal[App_Layer.AckYcIdx] = ycfval;

        float_to_char(ycfval,(char *)(&buf[writept]));
        writept += 4;
        buf[writept++] = 0;

        //App_Layer.AckYcIdx++;
        ycnum++;
    }

    buf[1] = ycnum|0x80;
#else
    writept = i;
    ycnum = 0;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command->dataList.begin();
    for(; iter != command->dataList.end(); iter++)
    {
        if( writept >= 246 )
            break;

        std::shared_ptr<YCParam_S> data = std::static_pointer_cast<YCParam_S>(*iter);
        //int rtuAddr = data.rtuAddr;
        int ptAddress = data->no;
        float32 ycfval = data->value;//std::stof(data->value);//.toFloat();

        buf[writept++] = (ptAddress+Config_Param.YcBaseAddr)%256%256;
        if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[writept++] = (ptAddress+Config_Param.YcBaseAddr)/256%256;
        if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
        {
            buf[writept++] = (ptAddress+Config_Param.YcBaseAddr)/256%256;
            buf[writept++] = (ptAddress+Config_Param.YcBaseAddr)/256/256;
        }
        float_to_char(ycfval,(char *)(&buf[writept]));
        writept += 4;
        buf[writept++] = 0;
        ycnum++;
    }

    buf[1] = ycnum;
#endif

    if( App_SendAppIFormat( buf, writept ) == 1 )
    {
        return 1;
    }
    return 0;

#endif
    return 0;
}


/*******************************************************************************
*
*    描述：发送某一组数据
*    参数：group  组号，取值为1 - 16
*    返回：1    发送成功
*          0 发送失败            
*
********************************************************************************/
int Iec104Zf::App_Ack_GroupData( int group )
{
    int ret;
    if( group >= 1 && group <= 8 )
        ret = App_Ack_GroupYx( group );
    else if( group >= 9 && group <= 12 )
        ret = App_Ack_GroupYc( group );
    
    return ret;
}



/*******************************************************************************
*
*    描述：发送某一组Yx帧
*    参数：组号group,取值为1 - 8
*    返回：1    发送成功
*          0 发送失败            
*
********************************************************************************/
int Iec104Zf::App_Ack_GroupYx( int group )
{
#if CFQ
    uint8 buf[256], yxnum = 0, ret, yxval, writept = 0,i;
    uint16  yxtotal, yxidx;

    yxtotal = pRtuInf->GetYxNum( rtu_no );
    if( yxtotal == 0 )
        return 1;

    if( yxtotal > 4096 )
        yxtotal = 4096;
    yxidx = (group-1)*IEC104_YXNUM_PERGROUP;    //起始遥信号yxidx,从0起

    i = 2;

    buf[0] = APPTYPE_SP_NT;    //1
    //buf[1] = 

    buf[i++] = (20+group)%256;                //传送原因为21-36
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = (20+group)/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = (yxidx+Config_Param.YxBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (yxidx+Config_Param.YxBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (yxidx+Config_Param.YxBaseAddr)/256%256;
        buf[i++] = (yxidx+Config_Param.YxBaseAddr)/256/256;
    }

    yxnum = 0;
    writept = i;

    while( yxnum < IEC104_YXNUM_PERGROUP )    //11.17 here  only 127 yx can be sent
    {
        if( yxidx >= yxtotal )
            break;

        ret = pRawDb->GetAYx( rtu_no, yxidx, &yxval );
    if( 0 == ret )/* 值无效 */
            buf[writept] = 0x80;
        else
            buf[writept] = 0x00;

        if( yxval&0x01 )
            buf[writept++] |= 0x01;
        else 
            buf[writept++] |= 0;

        yxnum++;
        yxidx++;
    }

    buf[1] = yxnum|0x80;    
    if( App_SendAppIFormat( buf, writept ) == 1 )
        return 1;
    else
        return 0;
#endif
    return 0;
}


/********************************************************************************
*
*    描述：发送某一组Yc帧
*    参数：组号group,取值为9 -12（参考《关于基本远动任务配套标准的说明》98.9 )
*    返回：1    发送成功
*          0 发送失败            
*
*********************************************************************************/
int Iec104Zf::App_Ack_GroupYc( int group )
{
#if CFQ
    uint8     buf[256], writept=0, ycnum,i;
    uint16     ycidx, yctotal;
    float32 ycfval;

    yctotal = pRtuInf->GetYcNum( rtu_no );

    if( yctotal == 0 )
        return 1;    
    if( yctotal > 512 )
        yctotal = 512;

    ycidx = (group-9)*IEC104_YCNUM_PERGROUP;

    i = 2;

    buf[0] = APPTYPE_ME_FLOAT;            //13

    buf[i++] = (20+group)%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = (20+group)/256;

    buf[i++] = ( m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )
        buf[i++] = ( m_rtuAddr)/256;

    buf[i++] = (ycidx+Config_Param.YcBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (ycidx+Config_Param.YcBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (ycidx+Config_Param.YcBaseAddr)/256%256;
        buf[i++] = (ycidx+Config_Param.YcBaseAddr)/256/256;
    }

    writept = i;
    ycnum = 0;

    while( ycnum < IEC104_YCNUM_PERGROUP )
    {
        if( ycidx >= yctotal )
            break;
        pRawDb->GetAYc( rtu_no, ycidx, &ycfval );
        App_Layer.LastYcVal[ycidx] = ycfval;

        float_to_char(ycfval,(char *)(&buf[writept]));
        writept += 4;
        buf[writept++] = 0;

        ycidx++;
        ycnum++;
    }

    buf[1] = ycnum|0x80;

    if( App_SendAppIFormat( buf, writept ) == 1 )
    {
        return 1;
    }
    else
        return 0;
#endif
    return 0;
}


/********************************************************************************
*
*    描述：发送变化遥信帧
*    参数：ack_type 发送帧类型（ACK_REQUESTED、ACK_UNSOLICITED）
*    返回：1    变化遥信数据缓冲区中数据发送完毕
*          0 变化遥信数据缓冲区中还有数据未发送            
*
*********************************************************************************/
int Iec104Zf::App_Ack_ChangeYx( uint8 ack_type )
{
    uint8 buf[256];
    uint8 writept = 0,i;
    int yxnum;
    IEC104_ChangeYx changeyx;

    i = 2;

    buf[0] = APPTYPE_SP_NT;        //1

    if( ack_type == ACK_REQUESTED )
    {
        buf[i++] = APP_COT_REQ%256;
        if( Config_Param.CotNum == 2 )//传输原因2字节
            buf[i++] = APP_COT_REQ/256;
    }
    else
    { 
        buf[i++] = APP_COT_SPONT%256;
        if( Config_Param.CotNum == 2 )//传输原因2字节
            buf[i++] = APP_COT_SPONT/256;
    }

    buf[i++] = (m_rtuAddr)%256;    //common addr
    if( Config_Param.ComAddrNum == 2 )//公共地址2字节
        buf[i++] = (m_rtuAddr)/256;

    writept = i;

    yxnum = 0;
    while( App_Layer.ChangeYxBuf.GetDataNum() > 0 )
    {
        if( writept >= 246 )                    //保证ASDU长度不超过249
            break;

        App_Layer.ChangeYxBuf.Read( &changeyx );
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104ZF:Iec104zf  有变化遥信 ChangeYxidx = %d  ---\n", changeyx.idx));

        buf[writept++] = (changeyx.idx+Config_Param.YxBaseAddr)%256%256;
        if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
            buf[writept++] = (changeyx.idx+Config_Param.YxBaseAddr)/256%256;
        if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
        {
            buf[writept++] = (changeyx.idx+Config_Param.YxBaseAddr)/256%256;
            buf[writept++] = (changeyx.idx+Config_Param.YxBaseAddr)/256/256;
        }

        if( changeyx.yxvalue&0x01 )
            buf[writept++] = 0x01;
        else 
            buf[writept++] = 0;

        yxnum++;
    }

    buf[1] = yxnum;
    
    if( App_SendAppIFormat( buf, writept ) != 1 )
        App_Layer.ChangeYxBuf.Rewind( yxnum );

    if( App_Layer.ChangeYxBuf.GetDataNum() == 0 )
    {
        //AckFinished = 1;
        return 1;
    }
    else
        return 0;
}


/********************************************************************************
*
*    描述：发送事项数据
*    参数：无
*    返回：1    事项数据缓冲区中数据发送完毕
*          0 事项数据缓冲区中还有数据未发送            
*
********************************************************************************/
int Iec104Zf::App_Ack_Soe( void )
{
    uint8 buf[256], writept = 0,i;
    int soenum;
    IEC104_Soe soe;
    
    i = 2;
    
    buf[0] = APPTYPE_SP_WT30;

    buf[i++] = APP_COT_SPONT%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = APP_COT_SPONT/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )
        buf[i++] = (m_rtuAddr)/256;

    writept = i;
    soenum = 0;
    while( App_Layer.SoeBuf.GetDataNum() > 0 )
    {
        if( writept >= 246 )
            break;

        App_Layer.SoeBuf.Read( &soe );
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104ZF:Iec104zf  有 SOE SOEidx = %d  ---\n",soe.idx ));

        buf[writept++] = (soe.idx+Config_Param.YxBaseAddr)%256%256;
        if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
            buf[writept++] = (soe.idx+Config_Param.YxBaseAddr)/256%256;
        if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
        {
            buf[writept++] = (soe.idx+Config_Param.YxBaseAddr)/256%256;
            buf[writept++] = (soe.idx+Config_Param.YxBaseAddr)/256/256;
        }

        if( soe.yxvalue&0x01 )
            buf[writept++] = 0x01;
        else 
            buf[writept++] = 0;

        for( int i = 0; i < 7; i++ )//7字节时标
            buf[writept++] = soe.time[i];
        
        soenum++;
    }

    buf[1] = soenum;
    
    if( App_SendAppIFormat( buf, writept ) != 1 )
        App_Layer.SoeBuf.Rewind( soenum );

    if( App_Layer.SoeBuf.GetDataNum() == 0 )
    {
        //AckFinished = 1;
        return 1;
    }
    else return 0;
}

/********************************************************************************
*
*    描述：发送变化遥测数据
*    参数：无
*    返回：1    变化遥测数据缓冲区中的数据发送完毕
*          0 变化遥测数据缓冲区中还有数据未发送            
*
*********************************************************************************/
int Iec104Zf::App_Ack_ChangeYc( void )
{
#if CFQ
    uint8 buf[256], writept = 0, change,i;
    int ycnum;
    uint16 ycidx;
    float32 ycfval;

    i = 2;

    buf[0] = APPTYPE_ME_FLOAT;            //13

    buf[i++] = APP_COT_SPONT%256;
    if( Config_Param.CotNum == 2 )//传输原因2字节
        buf[i++] = APP_COT_SPONT/256;

    buf[i++] = (m_rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )
        buf[i++] = (m_rtuAddr)/256;

    writept = i;
    ycnum = 0;

    while( App_Layer.ChangeYcBuf.GetDataNum() > 0 )
    {
        if( writept >= 246 )
            break;
        App_Layer.ChangeYcBuf.Read( &ycidx );
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:Iec104zf  有变化遥测 ChangeYcidx = %d  ---\n", ycidx );

        buf[writept++] = (ycidx+Config_Param.YcBaseAddr)%256%256;
        if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[writept++] = (ycidx+Config_Param.YcBaseAddr)/256%256;
        if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
        {
            buf[writept++] = (ycidx+Config_Param.YcBaseAddr)/256%256;
            buf[writept++] = (ycidx+Config_Param.YcBaseAddr)/256/256;
        }

        pRawDb->GetAYc( rtu_no, ycidx, &ycfval, &change );
        App_Layer.LastYcVal[ycidx] = ycfval;
    
        float_to_char(ycfval,(char *)(&buf[writept]));
        writept += 4;
        buf[writept++] = 0;
        ycnum++;
    }

    buf[1] = ycnum;

    if( App_SendAppIFormat( buf, writept ) != 1 )
        App_Layer.ChangeYcBuf.Rewind( ycnum );

    if( App_Layer.ChangeYcBuf.GetDataNum() == 0 )
    {
        //AckFinished = 1;
        return 1;
    }
    else return 0;
#endif
    return 0;
}

/********************************************************************************
*
*    描述：搜索变化遥测数据，写入变化遥测数据缓冲区
*    参数：无
*    返回：无        
*
*********************************************************************************/
void Iec104Zf::App_SearchChangeYc( void )
{
#if CFQ
    int ycnum;
    uint8 change, ret;
    uint16 i;
    float32 yc_fval;

    if( IfCheckLinkOk == 1 )/* 需要检测链路尚未建立 */
    {
        if( App_Layer.State == IEC104_APP_STATE_UNRESET )
            return;
    }

    ycnum = pRtuInf->GetYcNum( rtu_no );

    for( i = 0; i < ycnum; i++ )
    {
        ret = pRawDb->GetAYc( rtu_no, i, &yc_fval, &change );    //change的最低位为变化标志
        if(ret && (fabs(App_Layer.LastYcVal[i]-yc_fval) > 0.001))
        {
            App_Layer.LastYcVal[i] = yc_fval;
            App_Layer.ChangeYcBuf.Write( &i );
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104Zf Find a changeyc %d===\n", i );
        }
    }
#endif
}

/********************************************************************************
*
*    描述：搜索变化遥信数据，写入变化遥信数据缓冲区，同时生成事项数据写入
*          事项数据缓冲区
*    参数：无
*    返回：无        
*
*********************************************************************************/
void Iec104Zf::App_SearchChangeYx( void )
{
#if CFQ
    int yxnum;
    uint8 change, yxvalue, ret;
    uint32 i;
    IEC104_Soe    soe;
    IEC104_ChangeYx change_yx;

    if( IfCheckLinkOk == 1 )/* 需要检测链路尚未建立 */
    {
        if( App_Layer.State == IEC104_APP_STATE_UNRESET )
            return;
    }

    yxnum = pRtuInf->GetYxNum( rtu_no );

    /******************************如果是整站转发*****yay*****************************/
    if(pRtuInf->GetRtuType( rtu_no ) == 7)//整站转发
    {
        int yxno;
        SOEDATA changesoe;

        //yay 注意此循环必须用127-App_Layer.ChangeYxBuf.GetDataNum())才是可以写入的个数,
        //如果用128则套圈了就丢数据了.也就说为空时最多一次写入127个数据.
        while( MIN(pRawDb->GetVarYxNum(rtu_no,pRouteInf->GetChnId()),(127-App_Layer.ChangeYxBuf.GetDataNum()))>0 )
        {
            ret = pRawDb->GetAVarYx( rtu_no,pRouteInf->GetChnId(),&yxno, &yxvalue, &change );
            if( ret && (change&0x01) )
            {
                if( pRouteInf->GetDebugFlag() )
                    LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:\nApp_SearchChangeYx: GetAVarYx channo=%d yxno=%d yxvalue=%d\n",pRouteInf->GetChnId(),yxno, yxvalue);

                change_yx.idx = yxno;
                change_yx.yxvalue = yxvalue&0x01;
                App_Layer.ChangeYxBuf.Write( &change_yx );
            }
        }

        while( MIN(pRawDb->GetVarSoeNum(rtu_no,pRouteInf->GetChnId()),(127-App_Layer.SoeBuf.GetDataNum()))>0 )
        {
            ret = pRawDb->GetAVarSoe( rtu_no,pRouteInf->GetChnId(),&yxno, &changesoe );
            if( ret == 1 )
            {
                if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:\nApp_SearchChangeYx: GetAVarSoe channo=%d yxno=%d yxvalue=%d\n",pRouteInf->GetChnId(),yxno, (changesoe.YxValue)&0x01);

                soe.idx = yxno;
                soe.yxvalue = (changesoe.YxValue)&0x01;
                soe.time[0]=(changesoe.soetime.second*1000+changesoe.soetime.ms)%256;
                soe.time[1]=(changesoe.soetime.second*1000+changesoe.soetime.ms)/256;
                soe.time[2]=changesoe.soetime.minute;
                soe.time[3]=changesoe.soetime.hour;
                soe.time[4]=changesoe.soetime.day;
                soe.time[5]=changesoe.soetime.month;
                soe.time[6]=changesoe.soetime.year%100;
                App_Layer.SoeBuf.Write( &soe );
            }
        }
    }
    else
    /*******************************************************************************/
    {
        //该规约不支持整站转发处理模式时,执行该函数清理掉可能的Yx(Soe)_Var_Out
        ClearYxSoeVarOut();

        for( i = 0; i < yxnum; i++ )
        {
            ret = pRawDb->GetAYx( rtu_no, i, &yxvalue, &change );//change的最低位为变化标志
            if( ret && (change & 0x01) )
            {
                change_yx.idx = i;
                change_yx.yxvalue = yxvalue&0x01;
                soe.idx = i;
                soe.yxvalue = yxvalue&0x01;
                App_GetTime( soe.time );
    
                if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104Zf Find a changeyx %d===\n", i );

            App_Layer.ChangeYxBuf.Write( &change_yx );
            App_Layer.SoeBuf.Write( &soe );
            }
        }
    }
#endif
}

/********************************************************************************
*
*    描述：填写事项数据的时间
*    参数：time  指向uint8的指针
*    返回：无        
*
*********************************************************************************/
void Iec104Zf::App_GetTime( uint8 *time )
{
    SYSTIME curtime;
    uint16 ms;
    
    GetCurTime( &curtime );
    ms = curtime.second*1000+curtime.ms;

    time[0] = ms%256;
    time[1] = ms/256;
    time[2] = curtime.minute;
    time[3] = curtime.hour;
    time[4] = curtime.day;
    time[5] = curtime.month;
    time[6] = curtime.year%100;
}


/********************************************************************************
*
*    描述: 应用层将ASDU+APCI转换成APDU(I format)并发送
*
*    参数: asdu 指向应用服务数据单元(ASDU) 的指针
*          size 应用服务数据单元(ASDU) 中的数据字节数
*    返回: 1  发送成功.
*         -1  发送失败.
*
*********************************************************************************/

int Iec104Zf::App_SendAppIFormat( uint8 *asdu, int size )
{
    int i;
    uint8 *txData;
#if CFQ
    if( pTxBuf->GetWritableSize() < size+6 )
        return -1;
#endif
    do
    {        
        if( IfCheckLinkOk == 1 )/* 需要检测链路尚未建立 */
        {
            if( 0 == App_Layer.Inited_Flag )
                break;
        }

        txData = &App_Layer.txData[0];
        App_Layer.txSize = size+6;
        
        txData[0] = 0x68;
        txData[1] = size+4;
        if( SpecialCtrlField == 1 )
        {
        txData[2] = CtrlField[0];
        txData[3] = CtrlField[1];
        txData[4] = CtrlField[2];
        txData[5] = CtrlField[3];
        }
        else
        {
        txData[2] = App_Layer.Snd_seqnum%128<<1;
        txData[3] = App_Layer.Snd_seqnum/128;
        txData[4] = App_Layer.Rec_seqnum%128<<1;
        txData[5] = App_Layer.Rec_seqnum/128;
        }
        
        for( i=0; i<size; i++ )
        {
            txData[6+i] = asdu[i];
        }
        
        //每发送一I帧数据Snd_seqnum加1，清发送确认标志SendACKFlag。
        App_Layer.Snd_seqnum ++;        
        App_Layer.Snd_seqnum %= 0x8000;        //Snd_seqnum 为15位二进制数。
            
        AddNeedSendFrame( txData, size+6 );
//        pRouteInf->RecAFrame( PROTO_TXFRAME );

#if CFQ
      char file_name[FILE_NAME_LENGTH];
      char del_name[FILE_NAME_LENGTH];
      MakeByteFileName( file_name, del_name );
      if( del_name[0] )
      {
         if( 0 != strcmp(delete_name_saved,del_name) )
         {
              strcpy( delete_name_saved, del_name );
                 DeleteOneDiskFile( del_name );
         }
      }
        pTxRxOutBuf->WriteTransStart( DIR_DOWN, file_name );
        pTxRxOutBuf->WriteDirect( txData, size+6 );        
        pTxRxOutBuf->WriteTransEnd( );
#endif
    
        break;
    }while(false);

    App_Layer.SendACKFlag = 0;
    App_Layer.WaitACKFlag = 1;
    
    TIME curtime;
    GetCurSec( &curtime );
    App_Layer.LastTxTime = curtime.Sec;
    
    App_Layer.RxWindowNum = 0;

    return 1;
}

/********************************************************************************
*
*    描述: 应用层发送U format
*    参数: type  U帧的类型
*    返回: 1  发送成功.
*         -1  发送失败.
*
*********************************************************************************/
int Iec104Zf::App_SendAppUFormat(int type)
{
  char file_name[FILE_NAME_LENGTH];
  char del_name[FILE_NAME_LENGTH];
#if CFQ
  /******************************如果是整站转发*****yay*****************************/
  if(pRtuInf->GetRtuType( rtu_no) == 7)//整站转发
  {
      bool isvalid = pRawDb->IsOutRtuDataValid(rtu_no);
      //如果该站的数据没有效,就不给对方回答任何数据
      if(!isvalid)
      {
            ClearYxSoeVarOut();
          return 1;
      }
  }
  /*******************************************************************************/
#endif
    TIME curtime;
    uint8 *txData;

    GetCurSec( &curtime );
    if( type == APP_UFORMAT_TESTACT )
    {    
        if( (curtime.Sec-App_Layer.LastTxTime < Config_Param.OverTime3) ||
                (curtime.Sec-App_Layer.LastRxTime < Config_Param.OverTime3) )
        {
            //if( pRouteInf->GetDebugFlag() )
            //    LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf 空闲时间不到 AppLayer.State %d ===\n",App_Layer.State);

            return -1;
        }
    }    
#if CFQ
    if( pTxBuf->GetWritableSize() < 6 )
        return -1;
#endif
    txData = &App_Layer.txData[0];
    App_Layer.txSize = 6;
    
    txData[0] = 0x68;
    txData[1] = 4;
    if( SpecialCtrlField == 1 )
        txData[2] = 0;
    else
        txData[2] = type | 0x03;
    txData[3] = 0;
    txData[4] = 0;
    txData[5] = 0;
        
    AddNeedSendFrame( txData, 6 );
//    pRouteInf->RecAFrame( PROTO_TXFRAME );

#if CFQ
  MakeByteFileName( file_name, del_name );
  if( del_name[0] )
  {
     if( 0 != strcmp(delete_name_saved,del_name) )
     {
          strcpy( delete_name_saved, del_name );
             DeleteOneDiskFile( del_name );
     }
  }
    pTxRxOutBuf->WriteTransStart( DIR_DOWN, file_name );
    pTxRxOutBuf->WriteDirect( txData, 6 );        
    pTxRxOutBuf->WriteTransEnd( );
#endif
    if( App_Layer.State == IEC104_APP_STATE_IDLE || App_Layer.State == IEC104_APP_STATE_UNRESET )         //空闲状态下才能刷新LastTxTime！
        App_Layer.LastTxTime = curtime.Sec;
    
    if( type == APP_UFORMAT_TESTACT )
        AckFinished = 0;

    return 1;
}

/********************************************************************************
*
*    描述: 应用层发送S format
*    参数: atonceflag  true: 立刻发送S帧
*    返回: 1  发送成功.
*         -1  发送失败.
*
*********************************************************************************/
int Iec104Zf::App_SendAppSFormat( bool atonceflag )
{
  char file_name[FILE_NAME_LENGTH];
  char del_name[FILE_NAME_LENGTH];

  #if CFQ
  /******************************如果是整站转发*****yay*****************************/
  if(pRtuInf->GetRtuType( rtu_no) == 7)//整站转发
  {
      bool isvalid = pRawDb->IsOutRtuDataValid(rtu_no);
      //如果该站的数据没有效,就不给对方回答任何数据
      if(!isvalid)
      {
            ClearYxSoeVarOut();
          return 1;
      }
  }
  /*******************************************************************************/
#endif
    TIME curtime;
    uint8 txData[6];            //此处并未改变App_Layer.txData[],不影响重发数据
    
    GetCurSec( &curtime );
    if(atonceflag == false)
    if( (App_Layer.RxWindowNum < IEC104_WINDOW_MAX_SIZE) && ((curtime.Sec-App_Layer.LastTxTime) <= Config_Param.OverTime2) )//t2推荐10秒,应该比t1(15秒)短,这样大于10秒小于15秒的情况下可以发S帧
            return -1;
#if CFQ
    if( pTxBuf->GetWritableSize() < 6 )
        return -1;
#endif
    txData[0] = 0x68;
    txData[1] = 4;
    if( SpecialCtrlField == 1 )
    {
    txData[2] = 0;
    txData[3] = 0;
    txData[4] = 0;
    txData[5] = 0;
    }
    else
    {
    txData[2] = 1;
    txData[3] = 0;
    txData[4] = App_Layer.Rec_seqnum%128<<1;
    txData[5] = App_Layer.Rec_seqnum/128;
    }
            
    AddNeedSendFrame( txData, 6 );
//    pRouteInf->RecAFrame( PROTO_TXFRAME );

#if CFQ
  MakeByteFileName( file_name, del_name );
  if( del_name[0] )
  {
     if( 0 != strcmp(delete_name_saved,del_name) )
     {
          strcpy( delete_name_saved, del_name );
             DeleteOneDiskFile( del_name );
     }
  }
    pTxRxOutBuf->WriteTransStart( DIR_DOWN, file_name );
    pTxRxOutBuf->WriteDirect( txData, 6 );        
    pTxRxOutBuf->WriteTransEnd( );
#endif
    
    if( App_Layer.State == IEC104_APP_STATE_IDLE )    //空闲状态下才能刷新LastTxTime！
        App_Layer.LastTxTime = curtime.Sec;
    
    //每发送一S帧数据Snd_seqnum不变，清发送确认标志SendACKFlag。
    App_Layer.SendACKFlag = 0;
    App_Layer.RxWindowNum = 0;
    return 1;
}


/********************************************************************************
*
*    描述: 接收处理函数.
*    参数: 无.
*    返回: 0  本轮问答过程未结束
*          1  本轮问答过程结束
*
********************************************************************************/

int Iec104Zf::RxProc( void )
{
    if( 1 == App_Layer.Inited_Flag )
        App_SearchChangeYx();

    //lql modified 20081231
    while(1)
    {
        if( rxStep == 0 )
        {
            App_SearchStartChar();            //各步可执行多次
        }

        if( rxStep == 1 )
        {
            App_SearchFrameHead();
        }
        else
            break;

        if(rxStep!=0)
            break;
    }

    return AckFinished;
}


/********************************************************************************
*
*    描述: 搜索起始字 68H，收到后将其丢掉
*    参数: 无.
*    返回: 无.
*
********************************************************************************/
void Iec104Zf::App_SearchStartChar( void )
{
    while( pRxBuf->getReadableSize() >= 2 )
    {
        pRxBuf->read( &App_Layer.rxData[0], MOVE );
        if( App_Layer.rxData[0]  == 0x68 )
        {
            pRxBuf->read( &App_Layer.rxData[1], NOTMOVE );
            rxStep = 1;
            break;
        }
        else
        {
            if( StartCharErr == 1 )
            {
                LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("\nIec104Zf::App_SearchStartChar rtuaddr=%d startchar=%02X错误\n", m_rtuAddr, App_Layer.rxData[0]));
    
                InitData( );         //重新初始化
                pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay
            }
        }
    }
}

/********************************************************************************
*
*    描述: 检验报头
*    参数: 无.
*    返回: 无.
*
********************************************************************************/
void Iec104Zf::App_SearchFrameHead( void )
{
    if( pRxBuf->getReadableSize() >= App_Layer.rxData[1]+1 )
    {
        pRxBuf->read( &App_Layer.rxData[1], App_Layer.rxData[1]+1, MOVE );
                        
        TIME curtime;
        GetCurSec( &curtime );
        App_Layer.LastRxTime = curtime.Sec;                //收到帧，刷新接收计时器

#if CFQ
        char file_name[FILE_NAME_LENGTH];
        char del_name[FILE_NAME_LENGTH];
        MakeByteFileName( file_name, del_name );
        if( del_name[0] )
        {
           if( 0 != strcmp(delete_name_saved,del_name) )
           {
                strcpy( delete_name_saved, del_name );
                  DeleteOneDiskFile( del_name );
           }
        }
        pTxRxOutBuf->WriteTransStart( DIR_UP, file_name );
         pTxRxOutBuf->WriteDirect(&App_Layer.rxData[0],App_Layer.rxData[1]+2);    
        pTxRxOutBuf->WriteTransEnd( );
#endif
        if( App_Layer.rxData[2] & 0x01 )                //U or S format
        {
            App_RxFixFrame( );
        }
        else if( App_Layer.rxData[1] < 254 && App_Layer.rxData[1] > 4)    //I format
        {
            App_RxVarFrame( &App_Layer.rxData[0],App_Layer.rxData[1]+2);
        }
        else
        {
            pRxBuf->read( &App_Layer.rxData[0], MOVE );
        }
        
        rxStep = 0;
    }
}

/********************************************************************************
*
*    描述: 处理定长控制帧
*    参数: 无.
*    返回: 无.
*
********************************************************************************/
void Iec104Zf::App_RxFixFrame( void )
{
    if( App_Layer.rxData[1] != 4 )
    {
//        pRouteInf->RecAFrame( PROTO_RXERRORS );

        LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("\nIec104Zf::App_RxFixFrame App_Layer.rxData[1]=%d != 4\n",App_Layer.rxData[1]));
        InitData( );            //重新初始化
        pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay

        return;
    }
        
//    pRouteInf->RecAFrame( PROTO_RXFRAME );
        
    if( App_Layer.rxData[2] & 0x02 )        //U format
    {    
        uint8 type = App_Layer.rxData[2] & 0xfc;    
        
        if( type == APP_UFORMAT_STARACT)
        {
            InitData( );         //重新初始化
            App_Layer.State = IEC104_APP_STATE_IDLE;
            App_Layer.Inited_Flag = 1;
            AckFinished = 1;
            App_SendAppUFormat( APP_UFORMAT_STARCON );
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Send StartConf ===\n");
        }
        else if( type == APP_UFORMAT_STOPACT)
        {
            InitData( );         //重新初始化
            App_SendAppUFormat( APP_UFORMAT_STOPCON );
        }
        else if( type == APP_UFORMAT_TESTACT)
        {
            App_SendAppUFormat( APP_UFORMAT_TESTCON );
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Send TestConf ===\n");
        }
        else if( type == APP_UFORMAT_TESTCON)
        {
            if( App_Layer.State == IEC104_APP_STATE_WAITTESTCONF )
            {
                App_Layer.State = IEC104_APP_STATE_IDLE;
                AckFinished = 1;
                if( pRouteInf->GetDebugFlag() )
                    LOG_DEBUG(pRouteInf->GetChnId(), "IEC104ZF:IEC104zf Receive TestConf ===\n");
            }
        }
        else
        {
        }
    }
    else if(App_Layer.rxData[2] & 0x01 )        //S format  
    {    
        int Seqnum = App_Layer.rxData[4]/2 + App_Layer.rxData[5]*256;
        
        if( Seqnum == App_Layer.Snd_seqnum)    //得到主站对子站最后I format的确认
        {
            App_Layer.WaitACKFlag = 0;
            App_Layer.Ack_num = Seqnum;    
        }
    }
}

/********************************************************************************
*
*    描述: 处理变长信息帧
*    参数: apdu  指向应用协议数据单元(APDU) 的指针.
*          size  应用协议数据单元(APDU) 的长度
*    返回: 无.
*
********************************************************************************/
bool Iec104Zf::IfCheckSeqNum()
{
    return true;
}

void Iec104Zf::App_RxVarFrame( uint8 *apdu, int size )
{
    if( IfCheckLinkOk == 1 )/* 需要检测链路尚未建立 */
    {
        if( App_Layer.State == IEC104_APP_STATE_UNRESET )
            return;    
    }
    if( SpecialCtrlField == 1 )
    {
        CtrlField[0] = apdu[2];
        CtrlField[1] = apdu[3];
        CtrlField[2] = apdu[4];
        CtrlField[3] = apdu[5];
    }

    int Seqnum = apdu[2]/2 + apdu[3]*128;    //校验主站的Snd_seqnum

  if( !IfCheckSeqNum() )
      App_Layer.Rec_seqnum = Seqnum;

    if( Seqnum != App_Layer.Rec_seqnum )    //主站的I format 出现丢失或重复
    {
        LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("\nIec104Zf::App_RxVarFrame apdu[2]=%02X apdu[3]=%02X 错误\n",apdu[2],apdu[3]));

        //此种情况下确认完已接受正确的帧后再去主动断开socket连接yay
        if( App_Layer.RxWindowNum>0 )
            App_SendAppSFormat(true);

        InitData( );         //重新初始化
        pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay

        AckFinished = 1;    //接收结束        
//        pRouteInf->RecAFrame( PROTO_RXERRORS );
        return;
    }
    
//    pRouteInf->RecAFrame( PROTO_RXFRAME );
    
    Seqnum = apdu[4]/2 + apdu[5]*128;    //校验主站的Rec_seqnum
    if( Seqnum == App_Layer.Snd_seqnum )    //得到主站对子站最后I format的确认
        App_Layer.WaitACKFlag = 0;

    int thegap = 0;
    if( App_Layer.Snd_seqnum >= Seqnum )
        thegap = App_Layer.Snd_seqnum-Seqnum;
    else
        thegap = App_Layer.Snd_seqnum+0x8000-Seqnum;

      if( IfCheckSeqNum() )
    if( thegap >= 12 )
    {
        LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("\nIec104Zf::App_RxVarFrame apdu[4]=%02X apdu[5]=%02X 错误\n",apdu[4],apdu[5]));

        InitData( );         //重新初始化
        pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay

        AckFinished = 1;    //接收结束        
//        pRouteInf->RecAFrame( PROTO_RXERRORS );
        return;
    }

    //每接收一I帧数据Rec_seqnum加1，置发送确认标志SendACKFlag。    
    App_Layer.Rec_seqnum ++;        
    App_Layer.Rec_seqnum %= 0x8000;        //Rec_seqnum 为15位二进制数。
        
    App_Layer.SendACKFlag = 1;
    App_Layer.Ack_num = Seqnum;
    
    App_Layer.RxWindowNum ++;

    if( IfCheckLinkOk == 1 )/* 需要检测链路尚未建立 */
    {
      if( 0 == App_Layer.Inited_Flag )
         return;
    }

    uint8 *asdu = &apdu[6];
    size -=6;

    if( SpecialCtrlField == 1 )
    {
        if( asdu[0] == 0x46 )/* 不处理金智的0x46 */
            return;

        CtrlField[0] = apdu[2];
        CtrlField[1] = apdu[3];
        CtrlField[2] = apdu[4];
        CtrlField[3] = apdu[5];
    }
    
    App_rxApdu( asdu, size );
}

/********************************************************************************
*
*    描述: 处理应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104Zf::App_rxApdu( uint8 *asdu, int size )
{
    App_Recv_OtherType(asdu,size);

    if( App_Layer.State == IEC104_APP_STATE_ACK )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104ZF:IEC104zf 应用层非闲而收到变长请求帧%d,此帧被丢弃! App_Layer.State =%d, App_Layer.AckStep=%d \n",asdu[0],App_Layer.State, App_Layer.AckStep));

        return;
    }
    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("asdu[0]=%d", asdu[0]));
    switch( asdu[0] )
    {
    case APPTYPE_YK45:
    case APPTYPE_YK46:
        if(1 == App_RxYk(asdu, size ))
        {
            App_Layer.State = IEC104_APP_STATE_WAITYKCONF;
            AckFinished = 0;
        }
        break;
    default:
        App_Layer.rxReq.type = asdu[0];
        App_Layer.rxReq.cause = asdu[2];
        App_Layer.rxReq.detail = asdu[size-1];
        memcpy( App_Layer.rxApdu, asdu, size );
        App_Layer.rxSize = size;
        App_Layer.State = IEC104_APP_STATE_ACK;
        App_Layer.AckStep = 0;
        AckFinished = 0;
        break;
    }
}


/********************************************************************************
*
*    描述: 处理遥控应用服务数据单元(ASDU)
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
int Iec104Zf::App_RxYk( uint8 *asdu, int size )
{
    LOG_DEBUG(pRouteInf->GetChnId(), "");
    uint32    infnum = asdu[1]&0x7f;
    if(infnum!=1)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104:\nApp_RxSetPointConf received inf number!=1  ---\n" );

        return -1;
    }
    uint16    rtuaddr;
    int j,k;

    j = 3;

    if( Config_Param.CotNum == 2 )
            j++;

    if( Config_Param.ComAddrNum == 2 )
    {
            rtuaddr = asdu[j] + asdu[j+1]*256;
            j+=2;
    }
    else
    {
            rtuaddr = asdu[j];
            j++;
    }
    int ykno = asdu[j+1]*256+asdu[j];
    if( Config_Param.InfAddrNum == 3 )
        ykno += asdu[j+2]*256*256;

    ykno -= Config_Param.YkBaseAddr;

    if( Config_Param.InfAddrNum == 3 )
        k=j+3;
    else
        k=j+2;

    int value;
    if( asdu[0] == APPTYPE_YK45)//单点遥控
    {
        if(1==(asdu[k]&0x3))
        {
            value = 1;//合闸
        }
        else
        {
            value = 0;//分闸
        }
    }
    else //双点遥控
    {
        if(2==(asdu[k]&0x3))
        {
            value = 1;//合闸
        }
        else
        {
            value = 0;//分闸
        }
    }

    YKReqParam_S data;
    if(0x80 == (asdu[k]&0x80))//选择
    {
        LOG_DEBUG(pRouteInf->GetChnId(), "YK_CMD_SELECT");
        data.type = YK_CMD_SELECT;
    }
    else//执行
    {
        data.type = YK_CMD_EXECUTE;
    }
    data.cchId =pRouteInf->GetChnId();
    data.rtuId = pRtuInf->GetRtuIdByAddr(rtuaddr);
    data.no =ykno;
    data.value= value;
    this->pRawDb->SendCmdYk(data);//上送遥控命令到智能分析应用
    return 1;
}


int Iec104Zf::App_Recv_OtherType(uint8 *appdata, int datalen)
{
    return 1;
}


#if 0
void Iec104Zf::ReadConfigParam( void )
{

    memset( &Config_Param, 0, sizeof(Config_Param) );
    Config_Param.InfBaseAddrVersion = 2002;        //ASDU信息体基地址版本号
    Config_Param.YKCmdMode = 2;        //遥控命令使用(1:单点命令)还是(2:双点命令)
    Config_Param.SPCmdMode = 1;        //设点命令使用(1:单个设点命令)还是(2:连续设点命令)
    Config_Param.SOETransMode = 1;    //SOE是(1:一次传输,要根据SOE生成YX)还是(2:二次传输) //yay 20111209 许继优特104需要一次传输
    Config_Param.QcEnable = 0;            //处理数据质量码
    Config_Param.CotNum = 2;            //ASDU传输原因字节数
    Config_Param.ComAddrNum = 2;        //ASDU公共地址字节数
    Config_Param.InfAddrNum = 3;        //ASDU信息体地址字节数
    Config_Param.YxBaseAddr = 0x1;        //ASDU状态量起始地址
    Config_Param.YcBaseAddr = 0x4001;        //ASDU模拟量起始地址
    Config_Param.YkBaseAddr = 0x6001;        //ASDU控制量起始地址
    Config_Param.YkLastAddr = 0x6200;        //ASDU控制量结束地址
    Config_Param.SpBaseAddr = 0x6201;        //ASDU调节量起始地址
    Config_Param.SpLastAddr = 0x6400;        //ASDU调节量结束地址
    Config_Param.YtBaseAddr = 0x6601;        //ASDU分接头起始地址
    Config_Param.YtLastAddr = 0x6700;        //ASDU分接头结束地址
    Config_Param.DdBaseAddr = 0x6401;    //ASDU电度量起始地址
    Config_Param.MaxIframeNum = 1;    //最后确认的I帧最大数目
    Config_Param.MaxIsendNum = 12;    //未被确认的I帧最大数目
    Config_Param.OverTime1 = 15;//15;    //I帧或U帧发送后,经过OverTime1超时
    Config_Param.OverTime2 = 10;    //收到I帧后,经过OverTime2无数据报文时,发送S帧,OverTime2<OverTime1
    Config_Param.OverTime3 = 20;//20;    //长期空闲,经过OverTime3超时,发送测试帧,OverTime3>OverTime1
    Config_Param.UseQuality = 1;

    IfCheckLinkOk = 1;
    IfAndPnBit = 1;
    StartCharErr = 0;
    AppTypeErr = 0;
    CotErr = 0;
    ComAddrErr = 0;
    SpecialCtrlField = 0;
    CtrlField[0]=CtrlField[1]=CtrlField[2]=CtrlField[3]=0;
    IfCheckComAddr = 1;
    SpecialHighCot = 0;
    HighCot = 1;
    SpecialIpAddress = 0;


    uint32 temp;
    char filename[64];
    FILE *file = NULL;
#ifdef PLATFORM_WIN32
    sprintf(filename,"c:\\iec104zf_channo_%d_rtuaddr_%d.ini",pRouteInf->GetChnId()+1,m_rtuAddr);
#else
    sprintf(filename,"/utplat/.config/iec104zf_channo_%d_rtuaddr_%d.ini",pRouteInf->GetChnId()+1,m_rtuAddr);
#endif
    file = fopen(filename,"r");
    if(file != NULL)
    {
        int ret;
        char str[200];
        char *str1;
        while(1)
        {
            ret = fscanf(file,"%s",str);
            if(ret != EOF)
            {
                if( (str)&&(strstr(str,"InfBaseAddrVersion")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.InfBaseAddrVersion = atoi(str1+1);
                        if((Config_Param.InfBaseAddrVersion != 1997) && (Config_Param.InfBaseAddrVersion != 2002))
                            Config_Param.InfBaseAddrVersion = 2002;
                        if(Config_Param.InfBaseAddrVersion == 2002) //2002版
                        {
                                Config_Param.YxBaseAddr = 0x1;          //ASDU状态量起始地址
                                Config_Param.YcBaseAddr = 0x4001;        //ASDU模拟量起始地址
                                Config_Param.YkBaseAddr = 0x6001;        //ASDU控制量起始地址
                                Config_Param.YkLastAddr = 0x6200;        //ASDU控制量结束地址
                                Config_Param.SpBaseAddr = 0x6201;        //ASDU调节量起始地址
                                Config_Param.SpLastAddr = 0x6400;        //ASDU调节量结束地址
                                Config_Param.YtBaseAddr = 0x6601;        //ASDU分接头起始地址
                                Config_Param.YtLastAddr = 0x6700;        //ASDU分接头结束地址
                                Config_Param.DdBaseAddr = 0x6401;    //ASDU电度量起始地址
                        }
                        else //1997版
                        {
                                Config_Param.YxBaseAddr = 0x1;          //ASDU状态量起始地址
                                Config_Param.YcBaseAddr = 0x701;        //ASDU模拟量起始地址
                                Config_Param.YkBaseAddr = 0xb01;        //ASDU控制量起始地址
                                Config_Param.YkLastAddr = 0xbff;        //ASDU控制量结束地址
                                Config_Param.SpBaseAddr = 0xb81;        //ASDU调节量起始地址
                                Config_Param.SpLastAddr = 0xc00;        //ASDU调节量结束地址
                                Config_Param.YtBaseAddr = 0xc81;        //ASDU分接头起始地址
                                Config_Param.YtLastAddr = 0xca0;        //ASDU分接头结束地址
                                Config_Param.DdBaseAddr = 0xc01;      //ASDU电度量起始地址
                        }
                    }
                    
                }
                else if( (str)&&(strstr(str,"YKCmdMode")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.YKCmdMode=atoi(str1+1);
                        if((Config_Param.YKCmdMode != 1) && (Config_Param.YKCmdMode != 2))
                            Config_Param.YKCmdMode=2;
                    }
                }
                else if( (str)&&(strstr(str,"YkBaseAddr")) )
                {
                    str1 = strstr(str,"=0x");
                    if(str1 &&(str1+3))
                    {
                        sscanf( str1+3, "%x", &temp );
                        Config_Param.YkBaseAddr=temp;
                    }
                }
                else if( (str)&&(strstr(str,"SPCmdMode")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.SPCmdMode=atoi(str1+1);
                        if((Config_Param.SPCmdMode != 1) && (Config_Param.SPCmdMode != 2))
                            Config_Param.SPCmdMode=1;
                    }
                }
                else if( (str)&&(strstr(str,"SOETransMode")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.SOETransMode=atoi(str1+1);
                        if((Config_Param.SOETransMode != 1) && (Config_Param.SOETransMode != 2))
                            Config_Param.SOETransMode=1;
                    }
                }
                else if( (str)&&(strstr(str,"QcEnable")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.QcEnable = atoi(str1+1);
                        if((Config_Param.QcEnable != 1) && (Config_Param.QcEnable != 0))
                            Config_Param.QcEnable = 0;
                    }
                }
                else if( (str)&&(strstr(str,"CotNum")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.CotNum=atoi(str1+1);
                        if((Config_Param.CotNum != 1) && (Config_Param.CotNum != 2))
                            Config_Param.CotNum=1;
                    }
                }
                else if( (str)&&(strstr(str,"ComAddrNum")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.ComAddrNum=atoi(str1+1);
                        if((Config_Param.ComAddrNum != 1) && (Config_Param.ComAddrNum != 2))
                            Config_Param.ComAddrNum=1;
                    }
                }
                else if( (str)&&(strstr(str,"InfAddrNum")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                    {
                        Config_Param.InfAddrNum = atoi(str1+1);
                        if((Config_Param.InfAddrNum != 2) && (Config_Param.InfAddrNum != 3))
                            Config_Param.InfAddrNum=2;
                    }
                }
                else if( (str)&&(strstr(str,"MaxIframeNum")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        Config_Param.MaxIframeNum=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"MaxIsendNum")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        Config_Param.MaxIsendNum=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"OverTime1")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        Config_Param.OverTime1=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"OverTime2")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        Config_Param.OverTime2=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"OverTime3")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        Config_Param.OverTime3=atoi(str1+1);
                }                
                else if( (str)&&(strstr(str,"IfCheckLinkOk")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        IfCheckLinkOk=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"IfAndPnBit")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        IfAndPnBit=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"StartCharErr")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        StartCharErr=atoi(str1+1);
                }                
                else if( (str)&&(strstr(str,"AppTypeErr")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        AppTypeErr=atoi(str1+1);
                }                
                else if( (str)&&(strstr(str,"CotErr")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        CotErr=atoi(str1+1);
                }                
                else if( (str)&&(strstr(str,"ComAddrErr")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        ComAddrErr=atoi(str1+1);
                }                
                else if( (str)&&(strstr(str,"SpecialCtrlField")) )
                {
                        str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        SpecialCtrlField=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"SpecialHighCot")) )
                {
                        str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        SpecialHighCot=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"IfCheckComAddr")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        IfCheckComAddr=atoi(str1+1);
                }
                else if( (str)&&(strstr(str,"SpecialIpAddress")) )
                {
                    str1 = strchr(str,'=');
                    if(str1 &&(str1+1))
                        SpecialIpAddress=atoi(str1+1);
                }
            }
            else
                break;
        }
        fclose(file);
    }

    LOG_DEBUG(pRouteInf->GetChnId(), "Iec104Zf::ReadConfigParam SpecialCtrlField=%d SpecialHighCot=%d IfCheckComAddr=%d SpecialIpAddress=%d rtuaddr=%d channo=%d\n",
                SpecialCtrlField, SpecialHighCot, IfCheckComAddr, SpecialIpAddress, m_rtuAddr,pRouteInf->GetChnId()+1);
}
#endif

void Iec104Zf::DeleteOneDiskFile( char *filename )
{
   if( NULL == filename )
      return;

   unlink( filename );
}

#if CFQ
void Iec104Zf::MakeByteFileName( char *filename, char *delname )
{
   SYSTIME curtime;
   SYSTIME deltime;
   char *datadir;

   if( filename==NULL || delname==NULL )
   {
      LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("Iec104Zf::MakeByteFileName Rtuaddr=%ld ChnId=%d filename=NULL delname=NULL error\n", m_rtuAddr, pRouteInf->GetChnId()+1 ));
      return;
   }

#ifdef PLATFORM_WIN32
   sprintf(filename,"c:\\iec104zf");
#else
   sprintf(filename,"/utplat/proj/iec104zf");
#endif

   sprintf(delname,"%s",filename);

   GetCurTime( &curtime );
   deltime = GetTime( curtime, HOW_MANY_SENCODS_AGO, 0 );
   sprintf(filename,"%s_%d_%d_%04d%02d%02d.txt",filename,pRouteInf->GetChnId()+1,m_rtuAddr,curtime.year,curtime.month,curtime.day);
   sprintf(delname,"%s_%d_%d_%04d%02d%02d.txt",delname,pRouteInf->GetChnId()+1,m_rtuAddr,deltime.year,deltime.month,deltime.day);
}
#endif
int Iec104Zf::SendBackUnknownInfoData( int apptype, int cot, uint8* data, int len )
{
        uint8 buf[256];
        int i = 2;

        buf[0] = apptype;
        buf[1] = 1;

        buf[i++] = cot;
        if( Config_Param.CotNum == 2 )//传输原因2字节
            buf[i++] = 0;

        buf[i++] = (m_rtuAddr)%256;
        if( Config_Param.ComAddrNum == 2 )//公共地址2字节
            buf[i++] = ( m_rtuAddr)/256;

        for(int index=0;index<len;index++)
            buf[i++] = data[index];

        if( App_SendAppIFormat( buf, i ) == 1 )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("Sk104Sub:SendBackUnknownInfoData apptype=%0X cot=%02X\n", apptype, cot ));
            return 1;
        }
        else
            return 0;
}

/**
 * @brief 设置规约配置参数
 * @param protocolCfg 规约配置参数
 */
void Iec104Zf::SetConfigParam(ProtocolCfgParam_S &protocolCfg)
{
    LOG_DEBUG(pRouteInf->GetChnId(), "");
    memset( &Config_Param, 0, sizeof(Config_Param) );
    Config_Param.InfBaseAddrVersion = 2002;        //ASDU信息体基地址版本号
    Config_Param.YKCmdMode = protocolCfg.ykCmdMode;        //遥控命令使用(1:单点命令)还是(2:双点命令)
    Config_Param.SPCmdMode = protocolCfg.spCmdMode;        //设点命令使用(1:单个设点命令)还是(2:连续设点命令)
    Config_Param.SOETransMode = protocolCfg.soeTransMode;    //SOE是(1:一次传输,要根据SOE生成YX)还是(2:二次传输) //yay 20111209 许继优特104需要一次传输
    Config_Param.QcEnable = 0;            //处理数据质量码
    Config_Param.CotNum = protocolCfg.cotNum;            //ASDU传输原因字节数
    Config_Param.ComAddrNum = protocolCfg.comAddrNum;        //ASDU公共地址字节数
    Config_Param.InfAddrNum = protocolCfg.infAddrNum;        //ASDU信息体地址字节数
    Config_Param.YxBaseAddr = protocolCfg.yxBaseAddr;        //ASDU状态量起始地址
    Config_Param.YcBaseAddr = protocolCfg.ycBaseAddr;        //ASDU模拟量起始地址
    Config_Param.YkBaseAddr = protocolCfg.ykBaseAddr;        //ASDU控制量起始地址
    Config_Param.YkLastAddr = 0;        //ASDU控制量结束地址
    Config_Param.SpBaseAddr = protocolCfg.spBaseAddr;        //ASDU设置点起始地址
    Config_Param.SpLastAddr = protocolCfg.spLastAddr;        //ASDU设置点结束地址
    Config_Param.YtBaseAddr = protocolCfg.ytBaseAddr;        //ASDU调节量起始地址
    Config_Param.YtLastAddr = protocolCfg.ytLastAddr;        //ASDU调节量结束地址
    Config_Param.DdBaseAddr = protocolCfg.ddBaseAddr;      //ASDU电度量起始地址
    Config_Param.MaxIframeNum = 8;    //最后确认的I帧最大数目
    Config_Param.MaxIsendNum = 12;    //未被确认的I帧最大数目
    Config_Param.OverTime1 = 15;    //I帧或U帧发送后,经过OverTime1超时
    Config_Param.OverTime2 = 10;    //收到I帧后,经过OverTime2无数据报文时,发送S帧,OverTime2<OverTime1
    Config_Param.OverTime3 = 20;    //长期空闲,经过OverTime3超时,发送测试帧,OverTime3>OverTime1
    Config_Param.UseQuality = 1;

    Config_Param.ackCalldataTimeout = 5;
    Config_Param.ackYkTimeout = 5;
    Config_Param.SpFullCodeVal = protocolCfg.spFullCodeVal;


    IfCheckLinkOk = 1;
    IfAndPnBit = 1;
    StartCharErr = 0;
    AppTypeErr = 0;
    CotErr = 0;
    ComAddrErr = 0;
    SpecialCtrlField = 0;
    CtrlField[0]=CtrlField[1]=CtrlField[2]=CtrlField[3]=0;
    IfCheckComAddr = 1;
    SpecialHighCot = 0;
    HighCot = 1;
    SpecialIpAddress = 0;
}


/**
 * @brief 应用层发送后台命令处理
 * @param command 发送的命令数据
 * @return  -1  发送失败，
 *            1  发送成功.
 */
int Iec104Zf::App_SendCommand(COMMAND *command )
{
    if(NULL == command)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "command is null!");
        return -1;
    }
    int ret =-1;
    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("CmdType:%d", command->cmdType));
    switch( command->cmdType )
    {
    case CMD_TYPE_YX: //遥信上送
        if(1 == App_Ack_Yx(command))
        {
            ret = 1;
        }
        break;
    case CMD_TYPE_YC://遥测上送
        if(1 == App_Ack_Yc(command))
        {
            ret = 1;
        }
        break;
    case CMD_TYPE_CALLALLDATAEND://总召结束
        if(3 == App_Layer.AckStep) //等待结束状态
        {
            if(1 ==  App_All_Finish())
            {
                App_Layer.State = IEC104_APP_STATE_IDLE;
                App_Layer.AckStep = 0;
                AckFinished = 1;
                ret = 1;
            }
        }
        else
        {
            ret = 1;//如果不是等待结束状态，说明没有发起总召，直接过滤掉此命令
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("Not the waitting finish state of calldata! will skip this command! cmdType:%d", command->cmdType));
        }
        break;
    case CMD_TYPE_YKRESULT://遥控（选择或执行）结果
        if(IEC104_APP_STATE_WAITYKCONF == App_Layer.State) //等待遥控确认状态
        {
            if(1 == App_Ack_Yk(command))
            {
                App_Layer.State = IEC104_APP_STATE_IDLE;
                AckFinished = 1;
                ret = 1;
            }
        }
        else
        {
            ret = 1;//如果不是等待遥控确认状态，直接过滤掉此命令
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("Not the waitting yk confirming state of calldata! will skip this command! cmdType:%d", command->cmdType));
        }
        break;
    default:
        LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("Unsurported command! cmdType:%d", command->cmdType));
        ret = 1;
        break;
    }

    return ret;
}

#endif
