#ifndef __PLUGIN104_DEFINE_H
#define __PLUGIN104_DEFINE_H
#include <string>
#include <list>
#include <sstream>
#include <map>
#include <memory>
#include <QDebug>
#include "acq_define.h"
#include "../acq-protocol-plugin-utils/plugin_def.h"


#ifdef WIN32
#define  LITTLE_ENDIAN	      1234
#define  BIG_ENDIAN	      4321
#define  BYTE_ORDER	      LITTLE_ENDIAN
#else
#include <arpa/nameser_compat.h>               // defined BYTE_ORDER
#endif

#if PLATFORM_WIN32
#define snprintf _snprintf
#endif


//#define SCN_SYNCH_BUFF_LEN (5120)
#define DISPLAY_FRAME_MAX_SIZE (128)
#define SCN_PROTOCOL_SLEEPTIME    (100)         //规约处理线程睡眠时间间隔（毫秒）
#define SEND_THREAD_SLEEPTIME    (100)         //下发数据监控线程睡眠时间间隔（毫秒）

#define INVALID_SOCKET_DESCRIPTOR (-1) //无效socket descriptor
#define INVALID_CMDTYPE (-1) //无效命令类型
#define INVALID_CTRLFUNC (-1) //无效功能码

#define CFQ 0


//遥控命令回复状态
#define COMMAND_SELECT_FAIL        0 //遥控预置失败
#define COMMAND_SELECT_SUCCESS    1 //遥控预置成功
#define COMMAND_LINK_BUSY        2 //链路忙
#define COMMAND_UNKNOWN_YKNO        3 //未知的遥控号
#define COMMAND_YKCANCEL_SUCCESS        4 //遥控撤销成功
#define COMMAND_YKEXEC_SUCCESS            5 //遥控执行成功
#define COMMAND_YKEXEC_FAIL            6 //遥控执行失败
#define COMMAND_YKCANCEL_FAIL        7 //遥控撤销失败
#define COMMAND_WF_SUCCESS        8 //防误校验成功
#define COMMAND_YJMS_SUCCESS        9 //应急模式校验成功
#define COMMAND_WF_FAIL        10 //防误校验失败
#define COMMAND_YJMS_FAIL        11 //应急模式校验失败

#define COMMAND_YKEXEC_TIMEOUT    12 //遥控执行超时
#define COMMAND_WF_TIMEOUT    13 //防误校验超时
#define COMMAND_YJMS_TIMEOUT    14 //应急模式校验超时



//通道类型
typedef enum
{
    CHN_TYPE_CLIENT =0, //客户端
    CHN_TYPE_SERVER //服务端
}CHN_TYPE;

//104类型
typedef enum
{
    TYPE_104_MAIN =0, //主站（客户端）
    TYPE_104_SUB //子站(服务端)
}TYPE_104;


enum FrameDirection  /* 报文的收发方向 */
{
    DSendFrame = 0,
    DRecvFrame = 1
};

//命令类型枚举值
typedef enum
{
    CMD_TYPE_YX = 0,//遥信
    CMD_TYPE_YC,//遥测
    CMD_TYPE_YK,//遥控
    CMD_TYPE_DD,//电度
    CMD_TYPE_CALLDATA,//总召
    CMD_TYPE_CALLDATAEND,//总召结束
    CMD_TYPE_YKRESULT//遥控结果
}CMD_TYPE_E;

//命令类型字符串形式
static const std::string CMD_TYPE_STR_YXCHG = CODE_YXCHG;//"YXCHG";//突发遥信
static const std::string CMD_TYPE_STR_YXCYC = CODE_YXCYC;//"YXCYC";//总召遥信
static const std::string CMD_TYPE_STR_YCCHG = CODE_YCCHG;//"YCCHG";//突发遥测
static const std::string CMD_TYPE_STR_YCCYC = CODE_YCCYC;//"YCCYC";//总召遥测
static const std::string CMD_TYPE_STR_YK = CODE_YKREQ;//"YKReq";//遥控
static const std::string CMD_TYPE_STR_YT = CODE_YTREQ;//"YTReq";//遥调
static const std::string CMD_TYPE_STR_SP = CODE_SETREQ;//"SetpointReq";//设置点
static const std::string CMD_TYPE_STR_WF = CODE_FWREQ;//"FWReq";//五防点 liujie add 20230525
static const std::string CMD_TYPE_STR_WFRESULT = CODE_FWRSP;//"FWRsp";//五防点 liujie add 20230525
static const std::string CMD_TYPE_STR_SOE = CODE_SOE;//SOE
static const std::string CMD_TYPE_STR_DDCHG = CODE_DDCHG;//"DDCHG";//突发电度（脉冲）
static const std::string CMD_TYPE_STR_DDCYC = CODE_DDCYC;//"DDCYC";//总召电度（脉冲）
static const std::string CMD_TYPE_STR_CALLDATA = CODE_CALLREQ;//"CallReq";//总召
static const std::string CMD_TYPE_STR_CALLDATAEND = CODE_CALLRSP;//"CallRsp";//总召应答（结束）
static const std::string CMD_TYPE_STR_YKRESULT = CODE_YKRSP;//"YKRsp";//遥控应答
static const std::string CMD_TYPE_STR_YTRESULT = CODE_YTRSP;//"YTRsp";//遥调应答
static const std::string CMD_TYPE_STR_SPRESULT = CODE_SETRSP;//"SetRsp";//设置点应答

static const std::string CMD_TYPE_STR_CALLDD = CODE_CALLDDREQ;//"CallDDReq";//总召电度（脉冲）
static const std::string CMD_TYPE_STR_CALLDDEND = CODE_CALLDDRSP;//"CallDDRsp";//总召电度（脉冲）应答（结束）

static const std::string CMD_TYPE_STR_SetPasswordReg = CODE_SetPasswordReg;//"SetPasswordReg";//密码锁下发密码命令

//遥控结果, 1:成功； -1：超时；-2：其他原因
typedef enum
{
    CMD_RESULT_FAIL =-2, //失败:其他原因
    CMD_RESULT_TIMEOUT=-1, //超时
    CMD_RESULT_SUCCESS=1 //成功
}CMD_RESULT;


//遥控命令类型: 0-撤销， 1-选择，2-执行
typedef enum YK_CMD
{
    YK_CMD_CANCEL=0,  //遥控(遥调)撤销
    YK_CMD_SELECT,//遥控(遥调)选择(预置)
    YK_CMD_EXECUTE,  //遥控(遥调)执行

}YK_CMD_E;

typedef enum WF_CMD
{
    WF_CMD_EXECUTE = 1, //liujie add 添加五防操作类型
    WF_CMD_YJMS //liujie add 添加五防应急模式

}WF_CMD_E;

//遥控结果, 1:成功； -1：超时；-2：其他原因
typedef enum
{
    YK_RESULT_FAIL =-2, //失败:其他原因
    YK_RESULT_TIMEOUT=-1, //超时
    YK_RESULT_SUCCESS=1 //成功
}YK_RESULT;

//数据参数基类
typedef struct BaseParam
{
    QString rtuId; //RTU号
    BaseParam& operator=(const BaseParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        return *this;
    }
}BaseParam_S;


//上送的遥信数据参数
typedef struct YXParam:public BaseParam_S
{
    int no;//遥信点序号
    int value;//遥信值
    int quality;//质量码，有效位等
    long long time;//SOE时间戳(秒+毫秒，相对于UTC时间)
    YXParam& operator=(const YXParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->no = data.no;
        this->value = data.value;
        this->quality = data.quality;
        this->time = data.time;
        return *this;
    }
}YXParam_S;

//上送的遥测数据参数
typedef struct YCParam:public BaseParam_S
{
    int no;//遥测点序号
    float value;//遥测值
    int quality;//质量码，有效位等
    long long time;//SOE时间戳(秒+毫秒，相对于UTC时间)
    YCParam& operator=(const YCParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->no = data.no;
        this->value = data.value;
        this->quality = data.quality;
        this->time = data.time;
        return *this;
    }
}YCParam_S;

//上送的电度数据参数
typedef struct DDParam:public BaseParam_S
{
    int no;//点序号
    float value;//值
    DDParam& operator=(const DDParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->no = data.no;
        this->value = data.value;
        return *this;
    }
}DDParam_S;

//遥控[或遥调]请求数据参数
typedef struct YKReqParam:public BaseParam_S
{
    QString cchId;//通道号
    int no;//遥控点序号
    int type;//遥控类型：0：取消、2：预置、1：执行
    int value;//遥控值(0-分(降)，1-合(升))

    YKReqParam()
    {
        cchId="";
        no=0;
        type=0;
        value=0;
    }
    YKReqParam& operator=(const YKReqParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->no = data.no;
        this->type = data.type;
        this->value = data.value;
        return *this;
    }
}YKReqParam_S;


//遥控[或遥调]应答数据参数
typedef struct YKRspParam:public BaseParam_S
{
    QString cchId;//通道号
    int no;//遥控点序号
    int type;//遥控类型：0：取消、2：预置、1：执行
    int value;//遥控值(0-分(降)，1-合(升))
    int result;//结果状态(0-失败，1-成功）;
    QString reason;//遥控失败原因

    YKRspParam()
    {
        cchId="";
        no=0;
        type=0;
        value=0;
    }
    YKRspParam& operator=(const YKRspParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->no = data.no;
        this->type = data.type;
        this->value = data.value;
        this->result = data.result;
        this->reason = data.reason;
        return *this;
    }
}YKRspParam_S;


//防误校验 请求数据参数
typedef struct FWReqParam:public BaseParam_S
{
    QString cchId;//通道号
    int no;//遥控点序号
    int type;//防误类型：1：防误校验、2：应急模式
    int value;//值(0-分(降)，1-合(升))

    FWReqParam()
    {
        cchId="";
        no=0;
        type=0;
        value=0;
    }
    FWReqParam& operator=(const FWReqParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->no = data.no;
        this->type = data.type;
        this->value = data.value;
        return *this;
    }
}FWReqParam_S;

//防误校验 应答数据参数
typedef struct FwRspParam:public BaseParam_S
{
    QString cchId;//通道号
    int no;//遥控点序号
    int type;//防误类型：1：防误校验、2：应急模式
    int value;//遥控值(0-分(降)，1-合(升))
    int result;//结果状态(0-失败，1-成功）;
    QString reason;//遥控失败原因

    FwRspParam()
    {
        cchId="";
        no=0;
        type=0;
        value=0;
    }
    FwRspParam& operator=(const FwRspParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->no = data.no;
        this->type = data.type;
        this->value = data.value;
        this->result = data.result;
        this->reason = data.reason;
        return *this;
    }
}FwRspParam_S;


//设置点请求数据参数
typedef struct SPReqParam:public BaseParam_S
{
    QString cchId;//通道号
    int no;//遥调点序号
    int type;//设点类型：0: 小数、1：整数、2：浮点、3：位串
    float decValue;//小数值
    int intValue;//整数值
    float floatValue;//浮点值
    QString strValue;//位串值(最大长度为4)
    SPReqParam()
    {
        cchId="";
        no=0;
        type=0;
        decValue=0.0;
        intValue=0;
        floatValue=0.0;
        strValue="";
    }
    SPReqParam& operator=(const SPReqParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->no = data.no;
        this->type = data.type;
        this->decValue = data.decValue;
        this->intValue = data.intValue;
        this->floatValue = data.floatValue;
        this->strValue = data.strValue;
        return *this;
    }
}SPReqParam_S;

//设置点应答数据参数
typedef struct SPRspParam:public BaseParam_S
{
    QString cchId;//通道号
    int no;//设置点序号
    int type;//设点类型：0: 小数、1：整数、2：浮点、3：位串
    float decValue;//小数值
    int intValue;//整数值
    float floatValue;//浮点值
    QString strValue;//位串值(最大长度为4)
    int result;//结果状态(0-失败，1-成功）;
    QString reason;//设置点失败原因
    SPRspParam()
    {
        cchId="";
        no=0;
        type=0;
        decValue=0.0;
        intValue=0;
        floatValue=0.0;
        strValue="";
        result=0;
        reason="";
    }
    SPRspParam& operator=(const SPRspParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->no = data.no;
        this->type = data.type;
        this->decValue = data.decValue;
        this->intValue = data.intValue;
        this->floatValue = data.floatValue;
        this->strValue = data.strValue;
        this->result = data.result;
        this->reason = data.reason;
        return *this;
    }
}SPRspParam_S;

//下发总召命令数据参数
typedef struct CallReqParam:public BaseParam_S
{
    QString cchId;//通道号
    CallReqParam& operator=(const CallReqParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        return *this;
    }
}CallReqParam_S;

//总召应答数据参数
typedef struct CallRspParam:public BaseParam_S
{
    QString cchId;//通道号
    int result;//返回结果(1:总召成功；-1:Json数据异常 -2:其他失败原因)
    QString reason;//遥控失败原因
    CallRspParam& operator=(const CallRspParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->result = data.result;
        this->reason = data.reason;
        return *this;
    }
}CallRspParam_S;

//下发校时命令数据参数
typedef struct SyncClockReqParam:public BaseParam_S
{
    QString cchId;//通道号
    SyncClockReqParam& operator=(const SyncClockReqParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        return *this;
    }
}SyncClockReqParam_S;

//下发校时应答数据参数
typedef struct SyncClockRspParam:public BaseParam_S
{
    QString cchId;//通道号
    int result;//返回结果(1:总召成功；-1:Json数据异常 -2:其他失败原因)
    QString reason;//遥控失败原因
    SyncClockRspParam& operator=(const SyncClockRspParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->result = data.result;
        this->reason = data.reason;
        return *this;
    }
}SyncClockRspParam_S;

//下发校时应答数据参数
typedef struct SetPasswordRegParam:public BaseParam_S
{
    QString cchId;//通道号
    int passwd;//密码
    int lockno;//遥控失败原因
    SetPasswordRegParam& operator=(const SetPasswordRegParam &data)
    {
        if(this == &data)
        {
            return *this;
        }
        //this->rtuId = data.rtuId;
        this->cchId = data.cchId;
        this->passwd = data.passwd;
        this->lockno = data.lockno;
        return *this;
    }
}SetPasswordRegParam_S;


//命令数据基类
typedef struct BaseCmdData
{
    QString code;//功能码
    QString taskId;//任务号
    std::list<std::shared_ptr<BaseParam_S>> param;//数据
}BaseCmdData_S;


//遥信命令
typedef struct YXCmdData: public BaseCmdData_S
{
}YXCmdData_S;

//遥测命令
typedef struct YCCmdData: public BaseCmdData_S
{
}YCCmdData_S;

//电度命令
typedef struct DDCmdData: public BaseCmdData_S
{
}DDCmdData_S;
//下发遥控命令
typedef struct YKReqCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}YKReqCmdData_S;

//遥控应答命令
typedef struct YKRspCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}YKRspCmdData_S;

//下发总召
typedef struct CallReqCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}CallReqCmdData_S;

//下发设置点命令
typedef struct SPReqCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}SPReqCmdData_S;

//设置点应答命令
typedef struct SPRspCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}SPRspCmdData_S;

//下发防误校验命令
typedef struct FWReqCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}FWReqCmdData_S;

//防误校验应答命令
typedef struct FWRspCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}FWRspCmdData_S;

//密码锁设置密码命令
typedef struct SetPasswordRegCmdData: public BaseCmdData_S
{
    qlonglong time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
}SetPasswordRegCmdData_S;
#endif // __PLUGIN104_DEFINE_H
