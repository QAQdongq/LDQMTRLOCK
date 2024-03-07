/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  protocol.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：定义规约类库的抽象基类Protocol.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef    PROTOCOL_H
#define PROTOCOL_H

//#include "data_type.h"
#include "../acq-protocol-plugin-utils/plugin_def.h"
#include "proto_def.h"
#include "prottype_def.h"
#include "proto_cmdmem.h"
#include "scn_routeinf.h"
#include "scn_rawdatadb.h"
#include "scn_rtuinf.h"
#include "scn_tasklist.h"
#ifdef PLATFORM_WIN32
 #include <windows.h>
extern struct timeval;
#else
 #include <sys/time.h>
#endif
#include "../acq-protocol-plugin-utils/data_buffer.h"
#include "data/send_json_data.h"
#include "data/init_json_data.h"
#include <QString>

#define ZIGBEE_CHANNEL_LIMIT
#undef ZIGBEE_CHANNEL_LIMIT

//定义解析规约的帧类型
#define TRANS_FRAME_LINK        1
#define TRANS_FRAME_YX            2
#define TRANS_FRAME_SOE            3
#define TRANS_FRAME_YK            4
#define TRANS_FRAME_YC            5
#define TRANS_FRAME_KWH            6
#define TRANS_FRAME_ALL            0xff

#define TRANS_PROTOCOL                  1
#define GET_RAWDATA                     2
#define GET_RAWDATA_ALL                 3

#define    GET_RAWDATA_AI            1
#define    GET_RAWDATA_DI            2 
#define    GET_RAWDATA_SOE            3
#define    GET_RAWDATA_YK            4

#define    MAX_SYNTIME_PIERIOD            1000 /*>=1000分钟,永不对时*/

#define TRANS_PROC_PROTOCOL_BUFSIZE            2560

/*质量码定义，与sicd.h中定义的应该保持一致
  此处单独定义是为了不依赖于平台，但要注意与sicd.h中的保持一致*/

/* 遥信量质量码定义*/
#define DI_STS_ON_LINE                     0x00000001
#define DI_STS_RESTART                     0x00000002
#define DI_STS_COMM_LOST                 0x00000004
#define DI_STS_REMOTE_FORCED         0x00000008
#define DI_STS_LOCAL_FORCED         0x00000010
#define DI_STS_CHATTER_FILTER     0x00000020
#define DI_STS_RESERVED                 0x00000040
#define DI_STS_STATE                         0x00000080
#define DI_STS_TRANSITONE             0x00000100
#define DI_STS_TRANSITTWO             0x00000200
#define DI_STS_OV                             0x00000400
#define DI_STS_BL                             0x00000800
#define DI_STS_SB                             0x00001000
#define DI_STS_NT                             0x00002000

/*模拟量质量码定义*/
#define AI_STS_ON_LINE                     0x00000001
#define AI_STS_RESTART                     0x00000002
#define AI_STS_COMM_LOST                 0x00000004
#define AI_STS_REMOTE_FORCED         0x00000008
#define AI_STS_LOCAL_FORCED         0x00000010
#define AI_STS_OVER_RANGE             0x00000020
#define AI_STS_REF_CHECK                 0x00000040
#define AI_STS_RESERVED                 0x00000080
#define AI_STS_OV                             0x00000100
#define AI_STS_BL                             0x00000200
#define AI_STS_SB                             0x00000400
#define AI_STS_NT                             0x00000800

/* 电度量质量码定义 */
#define CO_STS_ON_LINE                     0x00000001
#define CO_STS_RESTART                     0x00000002
#define CO_STS_COMM_LOST                 0x00000004
#define CO_STS_REMOTE_FORCED         0x00000008
#define CO_STS_LOCAL_FORCED         0x00000010
#define CO_STS_ROLLOVER                 0x00000020
#define CO_STS_RESERVED1                 0x00000040
#define CO_STS_RESERVED2                 0x00000080

struct PROTOCOL_CONFIG
{
    SCN_TaskList        *pTaskList;              //任务队列对象指针
    PROTO_CommandMem    *pCommandMem;        //命令缓冲区对象指针
    SCN_RouteInf        *pRouteInf;            //Route参数对象指针
    SCN_RawdataDb        *pRawDb;            //实时库操对象指针
    SCN_RtuInf           *pRtuInf;            //Rtu参数对象指针
    void                *reserve;            //保留,以备特殊用途。一般为NULL
};

/*下面的结构用于报文数据提取*/
#ifndef __CP56TIME__DEF
#define __CP56TIME__DEF
/*CP56Time在prot_def.h也有定义*/
typedef struct            /*7字节时标*/
{
    uint16    ms;
    uint8    minute;
    uint8    hour;
    uint8    date;
    uint8    month;
    uint8    year;
}CP56Time;
#endif

typedef struct            /*yc数据结构*/
{                    
    float value;            /*值*/
    uint32 quaflag;        /*质量码*/                
}TIME_SEQ_VALUE_YC;    
                                        
typedef struct            /*yx数据结构*/
{    
    uint32 quaflag;        /*质量码*/                                
    uint8 value;            /*值*/                                            
}TIME_SEQ_VALUE_YX;
                                        
typedef struct            /*soe数据结构*/
{    
    CP56Time    soetime;/*时标*/
    uint32 quaflag;        /*质量码*/                                
    uint8 value;            /*值*/                                            
}TIME_SEQ_VALUE_SOE;
        
typedef struct            /*yk数据结构*/
{    
    uint8 oper;                /*操作:1-预置,2-执行,3-自动执行,4-撤销*/
    uint8 value;            /*值:1-闭合，2-断开*/                                            
    uint8 result;            /*结果:0-成功，其他：失败*/
}TIME_SEQ_VALUE_YK;        

typedef struct 
{
    uint8            dir;        /*0:接收报文,1:发送报文*/
    CP56Time    time;        /*时标*/
    union    
    {
        TIME_SEQ_VALUE_YC        ycvalue;    /*遥测值结构*/
        TIME_SEQ_VALUE_YX        yxval;        /*遥信值结构*/
        TIME_SEQ_VALUE_SOE     soevalue;    /*soe值结构*/
        TIME_SEQ_VALUE_YK         ykvalue;    /*遥控结构*/
    };
}TIME_SEQ_VAL;


typedef struct 
{
    CP56Time    time;                    /*时标*/
    uint8    dir;                            /*0:接收报文,1:发送报文*/
    int frame_len;                    /*报文长度*/
    int    start_place;                /*报文起始地址*/
}TIME_SEQ_FRAME_INFO;


typedef struct 
{
    uint8            dir;        /*0:接收报文,1:发送报文*/
    CP56Time    time;        /*时标*/
    uint16 pointno;   /*点号*/
    union    
    {
        TIME_SEQ_VALUE_YC        ycvalue;    /*遥测值结构*/
        TIME_SEQ_VALUE_YX        yxval;        /*遥信值结构*/
        TIME_SEQ_VALUE_SOE     soevalue;    /*soe值结构*/
        TIME_SEQ_VALUE_YK         ykvalue;    /*遥控结构*/
    };
}TIME_SEQ_VAL_ALL;

class Protocol: public QObject
{
    Q_OBJECT
protected:

    DataBuffer            *pRxBuf; //接收数据缓存
    SCN_RouteInf        *pRouteInf;
    SCN_RawdataDb        *pRawDb;
    SCN_RtuInf           *pRtuInf;
    PROTO_CommandMem    *pCommandMem;
    AcqProtocolInterface *m_protoInterface;
    SCN_TaskList        *pTaskList;

    void    GetCurTime( SYSTIME *);        //取得当前系统时间,写入SYSTIME结构
    void    GetCurSec( TIME *);            //取得当前系统时间,写入TIME结构
    SYSTIME    GetTime(SYSTIME starttime,uint64 diffms); //取得statrtime偏移diffms毫秒后的时间.
    SYSTIME    GetTime(SYSTIME starttime,int diffsec,int diffms); //取得statrtime偏移diffsec秒diffms毫秒后的时间.
    uint64    GetMilliSeconds(SYSTIME curtime,SYSTIME basetime);    //取得两个时刻的时间间隔,按毫秒计.
    int GetTimeOfDay(struct timeval *ptv);//0:success,-1:fail

    uint16 BinToBcd(uint16 sdata);

    //记录规约处理中的特殊事件,存于protocol.log中,该文件路径由环境变量PROTO_LOG_PATH设定.
    void Log_Event( uint8 *buf, uint32 num, const char *format, ... );
public:
    static uint8 api_61850_running;
    char *TransOutBuf;
    uint32 TransBufLen;
    uint8     RawDataType;                    /*提取的生数据类型*/    
    int      RawDatano;                        /*提取的生数据点号 */
    TIME_SEQ_VAL *pRawDataBuf;    /*提取的生数据缓存区*/
    TIME_SEQ_VAL_ALL *pAllRawDataBuf; /*提取的所有数据缓冲区*/
    int    RawDataNum;                            /*提取的生数据个数*/    

    CP56Time trans_frame_time;
    uint8 trans_dir,trans_func;
    int max_rawdata_num;
    uint8     TransFlag;

  bool restore_flag;
    Protocol( );
    virtual ~Protocol( );

    virtual int  RxProc()=0;
    virtual int  TxProc()=0;
    virtual void SetProtocolConfig( PROTOCOL_CONFIG protocol_config );
    virtual void SetTransProtocolConfig( const QString &rtuId, char *filename = NULL, uint8 Flag = TRANS_PROTOCOL );
    virtual void Restore()=0;
    virtual int  TransProc(uint8 dir,uint8 *inbuf,int inlen,char* &outbuf,uint8 frametype = 0xff) = 0;

    void ClearYxSoeVarOut();
    int WriteTransOutBuf(const char *buf);
    int WriteOneRawData(TIME_SEQ_VAL OneRawData,int max_rawdata_num);
    int WriteAllRawData(TIME_SEQ_VAL_ALL OneRawData);
    //static void GetDebugConfig();
    static sint32 test_rtuId;
    static bool rfid_flag;
    static uint8 rfid_code[32];
    static bool get_rfid_flag( );
    static void clr_rfid_flag( );
    static uint8 *get_rfid_code( );

    virtual bool FeedProtocolBytes(std::string bytes );
    virtual bool FeedProtocolBytes(const QByteArray &data);

    /* 下行报文帧 */
    virtual bool AddNeedSendFrame(std::string frame );
    virtual bool AddNeedSendFrame(uint8 *val, int size);

    virtual void SetConfigParam( ProtocolCfgParam_S &protocolCfg)=0;

    inline void setProtocolInterface(AcqProtocolInterface *protoInterface){m_protoInterface=protoInterface;}


};

#endif //PROTOCOL_H

