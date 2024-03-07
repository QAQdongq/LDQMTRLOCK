/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  proto_cmdmem.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：定义命令缓冲区接口类PROTO_CommandMem. 
            该类为一个抽象基类,用于为外部提供命令接口规范.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef PROTO_COMMANDMEM_H
#define PROTO_COMMANDMEM_H

#include <list>
#include "proto_def.h"
#include "data/send_json_data.h"
#include "acq_protocol_interface.h"
#include "ieccomm.h"

//调度系统遥控命令
#define CMD_TYPE_YK         1       //遥控
#define CMD_TYPE_YT        2        //遥调
#define CMD_TYPE_SYNCTIME    3
#define CMD_TYPE_PROT        4
#define CMD_TYPE_MNP        5
#define CMD_TYPE_CALLALLDATA    6
#define CMD_TYPE_SELECT_FAIL                7//遥控预置失败
#define CMD_TYPE_SELECT_SUCCESS            8//遥控预置成功
#define CMD_TYPE_LINK_BUSY                    9//链路忙
#define CMD_TYPE_UNKNOWN_YKNO                10//未知的遥控号
#define CMD_TYPE_YKCANCEL_SUCCESS        11//遥控撤销成功
#define CMD_TYPE_YKEXEC_SUCCESS            12//遥控执行成功
#define CMD_TYPE_GROUP_CALL            13//分组召唤命令
#define CMD_TYPE_LOCK                14

#define CMD_TYPE_SP            15     //设置值
#define CMD_TYPE_CALLALLDATAEND    16 //总召结束
#define CMD_TYPE_YX    17 //遥信
#define CMD_TYPE_YC    18 //遥测
#define CMD_TYPE_YKRESULT 19 //遥控(预置或执行)结果
#define CMD_TYPE_YTRESULT 20 //遥调(预置或执行)结果
#define CMD_TYPE_WF       21 //五防操作 liujie add 20230525
#define CMD_TYPE_WFRESULT 22 //五防操作结果（防误） liujie add 20230525


#define CTRL_FUNC_SELECT    1
#define CTRL_FUNC_EXECUTE    2
#define CTRL_FUNC_CANCEL    3
#define CTRL_FUNC_AUTOEXECUTE   4
#define CTRL_FUNC_LINEEXECUTE   5  /*ADD FOR SHAOGUAN:选线执行*/
#define CTRL_FUNC_LOCK          6  //遥控闭锁功能
#define CTRL_FUNC_UNLOCK    7        //请求五防解锁应答
#define CTRL_FUNC_WF    8        //五防操作(防误校验) liujie add 20230525
#define CTRL_FUNC_YJMS    9        //五防操作(应急模式) liujie add 20230525

#define CTRL_TYPE_TRIP        1
#define CTRL_TYPE_CLOSE        2
#define CTRL_TYPE_RAISE        3
#define CTRL_TYPE_LOWER        4
#define CTRL_TYPE_LOCK      5//闭锁


#define UNLOCK_MAX_NUM    128 

#define PROTOCOL_CMDDATA_LEN        5120

#define PROTECT_DATA_LEN                4096

#define STRAP_LOCK_UNLOCK_MAX_NUM    32 

#define CTRL_CMD_PROTO_DECIMAL   1
#define CTRL_CMD_PROTO_INT       2
#define CTRL_CMD_PROTO_FLOAT     3
#define CTRL_CMD_PROTO_DIGIT     4
#define CTRL_CMD_PROTO_YK_SP     5
#define CTRL_CMD_PROTO_YK_DP     6

//设置点类型：
#define SP_TYPE_DECIMAL   0    //归一化
#define SP_TYPE_INT       1    //标度化
#define SP_TYPE_FLOAT     2    //佛点数
#define SP_TYPE_DIGIT     3


#define SCN_SYNCH_BUFF_LEN          102400
#define SCN_ASYNCH_BUFF_LEN         1024    /*lql modify 512--> 1024 */
#define COM_MAX_CHANNUMS            1000/*256 now set 500 for test 2003.8.7*/
#define SCN_MAX_RTUNUMS                500/*512 now set 1000 for test 2003.8.7*/
#define COM_MAX_DEVNAMELEN            64

struct CtrlCmd
{
    uint32 protoCmdType;
    uint32 rtuAddr;//stationId;
    uint32 functionCode;
    uint32 ptAddress;
    uint32 reply; 
    uint32 execDuration;

    union
    {
        uint32 ctrlType;       /* 分合 升降 */
        float32 spDecimal;     /* 小数值 */
        sint32 spInt;          /* 整数值 */
        float32 spFloat;       /* 浮点值 */
        uint8 spDigit[4];      /* 位串值 */
    };
};

struct CtrlDnys
{
    uint16 stationNo;    /*    站号*/
    uint16 taskNo;        /*    操作票任务号*/
    uint16 channel;        /*    操作票任务号*/
    uint16 success;        /*    用于B0B8结果*/
    uint16 stepExec;    /*    已执行步数*/
    uint16 itemExec;    /*    已执行项数*/
    uint32 reply;        /*    返回状态*/
};

struct CtrlDxgl/*----dingli added 09.04.01*/
{
    uint8 num;
    uint16 ellock[UNLOCK_MAX_NUM]; /*解闭锁命令----*/
};

struct CtrlWlkzq
{
    uint16 stationNo;
    uint16 taskNo;
    uint16 stepExec;
    uint16 itemExec;
    uint16 reply;
    uint16 unlockno;//解闭锁请求解锁点号
    uint16 retCode;    
};

struct ProtData
{
    uint32 stationId;
    uint16 prot_type;
    uint16 datalen;
    uint8  ProtBuf[PROTECT_DATA_LEN-8];
};

/*  上海大集控远方解锁和复位功能  */
struct RemoteSoftLock
{
    uint16 stationNo;
    uint8 func;
    uint8 username[17];
    uint8  password[5];
};

struct RemoteHardLock
{
    uint16 stationNo;
    uint8 func;
    uint8  password[11];
};

struct RemoteHardSoftLockReply
{
    uint8  func;
    uint8  status;
    uint8  error;
};

/*  上海黑匣子召唤功能  */
struct CallBlackBoxData
{
    void   *hdfq;
    QString rtuId;
    uint16 stano;
    uint16 guicmd;
    uint8  status;
    char   hostname[32];
    char   filename[64];
};

struct StrapLockUnlockData
{
    QString rtuId;                                    /*    站号*/
    sint16 strapNum;                                /*    压板个数*/
    sint16 strapId[STRAP_LOCK_UNLOCK_MAX_NUM];      /*    压板控制器ID*/
    sint16 strapAddr[STRAP_LOCK_UNLOCK_MAX_NUM];    /*    压板地址,=-1表示该压板控制器的所有压板*/
};

struct RemoteViewData
{
    uint32 rvServerIP;                            /*视频服务器IP*/
    uint8 stationId;
    uint8 ptAddress;
    uint8 rvChanValid;                            /*视频通道是否有效*/
    uint8 rvChan;                                 /*视频通道*/
    uint8 presetPointValValid;                    /*预置点值是否有效*/
    uint8 presetPointVal;                         /*预置点值*/
    uint8 inputPortValid;                         /*矩阵输入端口是否有效*/
    uint8 inputPort;                              /*矩阵输入端口*/
    uint8 outputPortValid;                        /*矩阵输出端口是否有效*/
    uint8 outputPort;                             /*矩阵输出端口*/
};

struct  WirelessDX    
{
    char    DXRfid[12];
    uint8    DXDNo;
    uint8    status;
};

struct  DxglKzqData    
{
    uint32    staNo;
    uint32    taskNo;
};

struct  RemoteScheduleUnlock    
{
    uint32    devId;
    uint32    status;
};

struct ReplyCommand //回复命令
{
    uint16    CmdType;    //yk yt settime or prot etc.
    uint16    CmdLen;
    uint8   CmdData[PROTOCOL_CMDDATA_LEN];
    uint32    CmdDummy;/*yay add this dummyval for cmdReply.CmdData is 4 alignment on HPUX platform*/
};

struct COMMAND
{
    COMMAND(){}
    ~COMMAND(){/*qDebug()<<"~COMMAND";*/}
    explicit COMMAND(const COMMAND &command)
    {
        this->cmdType = command.cmdType;
        std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command.dataList.begin();
        for(; iter != command.dataList.end(); iter++)
        {
            this->dataList.push_back(*iter);
        }
        this->taskId = command.taskId;
    }

    uint16 cmdType;//命令类型：yx-遥信, yc-遥测 ,yk-遥控, mc-脉冲, calldata-总召, calldataend-总召结束
    //uint32 ctrlCmdProto;//控制数据类型：如：CTRL_CMD_PROTO_DECIMAL，CTRL_CMD_PROTO_INT等
    std::list<std::shared_ptr<BaseParam_S>> dataList;
    QString taskId;//任务号
    COMMAND& operator=(const COMMAND &command)
    {
        if(this == &command)
        {
            return *this;
        }
        this->cmdType = command.cmdType;
        //this->ctrlCmdProto = command.ctrlCmdProto;
        std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  command.dataList.begin();
        for(; iter != command.dataList.end(); iter++)
        {
            this->dataList.push_back(*iter);
        }
        this->taskId = command.taskId;
        return *this;
    }
};

#define YSGLJ_RECORD_TYPE_OPER        1
#define YSGLJ_RECORD_TYPE_USER        2

/* 钥匙管理机读取记录 */
struct YsgljReadRecord
{
    void *hdfq;
    QString rtuId;
    uint16 stano;
    uint16 type;
    uint16 startno;
    uint16 maxcount;
    uint8  status;
    char   filename[64];
};

struct OPS_SEND_COMPLETE
{
    uint16    rtuId;
    COMMAND    *senddata;
};

class TaskInfo;
class PROTO_CommandMem//: public QObject
{
    //Q_OBJECT
public:
    virtual ~PROTO_CommandMem() {}
    virtual int     GetCommandNum() = 0;
    virtual int    PutACommand(COMMAND &cmd )=0;
    virtual void     RemoveAllCommand() = 0;
    //读取命令return=1 有效, =-1 无效
    virtual int    GetACommand(std::shared_ptr<COMMAND> &cmd )=0;    //读指针不移动
    virtual int    RemoveACommand()=0;        //移动读指针
    virtual int SendYkYtReply(int cmdType,const QString &rtuId, int pointNo, int ctrlType, int ctrlReply, std::shared_ptr<TaskInfo> taskInfoPtr)=0;//发送遥控[或遥调]结果响应到网关
    virtual int SendSpReply(int cmdType,const QString &rtuId, int pointNo, int ctrlReply, std::shared_ptr<TaskInfo> taskInfoPtr)=0;//发送设置点结果响应到网关

    virtual int SendFWReply(int cmdType,const QString &rtuId, const QString &taskId, QList<FwRspParam_S> &fwRspList)=0;//发送防误结果响应到网关
    
    /**
     * @brief 设置所连接的设备IP和端口(发送源头端)
     * @param source 所连接的设备IP和端口
     */
    virtual void SetSource(const std::string &source)=0;
//Q_SIGNALS:
    //void signalCmdReply(const std::string &dataJson);
//public slots:
    virtual void slotPutCmd(std::list<COMMAND> &cmdList) =0;

    inline void setProtocolInterface(AcqProtocolInterface *protoInterface){m_protoInterface=protoInterface;}
    inline void setCchId(QString cchId){m_cchId=cchId;}
protected:
    AcqProtocolInterface *m_protoInterface;

    QString m_cchId;
};

#endif //PROTO_COMMANDMEM_H
