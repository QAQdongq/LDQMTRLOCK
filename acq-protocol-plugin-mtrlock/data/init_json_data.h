#ifndef __INITJSONDATA_H
#define __INITJSONDATA_H
#include <string>
#include <list>
#include <QString>

//通道信息
typedef struct ChnCfg
{
    QString id;//通道号
    QString name;//通道名称
    QString desc;//通道描述
    int type;//通道类型
}ChnCfg_S;

//RTU信息
typedef struct Rtu
{
    QString rtuId; //RTU序号
    QString rtuName;//RTU名称
    QString rtuDesc;//RTU描述
    int rtuAddr;//RTU地址
    int yxCounts;//遥信个数
    int ycCounts;//遥测个数
    int ykCounts;//遥控个数
    int ddCounts;//遥脉(电度量)个数
    int setCounts;//设点个数
    QString extend;//预留扩展信息
    int weight;//权重大小: 1-主通道，2-备通道1，3-备通道2，...
}Rtu_S;

//规约配置
typedef struct ProtocolCfgParam
{
    int scanTime;//扫描周期
    int scanFullTime;//全数据召唤周期
    int scanPulseTime;//脉冲(电度)召唤周期
    int syncTime;//校时周期
    int retryTimes;//重试次数

    int linkTimeout;//链路应答超时
    int ykTimeout;//遥控超时时间（秒）
    int spTimeout;//设置点执行命令超时（秒）
    int fwTimeout;//防误执行命令超时（秒）

    int ykCmdMode; //遥控命令模式(1:单点命令模式，2:双点命令模式)(默认为1)
    int spCmdMode; //设点命令模式(1:单个设点模式,2:连续设点模式)(默认为1)
    int soeTransMode; //SOE传输模式(1:一次传输,要根据SOE生成YX, 2:二次传输)(默认为1)
    int cotNum;       //ASDU传送原因字节数(默认为2)
    int comAddrNum;   //ASDU公共地址字节数(默认为2)
    int infAddrNum;   //ASDU信息体地址字节数(默认为3)

    int yxBaseAddr;   //ASDU状态量(YX)起始地址
    int ycBaseAddr;   //ASDU模拟量(YC)起始地址
    int ykBaseAddr;   //ASDU控制量(YK)起始地址
    int spBaseAddr;   //ASDU设置点(SP)起始地址
    int ytBaseAddr;   //ASDU调节量(YT)起始地址
    int ddBaseAddr;  //ASDU电度量起始地址
    int fwBaseAddr;  //防误起始地址

    bool isCheckSeqNum;//是否检查接受发送序列号
    bool isYkWaitExecFinish;//遥控执行是否需要等待执行结束（遥控执行确认后，还会有一个遥控执行结束报文）
    bool isYtWaitExecFinish;//遥调执行是否需要等待执行结束（遥调执行确认后，还会有一个遥调执行结束报文）

    int spFullCodeVal;//设点归一化满码值(需要通讯双方约定，一般为：2<<15 即 32768)

    ProtocolCfgParam()
    {
        ykCmdMode = 1;
        spCmdMode = 1;
        soeTransMode = 1;
        cotNum = 2;
        comAddrNum = 2;
        infAddrNum=3;
        isCheckSeqNum=false;
        isYkWaitExecFinish=true;
        isYtWaitExecFinish=false;
    }
}ProtocolCfgParam_S;

//初始化时结构信息
typedef struct InitData
{
    ChnCfg_S chnCfg;
    ProtocolCfgParam_S protocolCfgParam;
    std::list<Rtu_S> rtuList;//该通道下的所有RTU列表

    Rtu_S curUsedRtu;////当前启用的RTU
}InitData_S;

#endif // __INITJSONDATA_H
