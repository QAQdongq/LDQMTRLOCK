/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  iec104.cxx
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：实现规约类Iec104
 *其它说明：description
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef PLATFORM_WIN32
#include <WinSock2.h>
 #include <windows.h>
 extern struct timeval;
 #include <time.h>
#else
 #include <sys/time.h>
 #include <unistd.h>
 #include <time.h>
#endif

#include "iec104.h"
#include "utils/converter_util.h"
#include "../acq-protocol-plugin-utils/string_util.h"
#include <list>
#include <thread>
#include <chrono>
#include "scn_tasklist.h"


void Iec104::Restore()
{
    //Restore 清空接受缓冲区是必须的,已免处理上次连接的残留字节流yay
    pRxBuf->clear();
    //pTxBuf->clear();
    InitData( );
    Time_cor = 0;
    rxStep = 0;
    restore_flag = false;
}

/********************************************************************************
*
*    描述: 规约参数设置函数,负责初始化规约接口对象和规约
*          对象内部数据.
*    参数: protocol_config: 规约配置参数(输入)；
*    返回: 无.
*
********************************************************************************/

void Iec104::SetProtocolConfig( PROTOCOL_CONFIG protocol_config )
{
    Poll::SetProtocolConfig( protocol_config );
    Restore();
}


/********************************************************************************
*
*    描述: 初始化规约数据.
*    参数: 无.
*    返回: 无.
*
********************************************************************************/

void Iec104::InitData( void )
{
    App_Layer.RxWindowNum = 0;
    App_Layer.Resend_Times = 0;
    App_Layer.Inited_Flag = 0;
    App_Layer.SendACKFlag = 0;
    App_Layer.WaitACKFlag = 0;
    if(pRouteInf->GetAllScan() > 0)
    {
        App_Layer.CallAllDataFlag = 1;
    }
    else
    {
        App_Layer.CallAllDataFlag = 0;
    }
    App_Layer.CallGroupDataFlag = 0;
    App_Layer.CallGroupNo = 0;
    //QString rtuId = pRouteInf->GetRtuId();
    //if(pRtuInf->GetKwhNum(rtuId) > 0)
    if(pRouteInf->GetScanPulseTime()>0)
    {
        App_Layer.CallAllKwhFlag = 1;
    }
    else
    {
        App_Layer.CallAllKwhFlag = 0;
    }

    /*chenfuqing 20230914 modified 轨道需求：校时周期（秒）为 0时 ，不要校时命令*/
    //if(pRtuInf->GetSyncTimeInt(0) < MAX_SYNTIME_PIERIOD)
    if(pRtuInf->GetSyncTimeInt(0)>0)
    {
        App_Layer.TimeSyncFlag = 1;
    }
    else
    {
        App_Layer.TimeSyncFlag = 0;
    }        

    App_Layer.Snd_seqnum = 0;
    App_Layer.Rec_seqnum = 0;
    App_Layer.Ack_num = 0;
    App_Layer.State = IEC104_APP_STATE_UNRESET;
    App_Layer.Command_Enable = 0;

    strcpy(delete_name_saved,"initial_name");

    TIME curtime;
    GetCurSec( &curtime );
    App_Layer.LastTxTime = curtime.Sec;
    App_Layer.LastCallAllTime = curtime.Sec;
    App_Layer.LastSyncTime = curtime.Sec;
    App_Layer.LastCallAllKwhTime = curtime.Sec;

}

/********************************************************************************
*
*    描述: 发送处理函数.
*    参数: 无.
*    返回: 0  本轮问答过程未结束
*          1  本轮问答过程结束
*
********************************************************************************/

int Iec104::TxProc( void )
{
    if(restore_flag)
        Restore();

/*    if( pRouteInf->ReqPermission( Ordinarily_Send ) == 0 )
    {
        InitData( );
        return 1;
    }*/
    int rtuaddr = pRouteInf->GetRtuAddr();
    //目前所有发出的I format都有I format的响应，因而不会有子站S format
    if( App_Layer.State == IEC104_APP_STATE_IDLE )    //route窗口尺寸为1
    {

        App_Layer.Resend_Times = 0;        //清上次重发的重发次数记忆值        
    #if CFQ
        COMMAND *command;
        command = (COMMAND*)malloc( sizeof(COMMAND));
        if( 0 == command )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "Iec104::TxProc command malloc error!\n");
            return 1;
        }
        memset( command, 0, sizeof(COMMAND) );
        if( pRtuInf->GetRtuType( rtuaddr) == 1 )//special command
        {    
            if( pCommandMem->GetACommand( rtuaddr, command) == 1 )
            {
                if((command->CmdType != CMD_TYPE_YK) || (App_Layer.Command_Enable != 0))
                {
                    if( App_SendCommand( rtuaddr, command ) == 1 )
                    {
                        pCommandMem->RemoveACommand( rtuaddr );
                    }
                    free(command);
                    return 1;
                }
            }
        }
        else
        {
            if( pRouteInf->ReqPermission( Command_Send ) == 1 )
            {
                if( pCommandMem->GetACommand( rtuaddr, command) == 1 )
                {
                    if( App_SendCommand( rtuaddr, command ) == 1 )
                    {
                        pCommandMem->RemoveACommand( rtuaddr );
                    }
                    free(command);
                    return 1;
                }
            }
        }
        free(command);
#else
        std::shared_ptr<COMMAND> command;
        if( pCommandMem->GetACommand(command) == 1 )
        {
            int iRet = App_SendCommand(command.get());
            if(iRet <= 0 )
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("Failed to execute command! cmdType:%1, taskId:%2").arg(command->cmdType).arg(command->taskId));
            }
            else
            {
                //start: chenfuqing added 20230805: 采集104在处理遥控，遥调等命令后，不要再发总召，所以要重置总召周期最后接收时间，这样就会重新开始计时。
                App_ResetLastTime();
                //end: chenfuqing added 20230805
            }
            pCommandMem->RemoveACommand();
            return 1;
        }
#endif

#if 1//CFQ
        if( App_Layer.CallAllDataFlag ) 
        {
            if( App_CallAllData(rtuaddr) == 1 )
            {
                if( pRouteInf->GetDebugFlag() )
                    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 Call All Data AppLayer State %d===\n",App_Layer.State));

                App_Layer.CallAllDataFlag = 0; 
            }
            return 1; 
        }
        if( App_Layer.CallGroupDataFlag ) 
        {
            if( App_CallGrpData(rtuaddr, App_Layer.CallGroupNo) == 1 )
            {
                if( pRouteInf->GetDebugFlag() )
                    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 Call Group No=%d Data===\n",App_Layer.CallGroupNo));

                App_Layer.CallGroupDataFlag = 0;
            }
            return 1; 
        }
        if( App_Layer.TimeSyncFlag ) 
        {
            if( App_SyncTime(rtuaddr) == 1 )
            {
                App_Layer.TimeSyncFlag = 0; 
            }
            return 1; 
        }
        if( App_Layer.CallAllKwhFlag ) 
        {
            if( App_CallAllKwh(rtuaddr) == 1 )
            {
                App_Layer.CallAllKwhFlag = 0; 
            }
            return 1; 
        }
#else

        if( App_Layer.TimeSyncFlag )
        {
            bool isAllSuccess = true;
            std::list<int>::const_iterator iter =  m_rtuAddrList.begin();
            for(; iter != m_rtuAddrList.end(); iter++)
            {
                if( App_SyncTime(*iter) != 1 )
                {
                    isAllSuccess = false;
                    break;
                }
            }
            if(isAllSuccess)
            {
                App_Layer.TimeSyncFlag = 0;
            }
            return 1;
        }
#endif
        if( App_SendAppUFormat( APP_UFORMAT_TESTACT )==1 )    //Send U(TESTFR act) format
        {
            App_Layer.State = IEC104_APP_STATE_WAITTESTCONF;
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Send Test Act Frame ===\n");

            return 1;
        }
    }
    else
    {
        App_CheckState( );
    }
    if( App_Layer.SendACKFlag == 1 )                //Send S format
    {
        if( App_SendAppSFormat() == 1 )
        {
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 Send S(%d) Frame ===\n",App_Layer.Rec_seqnum));
        }
    }

    return AckFinished;
}

/********************************************************************************
*
*    描述: 检查应用层状态，并作相应处理
*    参数: 无.
*    返回: 无.
*
********************************************************************************/
void Iec104::App_CheckState( void )
{
    TIME curtime;
    GetCurSec( &curtime );
    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_IDLE:
        break;
    case IEC104_APP_STATE_UNRESET:
        if( App_SendAppUFormat(APP_UFORMAT_STARACT) == 1 )    //通道复位由设备驱动负责
        {
            App_Layer.State = IEC104_APP_STATE_WAITSTARCONF;

            //发送启动帧时，RTU链路状态置为: 0-未连接
            if(m_protoInterface)
            {
                emit m_protoInterface->setRTUStatus(pRouteInf->GetRtuId(), 0);
            }
        }
        break;
    case IEC104_APP_STATE_WAITALLDATA:                
    case IEC104_APP_STATE_WAITALLDATACONF:
    case IEC104_APP_STATE_WAITTIMECONF:
    case IEC104_APP_STATE_WAITALLKWH:
    case IEC104_APP_STATE_WAITALLKWHCONF:
    case IEC104_APP_STATE_WAITSTARCONF:
    case IEC104_APP_STATE_WAITSTOPCONF:
    case IEC104_APP_STATE_WAITTESTCONF:
    case IEC104_APP_STATE_WAITGRPDATA:
    case IEC104_APP_STATE_WAITGRPKWH:
    {
        if( curtime.Sec-App_Layer.LastTxTime > pRouteInf->GetRxTimeouts() )//t1推荐15秒,应该比TimeOut2(10秒)长,这样大于10秒小于15秒的情况下可以发S帧
        {
            //pRouteInf->RecAFrame( PROTO_RXTIMEOUTS );
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_CheckState...App_Layer.State=%d...TIMEOUT(%d) !!!", App_Layer.State, pRouteInf->GetRxTimeouts()));

            if( App_Layer.Resend_Times >= pRouteInf->GetRetryTimes( ) )
            {
                LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 App_Lyer.State %d time out %d s!===\n",App_Layer.State,pRouteInf->GetRxTimeouts( )));

                //测试帧连续3次异常(一般为超时)时, RTU链路状态置为: 0-未连接
                if(IEC104_APP_STATE_WAITTESTCONF == App_Layer.State && m_protoInterface)
                {
                    emit m_protoInterface->setRTUStatus(pRouteInf->GetRtuId(), 0);
                }

                InitData( );         //重新初始化
                pRouteInf->UpdateRouteDev();//此种情况下还要去主动断开socket连接,使对方知道重新初始化yay

                AckFinished = 1;    //接收结束    
                //if( App_SendAppUFormat(APP_UFORMAT_STOPACT) ==1 )
                //    App_Layer.State = IEC104_APP_STATE_WAITSTOPCONF;
            }
            else if( App_ResendData( ) == 1 )
            {
                App_Layer.Resend_Times++;
                if( pRouteInf->GetDebugFlag() )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104===resend data- times: %d, Curtime_Sec: %lu\n", App_Layer.Resend_Times,curtime.Sec));
                    LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104===resend data- App_Lyer.State %d time out %d s!===\n",App_Layer.State,pRouteInf->GetRxTimeouts( )));
                }        
            }
        }
        break;
    }
    //防误操作
    case IEC104_APP_STATE_WAITFWCONF:
    {
        if( curtime.Sec-App_Layer.LastTxTime > Config_Param.FwTimeout)//遥控超时
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;    //接收结束
            std::string operDesc ="FW";
            QString codeQStr = QString::fromStdString(CMD_TYPE_STR_WF);
            int cmdType = CMD_TYPE_WF;

            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_CheckState...[%1]...TIMEOUT(%2) !!! CtrlType:%3").arg(operDesc.c_str()).arg(Config_Param.FwTimeout).arg(App_Layer.CtrlType));

            std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, -1);
            if(nullptr == taskInfoPtr)
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("[%1] [TIMEOUT]: Not found task! codeQStr:%2").arg(operDesc.c_str()).arg(codeQStr));
                return;
            }
            if(taskInfoPtr->dataList.size() <=0)
            {
                LOG_ERROR(pRouteInf->GetChnId(), QString("[%1] Not found 'dataList' in task info! taskId:%2").arg(operDesc.c_str()).arg(taskInfoPtr->taskId));
                return;
            }

            QList<FwRspParam_S> fwRspList;
            for(int m=0; m<taskInfoPtr->dataList.size(); m++)
            {
                std::shared_ptr<FWReqParam_S> data = std::static_pointer_cast<FWReqParam_S>(taskInfoPtr->dataList.at(m));
                FwRspParam_S rspData;
                rspData.rtuId = data->rtuId;
                rspData.cchId = data->cchId;
                rspData.no = data->no;
                rspData.type = data->type;
                rspData.value = data->value;
                rspData.reason = "";
                rspData.result = CMD_RESULT_TIMEOUT;
                //防误类型：1：防误校验、2：应急模式
                if(1 == rspData.type)
                {
                    rspData.reason = (rspData.value == 1? "防误模式校验:合-超时！":"防误模式校验:分-超时！");
                }
                else
                {
                    rspData.reason = (rspData.value == 1? "应急模式校验:合-超时！":"应急模式校验:分-超时！");
                }
                fwRspList.append(rspData);
            }
            pCommandMem->SendFWReply(cmdType, pRouteInf->GetRtuId(), taskInfoPtr->taskId, fwRspList);

            pTaskList->removeATask(codeQStr, -1);
            LOG_INFO(pRouteInf->GetChnId(), QString("<====[%1] [TIMEOUT]: remove task, codeQStr:%2").arg(operDesc.c_str()).arg(codeQStr));

        }
        break;
    }
    //遥控操作
    case IEC104_APP_STATE_WAITYKCONF:
    case IEC104_APP_STATE_WAITYKFINISH:
    {
        if( curtime.Sec-App_Layer.LastTxTime > Config_Param.YkTimeout)//遥控超时
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;    //接收结束
            std::string operDesc ="YK";
            QString codeQStr = QString::fromStdString(CMD_TYPE_STR_YK);
            int ctrlReply = COMMAND_YKEXEC_TIMEOUT;
            int cmdType = CMD_TYPE_YK;
            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_CheckState...[%1]...TIMEOUT(%2) !!! CtrlType:%3").arg(operDesc.c_str()).arg(Config_Param.YkTimeout).arg(App_Layer.CtrlType));

            int ykNo = App_Layer.CtrlNo;
            std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, ykNo);
            if(nullptr == taskInfoPtr)
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("[%1] [TIMEOUT]: Not found task! codeQStr:%2, no:%3").arg(operDesc.c_str()).arg(codeQStr).arg(ykNo));
                return;
            }
            QString rtuId = pRouteInf->GetRtuId();
            pCommandMem->SendYkYtReply(cmdType, rtuId, ykNo, App_Layer.CtrlAttr, ctrlReply, taskInfoPtr);

            pTaskList->removeATask(codeQStr, ykNo);
            LOG_INFO(pRouteInf->GetChnId(), QString("<====[%1] [TIMEOUT]: remove task, codeQStr:%2, no:%3").arg(operDesc.c_str()).arg(codeQStr).arg(ykNo));
        }
        break;
    }
    //遥调操作
    case IEC104_APP_STATE_WAITYTCONF:
    case IEC104_APP_STATE_WAITYTFINISH:
    {
        if( curtime.Sec-App_Layer.LastTxTime > Config_Param.YkTimeout)//遥调超时
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;    //接收结束
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_CheckState...YT...TIMEOUT(%d) !!!", Config_Param.YkTimeout));
            QString codeQStr = QString::fromStdString(CMD_TYPE_STR_YT);
            int ytNo = App_Layer.CtrlNo;
            std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, ytNo);
            if(nullptr == taskInfoPtr)
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("YT [TIMEOUT]: Not found task! codeQStr:%1, no:%2").arg(codeQStr).arg(ytNo));
                return;
            }
            QString rtuId = pRouteInf->GetRtuId();
            pCommandMem->SendYkYtReply(CMD_TYPE_YT, rtuId, ytNo, App_Layer.CtrlAttr, COMMAND_YKEXEC_TIMEOUT, taskInfoPtr);
            pTaskList->removeATask(codeQStr, ytNo);
            LOG_INFO(pRouteInf->GetChnId(), QString("<====YT [TIMEOUT]: remove task, codeQStr:%1, no:%2").arg(codeQStr).arg(ytNo));

        }
        break;
    }
    //设置点操作
    case IEC104_APP_STATE_WAITSPCONF:
    case IEC104_APP_STATE_WAITSPFINISH:
    {
        if( curtime.Sec-App_Layer.LastTxTime > Config_Param.SpTimeout)//设置点超时
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;    //接收结束
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_CheckState...SP...TIMEOUT(%d) !!!", Config_Param.SpTimeout));
            QString codeQStr = QString::fromStdString(CMD_TYPE_STR_SP);
            int spNo = App_Layer.CtrlNo;
            std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, spNo);
            if(nullptr == taskInfoPtr)
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("SP [TIMEOUT]: Not found task! codeQStr:%1, no:%2").arg(codeQStr).arg(spNo));
                return;
            }
            QString rtuId = pRouteInf->GetRtuId();
            pCommandMem->SendSpReply(CMD_TYPE_SP, rtuId, spNo, COMMAND_YKEXEC_TIMEOUT, taskInfoPtr);
            pTaskList->removeATask(codeQStr, spNo);
            LOG_INFO(pRouteInf->GetChnId(), QString("<====SP [TIMEOUT]: remove task, codeQStr:%1, no:%2").arg(codeQStr).arg(spNo));
        }
        break;
    }
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Invalid primary app state\n");

        break;
    }
}

/********************************************************************************
*
*    描述: 应用层重发上次发送的数据，同步时钟命令需重新取时间.
*    参数: 无.
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_ResendData( void )
{
    TIME curtime;
#if CFQ
    if( pTxBuf->GetWritableSize() < App_Layer.txSize )
        return -1;
#endif

    GetCurSec( &curtime );
    App_Layer.LastTxTime = curtime.Sec;
    if( App_Layer.State == IEC104_APP_STATE_WAITTIMECONF )
    {
        App_SyncTimeCoding( &App_Layer.txData[12] );
    }
    
    /*应用层重发命令，发送序列号+1*/
    if( (App_Layer.State != IEC104_APP_STATE_WAITSTARCONF) &&
          (App_Layer.State != IEC104_APP_STATE_WAITSTOPCONF)  &&
          (App_Layer.State != IEC104_APP_STATE_WAITTESTCONF) )
    {
        
        App_Layer.txData[2] = App_Layer.Snd_seqnum%128<<1;
        App_Layer.txData[3] = App_Layer.Snd_seqnum/128;
        App_Layer.txData[4] = App_Layer.Rec_seqnum%128<<1;
        App_Layer.txData[5] = App_Layer.Rec_seqnum/128;
        
        //每发送一I帧数据Snd_seqnum加1，清发送确认标志SendACKFlag。
        App_Layer.Snd_seqnum ++;        
        App_Layer.Snd_seqnum %= 0x8000;        //Snd_seqnum 为15位二进制数。
    }
    AddNeedSendFrame(&App_Layer.txData[0], App_Layer.txSize);
    //pRouteInf->RecAFrame( PROTO_TXFRAME );

#if CFQ
  pRouteInf->SetDevSendEnable(1);
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
    pTxRxOutBuf->WriteDirect(&App_Layer.txData[0],App_Layer.txSize);
    pTxRxOutBuf->WriteTransEnd( );
#endif
    return 1;
}


/********************************************************************************
*
*    描述: 应用层发送后台命令.
*    参数: rtuAddr    目标RTU的地址
*          command  待发送的命令数据. 
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_SendCommand(COMMAND *command )
{
    LOG_DEBUG(pRouteInf->GetChnId(), "");
    if(NULL == command)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "command is null!");
        return -1;
    }
    int ret =-1;
    //int i=0;
    //CtrlCmd *pYk = (CtrlCmd *)command->CmdData;
    //int rtuAddr = pYk->rtuAddr;
    LOG_DEBUG(pRouteInf->GetChnId(), QString("cmdType: %1, taskId: %2").arg(command->cmdType).arg(command->taskId));
    switch( command->cmdType )
    {
    case CMD_TYPE_WF://liujie add 添加五防操作
        ret = App_SendWFJS(command );
        break;
    case CMD_TYPE_SetPasswordReg://ldq add 添加添加设置密码锁密码命令
        ret = App_SetPasswordReg(command );
        break;
    case CMD_TYPE_YK:
        if( Config_Param.YKCmdMode == 1 )
            ret = App_SendYk45(command );
        else
            ret = App_SendYk(command );
        break;
    case CMD_TYPE_YT:
        ret = App_SendYt47(command);
        break;
    case CMD_TYPE_SP:
        ret = App_SendSetPoint(command );
        break;
    case CMD_TYPE_PROT:
        break;
    case CMD_TYPE_SYNCTIME:
    {
        //App_Layer.TimeSyncFlag = 1;
        std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command->dataList.begin();
        for(; iter != command->dataList.end(); iter++)
        {
            int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr((*iter)->rtuId);
            App_SyncTime(rtuAddr);
        }
        ret = 1;
        break;
    }
    case CMD_TYPE_CALLALLDATA:
    {
        //App_Layer.CallAllDataFlag = 1;
        std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command->dataList.begin();
        for(; iter != command->dataList.end(); iter++)
        {
            int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr((*iter)->rtuId);
            App_CallAllData(rtuAddr);
        }
        ret = 1;
        break;    
    }
#if CFQ
    case CMD_TYPE_GROUP_CALL:
    {
        int groupno;
        groupno = (((CtrlCmd *)(command->CmdData))->ptAddress);//组号
      //是否考虑RTU的组号有最大16组的限制??
        if( (groupno > 0) && (groupno <= pRtuInf->GetGroupNum(rtuaddr)) )
        {
            App_CallGrpData(groupno);
                if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 COMMAND Call Group No=%d Data===\n",groupno));
            }
            else
            {
                if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 COMMAND Call Group No=%d Error===\n",groupno));
            }
            ret = 1;    
            break;
    }
#endif
    default:
            App_SendOtherCmd(command);
            ret = 1;
            break;
    }

    return ret;
}

int Iec104::App_SendOtherCmd(COMMAND *command )
{
    if( pRouteInf->GetDebugFlag() )
        LOG_DEBUG(pRouteInf->GetChnId(), "IEC104::App_SendOtherCmd 104 unsurported command \n");

    return 1;
}

Iec104::Iec104()
{
}

Iec104::~Iec104()
{
}

/********************************************************************************
*        描述: 应用层发送设点命令48
*        参数:
*              command: 待发命令
*        返回: = 1,发送成功.
*              =-1,发送失败.
********************************************************************************/
int Iec104::App_SendSetPoint(COMMAND *command )
{
    //CtrlCmd *pSp = (CtrlCmd *)command->CmdData;
#if 1
    return App_SendSp(command);
#else
    std::shared_ptr<SPReqParam_S> data = std::static_pointer_cast<SPReqParam_S>(command->dataList.front());
    int spType = data->type;//command->ctrlCmdProto;//pSp->protoCmdType;
    if( spType ==  SP_TYPE_DECIMAL )
        return App_SendSp48(command);
    else if( spType ==  SP_TYPE_INT )
        return App_SendSp49(command);
    else if( spType ==  SP_TYPE_FLOAT )
        return App_SendSp50(command);
    else if( spType ==  SP_TYPE_DIGIT )
        return App_SendSp51(command);
    else
        return 1;
#endif
}

int Iec104::App_SendSp(COMMAND *command)
{
    uint8 buf[32],i,j;
    std::shared_ptr<SPReqParam_S> data = std::static_pointer_cast<SPReqParam_S>(command->dataList.front());
    int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr(data->rtuId);
    int ptAddress = data->no;
    int functionCode = CTRL_FUNC_EXECUTE;//ConverterUtil::ykCmdToCtrlFunc(data->type);
    //CtrlCmd *pSp = (CtrlCmd *)command->CmdData;
    LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp: channo=%s,rtuId=%s\n", pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str()));

    i = 2;
    int spType = data->type;//pSp->protoCmdType;
    if( spType ==  SP_TYPE_DECIMAL )
    {
        buf[0] = APPTYPE_SP48;
    }
    else if( spType ==  SP_TYPE_INT )
    {
        buf[0] = APPTYPE_SP49;
    }
    else if( spType ==  SP_TYPE_FLOAT )
    {
        buf[0] = APPTYPE_SP50;
    }
    else if( spType ==  SP_TYPE_DIGIT )
    {
        buf[0] = APPTYPE_SP51;
    }
    else
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp: Unsupport SP type '%d'! channo=%s,rtuId=%s\n", spType, pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str()));
        return -1;
    }

    buf[1] = 1;

    if(functionCode == CTRL_FUNC_CANCEL )
    {
        buf[i++] = APP_COT_DEACT%256;
        if( Config_Param.CotNum == 2 )//传输原因两字节
            buf[i++] = APP_COT_DEACT/256;
    }
    else
    {
        buf[i++] = APP_COT_ACT%256;
        if( Config_Param.CotNum == 2 )//传输原因两字节
            buf[i++] = APP_COT_ACT/256;
    }

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = (ptAddress+Config_Param.SpBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (ptAddress+Config_Param.SpBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (ptAddress+Config_Param.SpBaseAddr)/256%256;
        buf[i++] = (ptAddress+Config_Param.SpBaseAddr)/256/256;
    }

    if( spType ==  SP_TYPE_DECIMAL )
    {
        int spvalue = (int)data->decValue;//data->decValue*(2<<15);
        buf[i++] = spvalue;
        buf[i++] = spvalue/256;
        LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp:APPTYPE_SP48 channo=%s,rtuId=%s,decValue=%0.3f\n",pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str(), data->decValue));
    }
    else if( spType ==  SP_TYPE_INT )
    {
        int spInt = data->intValue;
        buf[i++] = spInt;
        buf[i++] = spInt/256;
        LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp:APPTYPE_SP49 channo=%s,rtuId=%s,intValue=%d\n",pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str(), data->intValue));
    }
    else if( spType ==  SP_TYPE_FLOAT )
    {
        float32 spfloat = data->floatValue;
        float_to_char(spfloat,(char *)(&buf[i]));
        i += 4;
        LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp:APPTYPE_SP50 channo=%s,rtuId=%s,floatValue=%0.3f\n",pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str(), data->floatValue));
    }
    else if( spType ==  SP_TYPE_DIGIT )
    {
        if(data->strValue.size() != 4)
        {
            LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp: channo=%s,rtuId=%s, wrong strValue size! It must be 4! \n", pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str()));
            return -1;
        }
        uint8 spdigit[4]={0};
        memcpy(spdigit, data->strValue.toStdString().c_str(), 4);
        buf[i++] = spdigit[0];
        buf[i++] = spdigit[1];
        buf[i++] = spdigit[2];
        buf[i++] = spdigit[3];
        LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp:APPTYPE_SP50 channo=%s,rtuId=%s,strValue=%s\n",pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str(), data->strValue.toStdString().c_str()));
        LOG_INFO(pRouteInf->GetChnId(),  StringUtil::toQString("Iec104::App_SendSp:APPTYPE_SP50 channo=%s,rtuId=%s,spdigit=%02X%02X%02X%02X\n", pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str(),spdigit[0],spdigit[1],spdigit[2],spdigit[3]));
    }
    else
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_SendSp: Unsupport SP type '%d'! channo=%s,rtuId=%s\n", spType, pRouteInf->GetChnId().toStdString().c_str(),data->rtuId.toStdString().c_str()));
        return -1;
    }

    j = i;

    if( functionCode == CTRL_FUNC_EXECUTE )
        buf[i++] = 0;
    else buf[i++] = 0x80;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {

        //缓存设置点请求数据
        /*App_Layer.spReqData.clear();
        App_Layer.spReqData.cchId = data->cchId;
        App_Layer.spReqData.rtuId = data->rtuId;
        App_Layer.spReqData.no = data->no;
        App_Layer.spReqData.type = data->type;
        App_Layer.spReqData.decValue = data->decValue;
        App_Layer.spReqData.intValue = data->intValue;
        App_Layer.spReqData.floatValue = data->floatValue;
        App_Layer.spReqData.strValue = data->strValue.toStdString();*/

        App_Layer.CtrlNo = ptAddress;
        //App_Layer.CtrlAttr = ctrlType;
        App_Layer.CtrlType = functionCode;
        App_Layer.State = IEC104_APP_STATE_WAITSPCONF;
        //App_Layer.taskId= command->taskId;
        AckFinished = 0;

        LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104:App_SendSp: success! ChnId=%1, rtuId=%2\n").arg(pRouteInf->GetChnId()).arg(data->rtuId));
        return 1;
    }
    return -1;
}


/********************************************************************************
*
*    描述: 应用层发送Yk命令45,和遥控命令仅仅类型标识和单点不一样.
*    参数: rtuAddr    目标RTU的地址
*          command  待发送的命令数据. 
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_SendYk45(COMMAND *command )
{
    if(command->dataList.size() != 1) //一次只能发送一个遥控命令，因为遥控需要确认；所以下发时只能一个一个下发
    {
        LOG_ERROR(pRouteInf->GetChnId(), "Iec104::App_SendYk45: Only can send one yk command at one time!");
        return -1;
    }
    std::shared_ptr<YKReqParam_S> data = std::static_pointer_cast<YKReqParam_S>(command->dataList.front());
    int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr(data->rtuId);
    int ptAddress = data->no;
    int ctrlType = (1 == data->value)?CTRL_TYPE_CLOSE:CTRL_TYPE_TRIP;//控合-1(关闭close), 控分-0(跳闸trip)
    int functionCode = ConverterUtil::ykCmdToCtrlFunc(data->type);
    if(INVALID_CTRLFUNC == functionCode)
    {
       LOG_ERROR(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: Invalid function code! functionCode=%1").arg(functionCode));
       return -1;
    }
    LOG_DEBUG(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: rtuId=%1, App_Layer.State=%2, functionCode=%3").arg(data->rtuId).arg(App_Layer.State).arg(functionCode));

    uint8 buf[12],i,j;
    //执行到此处一定有App_Layer.State == IEC104_APP_STATE_IDLE
    if( App_Layer.State != IEC104_APP_STATE_IDLE
       && functionCode != CTRL_FUNC_CANCEL)
       return -1;

    i = 2;
    buf[0] = APPTYPE_YK45;
    buf[1] = 1;
    if( functionCode == CTRL_FUNC_CANCEL )
    {
       buf[i++] = APP_COT_DEACT%256;
       if( Config_Param.CotNum == 2 )//传输原因两字节
           buf[i++] = APP_COT_DEACT/256;
    }
    else
    {
       buf[i++] = APP_COT_ACT%256;
       if( Config_Param.CotNum == 2 )//传输原因两字节
           buf[i++] = APP_COT_ACT/256;
    }

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
       buf[i++] = (rtuAddr)/256;

    buf[i++] = (ptAddress+Config_Param.YkBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
       buf[i++] = (ptAddress+Config_Param.YkBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
       buf[i++] = (ptAddress+Config_Param.YkBaseAddr)/256%256;
       buf[i++] = (ptAddress+Config_Param.YkBaseAddr)/256/256;
    }

    j = i;

    if( functionCode == CTRL_FUNC_EXECUTE)
       buf[i++] = 0;
    else buf[i++] = 0x80;

    if( ctrlType == CTRL_TYPE_CLOSE)//合
       buf[j] |= 0x01;
    else if( ctrlType == CTRL_TYPE_TRIP)//分
       buf[j] |= 0x00;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
       App_Layer.CtrlNo = ptAddress;
       App_Layer.CtrlAttr = ctrlType;
       App_Layer.CtrlType = functionCode;
       App_Layer.State = IEC104_APP_STATE_WAITYKCONF;
       //App_Layer.taskId= command->taskId;
       AckFinished = 0;
       LOG_DEBUG(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: success! ChnId=%1, rtuId=%2\n").arg(pRouteInf->GetChnId()).arg(data->rtuId));
       return 1;
    }
    return -1;
}

/********************************************************************************
*
*    描述: 应用层发送Yk命令.
*    参数: rtuAddr    目标RTU的地址
*          command  待发送的命令数据. 
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_SendYk(COMMAND *command )
{
    if(command->dataList.size() != 1) //一次只能发送一个遥控命令，因为遥控需要确认；所以下发时只能一个一个下发
    {
        LOG_ERROR(pRouteInf->GetChnId(), "Only can send one yk command at one time!");
        return -1;
    }
    std::shared_ptr<YKReqParam_S> data = std::static_pointer_cast<YKReqParam_S>(command->dataList.front());
    int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr(data->rtuId);
    int ptAddress = data->no;
    int ctrlType = (1 == data->value)?CTRL_TYPE_CLOSE:CTRL_TYPE_TRIP;//控合-1(关闭close), 控分-0(跳闸trip)
    int functionCode = ConverterUtil::ykCmdToCtrlFunc(data->type);
    if(INVALID_CTRLFUNC == functionCode)
    {
       LOG_ERROR(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: Invalid function code! functionCode=%1").arg(functionCode));
       return -1;
    }
    LOG_DEBUG(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: rtuId=%1, App_Layer.State=%2, functionCode=%3").arg(data->rtuId).arg(App_Layer.State).arg(functionCode));

    uint8 buf[12],i,j;
    //执行到此处一定有App_Layer.State == IEC104_APP_STATE_IDLE
    if( App_Layer.State != IEC104_APP_STATE_IDLE
       && functionCode != CTRL_FUNC_CANCEL)
       return -1;

    i = 2;
    buf[0] = APPTYPE_YK46;
    buf[1] = 1;
    if( functionCode == CTRL_FUNC_CANCEL )
    {
       buf[i++] = APP_COT_DEACT%256;
       if( Config_Param.CotNum == 2 )//传输原因两字节
           buf[i++] = APP_COT_DEACT/256;
    }
    else
    {
       buf[i++] = APP_COT_ACT%256;
       if( Config_Param.CotNum == 2 )//传输原因两字节
           buf[i++] = APP_COT_ACT/256;
    }

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
       buf[i++] = (rtuAddr)/256;

    buf[i++] = (ptAddress+Config_Param.YkBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
       buf[i++] = (ptAddress+Config_Param.YkBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
       buf[i++] = (ptAddress+Config_Param.YkBaseAddr)/256%256;
       buf[i++] = (ptAddress+Config_Param.YkBaseAddr)/256/256;
    }

    j = i;

    if( functionCode == CTRL_FUNC_EXECUTE)
       buf[i++] = 0;
    else buf[i++] = 0x80;

    if( ctrlType == CTRL_TYPE_CLOSE)//合
       buf[j] |= 0x02;
    else if( ctrlType == CTRL_TYPE_TRIP)//分
       buf[j] |= 0x01;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
       App_Layer.CtrlNo = ptAddress;
       App_Layer.CtrlAttr = ctrlType;
       App_Layer.CtrlType = functionCode;
       App_Layer.State = IEC104_APP_STATE_WAITYKCONF;
       //App_Layer.taskId= command->taskId;
       AckFinished = 0;
       LOG_DEBUG(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: success! ChnId=%1, rtuId=%2\n").arg(pRouteInf->GetChnId()).arg(data->rtuId));
       return 1;
    }
    return -1;
}

/********************************************************************************
生成一帧防误校验请求报文
*●请求防误校验(可视化系统->防误系统) 报文格式：
*   启动字符	0x68
    APDU长度	L	长度<=253
    发送序列号L	N（S）
    控制域
    发送序列号H	N（S）
    接收序列号L	N（R）
    接受序列号H	N（R）
    类型标识 ASDU	0xC8 (200)
    可变结构限定词	VSQ	遥控点个数
    传送原因L	06
    传送原因H	00
    公共地址L	ADDR	装置单元地址
    公共地址H	ADDR
    命令识别符1	0x2D	遥控操作
    命令识别符2	CMD1	0：整票(多个)；1：单控(1个)
    分帧识别符	SEQ	见分帧说明
    点号地址1	Addr1	3字节
    遥控状态	Data1	00：控分，01：控合
    点号地址2	Addr2
    遥控状态	Data2
    ……	……	从点号地址开始重复

********************************************************************************/
int Iec104::makeOneWFJSAsdu(uint8 *buf, uint8 buf0, uint8 pointNum,  uint8 seq_bit7_bit6, uint8 frameCnt, uint8 *pointBuf, int pointBufSize)
{
    int bufSize=0;
    //memset(buf, 0x0, bufSize);
    buf[0] = buf0;
    buf[1] = pointNum;//遥控点个数
    uint8 i =2;
    buf[i++] = 0x06;//传送原因L
    buf[i++] = 0x00;//传送原因H
    buf[i++] = (pRouteInf->GetRtuAddr())%256;//公共地址L
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (pRouteInf->GetRtuAddr())/256;//公共地址H
    else
        buf[i++] = 0x00;

    buf[i++] = 0x2D;//命令识别符1: 0x2D-遥控操作
    if(pointNum > 1)//多个
    {
        buf[i++] = 0x00;//命令识别符2: 0：整票(多个)；1：单控(1个)
    }
    else
    {
        buf[i++] = 0x01;//命令识别符2: 0：整票(多个)；1：单控(1个)
    }


    /*
     * 分帧识别符
     * 分帧说明(SEQ)
        SEQ定义如下：
        位	: BIT7|BIT6|BIT5|BIT4|BIT3|BIT2|BIT1|BIT0|
        说明	: FIR |FIN |           CNT               |
        FIR=1，FIN=0表示起始帧 即： seq_bit7_bit6=0x02
        FIR=0，FIN=0表示中间帧 即： seq_bit7_bit6=0x00
        FIR=0，FIN =1 表示结束帧 即： seq_bit7_bit6=0x01
        每发送一帧CNT自动加1。
        只有一帧时FIR=0，FIN =1，CNT=0表示结束。
     */
    uint8 seq = ((seq_bit7_bit6&0x03)<<6 | (frameCnt&0x3F)) & 0xFF;
    buf[i++] = seq;//分帧识别符 //0x40;

    //点信息:   点号地址1 + 遥控状态1 .... 点号地址n + 遥控状态n
    uint8 startIdx = i;
    for( int k=0; k< pointBufSize; k++ )
    {
        buf[startIdx+k] = pointBuf[k];
        i++;
    }
    bufSize = i;
    return bufSize;
}

int Iec104::App_SendWFJS(COMMAND *command )
{
    if(command->dataList.size() == 0)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "No WF command need to be sent!");
        return -1;
    }
    if( App_Layer.State != IEC104_APP_STATE_IDLE)
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("Invalid App_Layer.State:%1, It should be IEC104_APP_STATE_IDLE").arg(App_Layer.State));
        return -1;
    }
    LOG_INFO(pRouteInf->GetChnId(), "App_SendWFJS..start");
    int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr(data->rtuId);//data.rtuAddr;

    //注意：因为IEC104的ASDU长度限制为249， 所以每批发送的防误校验点需要限制一下个数，防止越界!!!!
    //每个点占用：4字节， ASDU头占用：9字节， 所以每一批的点个数最大为：60, 稳妥则取：50
    static const int MAX_POINT_EACH=50;
    static const int MAX_BUF_SIZE=256;
    uint8 pointNum=0;
    uint8 pointCnt=0;//点总计数
    uint8 buf[MAX_BUF_SIZE];
    uint8 buf0=0xC8;//0xC8-防误校验，0xC9-应急模式
    uint8 seq_bit7_bit6=0;//分帧识别符
    uint8 frameCnt=0;//帧数
    bool isFirstFrame=true;

    std::shared_ptr<YKReqParam_S> data = std::static_pointer_cast<YKReqParam_S>(command->dataList.front());

    //int ctrlType = (1 == data->value)?CTRL_TYPE_CLOSE:CTRL_TYPE_TRIP;//1-合， 0-分
    int functionCode = ConverterUtil::wfCmdToCtrlFunc(data->type);
    if(INVALID_CTRLFUNC == functionCode)
    {
       LOG_ERROR(pRouteInf->GetChnId(), QString("Invalid function code:%1 (Unsupport type:%2, no:%3) !").arg(functionCode).arg(data->type).arg(data->no));
       return -1;
    }
    //liujie add 添加五防操作0xC8五防解锁，0xC9应急模式
    if(functionCode == CTRL_FUNC_YJMS )
    {
        buf0 = 0xC9;
    }
    else//CTRL_FUNC_WF
    {
        buf0 = 0xC8;
    }
    LOG_INFO(pRouteInf->GetChnId(), QString("rtuAddr:%1, App_Layer.State:%2, functionCode:%3").arg(rtuAddr).arg(App_Layer.State).arg(functionCode));


    uint8 pointBuf[MAX_BUF_SIZE]={0}; // 4个字节*最大50个点
    int i=0;

    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter = command->dataList.begin();
    for(; iter != command->dataList.end(); iter++)
    {
        std::shared_ptr<FWReqParam_S> data = std::static_pointer_cast<FWReqParam_S>(*iter);

        pointCnt++;
        int ptAddress = data->no;//data.pointAddr;
        //liujie add 20230525 添加五防操作及应急模式
        int tmpCtrlFunc = ConverterUtil::wfCmdToCtrlFunc(data->type);
        if(INVALID_CTRLFUNC == tmpCtrlFunc)
        {
           LOG_WARN(pRouteInf->GetChnId(), QString("Invalid function code:%1 (Unsupport type:%2, no:%3) SKIP!!!!!").arg(functionCode).arg(data->type).arg(data->no));
           //return -1;
           continue;
        }
        if(functionCode != tmpCtrlFunc)
        {
            //同一个五防命令下，类型必须是同一类，要么是：1-防误校验， 要么是：2-应急模式。不能混在一起。因为它们的类型标识不同，一个是0xC8,一个是：0xC9
            LOG_WARN(pRouteInf->GetChnId(), QString("All type must be same in one 'FW' command! SKIP!!!! tmpCtrlFunc:%1, functionCode:%2 , type:%3, no:%4").arg(tmpCtrlFunc).arg(functionCode).arg(data->type).arg(data->no));
            //return -1;
            continue;
        }
        pointBuf[i++] = (ptAddress+Config_Param.FwBaseAddr)%256%256;//10
        if( Config_Param.InfAddrNum == 2 )//信息体地址两字节，H
        {
            pointBuf[i++] = (ptAddress+Config_Param.FwBaseAddr)/256%256;//17
            pointBuf[i++] = 0x00;
        }
        else if( Config_Param.InfAddrNum == 3 )//信息体地址三字节 18
        {
            pointBuf[i++] = (ptAddress+Config_Param.FwBaseAddr)/256%256;
            pointBuf[i++] = (ptAddress+Config_Param.FwBaseAddr)/256/256;
        }
        else
        {
            pointBuf[i++] = 0x00;//11
            pointBuf[i++] = 0x00;//12
        }

        if( 0 == data->value)//控分
        {
            pointBuf[i++] = 0x00;//18
        }
        else if(1 == data->value)//控合
        {
            pointBuf[i++] = 0x01;//18
        }
        else
        {
            LOG_ERROR(pRouteInf->GetChnId(), QString("Invalid value :%1 (type:%2, no:%3) !").arg(data->value).arg(data->type).arg(data->no));
            //return -1;
            continue;
        }
        pointNum++;
        if(pointNum%MAX_POINT_EACH == 0)
        {
            if(pointCnt== command->dataList.size())//达到最后一个点
            {
                break;//退到外面：发送结束帧
            }
            else if(isFirstFrame)
            {
                seq_bit7_bit6=0x02;//起始帧 FIR=1，FIN=0表示起始帧 即： seq_bit7_bit6=0x02
                isFirstFrame=false;
            }else
            {
                seq_bit7_bit6=0x00;//中间帧 FIR=0，FIN=0表示中间帧 即： seq_bit7_bit6=0x00
            }
            memset(buf, 0x0, MAX_BUF_SIZE);
            int bufSize = makeOneWFJSAsdu(buf, buf0, pointNum, seq_bit7_bit6, frameCnt, pointBuf, i); //bufSize <=253
            if(bufSize >0 && bufSize<=253)
            {
                App_SendAppIFormat(buf, bufSize);
            }
            else
            {
                LOG_ERROR(pRouteInf->GetChnId(), QString("Invalid bufSize :%1, skip!!!! It must >0 and <=253! (type:%2, no:%3) !").arg(bufSize));
            }
            frameCnt++;
            i=0;
            pointNum=0;
            memset(pointBuf, 0x0, MAX_BUF_SIZE);
            memset(buf, 0x0, MAX_BUF_SIZE);
        }
    }
    if(pointNum >0)
    {
        seq_bit7_bit6=0x01;//结束帧 FIR=0，FIN =1 表示结束帧 即： seq_bit7_bit6=0x01
        memset(buf, 0x0, MAX_BUF_SIZE);
        int bufSize = makeOneWFJSAsdu(buf, buf0, pointNum, seq_bit7_bit6, frameCnt, pointBuf, i);
        if(bufSize >0 && bufSize<=253)
        {
            if( App_SendAppIFormat(buf, bufSize) == 1 )
            {
                //App_Layer.CtrlNo = ptAddress;
                //App_Layer.CtrlAttr = ctrlType;
                App_Layer.CtrlType = functionCode;
                App_Layer.State = IEC104_APP_STATE_WAITFWCONF;
                //App_Layer.taskId= command->taskId;
                AckFinished = 0;
                LOG_INFO(pRouteInf->GetChnId(), QString("IEC104: send WF cmd:ChnId=%1,rtuId=%2\n").arg(pRouteInf->GetChnId()).arg(data->rtuId));
                return 1;
            }
        }
        else
        {
            LOG_ERROR(pRouteInf->GetChnId(), QString("Invalid bufSize :%1, skip!!!! It must >0 and <=253! (type:%2, no:%3) !").arg(bufSize));
        }
    }
    return 1;
}

int Iec104::App_SetPasswordReg(COMMAND *command )
{
    LOG_INFO(pRouteInf->GetChnId(),  "App_SetPasswordReg执行中：将json中内容转成字节流");

    std::shared_ptr<SetPasswordRegParam_S> data = std::static_pointer_cast<SetPasswordRegParam_S>(command->dataList.front());
    LOG_INFO(pRouteInf->GetChnId(), "密码"+QString::number(data->passwd));//后续不打印ldq
    LOG_INFO(pRouteInf->GetChnId(),  "锁号"+QString::number(data->lockno));

    int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr(data->rtuId);
    //LOG_DEBUG(pRouteInf->GetChnId(), QString("Iec104::App_SendYk45: rtuId=%1, App_Layer.State=%2, functionCode=%3").arg(data->rtuId).arg(App_Layer.State).arg(functionCode));

    uint8 buf[19],i,j;
    i = 2;
    //同步头
    buf[0] = 0x68;

    //长度
    buf[1] = 0x11;

    //控制域4位
    buf[2] = 0x6A;
    buf[3] = 0x03;
    buf[4] = 0x1A;
    buf[5] = 0x09;

    //类型标识码
    buf[6] = 0x33;

    //可变结构限定词
    buf[7] = 0x01;

    //6:表示向设备设置密码信息过程
    buf[8] = 0x06;

    //默认为0
    buf[9] = 0x00;

    //1：表示通讯机地址。（低位前，高位后）
    buf[10] = 0x01;
    buf[11] = 0x00;

    //代表锁的编号（81 0B 00 作为锁具基地址 2945 开始）（低位前，高位后
    buf[12] = data->lockno & 0xFF;// 获取最低字节;
    buf[13] = (data->lockno >> 8) & 0xFF;  // 获取中间字节
    buf[14] = (data->lockno >> 16) & 0xFF; // 获取最高字节

    //四个字节表示密码信息。（低位前，高位后）密码最大值应小于268435455
    buf[15] = data->passwd & 0xFF;// 获取最低字节;
    buf[16] = (data->passwd >> 8) & 0xFF;  // 获取2字节
    buf[17] = (data->passwd >> 16) & 0xFF; // 获取3字节
    buf[18] = (data->passwd >> 24) & 0xFF; // 获取最高字节
    //设置字节最高4位，代表一些属性
    buf[18] = buf[18]+0x70;

    AddNeedSendFrame( buf, 19 );
    LOG_INFO(pRouteInf->GetChnId(),  "已发送密码信息命令设置");


    return 1;
}

int Iec104::App_SendYt47(COMMAND *command )
{
    LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_SendYt47\n");
    if(command->dataList.size() != 1) //一次只能发送一个设点命令，因为设点需要确认；所以下发时只能一个一个下发
    {
        LOG_ERROR(pRouteInf->GetChnId(), "Only can send one YT command at one time!");
        return -1;
    }
    std::shared_ptr<YKReqParam_S> data = std::static_pointer_cast<YKReqParam_S>(command->dataList.front());

    uint8 buf[12],i,j;
    //CtrlCmd *pYt = (CtrlCmd *)command->CmdData;
    int rtuAddr = pRouteInf->GetRtuAddr();//pRtuInf->GetRtuAddr(data->rtuId);//data.rtuAddr;
    QString rtuId = data->rtuId;
    int ptAddress = data->no;//data.pointAddr;
    int ctrlType = (1 == data->value)?CTRL_TYPE_RAISE:CTRL_TYPE_LOWER;//上调-1, 下调-0
    int functionCode = ConverterUtil::ykCmdToCtrlFunc(data->type);

    //执行到此处一定有App_Layer.State == IEC104_APP_STATE_IDLE
    if( App_Layer.State != IEC104_APP_STATE_IDLE
        && functionCode != CTRL_FUNC_CANCEL )
        return -1;

    i = 2;

    buf[0] = APPTYPE_YT47;
    buf[1] = 1;

    if(functionCode == CTRL_FUNC_CANCEL )
    {
        buf[i++] = APP_COT_DEACT%256;
        if( Config_Param.CotNum == 2 )//传输原因两字节
            buf[i++] = APP_COT_DEACT/256;
    }
    else
    {
        buf[i++] = APP_COT_ACT%256;
        if( Config_Param.CotNum == 2 )//传输原因两字节
            buf[i++] = APP_COT_ACT/256;
    }

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = (ptAddress+Config_Param.YtBaseAddr)%256%256;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = (ptAddress+Config_Param.YtBaseAddr)/256%256;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = (ptAddress+Config_Param.YtBaseAddr)/256%256;
        buf[i++] = (ptAddress+Config_Param.YtBaseAddr)/256/256;
    }

    j = i;

    if(functionCode == CTRL_FUNC_EXECUTE || functionCode == CTRL_FUNC_AUTOEXECUTE  || functionCode == CTRL_FUNC_LINEEXECUTE )
        buf[i++] = 0;
    else buf[i++] = 0x80;

    if( ctrlType == CTRL_TYPE_RAISE )
        buf[j] |= 0x02;
    else if( ctrlType == CTRL_TYPE_LOWER )
        buf[j] |= 0x01;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
        App_Layer.CtrlNo = ptAddress;
        App_Layer.CtrlAttr = ctrlType;
        App_Layer.CtrlType = functionCode;
        App_Layer.State = IEC104_APP_STATE_WAITYTCONF;
        //App_Layer.taskId= command->taskId;
        AckFinished = 0;

        LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104:App_SendYt47 send ytcmd:ChnId=%s,rtuId=%s\n",pRouteInf->GetChnId().toStdString().c_str(),rtuId.toStdString().c_str()));
        return 1;
    }
    else return -1;
}

void Iec104::App_RxYt47Conf( uint8 *asdu, int size )
{
    LOG_INFO(pRouteInf->GetChnId(), StringUtil::toQString("Iec104::App_RxYt47Conf, size:%d\n", size));
    uint32	infnum = asdu[1]&0x7f;
    if(infnum!=1)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "Iec104:App_RxYt47Conf received inf number!=1 ---\n" );
        return;
    }
    uint16	rtuaddr;
    int j,k;
    j = 3;
    if( Config_Param.CotNum == 2 )
        j++;

    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;
        j += 2;
    }
    else
    {
        rtuaddr = asdu[j];
        j++;
    }

    if( rtuaddr != pRouteInf->GetRtuAddr())
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104:App_RxYt47Conf： Wrong rtuaddr:%d!\n", rtuaddr) );
        return;
    }

    int ytno = asdu[j+1]*256+asdu[j];
    if( Config_Param.InfAddrNum == 3 )
        ytno += asdu[j+2]*256*256;

    if( ytno < Config_Param.YtBaseAddr )
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104:App_RxYt47Conf： Wrong ytno:%d! ytno must < YtBaseAddr: %d\n", ytno, Config_Param.YtBaseAddr) );
        return;
    }
    ytno -= Config_Param.YtBaseAddr;

    //遥调反校遥调号、遥调命令限定词
    if( App_Layer.CtrlNo != ytno )
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104:App_RxYt47Conf： App_Layer.CtrlNo:%d != ytno:%d\n", App_Layer.CtrlNo, ytno) );
        return;
    }

    if( Config_Param.InfAddrNum == 3 )
        k=j+3;
    else
        k=j+2;

    if( App_Layer.CtrlAttr == CTRL_TYPE_RAISE && (asdu[k]&0x3) != 2 ||
            App_Layer.CtrlAttr == CTRL_TYPE_LOWER && (asdu[k]&0x3) != 1 )
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104:App_RxYt47Conf： Wrong asdu[k]&0x3 ! App_Layer.CtrlAttr:%d,  (asdu[k]&0x3):%d, k:%d, asdu[k]:%02X\n", App_Layer.CtrlAttr,(asdu[k]&0x3), k, asdu[k]));
        return;
    }

    if( App_Layer.CtrlType == CTRL_FUNC_SELECT && (asdu[k]&0x80)!= 0x80 ||
            App_Layer.CtrlType == CTRL_FUNC_EXECUTE && (asdu[k]&0x80)!= 0x00 ||
            App_Layer.CtrlType == CTRL_FUNC_AUTOEXECUTE && (asdu[k]&0x80)!= 0x00 ||
            App_Layer.CtrlType == CTRL_FUNC_LINEEXECUTE && (asdu[k]&0x80)!= 0x00 )
    {
        LOG_ERROR(pRouteInf->GetChnId(), StringUtil::toQString("Iec104:App_RxYt47Conf： Wrong asdu[k]&0x80) ! App_Layer.CtrlType:%d,  (asdu[k]&0x3):%d, k:%d, asdu[k]:%02X\n", App_Layer.CtrlType,(asdu[k]&0x80), k, asdu[k]));
        return;
    }

    App_Layer.CtrlReply = -1;
    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_WAITYTCONF:
        if( (asdu[2]&0x3f) == APP_COT_DEACT_CON )
        {
            if(App_Layer.CtrlType == CTRL_FUNC_CANCEL )
            {
                LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYt47Conf Received YT cancel confirm\n");//遥调取消确认

                //遥调撤销成功
                App_Layer.CtrlReply = COMMAND_YKCANCEL_SUCCESS;
            }
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        else if( (asdu[2]&0x3f) == APP_COT_ACT_CON )
        {
            if( (asdu[2]&0x40) == 0x40 )	//neg conf
            {
                LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYt47Conf Received YT select(exec) neg conf\n");//遥调选择（执行）否认

                //遥调预置失败
                if(App_Layer.CtrlType == CTRL_FUNC_SELECT)
                {
                    App_Layer.CtrlReply = COMMAND_SELECT_FAIL;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_EXECUTE || App_Layer.CtrlType == CTRL_FUNC_AUTOEXECUTE  || App_Layer.CtrlType == CTRL_FUNC_LINEEXECUTE )
                {
                    App_Layer.CtrlReply = COMMAND_YKEXEC_FAIL;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_CANCEL)
                {
                    App_Layer.CtrlReply = COMMAND_YKCANCEL_FAIL;
                }
                App_Layer.State = IEC104_APP_STATE_IDLE;
                //AckFinished = 1;
            }
            else	//yes conf
            {
                if( App_Layer.CtrlType == CTRL_FUNC_SELECT )
                {
                    LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYt47Conf Received YT select confirm \n");//遥调选择确认

                    //遥调预置成功
                    App_Layer.CtrlReply = COMMAND_SELECT_SUCCESS;
                    App_Layer.State = IEC104_APP_STATE_IDLE;
                    //AckFinished = 1;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_EXECUTE || App_Layer.CtrlType == CTRL_FUNC_AUTOEXECUTE  || App_Layer.CtrlType == CTRL_FUNC_LINEEXECUTE )
                {
                    LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYt47Conf Received YT exec confirm \n");//遥调执行确认

                    //遥调执行成功
                    //AckFinished = 1;
                    App_Layer.CtrlReply=COMMAND_YKEXEC_SUCCESS;
                    if(Config_Param.isYtWaitExecFinish)//遥调执行是否需要等待执行结束（遥调执行确认后，还会有一个遥调执行结束报文）
                    {
                        App_Layer.State = IEC104_APP_STATE_WAITYTFINISH;
                    }
                    else
                    {
                        App_Layer.State = IEC104_APP_STATE_IDLE;
                    }
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_CANCEL)
                {
                    App_Layer.CtrlReply = COMMAND_YKCANCEL_SUCCESS;
                    App_Layer.State = IEC104_APP_STATE_IDLE;
                }
            }
            AckFinished = 1;
        }
        else if( (asdu[2]&0x3f) == APP_COT_UNKNOWN_INFOADDR )//未知的信息对象地址
        {
            //未知的遥调号
            App_Layer.CtrlReply=COMMAND_UNKNOWN_YKNO;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        break;
    case IEC104_APP_STATE_WAITYTFINISH:
        if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )
        {
            LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYt47Conf Received YT exec end\n");//遥调执行结束

            //遥调执行成功
            App_Layer.CtrlReply=COMMAND_YKEXEC_SUCCESS;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        return; //这里要退出，因为之前遥控执行确认时，已经返回将遥控执行响应消息到上层应用，这里(收到遥控执行结束报文)就不再返回响应消息了。
        break;
    default:
        {
            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYt47Conf, Wrong App_Layer.State: %1 !\n").arg(App_Layer.State));//遥控执行结束
            return;
            break;
        }
    }

    QString codeQStr = QString::fromStdString(CMD_TYPE_STR_YT);
    std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, ytno);
    if(nullptr == taskInfoPtr)
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("YT: Not found task! codeQStr:%1, no:%2").arg(codeQStr).arg(ytno));
        return;
    }
    if(App_Layer.CtrlReply >=0 )
    {
        QString rtuId = pRouteInf->GetRtuId();
        pCommandMem->SendYkYtReply(CMD_TYPE_YT, rtuId, ytno, App_Layer.CtrlAttr, App_Layer.CtrlReply, taskInfoPtr);
    }
    pTaskList->removeATask(codeQStr, ytno);
    LOG_INFO(pRouteInf->GetChnId(), QString("<====YT: remove task, codeQStr:%1, no:%2").arg(codeQStr).arg(ytno));

}

/********************************************************************************
*
*    描述: 应用层召唤全电度.
*    参数: 无 
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_CallAllKwh( int rtuAddr)
{
#if 1//CFQ
    uint8 buf[12],i;
    
    i = 2;

    buf[0] = APPTYPE_CALLKWH;
    buf[1] = 0x01;

    buf[i++] = APP_COT_ACT%256;
    if( Config_Param.CotNum == 2 )//传输原因两字节
        buf[i++] = APP_COT_ACT/256;
 
    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    buf[i++] = 0x45;

    if( App_SendAppIFormat( buf, i ) == 1 )
    {
        App_Layer.State = IEC104_APP_STATE_WAITALLKWHCONF;
        AckFinished = 0;
        
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 CallAllKWh to substation Appstate: %d\n", App_Layer.State));

        return 1;
    }
    else return -1;
#else
    return -1;
#endif
}

/********************************************************************************
*
*    描述: 应用层召唤全数据.
*    参数: 无 
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_CallAllData( int rtuAddr )
{
    std::list<int>::const_iterator iter =  m_rtuAddrList.begin();
    bool found = false;
    for(; iter != m_rtuAddrList.end(); iter++)
    {
        if(rtuAddr == *iter)
        {
            found = true;
            break;
        }
    }
    if(!found)
    {
        m_rtuAddrList.push_back(rtuAddr);
    }
#if 1//CFQ
    uint8 buf[12],i;
    
    i = 2;

    buf[0] = APPTYPE_CALLDATA;
    buf[1] = 0x01;

    buf[i++] = APP_COT_ACT%256;

    if( Config_Param.CotNum == 2 )//传输原因两字节
        buf[i++] = APP_COT_ACT/256;

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
                buf[i++] = 0;
                buf[i++] = 0;
    }

    buf[i++] = 20;            //QOI

    if( App_SendAppIFormat( buf, i ) == 1)
    {
        App_Layer.State = IEC104_APP_STATE_WAITALLDATACONF;    //主站总召唤无需确认和结束帧（暂定）
        AckFinished = 0;

        return 1;
    }
    else return -1;
#else
    return -1;
#endif
}

/********************************************************************************
*
*    描述: 应用层同步时钟.
*    参数: 无 
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_SyncTime(int rtuAddr)
{
    uint8    buf[24],i;
    i = 2;

    buf[0] = APPTYPE_TIMESYNC;
    buf[1] = 0x01;

    buf[i++] = APP_COT_ACT%256;
    if( Config_Param.CotNum == 2 )//传输原因两字节
        buf[i++] = APP_COT_ACT/256;

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    App_SyncTimeCoding( &buf[i] );
    i += 7;

    if( App_SendAppIFormat( buf, i ) == 1)
    {
        App_Layer.State = IEC104_APP_STATE_WAITTIMECONF;
        AckFinished = 0;
        
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 Sync time to substation Appstate: %d\n", App_Layer.State));

        return 1;
    }
    else
    {
        return -1;
    }
}

/********************************************************************************
*
*    描述: 同步时钟编码.
*    参数: buf  时钟数据 
*    返回: 无
*
********************************************************************************/
void Iec104::App_SyncTimeCoding( uint8 *buf )
{
    time_t    ti;
    struct    tm    *ttm;
    struct    timeval ttv;
    uint32    ms;

    time( &ti );
    /*gettimeofday( &ttv, NULL );*/
    GetTimeOfDay( &ttv );

    /*chenfuqing modified 20230424: Time_cor会导致时间(月，日等)变来变去
     * 问题日志：
     * [debug] [App_SyncTimeCoding:1355] min=33,hour=11,day=9,mon=5,year=123
     * 过一会就变成了：
     * [debug] [App_SyncTimeCoding:1355] min=4,hour=5,day=27,mon=4,year=123
     *
     * 参考C2代码，发现只有一个104代码(iec104.cxx)保留了Time_cor,其他(如：iec104ut.cxx, fzjk104wgj.cxx, iecut101.cxx,iec101_97.cxx)都注释了，现在把它注释掉
    */
    //ms = ttv.tv_usec/1000+Time_cor;
    ms = ttv.tv_usec/1000;
    ti += ms/1000;
    ms %= 1000;

    ttm = localtime( &ti );

    ms += ttm->tm_sec*1000;
    buf[0] = LOBYTE( ms );
    buf[1] = HIBYTE( ms );
    buf[2] = ttm->tm_min;
    buf[3] = ttm->tm_hour;
    if(ttm->tm_wday==0)
        buf[4] = ( 7<<5 ) | ttm->tm_mday;
    else
        buf[4] = ( (uint8)ttm->tm_wday<<5 ) | ttm->tm_mday;
    buf[5] = ttm->tm_mon+1;
    buf[6] = (ttm->tm_year+1900)%100;
    if( pRouteInf->GetDebugFlag() )
        LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("min=%d,hour=%d,day=%d,mon=%d,year=%d\n",ttm->tm_min,ttm->tm_hour,ttm->tm_mday,ttm->tm_mon,ttm->tm_year));
}

//重置总召等最后接收时间
void Iec104::App_ResetLastTime( void )
{
    TIME curtime;
    GetCurSec( &curtime );
    App_Layer.LastCallAllTime = curtime.Sec;
    App_Layer.LastCallAllKwhTime = curtime.Sec;
    App_Layer.LastSyncTime = curtime.Sec;
    LOG_INFO(pRouteInf->GetChnId(), QString("App_ResetLastTime......"));
}

/********************************************************************************
*
*    描述: 检测是否须召唤全数据、电度，同步时钟，并置相应标志.
*    参数: 无
*    返回: 无
*
********************************************************************************/
void Iec104::App_CheckTime( void )
{
    TIME curtime;
    GetCurSec( &curtime );

    //此函数主要为置标志Flag,所以链路连接未成功时就直接返回,不去置标志位
    if(    App_Layer.Inited_Flag == 0)
        return;    

    if(pRouteInf->GetAllScan()>0 && curtime.Sec-App_Layer.LastCallAllTime >= pRouteInf->GetAllScan())
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 call all time: %d s===\n",pRouteInf->GetAllScan()));
#if CFQ
        /*如果该RTU分组数>0,定时召唤一组数据,不进行总召唤*/
        if(pRtuInf->GetGroupNum(rtuaddr) > 0)
        {
            /*是否考虑RTU的组号有最大16组的限制*/
            App_Layer.CallGroupDataFlag = 1;
            App_Layer.CallGroupNo %= pRtuInf->GetGroupNum(rtuaddr);
            App_Layer.CallGroupNo++;
        }
        else
            App_Layer.CallAllDataFlag = 1;
#else
        App_Layer.CallAllDataFlag = 1;
#endif
            
        App_Layer.LastCallAllTime = curtime.Sec;
    }

    //全电度(脉冲)召唤
    if(pRouteInf->GetScanPulseTime()>0 && curtime.Sec-App_Layer.LastCallAllKwhTime >= pRouteInf->GetScanPulseTime())
    {
        App_Layer.CallAllKwhFlag = 1;
        App_Layer.LastCallAllKwhTime = curtime.Sec;
    }

    //校时周期
    /*chenfuqing 20230914 modified 轨道需求：校时周期（秒）为 0时 ，不要校时命令*/
    //if( (pRtuInf->GetSyncTimeInt(0) < MAX_SYNTIME_PIERIOD) && (curtime.Sec-App_Layer.LastSyncTime >=pRtuInf->GetSyncTimeInt( 0 )) )
    if( (pRtuInf->GetSyncTimeInt(0)>0) && (curtime.Sec-App_Layer.LastSyncTime >=pRtuInf->GetSyncTimeInt( 0 )) )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 sync time: %d s===\n",pRtuInf->GetSyncTimeInt( 0 )));

        App_Layer.TimeSyncFlag = 1;
        App_Layer.LastSyncTime = curtime.Sec;
    }
}

/********************************************************************************
*
*    描述: 召唤某一组电度
*    参数: group  组号，取值要求主站和子站应一致（分成4组，每组32个量）
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_CallGrpKwh(int rtuAddr, int group )
{
#if 1//CFQ
    uint8 buf[12],i;
    
    i = 2;

    buf[0] = APPTYPE_CALLKWH;
    buf[1] = 0x01;

    buf[i++] = APP_COT_REQ%256;
    if( Config_Param.CotNum == 2 )//传输原因两字节
        buf[i++] = APP_COT_REQ/256;


    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
                buf[i++] = 0;
                buf[i++] = 0;
    }

    buf[i++] = 0x40 | (group&0x3f);

    if( App_SendAppIFormat( buf, i ) == 1)
    {
        App_Layer.State = IEC104_APP_STATE_WAITGRPKWH;
        AckFinished = 0;
        
        return 1;
    }
    else return -1;
#else
    return -1;
#endif
}

/********************************************************************************
*
*    描述: 召唤某一组数据
*    参数: group  组号，取值1-16
*    返回: -1  发送失败
*           1  发送成功.
*
********************************************************************************/
int Iec104::App_CallGrpData(int rtuAddr, int group )
{
    #if 1//CFQ
    uint8 buf[12],i;
    
    i = 2;

    buf[0] = APPTYPE_CALLDATA;
    buf[1] = 0x01;

    buf[i++] = APP_COT_ACT%256;
    if( Config_Param.CotNum == 2 )//传输原因两字节
        buf[i++] = APP_COT_REQ/256;

    buf[i++] = (rtuAddr)%256;
    if( Config_Param.ComAddrNum == 2 )//公共地址两字节
        buf[i++] = (rtuAddr)/256;

    buf[i++] = 0;
    if( Config_Param.InfAddrNum == 2 )//信息体地址两字节
        buf[i++] = 0;
    if( Config_Param.InfAddrNum == 3 )//信息体地址三字节
    {
        buf[i++] = 0;
        buf[i++] = 0;
    }

    buf[i++] = 20+group;

    if( App_SendAppIFormat( buf, i ) == 1)
    {
        App_Layer.State = IEC104_APP_STATE_WAITGRPDATA;
        AckFinished = 0;

        return 1;
    }
    else return -1;
#else
    return -1;
#endif
}






/********************************************************************************
*
*    描述: 应用层将ASDU+APCI转换成APDU(I format).
*    参数: 
*            asdu  指向应用服务数据单元(ASDU) 的指针
*               size  应用服务数据单元（APDU)中的数据字节数.
*                    
*    返回:  -1 发送失败.
*        1 发送成功.
*
********************************************************************************/

int Iec104::App_SendAppIFormat( uint8 *asdu, int size )
{
  //LOG_DEBUG(pRouteInf->GetChnId(), "");
    int i;
    uint8 *txData;
#if CFQ
    if(  pTxBuf->GetWritableSize() < (size+6) )
            return -1;
#endif
    
    txData = &App_Layer.txData[0];
    App_Layer.txSize = size+6;
    
    txData[0] = 0x68;
    txData[1] = size+4;
    txData[2] = App_Layer.Snd_seqnum%128<<1;
    txData[3] = App_Layer.Snd_seqnum/128;
    txData[4] = App_Layer.Rec_seqnum%128<<1;
    txData[5] = App_Layer.Rec_seqnum/128;

    for( i=0; i<size; i++ )
    {
        txData[6+i] = asdu[i];
    }
    
    //每发送一I帧数据Snd_seqnum加1，清发送确认标志SendACKFlag。
    App_Layer.Snd_seqnum ++;        
    App_Layer.Snd_seqnum %= 0x8000;        //Snd_seqnum 为15位二进制数。

    //LOG_DEBUG(pRouteInf->GetChnId(), "");
    AddNeedSendFrame( txData, size+6 );
//    pRouteInf->RecAFrame( PROTO_TXFRAME );
//    pRouteInf->SetDevSendEnable(1);

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
    pTxRxOutBuf->WriteDirect(&App_Layer.txData[0],App_Layer.txSize);
    pTxRxOutBuf->WriteTransEnd( );
#endif

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
int Iec104::App_SendAppUFormat(int type)
{
    TIME curtime;
    uint8 *txData;
    
    GetCurSec( &curtime );
    if( type == APP_UFORMAT_TESTACT )
    {    if( curtime.Sec-App_Layer.LastTxTime < Config_Param.OverTime3 ||
            curtime.Sec-App_Layer.LastRxTime < Config_Param.OverTime3)//T3为20秒,
        {
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
    txData[2] = type | 0x03;
    txData[3] = 0;
    txData[4] = 0;
    txData[5] = 0;

    AddNeedSendFrame( txData, 6 );
//    pRouteInf->RecAFrame( PROTO_TXFRAME );
//    pRouteInf->SetDevSendEnable(1);

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
    pTxRxOutBuf->WriteDirect(&App_Layer.txData[0],App_Layer.txSize);
    pTxRxOutBuf->WriteTransEnd( );
#endif

    if( App_Layer.State == IEC104_APP_STATE_IDLE )         //空闲状态下才能刷新LastTxTime！
        App_Layer.LastTxTime = curtime.Sec;

    if( App_Layer.State == APP_UFORMAT_TESTACT || App_Layer.State == APP_UFORMAT_STARACT 
                            || App_Layer.State == APP_UFORMAT_STOPACT )
        AckFinished = 0;

    if( (App_Layer.State == IEC104_APP_STATE_UNRESET) && (type == APP_UFORMAT_STARACT) )
        App_Layer.LastTxTime = curtime.Sec;

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
int Iec104::App_SendAppSFormat( bool atonceflag )
{
    TIME curtime;
    uint8 txData[6];                    //此处并未改变App_Layer.txData[],不影响重发数据

    GetCurSec( &curtime );
    if(atonceflag == false)
    {
        if( (App_Layer.RxWindowNum < Config_Param.MaxIframeNum) && (curtime.Sec-App_Layer.LastTxTime <= Config_Param.OverTime2) )//t2推荐10秒,应该比t1(15秒)短,这样大于10秒小于15秒的情况下可以发S帧
            return -1;
    }        
#if CFQ
    if( pTxBuf->GetWritableSize() < 6 )
        return -1;
#endif

    txData[0] = 0x68;
    txData[1] = 4;
    txData[2] = 1;
    txData[3] = 0;
    txData[4] = App_Layer.Rec_seqnum%128<<1;
    txData[5] = App_Layer.Rec_seqnum/128;

    AddNeedSendFrame( txData, 6 );
//    pRouteInf->RecAFrame( PROTO_TXFRAME );
//    pRouteInf->SetDevSendEnable(1);

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
    pTxRxOutBuf->WriteDirect(&txData[0],6);
    pTxRxOutBuf->WriteTransEnd( );
#endif

    if( App_Layer.State == IEC104_APP_STATE_IDLE )         //空闲状态下才能刷新LastTxTime！
//yay 发S帧必须刷新LastTxTime,否则请求数据时(如总召唤)会导致超时
//(t1为15秒,t2为10秒,已经更新为:当收到对方最后一帧时,如果没有后继帧导致中断,先t2超时,发S帧,再t1超时...)
//    if( App_Layer.State != IEC104_APP_STATE_WAITYKCONF )         //yay 为了等待YKCONF超时foshan
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

int Iec104::RxProc( void )
{
  //该规约不支持整站转发处理模式时,执行该函数清理掉可能的Yx(Soe)_Var_Out
    //ClearYxSoeVarOut();

    App_CheckTime();        

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

        if( App_Layer.RxWindowNum >= Config_Param.MaxIframeNum )
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
void Iec104::App_SearchStartChar( void )
{
    while( pRxBuf->getReadableSize() >= 2 )
    {
        pRxBuf->read( &App_Layer.rxData[0], MOVE );
        //if( pRouteInf->GetDebugFlag() )
        //    LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Search StartChar...........................\n");

        if( App_Layer.rxData[0]  == 0x68 )
        {
            pRxBuf->read( &App_Layer.rxData[1], NOTMOVE );
            rxStep = 1;
            break;
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
void Iec104::App_SearchFrameHead( void )
{
    LOG_INFO(pRouteInf->GetChnId(), "ldqlalala1");
    if( pRxBuf->getReadableSize() >= App_Layer.rxData[1]+1 )
    {
        pRxBuf->read( &App_Layer.rxData[1], App_Layer.rxData[1]+1, MOVE );

        TIME curtime;
        GetCurSec( &curtime );
        App_Layer.LastRxTime = curtime.Sec;                //收到帧，刷新接收计时器
        LOG_INFO(pRouteInf->GetChnId(), "ldqlalala2");
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
        QString rxData2 = "lzf";
        for(int i=1;i<4;i++)
        {
            LOG_INFO(pRouteInf->GetChnId(), "ldq"+ QString::number(i));
            LOG_INFO(pRouteInf->GetChnId(), "ldq"+ QString::number(App_Layer.rxData[i]));
            if(i==2)
            {
                LOG_INFO(pRouteInf->GetChnId(), "lzf"+ QString::number(App_Layer.rxData[i]));
                rxData2 = QString::number(App_Layer.rxData[i]);
            }
        }


        LOG_INFO(pRouteInf->GetChnId(), "rxData2"+rxData2);
        if( rxData2=="106"  )                //密码信息命令设置反馈ldq
        {
            LOG_INFO(pRouteInf->GetChnId(), "ldqlalala4");
            App_RxMtrLockFrame(  &App_Layer.rxData[0],App_Layer.rxData[1]+2);
        }
        else if( App_Layer.rxData[2] & 0x01 )                //U or S format
        {
            App_RxFixFrame( );
        }
        else if( App_Layer.rxData[1] <= 255 && App_Layer.rxData[1] > 4)    //I format
        {
            App_RxVarFrame( &App_Layer.rxData[0],App_Layer.rxData[1]+2);
        }

        rxStep = 0;
    }
}



void Iec104::App_RxMtrLockFrame( uint8 *apdu, int size )
{
    LOG_INFO(pRouteInf->GetChnId(), "ldqlalala5");

    //解析报文至结构体中
    SetPasswordReqParam_S data;

    data.cchId =pRouteInf->GetChnId();
    data.passwd = 123456;
    data.lockno =2955;
    data.value= 1;
    data.message= "lalala";
    App_RxYkConf( apdu, size );
    //pRawDb->SendSetPassword(data);//上送遥控命令到智能分析应用


}



/********************************************************************************
*
*    描述: 处理定长控制帧
*    参数: 无.
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxFixFrame( void )
{
    if( App_Layer.rxData[1] != 4 )
    {
//        pRouteInf->RecAFrame( PROTO_RXERRORS );
        return;
    }

//    pRouteInf->RecAFrame( PROTO_RXFRAME );

    if( App_Layer.rxData[2] & 0x02 )        //U format
    {    
        uint8 type = App_Layer.rxData[2] & 0xfc;    
                
        switch( type )
        {
        case APP_UFORMAT_STARCON:
            if( App_Layer.State == IEC104_APP_STATE_WAITSTARCONF )
            {
                if( pRouteInf->GetDebugFlag() )
                    LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Receive StartConf ===\n");

                App_Layer.Snd_seqnum = 0;
                App_Layer.Rec_seqnum = 0;
                App_Layer.Ack_num = 0;
                App_Layer.State = IEC104_APP_STATE_IDLE;
                App_Layer.Inited_Flag = 1;
                AckFinished = 1;

                //链路连接成功(Inited_Flag = 1)后,置总召和同步标志,App_CheckTime函数内从此时开始算起
                if(pRouteInf->GetAllScan() > 0)
                {
                    App_Layer.CallAllDataFlag = 1;
                }
                else
                {
                    App_Layer.CallAllDataFlag = 0;
                }
                //QString rtuId = pRouteInf->GetRtuId();
                //if(pRtuInf->GetKwhNum(rtuId) > 0)
                if(pRouteInf->GetScanPulseTime()>0)
                {
                    App_Layer.CallAllKwhFlag = 1;
                }
                else
                {
                    App_Layer.CallAllKwhFlag = 0;
                }

                /*chenfuqing 20230914 modified 轨道需求：校时周期（秒）为 0时 ，不要校时命令*/
                //if(pRtuInf->GetSyncTimeInt(0) < MAX_SYNTIME_PIERIOD)
                if(pRtuInf->GetSyncTimeInt(0) > 0)
                {
                    App_Layer.TimeSyncFlag = 1;
                }
                else
                {
                    App_Layer.TimeSyncFlag = 0;    
                }

                App_Layer.CallGroupDataFlag = 0;
                App_Layer.CallGroupNo = 0;

                TIME curtime;
                GetCurSec( &curtime );
                App_Layer.LastCallAllTime = curtime.Sec;
                App_Layer.LastSyncTime = curtime.Sec;
                App_Layer.LastCallAllKwhTime = curtime.Sec;

                //收到确认帧时，RTU链路状态置为: 1-已连接
                if(m_protoInterface)
                {
                    emit m_protoInterface->setRTUStatus(pRouteInf->GetRtuId(), 1);
                }
            }
            break;

        case APP_UFORMAT_STOPCON:
            if( App_Layer.State == IEC104_APP_STATE_WAITSTOPCONF )
            {
                InitData( );         //重新初始化
                AckFinished = 1;    //接收结束
            }
            break;

        case APP_UFORMAT_TESTCON:
            if( App_Layer.State == IEC104_APP_STATE_WAITTESTCONF)
            {
                if( pRouteInf->GetDebugFlag() )
                    LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Receive TestConf ===\n");

                App_Layer.State = IEC104_APP_STATE_IDLE;
                AckFinished = 1;
            }
            break;

        case APP_UFORMAT_STARACT:
            App_Layer.Snd_seqnum = 0;
            App_Layer.Rec_seqnum = 0;
            App_Layer.Ack_num = 0;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;    
            break;

        case APP_UFORMAT_TESTACT:
            App_SendAppUFormat( APP_UFORMAT_TESTCON );                    
            break;

        case APP_UFORMAT_STOPACT:
            App_SendAppUFormat( APP_UFORMAT_STOPCON );                    
            //App_Layer.Snd_seqnum = 0;
            //App_Layer.Rec_seqnum = 0;
            //App_Layer.Ack_num = 0;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;    
            break;
        }
    }
    else if(App_Layer.rxData[2] & 0x01 )        //S format  
    {    
        int Seqnum = App_Layer.rxData[4]/2 + App_Layer.rxData[5]*128;
        
        if( Seqnum == App_Layer.Snd_seqnum)    //得到子站对主站最后I format的确认
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
void Iec104::App_RxVarFrame( uint8 *apdu, int size )
{
    int Seqnum = apdu[2]/2 + apdu[3]*128;    //校验子站的Snd_seqnum

    if(Config_Param.isCheckSeqNum && Seqnum != App_Layer.Rec_seqnum )    //子站的I format 出现丢失或重复
    {
        //if( pRouteInf->GetDebugFlag() )
        {
            QString rtuId = pRouteInf->GetRtuId();
            QString chnId = pRouteInf->GetChnId();
            QString errMsg =  StringUtil::toQString("IEC104 SequenceNum Check Error! ===>Rec_seq:%d, Snd_seq:%d. ",App_Layer.Rec_seqnum, Seqnum) +
                    StringUtil::toQString("[chnId:%s,rtuId:%s][%02X %02X %02X %02X %02X %02X]\n",chnId.toStdString().c_str(),rtuId.toStdString().c_str(),apdu[0],apdu[1],apdu[2],apdu[3],apdu[4],apdu[5]);
            LOG_ERROR(pRouteInf->GetChnId(), errMsg);
        }    

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

    Seqnum = apdu[4]/2 + apdu[5]*128;    //校验子站的Rec_seqnum
    if( Seqnum == App_Layer.Snd_seqnum )    //得到子站对主站最后I format的确认
        App_Layer.WaitACKFlag = 0;

    //每接收一I帧数据Rec_seqnum加1，置发送确认标志SendACKFlag。    
    App_Layer.Rec_seqnum ++;        
    App_Layer.Rec_seqnum %= 0x8000;        //Rec_seqnum 为15位二进制数。

    App_Layer.SendACKFlag = 1;
    App_Layer.Ack_num = Seqnum;

    App_Layer.RxWindowNum ++;

    uint8 *asdu = &apdu[6];
//    uint32 old = App_Layer.Command_Enable;
    App_Layer.Command_Enable = (asdu[2]&0x80);
//    if(old != App_Layer.Command_Enable)
//        pRouteInf->SetCommandEnable(App_Layer.Command_Enable);
    size -=6;
    LOG_DEBUG(pRouteInf->GetChnId(), QString("asdu[0]=%1").arg(asdu[0]));
    switch( asdu[0] )
    {
    case APPTYPE_CALLDATA:
        App_RxAllDataConf( asdu, size );
        break;
    case APPTYPE_CALLKWH:
        App_RxAllKwhConf( asdu, size );
        break;
    case APPTYPE_TIMESYNC:
        App_RxTimeConf( asdu, size );
        break;
    //liujie add 20230529 添加防误操作响应报文处理 begin
    case APPTYPE_WF200://五防：防误校验-0xC8
    case APPTYPE_WF201://五防：应急模式-0xC9
        App_RxWFConf(asdu, size);
        break;
   //liujie add 20230529 添加防误操作响应报文处理 end
    case APPTYPE_YK45:
        App_RxYkConf( asdu, size );
        break;
    case APPTYPE_YK46:
        App_RxYkConf( asdu, size );
        break;
    case APPTYPE_YT47:
        App_RxYt47Conf( asdu, size );
        break;
    case APPTYPE_SP48:
    case APPTYPE_SP49:
    case APPTYPE_SP50:
    case APPTYPE_SP51:
        App_RxSPConf(asdu, size);
        break;
    case APPTYPE_SP_NT:
    case APPTYPE_DP_NT:
    case APPTYPE_PS_STS_NT:
        App_RxYxFrame( asdu, size );
        break;
    case APPTYPE_SP_WT:
    case APPTYPE_SP_WT30:
    case APPTYPE_DP_WT31:
        App_RxSoeFrame( asdu, size );
        break;
    case APPTYPE_ME_NT:
    case APPTYPE_ME_WT:
    case APPTYPE_ME_ND:
    case APPTYPE_ME_NT11:
    case APPTYPE_ME_FLOAT:
        App_RxYcFrame( asdu, size );
        break;
    case APPTYPE_CO_NT:
    case APPTYPE_CO_WT:
    case APPTYPE_CO_TE:
        App_RxKwhFrame( asdu, size );
        break;
    case 142://DA故障处理结果报文
        App_RxDAResult( asdu, size );
        break;
    default:
        App_RxOtherType(asdu, size);
        break;
    }
}

void Iec104::App_RxOtherType(uint8 *appdata, int datalen)
{
        if( pRouteInf->GetDebugFlag() )
            LOG_WARN(pRouteInf->GetChnId(), StringUtil::toQString("IEC104::App_RxOtherType unsurported apptype: %d\n", appdata[0] ));
}

/********************************************************************************
*
*    描述: 处理全数据响应应用服务数据单元(ASDU) 的确认响应和结束响应 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxAllDataConf( uint8 *asdu, int size )
{
    uint32    infnum = asdu[1]&0x7f;
    int pos = 6;
    
    if(infnum != 1)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104:\nApp_RxAllDataConf received inf number!=1 ---\n" );

        return;
    }

    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_WAITALLDATACONF:
        if( (asdu[2]&0x3f) == APP_COT_ACT_CON )
        {
            App_Layer.State = IEC104_APP_STATE_WAITALLDATA;

            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104: IEC104 receive alldata ack conf\n");
        }
        else if( (asdu[2]&0x3f) == APP_COT_DEACT_CON )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
            pRawDb->SendCmdCallDataEnd(pRouteInf->GetRtuId(), pRouteInf->GetChnId(), CMD_RESULT_FAIL, "Callalldata get a nack!");
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive alldata nack\n");
        }
        break;
    case IEC104_APP_STATE_WAITALLDATA:
        if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
            pRawDb->SendCmdCallDataEnd(pRouteInf->GetRtuId(), pRouteInf->GetChnId(), CMD_RESULT_SUCCESS, "");
            App_SendAppSFormat(true);//收到总召结束后，马上发一个S帧确认
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive alldata finish\n");
        }
        break;
    /*如果RTU某一组没有数据,只发确认和结束,这里可以切换成IDLE*/
    case IEC104_APP_STATE_WAITGRPDATA:
        if( Config_Param.CotNum == 2 )
            pos++;
        if( Config_Param.ComAddrNum == 2 )
            pos++;
        if( Config_Param.InfAddrNum == 3 )
            pos++;

        //不是所要召唤的组号
        if( asdu[pos] != (20+App_Layer.CallGroupNo) )
            return;

        if( (asdu[2]&0x3f) == APP_COT_ACT_CON )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
            pRawDb->SendCmdCallDataEnd(pRouteInf->GetRtuId(), pRouteInf->GetChnId(), CMD_RESULT_SUCCESS, "Callalldata get a groupdata ack conf!");
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive groupdata ack conf\n");
        }
        else if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
            pRawDb->SendCmdCallDataEnd(pRouteInf->GetRtuId(), pRouteInf->GetChnId(), CMD_RESULT_SUCCESS, "Callalldata get a groupdata finish");
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive groupdata finish\n");
        }

        if( (asdu[2]&0x40) == 0x40 )    //neg conf
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
            pRawDb->SendCmdCallDataEnd(pRouteInf->GetRtuId(), pRouteInf->GetChnId(), CMD_RESULT_FAIL, "Callalldata get a groupdata neg conf");
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104:receive groupdata finish\n");
#if CFQ
        /*是否考虑RTU的组号有最大16组的限制??*/
            if(pRtuInf->GetGroupNum(rtuaddr) > 0)
            {
                App_Layer.CallGroupDataFlag = 1;
                App_Layer.CallGroupNo %= pRtuInf->GetGroupNum(rtuaddr);
                App_Layer.CallGroupNo++;
            }
#endif
        }
        break;
    }
}

/********************************************************************************
*
*    描述: 处理全电度响应应用服务数据单元(ASDU)的确认响应和结束响应 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxAllKwhConf( uint8 *asdu, int size )
{
    uint32    infnum = asdu[1]&0x7f;
    
    if(infnum != 1)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104:\nApp_RxAllKwhConf received inf number!=1 ---\n" );

        return;
    }

    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_WAITALLKWHCONF:
        if( (asdu[2]&0x3f) == APP_COT_ACT_CON )
        {
            App_Layer.State = IEC104_APP_STATE_WAITALLKWH;
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive kwh confirm\n");
        }
        else if( (asdu[2]&0x3f) == APP_COT_DEACT_CON )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        break;
    case IEC104_APP_STATE_WAITALLKWH:
        if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )
        {
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 recieve kwh finish\n");
        }
        break;
    }
}

/********************************************************************************
*
*    描述: 处理同步时钟响应应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxTimeConf( uint8 *asdu, int size )
{
    uint32    infnum = asdu[1]&0x7f;
    
    if(infnum != 1)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104:\nApp_RxTimeConf received inf number!=1 ---\n" );

        return;
    }

    time_t    ti, tiack;
    struct    tm    ttm;
    struct    timeval ttv;
    uint32    ms;
    int    j;
    
    j = 5;
    
    if( Config_Param.CotNum == 2 )
        j++;
    if( Config_Param.ComAddrNum == 2 )
        j++;
    if( Config_Param.InfAddrNum == 2 )
        j++;
    if( Config_Param.InfAddrNum == 3 )
        j+=2;

    if( (asdu[2]&0x3f) == APP_COT_ACT_CON )//激活确认
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive time confirm.\n");

        if( App_Layer.State == IEC104_APP_STATE_WAITTIMECONF )
        {
            time( &ti );
            /*gettimeofday( &ttv, NULL );*/
            GetTimeOfDay( &ttv );

            ttm.tm_sec = (asdu[j+3]*256+asdu[j+2])/1000;
            ms = (asdu[j+3]*256+asdu[j+2])%1000;
            ttm.tm_min = asdu[j+4]&0x3f;
            ttm.tm_hour = asdu[j+5]&0x1f;
            ttm.tm_mday = asdu[j+6]&0x1f;
            ttm.tm_mon = (asdu[j+7]&0x0f)-1;
            ttm.tm_year = (asdu[j+8]&0x7f)+100;
            ttm.tm_isdst = 0;
            tiack = mktime( &ttm );

            Time_cor = ( (ti-tiack)*1000 + ttv.tv_usec/1000 - ms + Time_cor )/2;
            if( pRouteInf->GetDebugFlag() )
                LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104:time_cor = %d\n", Time_cor ));

            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
    }
    else if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )//激活结束
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 receive time confirm end.\n");
    }
    else 
    {
        LOG_WARN(pRouteInf->GetChnId(), "IEC104 time conf error!!!\n");
    }    
}


/********************************************************************************
*
*    描述: 处理遥测数据响应应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxYcFrame( uint8 *asdu, int size )    //不带品质遥测数据变化响应帧
{
    uint32    ycnum = asdu[1]&0x7f;
    uint8    seqflag = asdu[1]&0x80, yc_des, cot;
    uint32    i, ycno, j;
    sint32    ycvalue;
    //uint32  yccode;
    uint16    rtuaddr;
    uint16 status = 0;
    uint8 testbit;
    float32 value;
    
    if( pRouteInf->GetDebugFlag() )
        LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104: IEC104 receive yc frame: ycnum: %d, type : %d ---\n", ycnum, asdu[0] ));
    
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
#if CFQ
    if( rtuaddr != m_rtuAddr )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "rtu address is different with current rtu address! rtuaddr:") << rtuaddr <<", current rtuaddr:" << m_rtuAddr;
        return;
    }
#endif

    cot = asdu[2]&0x3f;
    testbit = asdu[2]&0x80;

    //---
    if( cot == (20+App_Layer.CallGroupNo) && App_Layer.State == IEC104_APP_STATE_WAITGRPDATA )
    {
        App_Layer.State = IEC104_APP_STATE_IDLE;
        AckFinished = 1;
    }
    //---

    //是否周期总召数据
    bool isCycCallAll = false;
    if(APP_COT_RESP_CALLALL == cot)
    {
        isCycCallAll = true;
    }

    switch( asdu[0] )
    {
    case APPTYPE_ME_FLOAT:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 ycframe: APPTYPE_ME_FLOAT\n");

        if( seqflag )
        {
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;    
            ycno -= Config_Param.YcBaseAddr;
            
            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++, ycno++ )
            {
                value = char_to_float((char*)&asdu[j+5*i]);
                yc_des = asdu[j+4+5*i];

                status = App_YcQcExchange(yc_des);

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(value));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, value, asdu[j+1+3*i]*256+asdu[j+3*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =value;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);

        }
        else
        {
            int length = 7,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                ycno -= Config_Param.YcBaseAddr;

                value = char_to_float((char*)&asdu[j+pos+length*i]);

                yc_des = asdu[j+pos+4+length*i];

                status = App_YcQcExchange(yc_des);

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(value));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, value, asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =value;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
    break;
    case APPTYPE_ME_NT11:
        if( pRouteInf->GetDebugFlag() )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 ycframe: APPTYPE_ME_NT11\n");
        }

        if( seqflag )
        {
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;    
            ycno -= Config_Param.YcBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode(&asdu[j+3*i]);
                yc_des = asdu[j+2+3*i];
                status = App_YcQcExchange(yc_des);
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(ycvalue));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, ycvalue, asdu[j+1+3*i]*256+asdu[j+3*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =ycvalue;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        else
        {
            int length = 5,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                ycno -= Config_Param.YcBaseAddr;

                ycvalue = sint16(asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i]);
                yc_des = asdu[j+pos+2+length*i];

                status = App_YcQcExchange(yc_des);

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(ycvalue));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, ycvalue, asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =ycvalue;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        break;
    case APPTYPE_ME_NT:        //不带时标的测量值
        if( pRouteInf->GetDebugFlag() )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 ycframe: APPTYPE_ME_NT\n");
        }

        if( seqflag )
        {
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j+=3;
            }
            else
                j+=2;    
            ycno -= Config_Param.YcBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++, ycno++ )
            {
                value = App_YcDecode( &asdu[j+3*i] );                
                //value /= 65536.;
                yc_des = asdu[j+2+3*i];
                status = App_YcQcExchange(yc_des);

                if( testbit==0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(value));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, value, asdu[j+1+3*i]*256+asdu[j+3*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =value;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        else
        {
            int length = 5,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                ycno -= Config_Param.YcBaseAddr;

                value = App_YcDecode( &asdu[j+pos+length*i] );
                //value /= 65536.;
                yc_des = asdu[j+pos+2+length*i];

                status = App_YcQcExchange(yc_des);

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(value));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, value, asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =value;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        break;
    case APPTYPE_ME_WT:        /*带时标的测量值*/
        if( pRouteInf->GetDebugFlag() )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 ycframe: APPTYPE_ME_WT, seqflag = %02x\n", seqflag));
        }

        uint32 ms, minute;
        if( seqflag )
        {
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            ycno -= Config_Param.YcBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode( &asdu[j+6*i] );
                yc_des = asdu[j+2+6*i];

                status = App_YcQcExchange(yc_des);

                ms = asdu[j+4+6*i]*256+asdu[j+3+6*i];
                minute = asdu[j+5+6*i]&0x3f;

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(ycvalue));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, ycvalue, asdu[j+1+6*i]*256+asdu[j+6*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =ycvalue;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        else
        {
            int length = 8,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                ycno -= Config_Param.YcBaseAddr;
                ycvalue = App_YcDecode( &asdu[j+pos+length*i] );
                yc_des = asdu[j+pos+2+length*i];
                status = App_YcQcExchange(yc_des);
                ms = asdu[j+pos+4+length*i]*256+asdu[j+pos+3+length*i];
                minute = asdu[j+pos+5+length*i]&0x3f;

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(ycvalue));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, ycvalue, asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i], status );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =ycvalue;
                    data->quality =status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        break;
    case APPTYPE_ME_ND:        //不带品质描述的测量值
        if( pRouteInf->GetDebugFlag() )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104: IEC104 ycframe: APPTYPE_ME_ND, seqflag :%02x  \n", seqflag ));
        }
        if( seqflag )
        {
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            ycno -= Config_Param.YcBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode( &asdu[j+2*i] );
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(ycvalue));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, ycvalue, asdu[j+2*i]*256+asdu[j+2*i],SICD_M_AI_STS_ON_LINE );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =ycvalue;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        else
        {
            int length = 4,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                 ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                ycno -= Config_Param.YcBaseAddr;
                ycvalue = App_YcDecode( &asdu[j+pos+length*i] );

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYC\tChnId=%1\trtuaddr=%2\tYcNo=%3\tYcValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(ycno).arg(ycvalue));
                }
                else
                {
                    //pRawDb->PutAYc( rtuaddr, ycno, ycvalue, asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i],SICD_M_AI_STS_ON_LINE );
                    std::shared_ptr<YCParam_S> data = std::make_shared<YCParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =ycno;
                    data->value =ycvalue;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYcList(isCycCallAll, dataList);
        }
        break;
    default:
        if( pRouteInf->GetDebugFlag() )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received other YC frame ---\n");
        }
        break;
    }
}


/********************************************************************************
*
*    描述: 处理事项数据响应应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxSoeFrame( uint8 *asdu, int size )
{
    SOEDATA soe;
    uint32    soenum = asdu[1]&0x7f, ms, minute;
    uint32    i,  yxno, infosize,j;
    uint8    yxvalue, yx_des;
    uint16    rtuaddr;
    uint16 status = 0;

    if( soenum == 0 ) return;
    size -= 4;//为了计算SOE的字节数
    j = 3;

    uint8 cot = asdu[2]&0x3f;
    uint8 testbit = asdu[2]&0x80;

    if( Config_Param.CotNum == 2 )
    {
        j++;
        size--;
        cot += asdu[3]*256;
    }

    if( cot == APP_COT_SPONT )
    {
        /*cfq 20221103 注释掉下面这句，因为这可能会打断正在执行的到一半的遥控命令;当两个遥控执行一起过来时，第一个遥控点，可能关联了遥信，执行完成后，
        且还没有上送遥信SOE状态之前，开始了第二个点遥控执行,此时App_Layer.State = IEC104_APP_STATE_WAITYKCONF；
        此时当第一个关联的遥信SOE状态上送，并运行到此处，如果置App_Layer.State = IEC104_APP_STATE_IDLE，则第二个遥控就打断了。
        */
        //App_Layer.State = IEC104_APP_STATE_IDLE;
        AckFinished = 1;
    }

    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;
        j += 2;
        size--;
    }
    else
    {
        rtuaddr = asdu[j];
        j++;
    }
#if CFQ
    if( rtuaddr != m_rtuAddr )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "rtu address is different with current rtu address! rtuaddr:") << rtuaddr <<", current rtuaddr:" << m_rtuAddr;
        return;
    }
#endif
    infosize = size/soenum;                

    GetCurTime( &soe.soetime );
    
    if( infosize == 6 )    //normal frame(2字节信息体地址+1字节YX值+3字节短时标+)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received normal soe frame ~~~~~~~~~~~~~~~~~~\n");

        if( Config_Param.InfAddrNum != 2 )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received normal soe frame size=6 but infaddrnum !=2~~~~~\n");
            return;
        }

        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+2+infosize*i];
            ms = asdu[j+4+infosize*i]*256+asdu[j+3+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;

            soe.soetime.minute = minute;
            soe.soetime.second = ms/1000;
            soe.soetime.ms = ms%1000;

            //时标IV位无效            
            if( asdu[j+5+infosize*i]&0x80 )
                continue;

            yx_des = asdu[j+2+infosize*i]&0xFC;

            if( asdu[0] == 31 )
            {
                soe.YxValue = (yxvalue&0x03);
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&soe.YxValue);
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, status );
                }
            }
            else
            {
                soe.YxValue = yxvalue&0x01;
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, SICD_M_DI_STS_ON_LINE );
            }
        }
    }
    if( infosize == 7 )    //normal frame(3字节信息体地址+1字节YX值+3字节短时标+)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received normal soe frame ~~~~~~~~~~~~~~~~~~\n");

        if( Config_Param.InfAddrNum != 3 )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received normal soe frame size=6 but infaddrnum !=3~~~~~\n");
            return;
        }

        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+2+infosize*i]*256*256+asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+3+infosize*i];
            ms = asdu[j+5+infosize*i]*256+asdu[j+4+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;

            soe.soetime.minute = minute;
            soe.soetime.second = ms/1000;
            soe.soetime.ms = ms%1000;

            yx_des = asdu[j+3+infosize*i]&0xFC;

            if( asdu[0] == 31 )
            {
                soe.YxValue = (yxvalue&0x03);

                //时标IV位无效            
                if( asdu[j+5+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&soe.YxValue);
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, status);
                }
            }
            else
            {
                soe.YxValue = yxvalue&0x01;

                //时标IV位无效            
                if( asdu[j+5+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, SICD_M_DI_STS_ON_LINE );
            }
        }
    }
    else if( infosize == 9 )     //zd extended frame(2字节信息体地址+1字节YX值+6字节短时标+)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received  zd extended soe frame ~~~~~~~~~~~~~~~~~~\n");

        if( Config_Param.InfAddrNum != 2 )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received zd extended soe frame size=9 but infaddrnum !=2~~~~~\n");
            return;
        }

        for( i=0; i<soenum; i++ )
        {
            yxno = asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+2+infosize*i];
            ms = asdu[j+4+infosize*i]*256+asdu[j+3+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;
            soe.soetime.minute = minute;
            soe.soetime.second = ms/1000;
            soe.soetime.ms = ms%1000;
            soe.soetime.hour = asdu[j+6+infosize*i]&0x1f;
            soe.soetime.day = asdu[j+7+infosize*i]&0x1f;
            soe.soetime.month = asdu[j+8+infosize*i]&0xf;
        
            yx_des = asdu[j+2+infosize*i]&0xFC;

            if( asdu[0] == 31 )
            {
                soe.YxValue = (yxvalue&0x03);

                //时标IV位无效            
                if( asdu[j+5+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&soe.YxValue);
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, status);
                }
            }
            else
            {
                soe.YxValue = (yxvalue&0x01);

                //时标IV位无效            
                if( asdu[j+5+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, SICD_M_DI_STS_ON_LINE );
            }
        }
    }
    else if( infosize == 10 )    //df extend(2字节信息体地址+1字节YX值+7字节长时标)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received  extended soe frame ~~~~~~~~~~~~~~~\n");

        if( Config_Param.InfAddrNum != 2 )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received extended soe frame size=10 but infaddrnum !=2~~~~~\n");
            return;
        }

        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+2+infosize*i];
            ms = asdu[j+4+infosize*i]*256+asdu[j+3+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;

            soe.soetime.minute = minute;
            soe.soetime.second = ms/1000;
            soe.soetime.ms = ms%1000;
            soe.soetime.hour = asdu[j+6+infosize*i]&0x1f;
            soe.soetime.day = asdu[j+7+infosize*i]&0x1f;
            soe.soetime.month = asdu[j+8+infosize*i]&0xf;
            soe.soetime.year = (asdu[j+9+infosize*i]&0x7f)+2000;
            
            yx_des = asdu[j+2+infosize*i]&0xFC;

            if( asdu[0] == 31 )
            {
                soe.YxValue = (yxvalue&0x03);

                //时标IV位无效            
                if( asdu[j+5+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&soe.YxValue);
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, status);
                }
            }
            else
            {
                soe.YxValue = yxvalue&0x01;

                //时标IV位无效            
                if( asdu[j+5+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, SICD_M_DI_STS_ON_LINE );
            }
        }
    }
    else if( infosize == 11 )    //guangdong xuji(3字节信息体地址+1字节YX值+7字节长时标)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received  extended soe frame ~~~~~~~~~~~~~~~\n");

        if( Config_Param.InfAddrNum != 3 )
        {
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received extended soe frame size=11 but infaddrnum !=3~~~~~\n");
            return;
        }

        for( i=0; i<soenum; i++ )
        {
            yxno = asdu[j+2+infosize*i]*256*256+asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+3+infosize*i];
            ms = asdu[j+5+infosize*i]*256+asdu[j+4+infosize*i];
            minute = asdu[j+6+infosize*i]&0x3f;

            soe.soetime.minute = minute;
            soe.soetime.second = ms/1000;
            soe.soetime.ms = ms%1000;
            soe.soetime.hour = asdu[j+7+infosize*i]&0x1f;
            soe.soetime.day = asdu[j+8+infosize*i]&0x1f;
            soe.soetime.month = asdu[j+9+infosize*i]&0xf;
            soe.soetime.year = (asdu[j+10+infosize*i]&0x7f)+2000;
            
            yx_des = asdu[j+3+infosize*i]&0xFC;

            if( asdu[0] == 31 )
            {
                soe.YxValue = (yxvalue&0x03);

                //时标IV位无效            
                if( asdu[j+6+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&soe.YxValue);
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, status);
                }
            }
            else
            {
                soe.YxValue = yxvalue&0x01;

                //时标IV位无效            
                if( asdu[j+6+infosize*i]&0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tSOE TIMESTAMP INVALID\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                    continue;
                }

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTSOE\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(soe.YxValue));
                }
                else    
                    pRawDb->PutASoe( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), yxno, soe, SICD_M_DI_STS_ON_LINE );
            }
        }
    }
}


/********************************************************************************
*
*    描述: 处理电度响应应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxKwhFrame( uint8 *asdu, int size )
{
    uint32    kwhnum = asdu[1]&0x7f;
    uint8    seqflag = asdu[1]&0x80, kwh_sq, cot, testbit;
    uint32        kwhno, i,j;
    sint32    kwhvalue;
    uint16    rtuaddr;

    if( pRouteInf->GetDebugFlag() )
        LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104: IEC104 receive kwhframe: kwhnum = %d ---\n", kwhnum ));

    j = 3;

    if( Config_Param.CotNum == 2 )
        j++;

    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;
        j += 2;
    }
    else
    {
        rtuaddr = asdu[j];
        j++;
    }
#if CFQ
    if( rtuaddr != m_rtuAddr)
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "rtu address is different with current rtu address! rtuaddr:") << rtuaddr <<", current rtuaddr:" << m_rtuAddr;
        return;
    }
#endif
    cot = asdu[2]&0x3f;
    testbit = asdu[2]&0x80;

    if( cot == (20+App_Layer.CallGroupNo) && App_Layer.State == IEC104_APP_STATE_WAITGRPDATA )
    {
        App_Layer.State = IEC104_APP_STATE_IDLE;
        AckFinished = 1;
    }

    switch( asdu[0] )
    {
    case APPTYPE_CO_NT:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 Received kwhframe: APPTYPE_CO_NT seqflag = %02x\n", seqflag ));

        if( seqflag )
        {
            kwhno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                kwhno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            kwhno -= Config_Param.DdBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < kwhnum; i++, kwhno++ )
            {
                kwhvalue = App_KwhDecode( &asdu[j+5*i] );
                kwh_sq = asdu[j+4+5*i];
#if 0
                //if( kwh_sq&0x80 )
                pRawDb->PutAKwh( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), kwhno, kwhvalue,SICD_M_CO_STS_ON_LINE );
#else
                std::shared_ptr<DDParam_S> data = std::make_shared<DDParam_S>();
                data->rtuId = pRouteInf->GetRtuId();
                data->no =kwhno;
                data->value =kwhvalue;
                dataList.push_back(data);
#endif
            }
            pRawDb->PutKwhList(true, dataList);
        }
        else
        {
            int length = 7,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < kwhnum; i++ )
            {
                pos = 2;
                kwhno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    kwhno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                kwhno -= Config_Param.DdBaseAddr;

                kwhvalue = App_KwhDecode( &asdu[j+pos+length*i] );
                kwh_sq = asdu[j+pos+4+length*i];
#if 0
                //if( kwh_sq&0x80 )
                pRawDb->PutAKwh( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), kwhno, kwhvalue,SICD_M_CO_STS_ON_LINE );
#else
                std::shared_ptr<DDParam_S> data = std::make_shared<DDParam_S>();
                data->rtuId = pRouteInf->GetRtuId();
                data->no =kwhno;
                data->value =kwhvalue;
                dataList.push_back(data);
#endif
            }
            pRawDb->PutKwhList(true, dataList);
        }
        break;
    case APPTYPE_CO_WT:
        uint32 ms, minute;
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received kwhframe: APPTYPE_CO_WT\n");

        if( seqflag )
        {
            kwhno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                kwhno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            kwhno -= Config_Param.DdBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < kwhnum; i++, kwhno++ )
            {
                kwhvalue = App_KwhDecode( &asdu[j+8*i] );
                kwh_sq = asdu[j+4+8*i];
                ms = asdu[j+6+8*i]*256+asdu[j+5+8*i];
                minute = asdu[j+7+8*i]&0x3f;
#if 0
                //if( kwh_sq&0x80 )
                pRawDb->PutAKwh( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), kwhno, kwhvalue,SICD_M_CO_STS_ON_LINE );
#else
                std::shared_ptr<DDParam_S> data = std::make_shared<DDParam_S>();
                data->rtuId = pRouteInf->GetRtuId();
                data->no =kwhno;
                data->value =kwhvalue;
                dataList.push_back(data);
#endif
            }
            pRawDb->PutKwhList(true, dataList);
        }
        else
        {
            int length = 10,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < kwhnum; i++ )
            {
                pos = 2;
                kwhno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    kwhno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                kwhno -= Config_Param.DdBaseAddr;

                kwhvalue = App_KwhDecode( &asdu[j+pos+length*i] );
                kwh_sq = asdu[j+pos+4+length*i];
                ms = asdu[j+pos+6+length*i]*256+asdu[j+pos+5+length*i];
                minute = asdu[j+pos+7+length*i]&0x3f;
#if 0
                //if( kwh_sq&0x80 )
                pRawDb->PutAKwh( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), kwhno, kwhvalue,SICD_M_CO_STS_ON_LINE );
#else
                std::shared_ptr<DDParam_S> data = std::make_shared<DDParam_S>();
                data->rtuId = pRouteInf->GetRtuId();
                data->no =kwhno;
                data->value =kwhvalue;
                dataList.push_back(data);
#endif
            }
            pRawDb->PutKwhList(true, dataList);
        }
        break;
    case APPTYPE_CO_TE:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104 Received kwhframe: APPTYPE_CO_TE seqflag = %02x\n", seqflag ));
        if( seqflag )
        {
            kwhno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                kwhno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            kwhno -= Config_Param.DdBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < kwhnum; i++, kwhno++ )
            {
                kwhvalue = App_KwhDecode( &asdu[j+12*i] );
#if 0
                //if( kwh_sq&0x80 )
                pRawDb->PutAKwh( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), kwhno, kwhvalue,SICD_M_CO_STS_ON_LINE );
#else
                std::shared_ptr<DDParam_S> data = std::make_shared<DDParam_S>();
                data->rtuId = pRouteInf->GetRtuId();
                data->no =kwhno;
                data->value =kwhvalue;
                dataList.push_back(data);
#endif
            }
            pRawDb->PutKwhList(true, dataList);
        }
        else
        {
            int length = 14,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;
            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < kwhnum; i++ )
            {
                pos = 2;
                kwhno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    kwhno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                kwhno -= Config_Param.DdBaseAddr;

                kwhvalue = App_KwhDecode( &asdu[j+pos+length*i] );
#if 0
                //if( kwh_sq&0x80 )
                pRawDb->PutAKwh( pRouteInf->GetRtuId(), pRouteInf->GetChnId(), kwhno, kwhvalue,SICD_M_CO_STS_ON_LINE );
#else
                std::shared_ptr<DDParam_S> data = std::make_shared<DDParam_S>();
                data->rtuId = pRouteInf->GetRtuId();
                data->no =kwhno;
                data->value =kwhvalue;
                dataList.push_back(data);
#endif
            }
            pRawDb->PutKwhList(true, dataList);
        }
        break;
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received other kwh frame\n");

        break;
    }
}

/********************************************************************************
*
*    描述: 处理遥信数据响应应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxYxFrame( uint8 *asdu, int size )
{
    uint32    num = asdu[1]&0x7f;;
    uint8    seqflag = asdu[1]&0x80, yx_des, cot, testbit;
    uint32    yxno,i,j;
    uint8    yxvalue;
    uint16    rtuaddr;
    uint16 status = 0;
    
    j = 3;

    if( Config_Param.CotNum == 2 )
    {
        j++;
        size--;
    }

    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;
        j += 2;
        size--;
    }
    else
    {
        rtuaddr = asdu[j];
        j++;
    }
#if CFQ
    if( rtuaddr != m_rtuAddr )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "rtu address is different with current rtu address! rtuaddr:") << rtuaddr <<", current rtuaddr:" << m_rtuAddr;
        return;
    }
#endif
    cot = asdu[2]&0x3f;
    testbit = asdu[2]&0x80;

    if( cot == (20+App_Layer.CallGroupNo) && App_Layer.State == IEC104_APP_STATE_WAITGRPDATA )
    {
        App_Layer.State = IEC104_APP_STATE_IDLE;
        AckFinished = 1;
    }

    //是否周期总召数据
    bool isCycCallAll = false;
    if(APP_COT_RESP_CALLALL == cot)
    {
        isCycCallAll = true;
    }

    switch( asdu[0] )
    {
    case APPTYPE_DP_NT:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104: IEC104 Receive DP Yx frame without timeflag  Yxnum = %d ---\n", num ));

        if( seqflag )
        {
            yxno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                yxno += asdu[j+2]*256*256;
                j += 3;
                size--;
            }
            else
            j += 2;
            yxno -= Config_Param.YxBaseAddr;


            if( (num+6) > size )
                num = ( size-6 );

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < num; i++, yxno++ )
            {
                yxvalue = (asdu[j+i]&0x03);
                yx_des = asdu[j+i]&0xFC;
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxvalue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&yxvalue);
                    //pRawDb->PutAYx( rtuaddr, yxno, yxvalue, status);
                    std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =yxno;
                    data->value =yxvalue;
                    data->quality = status;
                    dataList.push_back(data);
                }
            }

            pRawDb->PutYxList(isCycCallAll, dataList);
        }
        else
        {
            int length = 3,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            if( (num*length+4) > size )
                num = ( size-4 )/3;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < num; i++ )
            {
                pos = 2;
                yxno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    yxno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                yxno -= Config_Param.YxBaseAddr;
                yxvalue = asdu[j+pos+length*i]&0x03;
                yx_des = asdu[j+pos+length*i]&0xFC;

                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tdpYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxvalue));
                }
                else
                {
                    status = App_YxQcExchange(yx_des,true,&yxvalue);
                    //pRawDb->PutAYx( rtuaddr, yxno, yxvalue, status );
                    std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =yxno;
                    data->value =yxvalue;
                    data->quality = status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYxList(isCycCallAll,dataList);
        }
        break;
    case APPTYPE_SP_NT:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), StringUtil::toQString("IEC104: IEC104 Receive SP Yx frame without timeflag  Yxnum = %d ---\n", num ));

        if( seqflag )
        {
            yxno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                yxno += asdu[j+2]*256*256;
                j += 3;
                size--;
            }
            else
                j += 2;
            yxno -= Config_Param.YxBaseAddr;

            if( (num+6) > size )
            num = ( size-6 );

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < num; i++, yxno++ )
            {
                yxvalue = asdu[j+i]&0x01;
                yx_des = asdu[j+i]&0xFC;
                status = App_YxQcExchange(yx_des);
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxvalue));
                }
                else
                {
                    //pRawDb->PutAYx( rtuaddr, yxno, yxvalue, status );
                    std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =yxno;
                    data->value =yxvalue;
                    data->quality = status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYxList(isCycCallAll,dataList);
        }
        else
        {
            int length = 3,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            if( (num*length+4) > size )
                num = ( size-4 )/3;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < num; i++ )
            {
                pos = 2;
                yxno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    yxno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                yxno -= Config_Param.YxBaseAddr;

                yxvalue = asdu[j+pos+length*i]&0x01;
                yx_des = asdu[j+pos+length*i]&0xFC;

                status = App_YxQcExchange(yx_des);
                
                if( testbit == 0x80 )
                {
                    LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxvalue));
                }
                else
                {
                    //pRawDb->PutAYx( rtuaddr, yxno, yxvalue, status );
                    std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                    data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                    data->no =yxno;
                    data->value =yxvalue;
                    data->quality = status;
                    dataList.push_back(data);
                }
            }
            pRawDb->PutYxList(isCycCallAll,dataList);
        }
        break;
    case APPTYPE_PS_STS_NT:
        uint8    yx_qds;
        uint16    yxcode, yxcd;
        if( seqflag )
        {
            yxno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                yxno += asdu[j+2]*256*256;
                j+=3;
            }
            else
                j += 2;
            yxno -= Config_Param.YxBaseAddr;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < num; i++ )
            {
                yxcode = asdu[j+1+5*i]*256+asdu[j+5*i];
                yxcd = asdu[j+3+5*i]*256+asdu[j+2+5*i];
                yx_qds = asdu[j+4+5*i];

                status = App_YxQcExchange(yx_qds);
                
                for( int k = 0; k < 16; k++ )
                {
                    if( yxcd & 0x01 )
                    {
                        if( testbit == 0x80 )
                        {
                            LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxcode&0x01));
                        }
                        else
                        {
                            //pRawDb->PutAYx( rtuaddr, yxno, (~yxcode)&0x01, status );
                            //pRawDb->PutAYx( rtuaddr, yxno, yxcode&0x01, status );
                            std::shared_ptr<YXParam_S> dataReverse = std::make_shared<YXParam_S>();
                            dataReverse->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                            dataReverse->no =yxno;
                            dataReverse->value =((~yxcode)&0x01);
                            dataList.push_back(dataReverse);
                            std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                            data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                            data->no =yxno;
                            data->value =(yxcode&0x01);
                            dataList.push_back(data);
                        }
                    }
                    else
                    {
                        if( testbit == 0x80 )
                        {
                            LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxcode&0x01));
                        }
                        else
                        {
                            //pRawDb->PutAYx( rtuaddr, yxno, yxcode&0x01, status );
                            std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                            data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                            data->no =yxno;
                            data->value =(yxcode&0x01);
                            data->quality = status;
                            dataList.push_back(data);
                        }
                    }

                    yxcd = yxcd>>1;
                    yxcode = yxcode>>1;
                    yxno++;
                }
            }            
            pRawDb->PutYxList(isCycCallAll,dataList);
        }
        else
        {
            int length = 7,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            std::list<std::shared_ptr<BaseParam_S>> dataList;
            for( i = 0; i < num; i++ )
            {
                pos = 2;
                yxno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    yxno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                yxno -= Config_Param.YxBaseAddr;
                yxcode = asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i];
                yxcd = asdu[j+pos+3+length*i]*256+asdu[j+pos+2+length*i];
                yx_qds = asdu[j+pos+4+length*i];
                status = App_YxQcExchange(yx_qds);

                for( int k = 0; k < 16; k++ )
                {
                    if( yxcd & 0x01 )    //连用两次，为置change_flag为1
                    {
                        if( testbit == 0x80 )
                        {
                            LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n ").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxcode&0x01));
                        }
                        else
                        {
                            //pRawDb->PutAYx( rtuaddr, yxno, (~yxcode)&0x01, status );
                            //pRawDb->PutAYx( rtuaddr, yxno, yxcode&0x01, status );
                            std::shared_ptr<YXParam_S> dataReverse = std::make_shared<YXParam_S>();
                            dataReverse->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                            dataReverse->no =yxno;
                            dataReverse->value =((~yxcode)&0x01);
                            dataList.push_back(dataReverse);
                            std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                            data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                            data->no =yxno;
                            data->value =(yxcode&0x01);
                            dataList.push_back(data);
                        }
                    }
                    else
                    {
                        if( testbit == 0x80 )
                        {
                            LOG_DEBUG(pRouteInf->GetChnId(), QString("IEC104\tTESTYX\tChnId=%1\trtuaddr=%2\tYxNo=%3\tspYxValue=%4\n").arg(pRouteInf->GetChnId()).arg(rtuaddr).arg(yxno).arg(yxcode&0x01));
                        }
                        else
                        {
                            //pRawDb->PutAYx( rtuaddr, yxno, yxcode&0x01, status );
                            std::shared_ptr<YXParam_S> data = std::make_shared<YXParam_S>();
                            data->rtuId = pRouteInf->GetRtuId();//pRtuInf->GetRtuIdByAddr(rtuaddr);
                            data->no =yxno;
                            data->value =(yxcode&0x01);
                            data->quality = status;
                            dataList.push_back(data);
                        }
                    }
                    yxcd = yxcd>>1;
                    yxcode = yxcode>>1;
                    yxno++;
                }
            }            
            pRawDb->PutYxList(isCycCallAll,dataList);
        }
        break;
    default:
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received other yx frame\n");

        break;
    }
}


/********************************************************************************
*        描述: 应用层处理设点确认帧.
*        参数: 
*              apdu: 应用层报文.
*              size: 应用层报文长度.
*        返回: 无.
********************************************************************************/
void Iec104::App_RxSPConf( uint8 *asdu, int size )
{
    uint32    infnum = asdu[1]&0x7f;
    if(infnum!=1)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "Iec104:App_RxSPConf received inf number!=1 ---\n" );
        return;
    }
    uint16 rtuaddr;
    int j,k;    
    j = 3;
    if( Config_Param.CotNum == 2 )
    {
        j++;
    }
    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;
        j += 2;
    }
    else
    {
        rtuaddr = asdu[j];
        j++;
    }
#if CFQ
    if( rtuaddr != m_rtuAddr )
    {
        if( pRouteInf->GetDebugFlag() )
           LOG_DEBUG(pRouteInf->GetChnId(), "rtu address is different with current rtu address! rtuaddr:") << rtuaddr <<", current rtuaddr:" << m_rtuAddr;

        return;
    }
#endif

    int spNo = asdu[j+1]*256+asdu[j];    
    j += 2;
    if( Config_Param.InfAddrNum == 3 )
    {
        spNo += asdu[j]*256*256;
        j++;
    }
    //if( spNo < Config_Param.SpBaseAddr || spNo >= (Config_Param.SpBaseAddr+512) )    //限制为512个遥调点
    if( spNo < Config_Param.SpBaseAddr)    //限制为512个遥调点
    {
        LOG_WARN(pRouteInf->GetChnId(), QString("Iec104:App_RxSPConf spNo:%1 < Config_Param.SpBaseAddr:%2 ---\n").arg(spNo).arg(Config_Param.SpBaseAddr) );
        return;
    }    
    spNo -= Config_Param.SpBaseAddr;

    if( App_Layer.CtrlNo != spNo )
    {
        LOG_WARN(pRouteInf->GetChnId(), QString("Iec104:App_RxSPConf App_Layer.CtrlNo:%1 != spNo:%2 ---\n").arg(App_Layer.CtrlNo).arg(spNo) );
        return;
    }

#if 0
    switch(asdu[0])
    {
        case APPTYPE_SP48:
            j+=2;//值占2位
            break;
        case APPTYPE_SP49:
            j+=2;//值占2位
            break;
        case APPTYPE_SP50:
            j+=4; //值占4位
            break;
        case APPTYPE_SP51:
            j+=4;//值占4位
            break;
        default:
            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104:App_RxSPConf Wrong asdu[0]:%1 ---\n").arg(asdu[0]) );
            return;
    }
    LOG_DEBUG(pRouteInf->GetChnId(), QString("Iec104:App_RxSPConf  asdu[j:%1]&0x80=%2 ---\n").arg(j).arg((asdu[j]&0x80)) );
    if( App_Layer.CtrlType == CTRL_FUNC_SELECT && (asdu[j]&0x80)!= 0x80 ||
            App_Layer.CtrlType == CTRL_FUNC_EXECUTE && (asdu[j]&0x80)!= 0x00 )
    {
        LOG_WARN(pRouteInf->GetChnId(), QString("Iec104:App_RxSPConf  App_Layer.CtrlType == CTRL_FUNC_SELECT && (asdu[j]&0x80)!= 0x80 ||  App_Layer.CtrlType == CTRL_FUNC_EXECUTE && (asdu[j]&0x80)!= 0x00 ---\n").arg(App_Layer.CtrlNo).arg(spNo) );
        return;
    }
#endif

    App_Layer.CtrlReply = -1;
    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_WAITSPCONF:
        if( (asdu[2]&0x3f) == APP_COT_DEACT_CON )
        {
            if(App_Layer.CtrlType == CTRL_FUNC_CANCEL)
            {
                LOG_INFO(pRouteInf->GetChnId(), "IEC104:App_RxSPConf Received SP cancel confirm.\n");//设置点取消确认

                //设置点撤销成功
                App_Layer.CtrlReply = COMMAND_YKCANCEL_SUCCESS;
            }
        }
        else if( (asdu[2]&0x3f) == APP_COT_ACT_CON )
        {
            if( (asdu[2]&0x40) == 0x40 )    //neg conf
            {
                LOG_INFO(pRouteInf->GetChnId(), "IEC104:App_RxSPConf Received SP select(exec) neg confirm. \n");//设置点选择（执行）否认

                //设置点预置失败
                if(App_Layer.CtrlType == CTRL_FUNC_SELECT)
                {
                    App_Layer.CtrlReply = COMMAND_SELECT_FAIL;
                }
                else if(App_Layer.CtrlType == CTRL_FUNC_EXECUTE)
                {
                    App_Layer.CtrlReply = COMMAND_YKEXEC_FAIL;
                }		
                else if( App_Layer.CtrlType == CTRL_FUNC_CANCEL)
                {
                    App_Layer.CtrlReply = COMMAND_YKCANCEL_FAIL;
                }
            }
            else    //yes conf 
            {
                if( App_Layer.CtrlType == CTRL_FUNC_SELECT )
                {
                    LOG_INFO(pRouteInf->GetChnId(), "IEC104:App_RxSPConf Received SP select confirm. \n");//设置点选择确认

                    //设置点预置成功
                    App_Layer.CtrlReply = COMMAND_SELECT_SUCCESS;
                    //App_Layer.State = IEC104_APP_STATE_IDLE;
                    //AckFinished = 1;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_EXECUTE )
                {
                    LOG_INFO(pRouteInf->GetChnId(), "IEC104:App_RxSPConf Received SP exec confirm.\n");//设置点执行确认

                    //设置点执行成功
                    App_Layer.CtrlReply=COMMAND_YKEXEC_SUCCESS;
                    //App_Layer.State = IEC104_APP_STATE_IDLE;
                    //AckFinished = 1;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_CANCEL)
                {
                    App_Layer.CtrlReply = COMMAND_YKCANCEL_SUCCESS;
                }
            }
        }
        else if( (asdu[2]&0x3f) == APP_COT_UNKNOWN_INFOADDR )//未知的信息对象地址
        {
            //未知的遥调号
            App_Layer.CtrlReply=COMMAND_UNKNOWN_YKNO;
        }
        break;
    case IEC104_APP_STATE_WAITSPFINISH:
        if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )
        {
            LOG_INFO(pRouteInf->GetChnId(), "IEC104:App_RxSPConf Received SP exec end. \n");//设置点执行结束
            //设置点执行成功
            App_Layer.CtrlReply=COMMAND_YKEXEC_SUCCESS;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        return; //这里要退出，因为之前遥控执行确认时，已经返回将遥控执行响应消息到上层应用，这里(收到遥控执行结束报文)就不再返回响应消息了。
        break;
    }
    App_Layer.State = IEC104_APP_STATE_IDLE;
    AckFinished = 1;

    QString codeQStr = QString::fromStdString(CMD_TYPE_STR_SP);
    std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, spNo);
    if(nullptr == taskInfoPtr)
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("SP: Not found task! code:%1 no:%2").arg(codeQStr).arg(spNo));
        return;
    }
    if(App_Layer.CtrlReply >=0)
    {
        pCommandMem->SendSpReply(CMD_TYPE_SP, pRouteInf->GetRtuId(), spNo, App_Layer.CtrlReply, taskInfoPtr);
    }    
    pTaskList->removeATask(codeQStr, spNo);
    LOG_INFO(pRouteInf->GetChnId(), "<====SP: remove task, codeQStr:"+ codeQStr +", no:" + QString::number(spNo));
}

/********************************************************************************
*
*    描述: 处理遥控响应应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/
void Iec104::App_RxYkConf( uint8 *asdu, int size )
{
    //LOG_DEBUG(pRouteInf->GetChnId(), "");
    uint32    infnum = asdu[1]&0x7f;
    if(infnum!=1)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "IEC104:App_RxYkConf received inf number!=1 ---\n" );
        return;
    }
    uint16 rtuaddr;
    int j,k;    
    j = 3;
    if( Config_Param.CotNum == 2 )
    {
        j++;
    }
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
#if CFQ
    if( rtuaddr != m_rtuAddr )
    {
        if( pRouteInf->GetDebugFlag() )
            LOG_DEBUG(pRouteInf->GetChnId(), "rtu address is different with current rtu address! rtuaddr:") << rtuaddr <<", current rtuaddr:" << m_rtuAddr;
        return;
    }
#endif
    int ykno = asdu[j+1]*256+asdu[j];
    if( Config_Param.InfAddrNum == 3 )
        ykno += asdu[j+2]*256*256;
    
    //if( ykno < Config_Param.YkBaseAddr || ykno >= (Config_Param.YkBaseAddr+512) )    //限制为512个遥控点
    if(ykno < Config_Param.YkBaseAddr)
    {
        LOG_WARN(pRouteInf->GetChnId(), "Iec104::App_RxYkConfWrong ykno! skip!");
        return;
    }    
    ykno -= Config_Param.YkBaseAddr;

    //遥控反校遥控号、遥控命令限定词
    if( App_Layer.CtrlNo != ykno )
    {
        LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYkConf Wrong CtrlNo! skip! ykno:%1, App_Layer.CtrlNo:%2, Config_Param.YkBaseAddr:%3").arg(ykno).arg(App_Layer.CtrlNo).arg(Config_Param.YkBaseAddr));
        return;
    }

    if( Config_Param.InfAddrNum == 3 )
        k=j+3;
    else
        k=j+2;

    if( asdu[0] == APPTYPE_YK45)//单点遥控
    {
        if(App_Layer.CtrlAttr == CTRL_TYPE_CLOSE && (asdu[k]&0x3) != 1 ||
                App_Layer.CtrlAttr == CTRL_TYPE_TRIP && (asdu[k]&0x3) != 0 )
        {
            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYkConf Wrong data! skip! App_Layer.CtrlAttr=%1,asdu[k]&0x3=%2").arg(App_Layer.CtrlAttr).arg(asdu[k]&0x3));
            return;
        }
    }
    else //双点遥控
    {
        if(App_Layer.CtrlAttr == CTRL_TYPE_CLOSE && (asdu[k]&0x3) != 2 ||
                App_Layer.CtrlAttr == CTRL_TYPE_TRIP && (asdu[k]&0x3) != 1 )
        {
            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYkConf Wrong data! skip! App_Layer.CtrlAttr=%1,asdu[k]&0x3=%2").arg(App_Layer.CtrlAttr).arg(asdu[k]&0x3));
            return;
        }
    }

    if( App_Layer.CtrlType == CTRL_FUNC_SELECT && (asdu[k]&0x80)!= 0x80 ||
            App_Layer.CtrlType == CTRL_FUNC_EXECUTE && (asdu[k]&0x80)!= 0x00 )
    {
        LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYkConf Wrong data! skip! App_Layer.CtrlType=%1,asdu[k]&0x80=%2").arg(App_Layer.CtrlType).arg(asdu[k]&0x80));
        return;
    }

    App_Layer.CtrlReply = -1;
    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_WAITYKCONF:
        if( (asdu[2]&0x3f) == APP_COT_DEACT_CON )
        {
            if(App_Layer.CtrlType == CTRL_FUNC_CANCEL)
            {
                LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYkConf Received YK cancel conf\n");//遥控取消确认

                //遥控撤销成功
                App_Layer.CtrlReply = COMMAND_YKCANCEL_SUCCESS;
            }
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        else if( (asdu[2]&0x3f) == APP_COT_ACT_CON )
        {
            if( (asdu[2]&0x40) == 0x40 )    //neg conf
            {
                LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYkConf Received YK select(execute) neg conf\n");//遥控选择（执行）否认

                //遥控预置失败
                if(App_Layer.CtrlType == CTRL_FUNC_SELECT)
                {
                    App_Layer.CtrlReply = COMMAND_SELECT_FAIL;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_EXECUTE )
                {
                    App_Layer.CtrlReply = COMMAND_YKEXEC_FAIL;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_CANCEL)
                {
                    App_Layer.CtrlReply = COMMAND_YKCANCEL_FAIL;
                }
                App_Layer.State = IEC104_APP_STATE_IDLE;
            }
            else    //yes conf 
            {
                if( App_Layer.CtrlType == CTRL_FUNC_SELECT )
                {
                    LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYkConf Received YK select conf \n");//遥控选择确认

                    //遥控预置成功
                    App_Layer.CtrlReply = COMMAND_SELECT_SUCCESS;
                    App_Layer.State = IEC104_APP_STATE_IDLE;
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_EXECUTE)
                {
                    LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYkConf Received YK execute conf \n");//遥控执行确认

                    //遥控执行成功
                    App_Layer.CtrlReply=COMMAND_YKEXEC_SUCCESS;
                    if(Config_Param.isYkWaitExecFinish)//遥控执行是否需要等待执行结束（遥控执行确认后，还会有一个遥控执行结束报文）
                    {
                        App_Layer.State = IEC104_APP_STATE_WAITYKFINISH;
                    }
                    else
                    {
                        App_Layer.State = IEC104_APP_STATE_IDLE;
                    }
                }
                else if( App_Layer.CtrlType == CTRL_FUNC_CANCEL)
                {
                    App_Layer.CtrlReply = COMMAND_YKCANCEL_SUCCESS;
                    App_Layer.State = IEC104_APP_STATE_IDLE;
                }
            }
            AckFinished = 1;
        }
        else if( (asdu[2]&0x3f) == APP_COT_UNKNOWN_INFOADDR )//未知的信息对象地址
        {
            //未知的遥控号
            App_Layer.CtrlReply=COMMAND_UNKNOWN_YKNO;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        break;
    case IEC104_APP_STATE_WAITYKFINISH:
        if( (asdu[2]&0x3f) == APP_COT_ACT_TERM )
        {
            LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxYkConf Received YK execute terminate  \n");//遥控执行结束

            //遥控执行成功
            App_Layer.CtrlReply=COMMAND_YKEXEC_SUCCESS;
            App_Layer.State = IEC104_APP_STATE_IDLE;
            AckFinished = 1;
        }
        return; //这里要退出，因为之前遥控执行确认时，已经返回将遥控执行响应消息到上层应用，这里(收到遥控执行结束报文)就不再返回响应消息了。
        break;
    default:
        {
            LOG_INFO(pRouteInf->GetChnId(), QString("Iec104::App_RxYkConf, Wrong App_Layer.State: %1 !\n").arg(App_Layer.State));//遥控执行结束
            return;
            break;
        }
    }
    QString codeQStr = QString::fromStdString(CMD_TYPE_STR_YK);
    std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, ykno);
    if(nullptr == taskInfoPtr)
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("YK: Not found task! codeQStr:%1, no:%2").arg(codeQStr).arg(ykno));
        return;
    }
    if(App_Layer.CtrlReply >=0 )
    {
        QString rtuId = pRouteInf->GetRtuId();
        pCommandMem->SendYkYtReply(CMD_TYPE_YK, rtuId, ykno, App_Layer.CtrlAttr, App_Layer.CtrlReply, taskInfoPtr);
    }

    pTaskList->removeATask(codeQStr, ykno);
    LOG_INFO(pRouteInf->GetChnId(), QString("<====YK: remove task, codeQStr:%1, no:%2").arg(codeQStr).arg(ykno));
}

/********************************************************************************
*
*    描述: 处理防误响应应用服务数据单元(ASDU)
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
*   启动字符	0x68
    APDU长度	L	长度<=253
    发送序列号L	N（S）	控制域
    发送序列号H	N（S）
    接收序列号L	N（R）
    接受序列号H	N（R）
    类型标识 ASDU	0xC8 (200)
    可变结构限定词	VSQ	遥控点个数
    传送原因L	07
    传送原因H	00
    公共地址L	ADDR	装置单元地址
    公共地址H	ADDR
    命令识别符1	0x2D	遥控操作
    命令识别符2	CMD1	0：整票(多个)；1：单控(1个)
    分帧识别符	SEQ	见分帧说明
    点号地址1	Addr1	3字节
    校核结果	Data1	00：校验分成功。
    01：校验合成功。
    02：校验分失败。
    04：校验合失败。
    错误码	ErrorData	0x00正确；
    0x01-0x0A保留；
    0x10表示其他错误（例如：整票校核的时候，某一个设备不通过，后续的设备均不允许操作，附带的就是这个错误码）；
    0x11表示设备被其他任务闭锁；
    0x12表示操作状态余当前状态不符合；
    0x13表示校核逻辑不通过；
    0X14表示不存在该设备;
    0x15表示程并控信息不符合。
    点号地址2	Addr2	3字节
    校核结果	Data2	见校核结果描述
    错误码	ErrorData	正确为0，否则，其他错误码
    ……	……	从点号地址开始重复

********************************************************************************/
void Iec104::App_RxWFConf( uint8 *asdu, int size )
{
    //LOG_DEBUG(pRouteInf->GetChnId(), "");
    uint32    infnum = asdu[1]&0x7f;  //遥控点个数
    if(infnum <= 0)
    {
        LOG_ERROR(pRouteInf->GetChnId(), "IEC104:App_RxFWConf received inf number!=1 ---\n" );
        return;
    }
    int ctrlFunc = CTRL_FUNC_WF;
    if(APPTYPE_WF201 == asdu[0])
    {
        ctrlFunc = CTRL_FUNC_YJMS;
    }
    QString codeQStr = QString::fromStdString(CMD_TYPE_STR_WF);
    std::shared_ptr<TaskInfo> taskInfoPtr = pTaskList->getATask(codeQStr, -1);
    if(nullptr == taskInfoPtr)
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("IEC104:App_RxFWConf: Not found task! codeQStr:%1, no:%2").arg(codeQStr).arg(-1));
        return;
    }
    if(taskInfoPtr->dataList.size() <=0)
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("IEC104:App_RxFWConf: Not found 'dataList' in task info! taskId:%1").arg(taskInfoPtr->taskId));
        return;
    }
    uint32 sendReason = asdu[2]&0x3f;  //传送原因：7
    //uint16 rtuaddr;
    //rtuaddr = asdu[4] + asdu[5]*256;//公共地址
    int j,k;
    j = 9;

    QList<FwRspParam_S> fwRspList;
    for(int i=0; i<infnum; i++)
    {
        int fwno = asdu[j+2]*256*256 + asdu[j+1]*256 + asdu[j];
        j+=3;
        if(fwno < Config_Param.FwBaseAddr)
        {
            LOG_WARN(pRouteInf->GetChnId(), "Iec104::App_RxFWConfWrong fwno! skip!");
            continue;
        }
        fwno -= Config_Param.FwBaseAddr;


        //liujie add 20230529 添加防误操作处理 start
        /* 校核结果	Data1
        0x00：校验分成功。
        0x01：校验合成功。
        0x02：校验分失败。
        0x04：校验合失败。
        */
        uint8 checkResult=asdu[j++];
        /*错误码
        0x00正确；
        0x01-0x0A保留；
        0x10表示其他错误（例如：整票校核的时候，某一个设备不通过，后续的设备均不允许操作，附带的就是这个错误码）；
        0x11表示设备被其他任务闭锁；
        0x12表示操作状态余当前状态不符合；
        0x13表示校核逻辑不通过；
        0X14表示不存在该设备;
        0x15表示程并控信息不符合。
        */
        uint8 errCode=asdu[j++];
        QString errMsg="";
        if(0 != errCode)
        {
            //errMsg = QString("错误码：%1, 错误原因：%2 ").arg(errCode).arg( ConverterUtil::toFwErrMsg(errCode));
            errMsg = QString("%1").arg( ConverterUtil::toFwErrMsg(errCode));
        }
        if(asdu[0] == 0xC8)
        {
            if(checkResult != 0x00 && checkResult != 0x01 && checkResult != 0x02 && checkResult!= 0x04)
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxWFJSConf Wrong data! skip! asdu[0]=%1,checkResult=%2").arg(asdu[0]).arg(checkResult));
                continue;
            }
        }
        else if(asdu[0] == 0xC9)
        {
            if(checkResult != 0x00 && checkResult != 0x01 && checkResult != 0x02 && checkResult != 0x04)
            {
                LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYJMSConf Wrong data! skip! asdu[0]=%1,checkResult=%2").arg(asdu[0]).arg(checkResult));
                continue;
            }
        }
        else
        {
            LOG_WARN(pRouteInf->GetChnId(), QString("Iec104::App_RxYJMSConf Wrong data! skip! asdu[0]=%1").arg(asdu[0]));
            continue;
        }

        //liujie add 20230529 添加防误操作处理 end
        for(int m=0; m<taskInfoPtr->dataList.size(); m++)
        {
            std::shared_ptr<FWReqParam_S> data = std::static_pointer_cast<FWReqParam_S>(taskInfoPtr->dataList.at(m));;
            if(fwno == data->no)
            {
                FwRspParam_S rspData;
                rspData.rtuId = data->rtuId;
                rspData.cchId = data->cchId;
                rspData.no = data->no;
                rspData.type = data->type;
                rspData.value = data->value;
                rspData.reason = "";
                rspData.result = CMD_RESULT_FAIL;

                if(sendReason == APP_COT_ACT_CON)//传送原因：7
                {

                    if( (asdu[2]&0x40) == 0x40 )//neg conf 否定应答
                    {
                        rspData.result = CMD_RESULT_FAIL;
                        rspData.reason = "防误校验拒绝！";
                    }
                    else//yes conf 肯定应答
                    {
                        /*
                         * 校核结果:
                         * 00：校验分成功
                           01：校验合成功
                           02：校验分失败
                           04：校验合失败
                         */
                        if((0x02 == checkResult || 0x04 == checkResult)) //--失败
                        {
                            //LOG_INFO(pRouteInf->GetChnId(), "Iec104::App_RxFWConf Received wf neg conf\n");//防误否认
                            //liujie edit 20230530 添加应急模式和防误操作响应
                            rspData.result = CMD_RESULT_FAIL;
                            if(ctrlFunc == CTRL_FUNC_WF)
                            {
                                rspData.reason = (0x04 == checkResult? "防误模式校验:合-失败！":"防误模式校验:分-失败！");
                            }
                            else
                            {
                                rspData.reason = (0x04 == checkResult? "应急模式校验:合-失败！":"应急模式校验:分-失败！");
                            }
                        }
                        else if((0x00 == checkResult || 0x01 == checkResult))  //--成功
                        {
                            rspData.result = CMD_RESULT_SUCCESS;
                            if(ctrlFunc == CTRL_FUNC_WF)
                            {
                                rspData.reason = (0x01 == checkResult? "防误模式校验:合-成功！":"防误模式校验:分-成功！");
                            }
                            else
                            {
                                rspData.reason = (0x01 == checkResult? "应急模式校验:合-成功！":"应急模式校验:分-成功！");
                            }
                        }
                    }
                }
                else if( (asdu[2]&0x3f) == APP_COT_UNKNOWN_INFOADDR )//未知的信息对象地址
                {
                    //未知的遥控号
                    LOG_WARN(pRouteInf->GetChnId(), "Unknown info addr!");
                    rspData.result = CMD_RESULT_FAIL;
                    rspData.reason = "未知的信息对象地址";
                }
                if(!errMsg.isEmpty())
                {
                    rspData.reason +=errMsg;
                }

                fwRspList.append(rspData);
                break;
            }
        }
    }
    pCommandMem->SendFWReply(CMD_TYPE_WF, pRouteInf->GetRtuId(), taskInfoPtr->taskId, fwRspList);

    pTaskList->removeATask(codeQStr, -1);
    LOG_INFO(pRouteInf->GetChnId(), QString("<====FW: remove task, codeQStr:%1, no:%2").arg(codeQStr).arg(-1));

    AckFinished = 1;
    switch( App_Layer.State )
    {
    case IEC104_APP_STATE_WAITFWCONF:
        App_Layer.State = IEC104_APP_STATE_IDLE;
        break;
    default:
        {
            LOG_INFO(pRouteInf->GetChnId(), QString("Iec104::App_RxYkConf, Wrong App_Layer.State: %1 !\n").arg(App_Layer.State));//遥控执行结束
            return;
            break;
        }
    }

}



/********************************************************************************
*
*    描述: 解码收到的遥测值
*    参数: buf  遥测数据.
*    返回: 遥测值.
*
********************************************************************************/
sint32 Iec104::App_YcDecode( uint8 *buf )
{
    sint32 ycvalue;

/*
    uint32 yccode;
    yccode = buf[1]*256+buf[0];
    
    if( yccode & 0x8000 )
        ycvalue = -( yccode&0x7fff);
    else ycvalue = yccode;
*/
    ycvalue = (sint16)(buf[1]*256+buf[0]);

    return ycvalue;
}

/********************************************************************************
*
*    描述: 解码收到的电度值 
*    参数: buf  电度数据.
*    返回: 电度值.
*
********************************************************************************/
sint32 Iec104::App_KwhDecode( uint8 *buf )
{
    sint32 kwhvalue;
    uint32 kwhcode;

    kwhcode = ( (buf[3]*256+buf[2])*256 + buf[1] )*256 + buf[0];

    if( kwhcode & 0x80000000 )
        kwhvalue = -( kwhcode&0x7fffffff );
    else kwhvalue = kwhcode;

    return kwhvalue; 
}


/********************************************************************************
*
*    描述: 将DA上送的时间（四字节分钟，两字节毫秒〕转化为FETIME表示的时间
*    参数: ptr    输入的时间数据
*          atime  得到的FETIME结构的时间数据.
*    返回: 无.
*
********************************************************************************/

void Iec104::MakeDAReportTime(uint8 *ptr , FETIME * atime)
{
  uint32 tm_min;
  time_t tm_sec;
  uint16 tm_ms;
  uint8 tmp0,tmp1,tmp2,tmp3;
  struct tm *tim;

  tmp0 = *ptr;
  tmp1 = *(ptr+1);
  tmp2 = *(ptr+2);
  tmp3 = *(ptr+3);
  tm_min = (uint32)tmp3*256*256*256 +
           (uint32)tmp2*256*256 +
           (uint32)tmp1*256 +
           (uint32)tmp0;
  tm_ms = (uint16)*(ptr+4) + (uint16)*(ptr+5)*256;
  tm_sec =(time_t) (tm_min*60 + tm_ms/1000 - 8*3600);
  tim = localtime(&tm_sec);
  atime->aMs = tm_ms%1000;
  atime->aSec = tim->tm_sec;
  atime->aMin = tim->tm_min;
  atime->aHour = tim->tm_hour;
  atime->aDay = tim->tm_mday;
  atime->aMon = tim->tm_mon +1;
  atime->aYear = tim->tm_year;
}

/********************************************************************************
*
*    描述: 处理DA故障处理结果应用服务数据单元(ASDU) 
*    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
*          size  应用服务数据单元(ASDU) 的长度
*    返回: 无.
*
********************************************************************************/

void Iec104::App_RxDAResult( uint8 *asdu, int size )
{
      int    i,j,k;
      uint8    danum = asdu[1]&0x7f;    //BYTE sq=DataHead.VSQ.SQ;
      uint8    cause = asdu[2]&0x3f;
      uint16    rtuaddr;
      
      if( danum==0 ) 
          return;
    
    j = 4;
    
    if( Config_Param.ComAddrNum == 2 )
        rtuaddr = asdu[3] + asdu[j++]*256;
    else    rtuaddr = asdu[3];

    //注：暂时不考虑是子站报文还是FTU报文，即暂时不判断公共地址域内容
    //if( rtuaddr != pRtuInf->GetRtuAddr( rtuId ) )
    //    return;
    
    
    if( pRouteInf->GetDebugFlag() )
        LOG_DEBUG(pRouteInf->GetChnId(), "IEC104 Received DA fault handle result message.\n");//故障处理结果报文

    //BYTE * buff=DSD[StaNo].A_ReadBuff;
    //hxh:特别注意：公共站址在此处FUCKEDSUB定为FTU的实际站址，所以所有接收处理函数都要改变
    //    WORD FTUNo = FTUAddrToNo(addr,ChNo);//hxh:此处是站址到站号的转换函数
    //    if(FTUNo == 0xffff) return;
    //    if(!C104_RequestMemory(StaNo,FTUNo)) return;
        
    SCDS_FAULTDEAL* faultdeal = new SCDS_FAULTDEAL[danum];
    //Scadaservice dasrv;
    FETIME atime;
    for(i=0;i<danum;i++)
    {
        //(faultdeal+i)->RTUNo = StaNo;
        MakeDAReportTime(&asdu[50*i+j],&atime);
        (faultdeal+i)->Year = atime.aYear + 1900; 
        (faultdeal+i)->Month= atime.aMon;
        (faultdeal+i)->Day = atime.aDay;
        (faultdeal+i)->Hour = atime.aHour;
        (faultdeal+i)->Minute = atime.aMin;
        (faultdeal+i)->Second = atime.aSec;
        (faultdeal+i)->Ms = atime.aMs; 
        (faultdeal+i)->DiagnoseStyle = asdu[50*i+j+6];
        (faultdeal+i)->ValidFlag = asdu[50*i+j+7];
        (faultdeal+i)->FailStep = asdu[50*i+j+8];
        (faultdeal+i)->FailReason = asdu[50*i+j+9];
        (faultdeal+i)->StartBreakId = asdu[50*i+j+10]; + asdu[50*i+j+11]*256 ; 
        (faultdeal+i)->EndBreakId =  asdu[50*i+j+12] + asdu[50*i+j+13]*256 ; 
        (faultdeal+i)->IslationBreakNum = asdu[50*i+j+14];
        (faultdeal+i)->IslationSuccessFlag = asdu[50*i+j+15];
        for(k=0;k<8;k++)
        {
          (faultdeal+i)->IslationBreakId[k] = asdu[50*i+j+16+k*2] + asdu[50*i+j+17+k*2]*256; 
        }
        (faultdeal+i)->RestoreBreakNum = asdu[50*i+j+32];
        (faultdeal+i)->RestoreSuccessFlag =  asdu[50*i+j+33];
        for(k=0;k<8;k++)
        {
          (faultdeal+i)->RestoreBreakId[k] = asdu[50*i+j+34+k*2] + asdu[50*i+j+35+k*2]*256 ;
        }
    }
    //dasrv.fault_deal_report("",faultdeal,number);
    delete faultdeal;
}

uint32 Iec104::App_YcQcExchange(uint8 source_qc)
{
        uint32 status = 0;

        if(source_qc&1)//OV
                status |= SICD_M_AI_STS_OV;

        if(source_qc&0x10)//BL
                status |= SICD_M_AI_STS_BL;

        if(source_qc&0x20)//SB
                status |= SICD_M_AI_STS_SB;

        if(source_qc&0x40)//NT
                status |= SICD_M_AI_STS_NT;

        if(!(source_qc&0x80))//IN
                status |= SICD_M_AI_STS_ON_LINE;

        return status;
}

uint32 Iec104::App_YxQcExchange(uint8 source_qc,bool dpflag,uint8    *yxvalue)
{
        uint32 status = 0;

        if(source_qc&0x10)//BL
                status |= SICD_M_DI_STS_BL;

        if(source_qc&0x20)//SB
                status |= SICD_M_DI_STS_SB;

        if(source_qc&0x40)//NT
                status |= SICD_M_DI_STS_NT;

        if(!(source_qc&0x80))//IN
                status |= SICD_M_DI_STS_ON_LINE;

        if(dpflag && yxvalue)
        {
            if((*yxvalue&0x03)==0x02)
                    *yxvalue = 1;
            else
            {
/* wuhaiyan delete start 20230509:实时双位遥信另行处理 */
#if 0

                    if((*yxvalue&0x03)==0x00)
                        status |= SICD_M_DI_STS_TRANSITONE;
                    if((*yxvalue&0x03)==0x03)
                        status |= SICD_M_DI_STS_TRANSITTWO;
#else
/* wuhaiyan delete end 20230509*/

/* wuhaiyan add start 20230509 */
/*
对于双位遥信的特殊处理：【 from yangshaowei 】
104规约的双位遥信的上送状态：
如果品质位为1（既 source_qc&0x80为真 ）则 status=0:
上送报文：80 -- 状态=0 --表示无效的中间态，上送结果为 status=0，v=0
上送报文：81 -- 状态=1 --表示无效的分，    上送结果为 status=0，v=0；
上送报文：82 -- 状态=2 --表示无效的合，    上送结果为 status=0，v=1；
如果品质位为0（既 source_qc&0x80为假 ）则如下：
上送报文：00 -- 状态=0 --表示有效的中间态，上送结果为status=2，v=0；
上送报文：01 -- 状态=1 --表示有效的分，    上送结果为status=1，v=0；
上送报文：02 -- 状态=2 --表示有效的合，    上送结果为status=1，v=1。
*/
                if((source_qc&0x80))
                    status = 0;  //代表无效
                else if(((*yxvalue&0x03)==0x00) || ((*yxvalue&0x03)==0x03))
                    status = 2; //代表中间态
#endif
/* wuhaiyan add end 20230509 */

                    *yxvalue = 0;
            }
        }


        if( 0 == Config_Param.UseQuality )
            status = SICD_M_DI_STS_ON_LINE;

        return status;
}

uint32 Iec104::App_KwhQcExchange(uint8 source_qc)
{
        uint32 status = 0;

        if(!(source_qc&0x80))//IN
                status |= SICD_M_CO_STS_ON_LINE;

        return status;
}

/************************************************************************
作者：   yay     
创建日期：2011.11.10
函数功能说明： 规约解析处理函数
输入参数：
             uint8 dir,    报文上下行方向
             uint8 *inbuf,   报文首地址
             int inlen,      报文长度
             char* &outbuf,    翻译结果首地址
             uint8 frametype    需要翻译的帧类型
输出参数：  
             无
            
返回: = 1, 解析成功返回;
      =-1, 解析未成功返回.
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/

int Iec104::TransProc(uint8 dir,uint8 *inbuf,int inlen,char* &outbuf,uint8 frametype)
{
    if( TransOutBuf )
        TransOutBuf[0] = 0;

    /*由于要用于优特104规约的翻译所以地址设置成1997*/
    Config_Param.YKCmdMode = 2;        //遥控命令使用(1:单点命令)还是(2:双点命令)
    Config_Param.SPCmdMode = 1;        //设点命令使用(1:单个设点命令)还是(2:连续设点命令)
    Config_Param.SOETransMode = 2;    //SOE是(1:一次传输,要根据SOE生成YX)还是(2:二次传输)
    Config_Param.CotNum = 2;            //ASDU传输原因字节数
    Config_Param.ComAddrNum = 2;        //ASDU公共地址字节数
    Config_Param.InfAddrNum = 3;        //ASDU信息体地址字节数
    Config_Param.InfBaseAddrVersion = 1997;        //ASDU信息体基地址版本号
    Config_Param.YxBaseAddr = 0x1;          //ASDU状态量起始地址
    Config_Param.YcBaseAddr = 0x701;        //ASDU模拟量起始地址
    Config_Param.YkBaseAddr = 0xb01;        //ASDU控制量起始地址
    Config_Param.YkLastAddr = 0xbff;        //ASDU控制量结束地址
    Config_Param.SpBaseAddr = 0xb81;        //ASDU调节量起始地址
    //Config_Param.SpLastAddr = 0xc00;        //ASDU调节量结束地址
    Config_Param.YtBaseAddr = 0xc81;        //ASDU分接头起始地址
    Config_Param.YtLastAddr = 0xca0;        //ASDU分接头结束地址
    Config_Param.DdBaseAddr = 0xc01;      //ASDU电度量起始地址
    Config_Param.MaxIframeNum = 8;    //最后确认的I帧最大数目
    Config_Param.OverTime1 = 12;//15;    //I帧或U帧发送后,经过OverTime1超时
    Config_Param.OverTime2 = 10;    //收到I帧后,经过OverTime2无数据报文时,发送S帧,OverTime2<OverTime1
    Config_Param.OverTime3 = 14;//20;    //长期空闲,经过OverTime3超时,发送测试帧,OverTime3>OverTime1

/**********************开始解析************************/
    char desctmp[100];
    outbuf = NULL;
    int timestamplen;
    int framelen;

    /*解析方向*/
    if( dir == DIR_UP )
        WriteTransOutBuf("接收:");
    else
        WriteTransOutBuf("发送:");

    /*解析时标*/
    timestamplen = inbuf[0];
    memcpy( desctmp, inbuf+1, timestamplen );
    desctmp[timestamplen] = 0;
    WriteTransOutBuf(desctmp);

    int i = 0;
    for(i = 0; i<inlen-timestamplen-1; i++ )
    {
        sprintf( desctmp," %02X", inbuf[i+timestamplen+1] );
        if( i%20 == 0 )
            WriteTransOutBuf("\r\n");
    WriteTransOutBuf(desctmp);
    }
    WriteTransOutBuf("\r\n");

    uint8 *startchar = &inbuf[ timestamplen+1 ];
    framelen = inbuf[ timestamplen+2 ];
    if( *startchar == 0x68 )
    {
        if( (framelen >= 254) || (framelen <= 3))
        {
            LOG_ERROR(pRouteInf->GetChnId(), QString("IEC104 TRANS ERROR: framelen = %1\n ").arg(framelen));
            return -1;
        }
        if( (frametype != TRANS_FRAME_ALL) && (framelen == 4) && (frametype != TRANS_FRAME_LINK))
            return -1;
        TransLpduLayer(startchar,frametype);
        if( framelen != 4 )
            TransApduLayer(startchar,frametype);
    }
    else
    {
        LOG_ERROR(pRouteInf->GetChnId(), QString("IEC104 TRANS ERROR:Unrecongnized start char\n "));
        return -1;
    }

    WriteTransOutBuf("\r\n");
/******************************************************/

    //返回解析结果
    if( TransOutBuf && (0 != TransOutBuf[0]) )
    {
        outbuf = TransOutBuf;
        return 1;
    }
    else
        return -1;
}

int Iec104::TransLpduLayer( uint8 *inbuf,uint8 frametype )
{
    char desctmp[100];
    uint8 ctrlfld1 = inbuf[2];
    uint8 ctrlfld2 = inbuf[3];
    uint8 ctrlfld3 = inbuf[4];
    uint8 ctrlfld4 = inbuf[5];

    if(ctrlfld1&0x1)
    {
        if(ctrlfld1&0x2)
        {
            WriteTransOutBuf("U 格式");
            if(ctrlfld1&APP_UFORMAT_STARACT)
                WriteTransOutBuf(",START命令\r\n");
            if(ctrlfld1&APP_UFORMAT_STARCON)
                WriteTransOutBuf(",START确认\r\n");
            if(ctrlfld1&APP_UFORMAT_STOPACT)
                WriteTransOutBuf(",STOP命令\r\n");
            if(ctrlfld1&APP_UFORMAT_STOPCON)
                WriteTransOutBuf(",STOP确认\r\n");
            if(ctrlfld1&APP_UFORMAT_TESTACT)
                WriteTransOutBuf(",TEST命令\r\n");
            if(ctrlfld1&APP_UFORMAT_TESTCON)
                WriteTransOutBuf(",TEST确认\r\n");
        }
        else
        {
            WriteTransOutBuf("S 格式");
            int recvnum = ctrlfld3/2+ctrlfld4*128;
            sprintf(desctmp,",接收序列号:%d\r\n",recvnum);
            WriteTransOutBuf(desctmp);
        }
    }
    else
    {
        WriteTransOutBuf("I 格式");
        int sendnum = ctrlfld1/2+ctrlfld2*128;
        int recvnum = ctrlfld3/2+ctrlfld4*128;
        sprintf(desctmp,",发送序列号:%d,接收序列号:%d\r\n",sendnum,recvnum);
        WriteTransOutBuf(desctmp);
    }
    return 1;
}

int Iec104::TransApduLayer( uint8 *inbuf,uint8 frametype )
{
    char desctmp[100];
    uint8 *asdu = inbuf+6;
    int size = inbuf[1];
    
    sprintf(desctmp,"类型标识-%d",asdu[0]);
    WriteTransOutBuf(desctmp);
    
    if(size <= 4 )return -1;
    size -= 4;
    
    switch(*asdu)
    {
        case APPTYPE_CALLDATA:
            if(frametype != TRANS_FRAME_ALL )
                return -1;
            TransAllDataConf( asdu, size );
            break;
        case APPTYPE_CALLKWH:
            if(frametype != TRANS_FRAME_ALL )
                return -1;
            TransAllKwhConf( asdu, size );
            break;
        case APPTYPE_TIMESYNC:
            if(frametype != TRANS_FRAME_ALL )
                return -1;
            TransTimeConf( asdu, size );
            break;
        case APPTYPE_YK45:
        case APPTYPE_YK46:
            if((frametype != TRANS_FRAME_ALL) && (frametype != TRANS_FRAME_YK))
                return -1;
            TransYkConf( asdu, size );
            break;
        case APPTYPE_YT47:
            if(frametype != TRANS_FRAME_ALL )
                return -1;
            break;
        case APPTYPE_SP48:
            if(frametype != TRANS_FRAME_ALL )
                return -1;
            TransSetPointConf( asdu, size );
            break;
        case APPTYPE_SP_NT:
        case APPTYPE_DP_NT:
        case APPTYPE_PS_STS_NT:
            if((frametype != TRANS_FRAME_ALL) && (frametype != TRANS_FRAME_YX))
                return -1;
            TransYxFrame( asdu, size );
            break;
        case APPTYPE_SP_WT:
        case APPTYPE_SP_WT30:
        case APPTYPE_DP_WT31:
            if((frametype != TRANS_FRAME_ALL) && (frametype != TRANS_FRAME_SOE))
                return -1;
            TransSoeFrame( asdu, size );
            break;
        case APPTYPE_ME_NT:
        case APPTYPE_ME_WT:
        case APPTYPE_ME_ND:
        case APPTYPE_ME_NT11:
        case APPTYPE_ME_FLOAT:
        case APPTYPE_ME_TD:
        case APPTYPE_ME_TE:    
            if((frametype != TRANS_FRAME_ALL) && (frametype != TRANS_FRAME_YC))
                return -1;
            TransYcFrame( asdu, size );
            break;
        case APPTYPE_CO_NT:
        case APPTYPE_CO_WT:
            if((frametype != TRANS_FRAME_ALL) && (frametype != TRANS_FRAME_KWH))
                return -1;
            TransKwhFrame( asdu, size );
            break;
        default:
            WriteTransOutBuf("不支持的类型标识\r\n");
            break;
    }

    WriteTransOutBuf("\r\n");
    return 1;
}

void Iec104::TransAllDataConf( uint8 *asdu, int size )
{
    char desc[100];
    
    uint16 rtuaddr = 0;
    uint32 infoaddr = 0;
    
  int pos = 3;

  sprintf(desc,",原因码-%d",(asdu[2]&0x3f));    
    WriteTransOutBuf(desc);
        
    if( Config_Param.CotNum == 2 )        
        pos++;
    
    if( Config_Param.ComAddrNum == 2 )
    {    
         rtuaddr = asdu[pos]+256*asdu[pos+1];
        pos += 2;
    }
    else
    {
        rtuaddr = asdu[pos];
        pos++;
    }        
    sprintf(desc,",公共地址码-%d",rtuaddr);    
    WriteTransOutBuf(desc);
                
    if( Config_Param.InfAddrNum == 3 )
    {
        infoaddr = asdu[pos]+256*asdu[pos+1]+256*256*asdu[pos+2];    
        pos += 3;
    }    
    else
    {
        infoaddr = asdu[pos]+256*asdu[pos+1];    
        pos +=2;    
    }
        
    sprintf(desc,",信息体地址-%d",infoaddr);    
    WriteTransOutBuf(desc);    

    if( asdu[pos] != 20 )
    {
        sprintf(desc,",分组召唤第%d组",asdu[pos]-20);
        WriteTransOutBuf(desc);
    }
    else
        WriteTransOutBuf(",总召唤");

    uint8 cot = (asdu[2]&0x3f);
    switch(cot)
    {
    case APP_COT_ACT:
        WriteTransOutBuf("激活");
        break;
    case APP_COT_ACT_CON:
        WriteTransOutBuf("激活确认");
        break;
    case APP_COT_DEACT:
        WriteTransOutBuf("停止激活");
        break;
    case APP_COT_DEACT_CON:
        WriteTransOutBuf("停止激活确认");
        break;
    case APP_COT_ACT_TERM:
        WriteTransOutBuf("激活终止");
        break;
    case APP_COT_UNKNOWN_TYPE:
        WriteTransOutBuf("未知的类型标识");
        break;
    case APP_COT_UNKNOWN_COT:
        WriteTransOutBuf("未知的传送原因");
        break;
    case APP_COT_UNKNOWN_COMADDR:
        WriteTransOutBuf("未知的公共地址");
        break;
    case APP_COT_UNKNOWN_INFOADDR:
        WriteTransOutBuf("未知的信息对象地址");
        break;
    default:
        WriteTransOutBuf("不支持的原因码");
        break;
    }
}

void Iec104::TransAllKwhConf( uint8 *asdu, int size )
{
    char desc[100];
    int pos = 6;
    
    if( Config_Param.CotNum == 2 )
        pos++;
    if( Config_Param.ComAddrNum == 2 )
        pos++;
    if( Config_Param.InfAddrNum == 3 )
        pos++;

    uint8 rqt = (asdu[pos]&0x3f);
    uint8 frz = ((asdu[pos]>>7)&0xc0);
    if(rqt == 5)
        WriteTransOutBuf("全电度");
    else
    {
        if( (rqt >= 1) && (rqt <= 4) )
        {
            sprintf(desc,"第%d组电度",rqt);
            WriteTransOutBuf(desc);
        }
        else
            WriteTransOutBuf("未定义RQT");
    }

    uint8 cot = (asdu[2]&0x3f);
    switch(cot)
    {
    case APP_COT_ACT:
        WriteTransOutBuf("激活");
        break;
    case APP_COT_ACT_CON:
        WriteTransOutBuf("激活确认");
        break;
    case APP_COT_DEACT:
        WriteTransOutBuf("停止激活");
        break;
    case APP_COT_DEACT_CON:
        WriteTransOutBuf("停止激活确认");
        break;
    case APP_COT_ACT_TERM:
        WriteTransOutBuf("激活终止");
        break;
    case APP_COT_UNKNOWN_TYPE:
        WriteTransOutBuf("未知的类型标识");
        break;
    case APP_COT_UNKNOWN_COT:
        WriteTransOutBuf("未知的传送原因");
        break;
    case APP_COT_UNKNOWN_COMADDR:
        WriteTransOutBuf("未知的公共地址");
        break;
    case APP_COT_UNKNOWN_INFOADDR:
        WriteTransOutBuf("未知的信息对象地址");
        break;
    default:
        WriteTransOutBuf("不支持的原因码");
        break;
    }
}

void Iec104::TransTimeConf( uint8 *asdu, int size )
{
    char desc[100];
    int    pos = 6;

    if( Config_Param.CotNum == 2 )
        pos++;
    if( Config_Param.ComAddrNum == 2 )
        pos++;
    if( Config_Param.InfAddrNum == 3 )
        pos++;

    if(asdu[0] == APPTYPE_TIMESYNC)
        WriteTransOutBuf(",时钟同步");
    else
        WriteTransOutBuf(",时延获取");

    uint8 cot = asdu[2]&0x3f;
    switch(cot)
    {
    case APP_COT_ACT:
        WriteTransOutBuf("激活");
        break;
    case APP_COT_ACT_CON:
        WriteTransOutBuf("激活确认");
        break;
    case APP_COT_ACT_TERM:
        WriteTransOutBuf("激活终止");
        break;
    case APP_COT_UNKNOWN_TYPE:
        WriteTransOutBuf("未知的类型标识");
        break;
    case APP_COT_UNKNOWN_COT:
        WriteTransOutBuf("未知的传送原因");
        break;
    case APP_COT_UNKNOWN_COMADDR:
        WriteTransOutBuf("未知的公共地址");
        break;
    case APP_COT_UNKNOWN_INFOADDR:
        WriteTransOutBuf("未知的信息对象地址");
        break;
    default:
        WriteTransOutBuf("不支持的原因码");
        break;
    }

    if(asdu[0] == APPTYPE_TIMESYNC)
    {
        SYSTIME curtime;
        curtime.ms = (asdu[pos+1]*256+asdu[pos])%1000;
        curtime.second = (asdu[pos+1]*256+asdu[pos])/1000;
        curtime.minute = asdu[pos+2]&0x3f;
        curtime.hour = asdu[pos+3]&0x1f;
        curtime.day = asdu[pos+4]&0x1f;
        curtime.month = (asdu[pos+5]&0x0f);
        curtime.year = (asdu[pos+6]&0x7f)+2000;
        sprintf(desc,"%d-%d-%d %d:%d:%d:%d",curtime.year,curtime.month,curtime.day,
                                curtime.hour,curtime.minute,curtime.second,curtime.ms);
        WriteTransOutBuf(",时标:");
        WriteTransOutBuf(desc);
    }
    else
    {
        SYSTIME curtime;
        curtime.ms = (asdu[pos+1]*256+asdu[pos])%1000;
        curtime.second = (asdu[pos+1]*256+asdu[pos])/1000;
        sprintf(desc,"%d秒%d毫秒",curtime.second,curtime.ms);
        WriteTransOutBuf(",时延:");
        WriteTransOutBuf(desc);
    }
}

void Iec104::TransYkConf( uint8 *asdu, int size )
{
    char desc[100];
    int j = 3,k;        
    uint16 rtuaddr = 0;
//    TIME_SEQ_VAL one_raw_data;

    sprintf(desc,",原因码-%d",(asdu[2]&0x3f));    
    WriteTransOutBuf(desc);
        
    if( Config_Param.CotNum == 2 )
    {    
        j++;
    }
        
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
    sprintf(desc,",公共地址码-%d",rtuaddr);
    WriteTransOutBuf(desc);
      
    if(asdu[0] == APPTYPE_YK46)
        WriteTransOutBuf(",双点遥控命令");
    else
        WriteTransOutBuf(",单点遥控命令");

    uint8 cot = (asdu[2]&0x3f);
    switch(cot)
    {
    case APP_COT_ACT:
        WriteTransOutBuf("激活");
        break;
    case APP_COT_ACT_CON:
        WriteTransOutBuf("激活确认");
        break;
    case APP_COT_DEACT:
        WriteTransOutBuf("停止激活");
        break;
    case APP_COT_DEACT_CON:
        WriteTransOutBuf("停止激活确认");
        break;
    case APP_COT_ACT_TERM:
        WriteTransOutBuf("激活终止");
        break;
    case APP_COT_UNKNOWN_TYPE:
        WriteTransOutBuf("未知的类型标识");
        break;
    case APP_COT_UNKNOWN_COT:
        WriteTransOutBuf("未知的传送原因");
        break;
    case APP_COT_UNKNOWN_COMADDR:
        WriteTransOutBuf("未知的公共地址");
        break;
    case APP_COT_UNKNOWN_INFOADDR:
        WriteTransOutBuf("未知的信息对象地址");
        break;
    default:
        WriteTransOutBuf("不支持的原因码");
        break;
    }

    int ykno = asdu[j+1]*256+asdu[j];
    if( Config_Param.InfAddrNum == 3 )
        ykno += asdu[j+2]*256*256;

/*    if( (ykno < Config_Param.YkBaseAddr) || (ykno > Config_Param.YkLastAddr) )
    {
        WriteTransOutBuf(",遥控号越界");
        return;
    }*/

    ykno -= Config_Param.YkBaseAddr;
    sprintf(desc,",遥控号%d",ykno);
    WriteTransOutBuf(desc);
    
    if( Config_Param.InfAddrNum == 3 )
        k = j+3;
    else
        k = j+2;

    if(asdu[0] == APPTYPE_YK46)
    {
        if( (asdu[k]&0x3) == 2)
            WriteTransOutBuf("控合");
        else if( (asdu[k]&0x3) == 1 )
            WriteTransOutBuf("控分");
        else
        {
            WriteTransOutBuf("控合控分状态错");
            return;
        }
    }
    else
    {
        if( (asdu[k]&0x3) == 1)
            WriteTransOutBuf("控合");
        else if( (asdu[k]&0x3) == 0 )
            WriteTransOutBuf("控分");
        else
        {
            WriteTransOutBuf("控合控分状态错");
            return;
        }
    }            

    if ( (asdu[k]&0x80) == 0x80 )
        WriteTransOutBuf(",预置");
    else
        WriteTransOutBuf(",执行");

/*        
    if(GET_RAWDATA == TransFlag)
    {
        if((ykno == RawDatano) && (GET_RAWDATA_YK == RawDataType))
        {
            TIME_SEQ_VAL one_raw_data;
                
            memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
            one_raw_data.dir = trans_dir;
                
            one_raw_data.ykvalue.oper = 0;
            if((asdu[k]&0x80) == 0x80)
                one_raw_data.ykvalue.oper = 1;
            else
                one_raw_data.ykvalue.oper = 2;
            
            if(APP_COT_DEACT == cot)
                one_raw_data.ykvalue.oper = 4;
                        
            one_raw_data.ykvalue.value = 0;
            if( (asdu[k]&0x3) == 1)
                one_raw_data.ykvalue.value = 1;
            else if( (asdu[k]&0x3) == 0 )
                one_raw_data.ykvalue.value = 2;
                                
            if((APP_COT_ACT_CON == cot) || (APP_COT_DEACT_CON == cot) || (APP_COT_ACT_TERM == cot))                            
                one_raw_data.ykvalue.result = 0;
            else
                one_raw_data.ykvalue.result = 1;    
                
            WriteOneRawData(one_raw_data,max_rawdata_num);
        }        
    }    
*/                    
}

void Iec104::TransSetPointConf( uint8 *asdu, int size )
{
    char desc[100];
    int j = 4,k;

    if( Config_Param.CotNum == 2 )
        j++;
    if( Config_Param.ComAddrNum == 2 )
        j++;

    WriteTransOutBuf(",遥调命令");

    uint8 cot = (asdu[2]&0x3f);
    switch(cot)
    {
    case APP_COT_ACT:
        WriteTransOutBuf("激活");
        break;
    case APP_COT_ACT_CON:
        WriteTransOutBuf("激活确认");
        break;
    case APP_COT_DEACT:
        WriteTransOutBuf("停止激活");
        break;
    case APP_COT_DEACT_CON:
        WriteTransOutBuf("停止激活确认");
        break;
    case APP_COT_ACT_TERM:
        WriteTransOutBuf("激活终止");
        break;
    case APP_COT_UNKNOWN_TYPE:
        WriteTransOutBuf("未知的类型标识");
        break;
    case APP_COT_UNKNOWN_COT:
        WriteTransOutBuf("未知的传送原因");
        break;
    case APP_COT_UNKNOWN_COMADDR:
        WriteTransOutBuf("未知的公共地址");
        break;
    case APP_COT_UNKNOWN_INFOADDR:
        WriteTransOutBuf("未知的信息对象地址");
        break;
    default:
        WriteTransOutBuf("不支持的原因码");
        break;
    }

    int ytno = asdu[j+1]*256+asdu[j];
    if( Config_Param.InfAddrNum == 3 )
        ytno += asdu[j+2]*256*256;

    //if( (ytno < Config_Param.SpBaseAddr) || (ytno > Config_Param.SpLastAddr) )
    if( (ytno < Config_Param.SpBaseAddr))
    {
        WriteTransOutBuf(",遥调号越界");
        return;
    }

    ytno -= Config_Param.SpBaseAddr;
    sprintf(desc,",遥调号%d",ytno);
    WriteTransOutBuf(desc);
    
    if( Config_Param.InfAddrNum == 3 )
        k = j+3;
    else
        k = j+2;
    
    sprintf(desc,",遥调值=%d",asdu[k]+256*asdu[k+1]);
    WriteTransOutBuf(desc);    
}

void Iec104::TransYxFrame( uint8 *asdu, int size )
{
    uint32    num = (asdu[1]&0x7f);
    uint8    seqflag = (asdu[1]&0x80);
    uint32    yxno,i,j = 3;
    uint8    yxvalue;
    char desc[200];
    uint16 rtuaddr = 0;
//    TIME_SEQ_VAL one_raw_data;

    sprintf(desc,",原因码-%d",(asdu[2]&0x3f));    
    WriteTransOutBuf(desc);
        
    if( Config_Param.CotNum == 2 )
    {    
        j++;
        size--;
    }
        
    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;            
        j += 2;
        size--;
    }    
    else
    {   
      rtuaddr = asdu[j];
        j++;
    }
    sprintf(desc,",公共地址码-%d",rtuaddr);
    WriteTransOutBuf(desc);
        
    sprintf(desc,",信息体个数-%d\r\n",num);
    WriteTransOutBuf(desc);

    switch( asdu[0] )
    {
    case APPTYPE_DP_NT:
        WriteTransOutBuf("[不带时标双点信息(序号,值,质量码)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            yxno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                yxno += asdu[j+2]*256*256;
                j += 3;
                size--;
            }
            else
                j += 2;
            yxno -= Config_Param.YxBaseAddr;

            if( (num+6) > size )
                num = ( size-6 );

            for( i = 0; i < num; i++, yxno++ )
            {
                yxvalue = (asdu[j+i]&0x03);
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",yxno,yxvalue,(asdu[j+i]&0xfc));
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((yxno == RawDatano) && (GET_RAWDATA_DI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.yxval.value = yxvalue;
                        one_raw_data.yxval.quaflag = (asdu[j+i]&0xfc);
                        one_raw_data.dir = trans_dir;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }    
                }
*/                
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 3,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            if( (num*length+4) > size )
                num = ( size-4 )/3;

            for( i = 0; i < num; i++ )
            {
                pos = 2;
                yxno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    yxno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                yxno -= Config_Param.YxBaseAddr;
                yxvalue = asdu[j+pos+length*i]&0x03;
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",yxno,yxvalue,(asdu[j+pos+length*i]&0xfc));
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((yxno == RawDatano) && (GET_RAWDATA_DI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.yxval.value = yxvalue;
                        one_raw_data.yxval.quaflag = (asdu[j+pos+length*i]&0xfc);
                        one_raw_data.dir = trans_dir;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }    
                }    */            
            }
        }
        break;
    case APPTYPE_SP_NT:
        WriteTransOutBuf("[不带时标单点信息(序号,值,质量码)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            yxno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                yxno += asdu[j+2]*256*256;
                j += 3;
                size--;
            }
            else
                j += 2;
            yxno -= Config_Param.YxBaseAddr;
            
            if( (num+6) > size )
                num = ( size-6 );

            for( i = 0; i < num; i++, yxno++ )
            {
                yxvalue = asdu[j+i]&0x01;
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",yxno,yxvalue,(asdu[j+i]&0xfe));
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((yxno == RawDatano) && (GET_RAWDATA_DI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.yxval.value = yxvalue;
                        one_raw_data.yxval.quaflag = (asdu[j+i]&0xfe);
                        one_raw_data.dir = trans_dir;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }    
                }        */        
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 3,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            if( (num*length+4) > size )
                num = ( size-4 )/3;

            for( i = 0; i < num; i++ )
            {
                pos = 2;
                yxno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    yxno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                yxno -= Config_Param.YxBaseAddr;
                yxvalue = asdu[j+pos+length*i]&0x01;
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",yxno,yxvalue,(asdu[j+pos+length*i]&0xfe));
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((yxno == RawDatano) && (GET_RAWDATA_DI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.yxval.value = yxvalue;
                        one_raw_data.yxval.quaflag = (asdu[j+pos+length*i]&0xfe);
                        one_raw_data.dir = trans_dir;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }    
                }*/                
            }
        }
        break;
    case APPTYPE_PS_STS_NT:
        WriteTransOutBuf("[带变位检出的成组单点信息(序号,值,变化,质量码)]");
        uint16    yxcode, yxcd;
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            yxno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                yxno += asdu[j+2]*256*256;
                j+=3;
            }
            else
                j += 2;
            yxno -= Config_Param.YxBaseAddr;

            for( i = 0; i < num; i++ )
            {
                yxcode = asdu[j+1+5*i]*256+asdu[j+5*i];
                yxcd = asdu[j+3+5*i]*256+asdu[j+2+5*i];
                for( int k = 0; k < 16; k++ )
                {                    
                    if(TRANS_PROTOCOL == TransFlag)
                    {
                        sprintf(desc,"(%d,%d,%d,%02x)",yxno,yxcode&0x01,yxcd&1,asdu[j+4+5*i]);
                        WriteTransOutBuf(desc);
                    }
/*                    else if(GET_RAWDATA == TransFlag)
                    {
                        if((yxno == RawDatano) && (GET_RAWDATA_DI == RawDataType))
                        {                
                            memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                            one_raw_data.yxval.value = yxvalue;
                            one_raw_data.yxval.quaflag = asdu[j+4+5*i];
                            one_raw_data.dir = trans_dir;
                            WriteOneRawData(one_raw_data,max_rawdata_num);
                        }    
                    }    */                
                    
                    yxcd = yxcd>>1;
                    yxcode = yxcode>>1;
                    yxno++;
                }
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 7,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < num; i++ )
            {
                pos = 2;
                yxno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    yxno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                yxno -= Config_Param.YxBaseAddr;
                yxcode = asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i];
                yxcd = asdu[j+pos+3+length*i]*256+asdu[j+pos+2+length*i];

                for( int k = 0; k < 16; k++ )
                {
                    if(TRANS_PROTOCOL == TransFlag)
                    {
                        sprintf(desc,"(%d,%d,%d,%02x)",yxno,yxcode&0x01,yxcd&1,asdu[j+pos+4+length*i]);
                        WriteTransOutBuf(desc);
                    }
/*                    else if(GET_RAWDATA == TransFlag)
                    {
                        if((yxno == RawDatano) && (GET_RAWDATA_DI == RawDataType))
                        {                
                            memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                            one_raw_data.yxval.value = yxvalue;
                            one_raw_data.yxval.quaflag = asdu[j+pos+4+length*i];
                            one_raw_data.dir = trans_dir;
                            WriteOneRawData(one_raw_data,max_rawdata_num);
                        }    
                    }*/
                    yxcd = yxcd>>1;
                    yxcode = yxcode>>1;
                    yxno++;
                }
            }
        }
        break;
    default:
        WriteTransOutBuf("不支持的遥信信息");
        break;
    }
}

void Iec104::TransYcFrame( uint8 *asdu, int size )
{
    uint32    ycnum = asdu[1]&0x7f;
    uint8    seqflag = asdu[1]&0x80;
    uint32    i, ycno, j = 3;
    sint32    ycvalue;
    float32 value;
    char desc[200];
    uint16 rtuaddr = 0;
//    TIME_SEQ_VAL one_raw_data;

  sprintf(desc,",原因码-%d",(asdu[2]&0x3f));    
  WriteTransOutBuf(desc);
        
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
    sprintf(desc,",公共地址码-%d",rtuaddr);
    WriteTransOutBuf(desc);
    
            
    sprintf(desc,",信息体个数-%d\r\n",ycnum);
    WriteTransOutBuf(desc);

    switch( asdu[0] )
    {
    case APPTYPE_ME_FLOAT:
        WriteTransOutBuf("[短浮点数(序号,值,质量码)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;    
            ycno -= Config_Param.YcBaseAddr;
            
            for( i = 0; i < ycnum; i++, ycno++ )
            {
                value = char_to_float((char*)&asdu[j+5*i]);
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%0.4f,%02x)",ycno,value,asdu[j+5*i+4]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = value;
                        one_raw_data.ycvalue.quaflag = asdu[j+5*i+4];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/    
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 7,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                ycno -= Config_Param.YcBaseAddr;
                value = char_to_float((char*)&asdu[j+pos+length*i]);
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%0.4f,%02x)",ycno,value,asdu[j+pos+length*i+4]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = value;
                        one_raw_data.ycvalue.quaflag = asdu[j+pos+length*i+4];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/    
            }
        }
    break;
    case APPTYPE_ME_NT11:
        WriteTransOutBuf("[标度化值(序号,值,质量码)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;    
            ycno -= Config_Param.YcBaseAddr;

            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = sint16(asdu[j+1+3*i]*256+asdu[j+3*i]);
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+2+3*i]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = asdu[j+2+3*i];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }    */
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 5,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                ycno -= Config_Param.YcBaseAddr;
                ycvalue = sint16(asdu[j+pos+1+length*i]*256+asdu[j+pos+length*i]);
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+pos+2+length*i]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = asdu[j+pos+2+length*i];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
            }
        }
        break;
    case APPTYPE_ME_NT:
        WriteTransOutBuf("[规一化值(序号,值,质量码)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j+=3;
            }
            else
                j+=2;    
            ycno -= Config_Param.YcBaseAddr;

            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode( &asdu[j+3*i] );
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+3*i+2]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = asdu[j+3*i+2];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/            
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 5,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                ycno -= Config_Param.YcBaseAddr;
                ycvalue = App_YcDecode( &asdu[j+pos+length*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+pos+length*i+2]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = asdu[j+pos+length*i+2];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
            }
        }
        break;
    case APPTYPE_ME_WT:
        WriteTransOutBuf("[带时标的规一化值(序号,值,质量码)]");
        uint32 ms, minute;
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            ycno -= Config_Param.YcBaseAddr;

            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode( &asdu[j+6*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+6*i+2]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = asdu[j+6*i+2];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/                
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 8,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                ycno -= Config_Param.YcBaseAddr;
                ycvalue = App_YcDecode( &asdu[j+pos+length*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+pos+length*i+2]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = asdu[j+pos+length*i+2];
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
            }
        }
        break;
    case APPTYPE_ME_ND:
        WriteTransOutBuf("[不带品质描述词的规一化值(序号,值)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            ycno -= Config_Param.YcBaseAddr;

            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode( &asdu[j+2*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d)",ycno,ycvalue);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = AI_STS_ON_LINE;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
                
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 4,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                 ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                ycno -= Config_Param.YcBaseAddr;
                ycvalue = App_YcDecode( &asdu[j+pos+length*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d)",ycno,ycvalue);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = AI_STS_ON_LINE;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
            }
        }
        break;
    case APPTYPE_ME_TD:
    case APPTYPE_ME_TE:
        if(asdu[0] == APPTYPE_ME_TD)
            WriteTransOutBuf("[带长时标的规一化值(序号,值,质量码)]");
        else
            WriteTransOutBuf("[带长时标的标度化值(序号,值,质量码)]");

        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            ycno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                ycno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            ycno -= Config_Param.YcBaseAddr;

            for( i = 0; i < ycnum; i++, ycno++ )
            {
                ycvalue = App_YcDecode( &asdu[j+10*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+10*i+2]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = AI_STS_ON_LINE;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 12,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < ycnum; i++ )
            {
                pos = 2;
                ycno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    ycno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }
                ycno -= Config_Param.YcBaseAddr;
                ycvalue = App_YcDecode( &asdu[j+pos+length*i] );
                
                if(TRANS_PROTOCOL == TransFlag)
                {
                    sprintf(desc,"(%d,%d,%02x)",ycno,ycvalue,asdu[j+pos+length*i+2]);
                    WriteTransOutBuf(desc);
                }
/*                else if(GET_RAWDATA == TransFlag)
                {
                    if((ycno == RawDatano) && (GET_RAWDATA_AI == RawDataType))
                    {                
                        memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        one_raw_data.dir = trans_dir;
                        one_raw_data.ycvalue.value = ycvalue;
                        one_raw_data.ycvalue.quaflag = AI_STS_ON_LINE;
                        WriteOneRawData(one_raw_data,max_rawdata_num);
                    }            
                }*/
            }
        }
        break;            
    default:
        WriteTransOutBuf("不支持的遥测信息");
        break;
    }
}

void Iec104::TransSoeFrame( uint8 *asdu, int size )
{
    SYSTIME soetime;
    uint32    soenum = asdu[1]&0x7f, ms, minute;
    uint32    i,  yxno, infosize,j = 3;
    uint8    yxvalue;
    char desc[200];
//    TIME_SEQ_VAL one_raw_data;

    sprintf(desc,",信息体个数-%d",soenum);
    WriteTransOutBuf(desc);

    if( soenum == 0 ) return;
    size -= 4;
       
    uint16 rtuaddr = 0;

    sprintf(desc,",原因码-%d",(asdu[2]&0x3f));    
    WriteTransOutBuf(desc);
        
    if( Config_Param.CotNum == 2 )
    {    
        j++;
        size--;
    }
        
    if( Config_Param.ComAddrNum == 2 )
    {
        rtuaddr = asdu[j] + asdu[j+1]*256;            
        j += 2;
        size--;
    }    
    else
    {   
        rtuaddr = asdu[j];
        j++;        
    }
        
    sprintf(desc,",公共地址码-%d\r\n",rtuaddr);
    WriteTransOutBuf(desc);      

    infosize = size/soenum;                
    GetCurTime( &soetime );

    if( infosize == 6 )
    {
        WriteTransOutBuf("[2字节信息体地址+1字节YX值+3字节短时标(序号,值,时标,质量码)]\r\n");
        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+2+infosize*i];
            ms = asdu[j+4+infosize*i]*256+asdu[j+3+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;

            soetime.minute = minute;
            soetime.second = ms/1000;
            soetime.ms = ms%1000;

            if(TRANS_PROTOCOL == TransFlag)
            {
                sprintf(desc,"(%d,%d,%d-%d-%d %d:%d:%d:%d,%02x)",yxno,yxvalue,soetime.year,soetime.month,soetime.day,
                            soetime.hour,soetime.minute,soetime.second,soetime.ms,(asdu[j+2+infosize*i]&0xfc));
                WriteTransOutBuf(desc);
            }
/*            else if(GET_RAWDATA == TransFlag)
            {
                if((yxno == RawDatano) && (GET_RAWDATA_SOE == RawDataType))
                {                
                    memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        
                    one_raw_data.soevalue.value = yxvalue;
                    one_raw_data.soevalue.quaflag = (asdu[j+2+infosize*i]&0xfc);
                    one_raw_data.soevalue.soetime.year = (soetime.year%2000);
                    one_raw_data.soevalue.soetime.month = soetime.month;
                    one_raw_data.soevalue.soetime.date = soetime.day;
                    one_raw_data.soevalue.soetime.hour = soetime.hour;
                    one_raw_data.soevalue.soetime.minute = soetime.minute;
                    one_raw_data.soevalue.soetime.ms = ms;
                    one_raw_data.dir = trans_dir;
                    WriteOneRawData(one_raw_data,max_rawdata_num);
                }
            }*/                            
        }
    }
    else if( infosize == 7 )
    {
        WriteTransOutBuf("[3字节信息体地址+1字节YX值+3字节短时标(序号,值,时标,质量码)]\r\n");
        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+2+infosize*i]*256*256+asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+3+infosize*i];
            ms = asdu[j+5+infosize*i]*256+asdu[j+4+infosize*i];
            minute = asdu[j+6+infosize*i]&0x3f;

            soetime.minute = minute;
            soetime.second = ms/1000;
            soetime.ms = ms%1000;
    
            if(TRANS_PROTOCOL == TransFlag)
            {
                sprintf(desc,"(%d,%d,%d-%d-%d %d:%d:%d:%d,%02x)",yxno,yxvalue,soetime.year,soetime.month,soetime.day,
                            soetime.hour,soetime.minute,soetime.second,soetime.ms,(asdu[j+3+infosize*i]&0xfc));
                WriteTransOutBuf(desc);
            }
/*            else if(GET_RAWDATA == TransFlag)
            {
                if((yxno == RawDatano) && (GET_RAWDATA_SOE == RawDataType))
                {                
                    memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        
                    one_raw_data.soevalue.value = yxvalue;
                    one_raw_data.soevalue.quaflag = (asdu[j+3+infosize*i]&0xfc);
                    one_raw_data.soevalue.soetime.year = (soetime.year%2000);
                    one_raw_data.soevalue.soetime.month = soetime.month;
                    one_raw_data.soevalue.soetime.date = soetime.day;
                    one_raw_data.soevalue.soetime.hour = soetime.hour;
                    one_raw_data.soevalue.soetime.minute = soetime.minute;
                    one_raw_data.soevalue.soetime.ms = ms;
                    one_raw_data.dir = trans_dir;
                    WriteOneRawData(one_raw_data,max_rawdata_num);
                }
            }*/
        }
    }
    else if( infosize == 9 )
    {
        WriteTransOutBuf("[2字节信息体地址+1字节YX值+6字节长时标(序号,值,时标,质量码)]\r\n");
        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+2+infosize*i];
            ms = asdu[j+4+infosize*i]*256+asdu[j+3+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;
            soetime.minute = minute;
            soetime.second = ms/1000;
            soetime.ms = ms%1000;
            soetime.hour = asdu[j+6+infosize*i]&0x1f;
            soetime.day = asdu[j+7+infosize*i]&0x1f;
            soetime.month = asdu[j+8+infosize*i]&0xf;
            
            if(TRANS_PROTOCOL == TransFlag)
            {
                sprintf(desc,"(%d,%d,%d-%d-%d %d:%d:%d:%d,%02x)",yxno,yxvalue,soetime.year,soetime.month,soetime.day,
                            soetime.hour,soetime.minute,soetime.second,soetime.ms,(asdu[j+2+infosize*i]&0xfc));
                WriteTransOutBuf(desc);
            }
/*            else if(GET_RAWDATA == TransFlag)
            {
                if((yxno == RawDatano) && (GET_RAWDATA_SOE == RawDataType))
                {                
                    memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        
                    one_raw_data.soevalue.value = yxvalue;
                    one_raw_data.soevalue.quaflag = (asdu[j+2+infosize*i]&0xfc);
                    one_raw_data.soevalue.soetime.year = (soetime.year%2000);
                    one_raw_data.soevalue.soetime.month = soetime.month;
                    one_raw_data.soevalue.soetime.date = soetime.day;
                    one_raw_data.soevalue.soetime.hour = soetime.hour;
                    one_raw_data.soevalue.soetime.minute = soetime.minute;
                    one_raw_data.soevalue.soetime.ms = ms;
                    one_raw_data.dir = trans_dir;
                    WriteOneRawData(one_raw_data,max_rawdata_num);
                }
            }*/
        }
    }
    else if( infosize == 10 )
    {
        WriteTransOutBuf("[2字节信息体地址+1字节YX值+7字节长时标(序号,值,时标,质量码)]\r\n");
        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+2+infosize*i];
            ms = asdu[j+4+infosize*i]*256+asdu[j+3+infosize*i];
            minute = asdu[j+5+infosize*i]&0x3f;

            soetime.minute = minute;
            soetime.second = ms/1000;
            soetime.ms = ms%1000;
            soetime.hour = asdu[j+6+infosize*i]&0x1f;
            soetime.day = asdu[j+7+infosize*i]&0x1f;
            soetime.month = asdu[j+8+infosize*i]&0xf;
            soetime.year = (asdu[j+9+infosize*i]&0x7f)+2000;

            if(TRANS_PROTOCOL == TransFlag)
            {
                sprintf(desc,"(%d,%d,%d-%d-%d %d:%d:%d:%d,%02x)",yxno,yxvalue,soetime.year,soetime.month,soetime.day,
                            soetime.hour,soetime.minute,soetime.second,soetime.ms,(asdu[j+2+infosize*i]&0xfc));
                WriteTransOutBuf(desc);
            }
/*            else if(GET_RAWDATA == TransFlag)
            {
                if((yxno == RawDatano) && (GET_RAWDATA_SOE == RawDataType))
                {                
                    memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        
                    one_raw_data.soevalue.value = yxvalue;
                    one_raw_data.soevalue.quaflag = (asdu[j+2+infosize*i]&0xfc);
                    one_raw_data.soevalue.soetime.year = (soetime.year%2000);
                    one_raw_data.soevalue.soetime.month = soetime.month;
                    one_raw_data.soevalue.soetime.date = soetime.day;
                    one_raw_data.soevalue.soetime.hour = soetime.hour;
                    one_raw_data.soevalue.soetime.minute = soetime.minute;
                    one_raw_data.soevalue.soetime.ms = ms;
                    one_raw_data.dir = trans_dir;
                    WriteOneRawData(one_raw_data,max_rawdata_num);
                }
            }*/            
        }
    }
    else if( infosize == 11 )
    {
        WriteTransOutBuf("[3字节信息体地址+1字节YX值+7字节长时标(序号,值,时标,质量码)]\r\n");
        for( i = 0; i < soenum; i++ )
        {
            yxno = asdu[j+2+infosize*i]*256*256+asdu[j+1+infosize*i]*256+asdu[j+infosize*i]-Config_Param.YxBaseAddr;
            yxvalue = asdu[j+3+infosize*i];
            ms = asdu[j+5+infosize*i]*256+asdu[j+4+infosize*i];
            minute = asdu[j+6+infosize*i]&0x3f;

            soetime.minute = minute;
            soetime.second = ms/1000;
            soetime.ms = ms%1000;
            soetime.hour = asdu[j+7+infosize*i]&0x1f;
            soetime.day = asdu[j+8+infosize*i]&0x1f;
            soetime.month = asdu[j+9+infosize*i]&0xf;
            soetime.year = (asdu[j+10+infosize*i]&0x7f)+2000;
            
            if(TRANS_PROTOCOL == TransFlag)
            {
                sprintf(desc,"(%d,%d,%d-%d-%d %d:%d:%d:%d,%02x)",yxno,yxvalue,soetime.year,soetime.month,soetime.day,
                            soetime.hour,soetime.minute,soetime.second,soetime.ms,asdu[j+3+infosize*i]&0xfc);
                WriteTransOutBuf(desc);
            }
/*            else if(GET_RAWDATA == TransFlag)
            {
                if((yxno == RawDatano) && (GET_RAWDATA_SOE == RawDataType))
                {                
                    memcpy(&one_raw_data.time,&trans_frame_time,sizeof(CP56Time));
                        
                    one_raw_data.soevalue.value = yxvalue;
                    one_raw_data.soevalue.quaflag = (asdu[j+3+infosize*i]&0xfc);
                    one_raw_data.soevalue.soetime.year = (soetime.year%2000);
                    one_raw_data.soevalue.soetime.month = soetime.month;
                    one_raw_data.soevalue.soetime.date = soetime.day;
                    one_raw_data.soevalue.soetime.hour = soetime.hour;
                    one_raw_data.soevalue.soetime.minute = soetime.minute;
                    one_raw_data.soevalue.soetime.ms = ms;
                    one_raw_data.dir = trans_dir;
                    WriteOneRawData(one_raw_data,max_rawdata_num);
                }
            }*/            
        }
    }
    else
        WriteTransOutBuf("不支持的SOE信息");
}

void Iec104::TransKwhFrame( uint8 *asdu, int size )
{
    uint32    kwhnum = (asdu[1]&0x7f);
    uint8    seqflag = (asdu[1]&0x80);
    uint32        kwhno, i,j = 3;
    sint32    kwhvalue;
    char desc[100];

    sprintf(desc,",信息体个数-%d\r\n",kwhnum);
    WriteTransOutBuf(desc);


    uint16 rtuaddr = 0;
    sprintf(desc,",原因码-%d",asdu[j]);    
        
    if( Config_Param.CotNum == 2 )
    {    
        j++;
    }
        
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
    sprintf(desc,",公共地址码-%d",rtuaddr);

    switch( asdu[0] )
    {
    case APPTYPE_CO_NT:
        WriteTransOutBuf("[累计量(序号,值)]");
        if( seqflag )
        {
            WriteTransOutBuf("-顺序传送-\r\n");
            kwhno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                kwhno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            kwhno -= Config_Param.DdBaseAddr;

            for( i = 0; i < kwhnum; i++, kwhno++ )
            {
                kwhvalue = App_KwhDecode( &asdu[j+5*i] );
                sprintf(desc,"(%d,%d)",kwhno,kwhvalue);
                WriteTransOutBuf(desc);
            }
        }
        else
        {
            WriteTransOutBuf("-非顺序传送-\r\n");
            int length = 7,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < kwhnum; i++ )
            {
                pos = 2;
                kwhno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    kwhno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                kwhno -= Config_Param.DdBaseAddr;
                kwhvalue = App_KwhDecode( &asdu[j+pos+length*i] );
                sprintf(desc,"(%d,%d)",kwhno,kwhvalue);
                WriteTransOutBuf(desc);
            }
        }
        break;
    case APPTYPE_CO_WT:
        WriteTransOutBuf("[带时标的累计量(序号,值)]");
        //uint32 ms, minute;
        if( seqflag )
        {
            kwhno = asdu[j+1]*256+asdu[j];
            if( Config_Param.InfAddrNum == 3 )
            {
                kwhno += asdu[j+2]*256*256;
                j += 3;
            }
            else
                j += 2;
            kwhno -= Config_Param.DdBaseAddr;

            for( i = 0; i < kwhnum; i++, kwhno++ )
            {
                kwhvalue = App_KwhDecode( &asdu[j+8*i] );
                sprintf(desc,"(%d,%d)",kwhno,kwhvalue);
                WriteTransOutBuf(desc);
            }
        }
        else
        {
            int length = 10,pos;
            if( Config_Param.InfAddrNum == 3 )
                length++;

            for( i = 0; i < kwhnum; i++ )
            {
                pos = 2;
                kwhno = asdu[j+1+length*i]*256+asdu[j+length*i];
                if( Config_Param.InfAddrNum == 3 )
                {
                    kwhno += asdu[j+2+length*i]*256*256;
                    pos = 3;
                }

                kwhno -= Config_Param.DdBaseAddr;
                kwhvalue = App_KwhDecode( &asdu[j+pos+length*i] );
                sprintf(desc,"(%d,%d)",kwhno,kwhvalue);
                WriteTransOutBuf(desc);
            }
        }
        break;
    default:
        WriteTransOutBuf("不支持的累计量信息");
        break;
    }
}

void Iec104::DeleteOneDiskFile( char *filename )
{
   if( NULL == filename )
      return;

   unlink( filename );
}


void Iec104::SetConfigParam(ProtocolCfgParam_S &protocolCfg)
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

    Config_Param.LinkTimeout = protocolCfg.linkTimeout;        //链路应答超时
    Config_Param.YkTimeout = protocolCfg.ykTimeout;        //遥控超时时间（秒）
    Config_Param.SpTimeout = protocolCfg.spTimeout;        //设置点执行命令超时（秒）
    Config_Param.FwTimeout = protocolCfg.fwTimeout;        //防误执行命令超时（秒）

    Config_Param.YxBaseAddr = protocolCfg.yxBaseAddr;        //ASDU状态量起始地址
    Config_Param.YcBaseAddr = protocolCfg.ycBaseAddr;        //ASDU模拟量起始地址
    Config_Param.YkBaseAddr = protocolCfg.ykBaseAddr;        //ASDU控制量起始地址
    Config_Param.YkLastAddr = 0;        //ASDU控制量结束地址
    Config_Param.SpBaseAddr = protocolCfg.spBaseAddr;        //ASDU设置点起始地址
    Config_Param.YtBaseAddr = protocolCfg.ytBaseAddr;        //ASDU调节量起始地址
    Config_Param.DdBaseAddr = protocolCfg.ddBaseAddr;    //ASDU电度量起始地址
    Config_Param.FwBaseAddr = protocolCfg.fwBaseAddr;        //ASDU防误点结束地址

    Config_Param.MaxIframeNum = 8;    //最后确认的I帧最大数目
    Config_Param.MaxIsendNum = 12;    //未被确认的I帧最大数目
    Config_Param.OverTime1 = 12;//15;    //I帧或U帧发送后,经过OverTime1超时
    Config_Param.OverTime2 = 10;    //收到I帧后,经过OverTime2无数据报文时,发送S帧,OverTime2<OverTime1
    Config_Param.OverTime3 = 20;//20;    //长期空闲,经过OverTime3超时,发送测试帧,OverTime3>OverTime1
    Config_Param.UseQuality = 1;

    Config_Param.isCheckSeqNum = protocolCfg.isCheckSeqNum;
    Config_Param.isYkWaitExecFinish = protocolCfg.isYkWaitExecFinish;
    Config_Param.isYtWaitExecFinish = protocolCfg.isYtWaitExecFinish;
}

