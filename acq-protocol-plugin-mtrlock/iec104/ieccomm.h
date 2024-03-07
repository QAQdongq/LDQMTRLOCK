/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  ieccomm.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：IEC规约通用结构定义.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef IECCOMM_H
#define IECCOMM_H

#include <string>
#include <QString>

#define APP_MAX_BUFSIZE                 256

//设置点请求数据
typedef struct SPReqData
{
    QString cchId;//通道号
    QString rtuId;//RTU号
    int no;//遥调点序号
    int type;//设点类型：0: 小数、1：整数、2：浮点、3：位串
    float decValue;//小数值
    int intValue;//整数值
    float floatValue;//浮点值
    std::string strValue;//位串值(最大长度为4)
    SPReqData& operator=(const SPReqData &data)
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
    void clear()
    {
        this->rtuId = "";
        this->cchId = "";
        this->no = 0;
        this->type = 0;
        this->decValue = 0.0;
        this->intValue = 0;
        this->floatValue = 0.0;
        this->strValue = "";
    }
}SPReqData_S;


//即SICD_D_DA_DATA_VAR自定义类型
struct SCDS_FAULTDEAL       //馈线及开闭所故障处理结果(DA)报文结构，是否放入proto_def.h中？
{
    //uint32    RTUNo;
    uint16  Year;
    uint8   Month;
    uint8   Day;
    uint8   Hour;
    uint8   Minute;
    uint8   Second;
    uint16  Ms;
    uint8   DiagnoseStyle;
    uint8   ValidFlag;
    uint8   FailStep;
    uint8   FailReason;
    uint16  StartBreakId;
    uint16  EndBreakId;

    uint8   IslationBreakNum;
    uint8   IslationSuccessFlag;
    uint16  IslationBreakId[8];

    uint8   RestoreBreakNum;
    uint8   RestoreSuccessFlag;
    uint16  RestoreBreakId[8];
};


struct FETIME
{
    uint16  aMs;
    uint8   aSec;
    uint8   aMin;
    uint8   aHour;
    uint8   aDay;
    uint8   aMon;
    uint16  aYear;
};

struct IEC104_APP_LAYER
{
    int State;          //应用层状态
    uint8   Inited_Flag;        //初始化标志
    int     Snd_seqnum;     //APDU 的发送序列号
    int Rec_seqnum;     //APDU 的接收序列号
    int Ack_num;        //DTE正确收到的APDU数目
    uint8   SendACKFlag;        //主站确认子站最后 I format标志
    uint8   WaitACKFlag;        //子站确认主站最后I format标志
    uint8   CallAllDataFlag;    //全数据请求标志
    uint8   CallGroupDataFlag;   //分组召唤标志
    uint8   CallGroupNo;   //分组召唤组号
    uint8   TimeSyncFlag;       //对钟标志
    uint8   CallAllKwhFlag;     //全电度请求标志
    uint32  CtrlNo;         //控制点号
    int     CtrlType;       //控制类型（预置/执行）
    int CtrlAttr;       //控制属性（分/合）
    int CtrlReply;      //控制应答
    uint64  LastCallAllTime;    //上次全数据请求时间
    uint64  LastSyncTime;       //上次对钟时间
    uint64  LastCallAllKwhTime;    //上次全电度请求时间
    uint64  LastTxTime;         //应用层上次发送时间
    uint64  LastRxTime;         //应用层上次接收时间
    int txSize;         //应用层发送数据长度
    uint8   txData[APP_MAX_BUFSIZE]; //应用层发送数据缓冲区
    int rxSize;         //应用层接收数据长度
    uint8   rxData[APP_MAX_BUFSIZE]; //应用层接收缓冲区
    uint8   Resend_Times;       //应用层重发次数（暂时未用）
    uint8   Command_Enable;         //本通道是否允许发送命令 lcq add for shaoxing
    uint8   RxWindowNum;

    //SPReqData_S spReqData;//设置点请求数据
    //QString taskId;//当前任务号
};

#endif

