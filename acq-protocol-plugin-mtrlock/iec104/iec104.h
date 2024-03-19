/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  iec104.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：定义Iec104规约类
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef IEC104_H
#define IEC104_H

#include <list>
#include "poll.h"
#include "ieccomm.h"

#include "iec104def.h"

#define FILE_NAME_LENGTH                      256
/* 逝去30天的秒数,该数值能被int型表示,不会溢出 */
#define HOW_MANY_SENCODS_AGO                    (-30*24*3600)

/**
 * @brief 104主站规约
 */
class Iec104: public Poll
{
protected:
    //uint32 m_rtuId;
    //uint32 m_rtuAddr;
    std::list<int> m_rtuAddrList;//RTU地址列表
    IEC104_APP_LAYER    App_Layer;
    IEC104_CONFIG_PARAM Config_Param;
private:
    int            Time_cor;
    int            rxStep;
    char delete_name_saved[FILE_NAME_LENGTH];

    void DeleteOneDiskFile( char *filename );

    void    InitData( void );
    
    //Txproc
    int     App_ResendData( void );
    int    App_SendCommand(COMMAND *command );
    int    App_CallAllKwh(int rtuAddr);
    int    App_CallAllData(int rtuAddr);
    int    App_SyncTime(int rtuAddr);
    void    App_CheckState( void );
    int    App_SendYk45(COMMAND *command );
    int    App_SendYk(COMMAND *command );

    int     makeOneWFJSAsdu(uint8 *buf, uint8 buf0, uint8 pointNum,  uint8 seq_bit7_bit6, uint8 frameCnt, uint8 *pointBuf, int pointBufSize);
    int    App_SendWFJS(COMMAND* command);//liujie add  添加防误操作请求发送
    int    App_SetPasswordReg(COMMAND* command);//lidongqing add  添加设置密码锁密码命令

    void    App_SyncTimeCoding( uint8 *buf );
    int    App_SendSetPoint(COMMAND *command );
    int    App_SendSp(COMMAND *command );
    int    App_SendYt47(COMMAND *command );
    
    //Rxproc
    //重置召唤时间
    void    App_ResetLastTime( void );
    void    App_CheckTime( void );
    void    App_SearchStartChar( void );
    void    App_SearchFrameHead( void );
    void    App_RxFixFrame( void );
    void    App_RxMtrLockFrame( uint8 *apdu, int size );
    void    App_RxVarFrame( uint8 *apdu, int size );
    void    App_RxDAResult( uint8 *asdu, int size );
    void    MakeDAReportTime(uint8 *ptr , FETIME * atime);

    void    App_RxAllDataConf( uint8 *asdu, int size );
    void    App_RxAllKwhConf( uint8 *asdu, int size );
    void    App_RxTimeConf( uint8 *asdu, int size );
    void    App_RxYcFrame( uint8 *asdu, int size );
    virtual     void    App_RxSoeFrame( uint8 *asdu, int size );
    void    App_RxKwhFrame( uint8 *asdu, int size );
    void    App_RxYxFrame( uint8 *asdu, int size );
    void    App_RxYkConf( uint8 *asdu, int size );
    void    App_RxWFConf( uint8 *asdu, int size );//liujie add 20230525 五防响应报文接收处理
    sint32    App_YcDecode( uint8 *buf );
    sint32    App_KwhDecode( uint8 *buf );
    void    App_RxSPConf( uint8 *apdu, int size );
    void    App_RxYt47Conf(uint8 *asdu, int size);
    
    uint32 App_YcQcExchange(uint8 source_qc);
    uint32 App_YxQcExchange(uint8 source_qc,bool dpflag=false,uint8    *yxvalue=NULL);
    uint32 App_KwhQcExchange(uint8 source_qc);

    int    App_CallGrpKwh(int rtuAddr,  int group );
    int    App_CallGrpData(int rtuAddr,  int group );
protected:
    //Transproc
    int  TransLpduLayer( uint8 *inbuf,uint8 frametype = 0xff );
    int  TransApduLayer( uint8 *inbuf,uint8 frametype = 0xff );
    void  TransYxFrame( uint8 *asdu, int size );
    void  TransAllDataConf( uint8 *asdu, int size );
    void  TransAllKwhConf( uint8 *asdu, int size );
    void  TransTimeConf( uint8 *asdu, int size );
    void  TransYkConf( uint8 *asdu, int size );
    void  TransSetPointConf( uint8 *asdu, int size );
    void  TransYcFrame( uint8 *asdu, int size );
    void  TransSoeFrame( uint8 *asdu, int size );
    void  TransKwhFrame( uint8 *asdu, int size );

    int    App_SendAppIFormat( uint8 *asdu, int size );
    int    App_SendAppUFormat(int type);
    int    App_SendAppSFormat( bool atonceflag=false );

    virtual void App_RxOtherType(uint8 *appdata, int datalen);
    virtual int App_SendOtherCmd(COMMAND *command );

public:
    Iec104();
    ~Iec104();
    virtual int    RxProc();
    virtual int    TxProc();
    virtual void SetProtocolConfig( PROTOCOL_CONFIG protocol_config );
    virtual void Restore();
    virtual int TransProc( uint8 dir, uint8 *inbuf, int inlen, char* &outbuf, uint8 frametype = 0xff );

    virtual void SetConfigParam( ProtocolCfgParam_S &protocolCfg);
};

#endif    //IEC104_H
