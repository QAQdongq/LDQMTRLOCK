/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  iec104zf.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：规约类Iec104Zf头文件
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/
#if 0//cfq 转发规约（子站）才用

#ifndef IEC104ZF_H
#define IEC104ZF_H

#include "poll.h"
#include "buffer_t.h"
#include "iec104def.h"

#define LINK_MAX_BUFSIZE    300
#define APP_MAX_BUFSIZE        256


struct IEC104ZF_RxReq
{
    uint8 type;
    uint8 cause;
    uint8 detail;
};

struct IEC104ZF_APP_LAYER
{
    uint8    State;            //应用层状态
    uint8    Inited_Flag;        //初始化标志
    uint16     Snd_seqnum;        //APDU 的发送序列号
    uint16    Rec_seqnum;        //APDU 的接收序列号
    uint16    Ack_num;        //DTE正确收到的APDU数目
    uint8    SendACKFlag;        //主站确认子站最后 I format标志
    uint8    WaitACKFlag;        //子站确认主站最后I format标志
    uint64    LastTxTime;        //应用层上次发送时间
    uint64    LastRxTime;        //应用层上次接收时间
    
    IEC104ZF_RxReq rxReq;        //应用层接收的转发请求
    uint8    AckStep;        //转发发送的步骤
    uint16    AckYcIdx;        //转发发送的Yc地址
    uint16     AckYxIdx;        //转发发送的Yx地址
    uint16     AckKwhIdx;        //转发发送的Kwh地址
    Buffer_T ChangeYxBuf;        //变化Yx缓冲区
    Buffer_T SoeBuf;        //变化事项缓冲区
    Buffer_T ChangeYcBuf;        //变化Yc缓冲区
    
    int    txSize;            //应用层发送数据长度
    uint8    txData[APP_MAX_BUFSIZE];//应用层发送数据缓冲区
    uint8    rxSize;            //应用层接收数据长度
    uint8    rxData[APP_MAX_BUFSIZE];//应用层接收缓冲区
    uint8    rxApdu[256];        //接收到的应用服务数据(ASDU),对钟时用上
    uint8    RxWindowNum;
    float32 LastYcVal[4096];//上次的遥测值
};

struct IEC104_ChangeYx
{
    uint16 idx;
    uint8  yxvalue;
};

struct IEC104_Soe
{
    uint16 idx;
    uint8 yxvalue;
    uint8 time[7];
};

#define FILE_NAME_LENGTH                      256
/* 逝去30天的秒数,该数值能被int型表示,不会溢出 */
#define HOW_MANY_SENCODS_AGO                    (-30*24*3600)

/**
 * @brief 104子站（或104转发站）规约
 */
class Iec104Zf: public Poll
{
protected:
    //sint32    rtu_no; //rtu索引号
    QString m_rtuId;        //rtu号
    int m_rtuAddr;      //rtu地址 目前智能分析子站时只有一个RTU
    IEC104ZF_APP_LAYER    App_Layer;
    IEC104_CONFIG_PARAM  Config_Param;
    int        rxStep;
    char delete_name_saved[FILE_NAME_LENGTH];

    int IfCheckLinkOk;   /* IfCheckLinkOk=0不检测 =1检测 */
    int IfAndPnBit;      /* IfAndPnBit=0不添加0x40 =1添加0x40 */
    int StartCharErr;    /* StartCharErr=0不断tcp链接 =1断tcp链接 */
    int AppTypeErr;      /* AppTypeErr=0返送"未知的类型标识" =1断tcp链接 */
    int CotErr;          /* CotErr=0返送"未知的传输原因" =1断tcp链接 */
    int ComAddrErr;      /* ComAddrErr=0返送"未知的公共地址" =1断tcp链接 */
    int SpecialCtrlField;
    unsigned char CtrlField[4];
    int IfCheckComAddr;   /* IfCheckComAddr=0不检测 =1检测 */
    int SpecialHighCot;
    unsigned char HighCot;
    int SpecialIpAddress;
    unsigned char IpAddress[8];

    //void MakeByteFileName( char *filename, char *delname );
    void DeleteOneDiskFile( char *filename );

    virtual void    InitData( void );

    int     App_Ack( void );
    int     App_Ack_GroupKwh( int group );
    int     App_Ack_Yk(COMMAND *command);
    int     App_Ack_Rst( void );
    int     App_Ack_SyncTime( void );
    int    App_Ack_AllKwh( void );
    int    App_Kwh_Conf( void );
    int    App_AckKwh_Finish( void );
    int    App_Ack_Kwh( void );
    int    App_Ack_AllData( void );
    int    App_AllData_Conf( void );
    int    App_All_Finish( void );
    int    App_Ack_Yx(COMMAND *command);
    int     App_Ack_Yc(COMMAND *command);
    int    App_Ack_GroupData( int group );
    int    App_Ack_GroupYx( int group );
    int    App_Ack_GroupYc( int group );
    int    App_Ack_ChangeYx( uint8 ack_type );
    int    App_Ack_Soe( void );
    int    App_Ack_ChangeYc( void );
    void    App_SearchChangeYc( void );
    void    App_SearchChangeYx( void );
    void    App_GetTime( uint8 *time );

    //Txproc
    int    App_SendAppIFormat( uint8 *asdu, int size );
    int    App_SendAppUFormat(int type);
    int    App_SendAppSFormat( bool atonceflag=false );
        
    //Rxproc
    void    App_SearchStartChar( void );
    void    App_SearchFrameHead( void );
    void    App_RxFixFrame( void );
    void    App_RxVarFrame( uint8 *apdu, int size );
    void    App_rxApdu( uint8 *asdu, int size );

    virtual int App_Ack_OtherType(int apptype, uint8 *appdata, int datalen);
    virtual int App_Recv_OtherType(uint8 *appdata, int datalen);
    virtual int EditLockFrame(COMMAND *cmd);//wyh 2019-12-05 add
    virtual bool IfCheckSeqNum();

    //void ReadConfigParam( void );
    int SendBackUnknownInfoData( int apptype, int cot, uint8* data, int len );

    /********************************************************************************
    *
    *    描述: 处理遥控应用服务数据单元(ASDU)
    *    参数: asdu  指向应用服务数据单元(ASDU) 的指针.
    *          size  应用服务数据单元(ASDU) 的长度
    *    返回: 无.
    *
    ********************************************************************************/
    int App_RxYk( uint8 *asdu, int size );
public:
    virtual int    RxProc();
    virtual int    TxProc();
    virtual void    SetProtocolConfig( PROTOCOL_CONFIG protocol_config );
    virtual void Restore();

    /**
     * @brief 设置规约配置参数
     * @param protocolCfg 规约配置参数
     */
    virtual void SetConfigParam(ProtocolCfgParam_S &protocolCfg);

    /**
     * @brief 应用层发送后台命令处理
     * @param command 发送的命令数据
     * @return  -1  发送失败，
     *            1  发送成功.
     */
    int App_SendCommand(COMMAND *command);
};

#endif    //IEC104ZF_H
#endif
