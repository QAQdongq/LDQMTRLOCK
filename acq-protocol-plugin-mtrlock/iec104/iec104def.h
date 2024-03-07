/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  iec104def.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：定义Iec104规约类中的宏
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef IEC104DEF_H
#define IEC104DEF_H

#if 1//CFQ
#include "sicd.h"
#endif

//帧类型定义
#define FRMTYPE_SEND_NC                    1
#define FRMTYPE_SEND_WC                    2
#define FRMTYPE_REQ_RESP                3

//定义应用层状态
#define IEC104_APP_STATE_IDLE                0    
#define IEC104_APP_STATE_WAITALLDATACONF        1
#define IEC104_APP_STATE_WAITTIMECONF            2
#define IEC104_APP_STATE_WAITALLKWHCONF            3
#define IEC104_APP_STATE_WAITALLDATA            4
#define IEC104_APP_STATE_WAITALLKWH            5
#define IEC104_APP_STATE_WAITGRPDATA            6
#define IEC104_APP_STATE_WAITGRPKWH            7
#define IEC104_APP_STATE_WAITYKCONF            8
#define IEC104_APP_STATE_WAITYKFINISH            9
#define IEC104_APP_STATE_UNRESET            10
#define IEC104_APP_STATE_WAITSTARCONF            11
#define IEC104_APP_STATE_WAITSTOPCONF            12
#define IEC104_APP_STATE_WAITTESTCONF            13
#define IEC104_APP_STATE_ACK                14
#define IEC104_APP_STATE_WAITYTCONF            15   //等待遥调确认
#define IEC104_APP_STATE_WAITYTFINISH            16  //等待遥调完成
#define IEC104_APP_STATE_WAITPROTFINISH                17  //等待保护命令完成
#define IEC104_APP_STATE_WAITWAVEFINISH            18  //等待召唤录波文件命令完成
#define IEC104_APP_STATE_WAITSPCONF            19      //等待设置点确认
#define IEC104_APP_STATE_WAITSPFINISH            20    //等待设置点完成
#define IEC104_APP_STATE_WAITFWCONF            21    //等待防误确认

//TYPEID类型标识
#define APPTYPE_SP_NT                    1    //不带时标遥信
#define APPTYPE_SP_WT                    2    //带时标遥信即SOE
#define APPTYPE_DP_NT                    3
#define APPTYPE_DP_WT                    4
#define APPTYPE_ME_NT                    9    //不带时标的测量值
#define APPTYPE_ME_WT                    10    //带时标的测量值
#define APPTYPE_ME_NT11                    11
#define APPTYPE_ME_FLOAT                13
#define APPTYPE_CO_NT                    15    //不带时标电能脉冲计数
#define APPTYPE_CO_WT                    16    //带时标电能脉冲计数
#define APPTYPE_PS_STS_NT                20
#define APPTYPE_ME_ND                    21    //不带品质描述的测量值
#define APPTYPE_SP_WT30                    30  //单点SOE
#define APPTYPE_DP_WT31                    31  //双点SOE
#define APPTYPE_ME_TD                    34    //带长时标的归一化测量值
#define APPTYPE_ME_TE                    35    //带长时标的标度化测量值
#define APPTYPE_CO_TE                    37    //带长时标的电度量值
#define APPTYPE_YK45                      45 //单点遥控
#define APPTYPE_YK46                      46 //双点遥控
#define APPTYPE_YT47                      47
#define APPTYPE_SP48                    48
#define APPTYPE_SP49                      49
#define APPTYPE_SP50                      50
#define APPTYPE_SP51                      51
#define APPTYPE_CALLDATA                100
#define APPTYPE_CALLKWH                    101
#define APPTYPE_READDATA                102
#define APPTYPE_TIMESYNC                103
#define APPTYPE_TEST                    104
#define APPTYPE_RSTPROC                    105
#define APPTYPE_CALLWAVE_FILE_READY            120    //文件准备就绪
#define APPTYPE_CALLWAVE_SECTION_READY            121    //节准备就绪
#define APPTYPE_CALLWAVE                122    //召唤目录，选择文件，召唤文件，召唤节
#define APPTYPE_CALLWAVE_LAST                123    //最后的节，最后的段
#define APPTYPE_CALLWAVE_CONFIRM            124    //认可文件，认可节
#define APPTYPE_CALLWAVE_FRAME                125    //段
#define APPTYPE_CALLWAVE_FILELIST            126    //目录

#define APPTYPE_WF200                    200 //五防：防误校验-0xC8
#define APPTYPE_WF201                    201 //五防：应急模式-0xC9


#define APPTYPE_UTPROTCMD                250      //优特保护定值命令

//传送原因
#define APP_COT_PER_CYC                    1
#define APP_COT_BACKSCAN                2
#define APP_COT_SPONT                    3
#define APP_COT_INIT                    4
#define APP_COT_REQ                    5
#define APP_COT_ACT                    6
#define APP_COT_ACT_CON                    7    //激活确认
#define APP_COT_DEACT                    8
#define APP_COT_DEACT_CON                9    //停止激活确认
#define APP_COT_ACT_TERM                10    //激活结束
#define APP_COT_FILE_TRANS                13    //文件传输
#define APP_COT_UNKNOWN_TYPE            44
#define APP_COT_UNKNOWN_COT                45
#define APP_COT_UNKNOWN_COMADDR            46
#define APP_COT_UNKNOWN_INFOADDR        47

#define APP_COT_RESP_CALLALL                20
#define APP_COT_RESP_GRP1                21    //何用？
#define APP_COT_RESP_GRP2                22
#define APP_COT_RESP_GRP3                23
#define APP_COT_RESP_GRP4                24
#define APP_COT_RESP_GRP5                25
#define APP_COT_RESP_GRP6                26
#define APP_COT_RESP_GRP7                27
#define APP_COT_RESP_GRP8                28
#define APP_COT_RESP_GRP9                29
#define APP_COT_RESP_GRP10                30
#define APP_COT_RESP_GRP11                31
#define APP_COT_RESP_GRP12                32
#define APP_COT_RESP_GRP13                33
#define APP_COT_RESP_GRP14                34
#define APP_COT_RESP_GRP15                35
#define APP_COT_RESP_GRP16                36

#define APP_COT_RESP_CO                    37
#define APP_COT_RESP_CO_GRP1                38        //何用？
#define APP_COT_RESP_CO_GRP2                39
#define APP_COT_RESP_CO_GRP3                40
#define APP_COT_RESP_CO_GRP4                41


#define APP_COT_ACT_CON_NO        71 //0x47 否定激活确认

#define APP_UFORMAT_STARACT        0x04
#define    APP_UFORMAT_STARCON        0x08
#define    APP_UFORMAT_STOPACT        0x10
#define    APP_UFORMAT_STOPCON        0x20
#define    APP_UFORMAT_TESTACT        0x40
#define    APP_UFORMAT_TESTCON        0x80

#define ACK_UNSOLICITED                    0
#define ACK_REQUESTED                    1

#define IEC104_YXNUM_PERGROUP            127
#define IEC104_YCNUM_PERGROUP            60    //121            //IEC104的ASDU长度限制为249
#define IEC104_KWHNUM_PERGROUP            16    //32

#define IEC104_GROUP_MAX_NUM            12
/*广东关于未被确认的I格式APDU的最大数目k值和最迟确认APDU的最大数目w值的要求是12和8*/
#define IEC104_WINDOW_MAX_SIZE            8/*w值*/

#if 0
#define COMMAND_SELECT_FAIL_REPLAY_TO_SCADA            0//遥控预置失败
#define COMMAND_SELECT_SUCCESS_REPLAY_TO_SCADA    1//遥控预置成功
#define COMMAND_LINK_BUSY_REPLAY_TO_SCADA                2//链路忙
#define COMMAND_UNKNOWN_YKNO_REPLAY_TO_SCADA        3//未知的遥控号
#define COMMAND_YKCANCEL_SUCCESS_TO_SCADA                4//遥控撤销成功
#define COMMAND_YKEXEC_SUCCESS_TO_SCADA                    5//遥控执行成功
#endif
#define PROCMD_COMMON_SERVER_READ_REPLY_CON 32  //return protectcmd wyh 2010-02-16
#define  PROCMD_COMMON_SERVER_READ_REPLY                     23             //通用服务读/写命令响应

struct IEC104_CONFIG_PARAM
{
    uint16    InfBaseAddrVersion;        //ASDU信息体基地址版本号
    uint8    YKCmdMode;        //遥控命令使用(1:单点命令)还是(2:双点命令)
    uint8    SPCmdMode;        //设点命令使用(1:单个设点命令)还是(2:连续设点命令)
    uint8    SOETransMode;    //SOE是(1:一次传输,要根据SOE生成YX)还是(2:二次传输)
    uint8    QcEnable;            //处理数据质量码
    
    uint8    UseCallGrop;    //0:不使用分组召唤,1:使用分组召唤
    uint8    UseTimeDelay;    //0:不使用延时采集,1:使用延时采集
    uint8    UseType9NVA;    //0:类型标识9传归一化值,1:类型标识9传标度化值
    uint8    UseQuality;        //0:不处理品质,1:处理品质

    uint8    CotNum;            //ASDU传输原因字节数
    uint8    ComAddrNum;        //ASDU公共地址字节数
    uint8    InfAddrNum;        //ASDU信息体地址字节数


    uint32  LinkTimeout;        //链路应答超时
    uint32  YkTimeout;         //遥控/遥调超时时间（秒）
    uint32  SpTimeout;         //设置点执行命令超时（秒）
    uint32  FwTimeout;         //防误执行命令超时（秒）

    uint32  YxBaseAddr;        //ASDU状态量起始地址
    uint32  YcBaseAddr;        //ASDU模拟量起始地址
    uint32  YkBaseAddr;        //ASDU控制量起始地址
    uint32  YkLastAddr;        //ASDU控制量结束地址
    uint32  SpBaseAddr;        //ASDU调节量起始地址
    uint32  YtBaseAddr;        //ASDU分接头起始地址
    uint32  YtLastAddr;        //ASDU分接头结束地址
    uint32  DdBaseAddr;    //ASDU电度量起始地址
    uint32  FwBaseAddr;    //防误点起始地址


    uint8    MaxIframeNum;    //最后确认的I帧最大数目
    uint8    MaxIsendNum;    //未被确认的I帧最大数目

    uint8    OverTime1;    //I帧或U帧发送后,经过OverTime1超时
    uint8    OverTime2;    //收到I帧后,经过OverTime2无数据报文时,发送S帧,OverTime2<OverTime1
    uint8    OverTime3;    //长期空闲,经过OverTime3超时,发送测试帧,OverTime3>OverTime1

    //uint8   ackCalldataTimeout; //总召确认超时时间
    //uint8   ackYkTimeout; //遥控确认超时时间


    bool isCheckSeqNum;//是否检查接受发送序列号
    bool isYkWaitExecFinish;//遥控执行是否需要等待执行结束（遥控执行确认后，还会有一个遥控执行结束报文）
    bool isYtWaitExecFinish;//遥调执行是否需要等待执行结束（遥调执行确认后，还会有一个遥调执行结束报文）

    uint32 SpFullCodeVal;//设点归一化满码值(需要通讯双方约定，一般为：2<<15 即 65536)
};

#endif
