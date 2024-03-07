/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  protocol.cxx
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：规约类库抽象基类Protocol的实现.
 *其它说明：description
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#include "stdafx.h"

#if defined (PLATFORM_WIN32)
    #include <WinSock2.h>
    #include <time.h>
    #include <windows.h>    
    #include <sys/timeb.h>
    #include <wtypes.h>
#else
    #include <sys/time.h>
    #include <stdarg.h>
    #include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "protocol.h"
#include "scn_routeinf.h"
#include "scn_rawdatadb.h"
#include "scn_rtuinf.h"
#include <QString>
#include "../acq-protocol-plugin-utils/string_util.h"

uint8 Protocol::api_61850_running = 0;
sint32 Protocol::test_rtuId = -1;
bool Protocol::rfid_flag = false;
uint8 Protocol::rfid_code[32] = {0};
/********************************************************************************
*        描述: 当该规约不支持整站转发处理模式时,执行该函数清理掉可能的Yx(Soe)_Var_Out.
*        参数: 无；
*        返回: 无.
********************************************************************************/
void Protocol::ClearYxSoeVarOut()
{
    int yxno;
        uint8 change, yxvalue;
        SOEDATA changesoe;
       while(pRawDb->GetVarYxNum(pRouteInf->GetRtuId(),pRouteInf->GetChnId())>0 )
       {
           pRawDb->GetAVarYx( pRouteInf->GetRtuId(),pRouteInf->GetChnId(),&yxno, &yxvalue, &change );
           printf("\nerror!rtuId=%s chano=%s have some varyx\n",pRouteInf->GetRtuId().toStdString().c_str(),pRouteInf->GetChnId().toStdString().c_str());
       }

       while(pRawDb->GetVarSoeNum(pRouteInf->GetRtuId(),pRouteInf->GetChnId())>0 )
       {
           pRawDb->GetAVarSoe( pRouteInf->GetRtuId(),pRouteInf->GetChnId(),&yxno, &changesoe );
           printf("\nerror!rtuId=%s chano=%s have some varsoe\n",pRouteInf->GetRtuId().toStdString().c_str(),pRouteInf->GetChnId().toStdString().c_str());
       }
}

/********************************************************************************
*        描述: 规约参数设置函数,负责初始化规约接口对象.
*        参数: protocol_config: 规约配置参数(输入)；
*        返回: 无.
********************************************************************************/
void Protocol::SetProtocolConfig( PROTOCOL_CONFIG protocol_config )
{
    pCommandMem = protocol_config.pCommandMem;
    pRawDb    = protocol_config.pRawDb;
    pRtuInf = protocol_config.pRtuInf;
    pRouteInf = protocol_config.pRouteInf;
    pTaskList= protocol_config.pTaskList;
}

/********************************************************************************
*        描述: 取得当前系统时间,写入curtime.
*        参数: 无；
*        返回: 无.
********************************************************************************/
void Protocol::GetCurTime( SYSTIME *curtime )
{

    #if defined (_WIN32)
        SYSTEMTIME  localtime;
        GetLocalTime( &localtime );

        curtime->year = localtime.wYear;
        curtime->month = (uint8) localtime.wMonth;
        curtime->day = (uint8) localtime.wDay;
        curtime->week = (uint8) localtime.wDayOfWeek;//Sunday is 0,Monday is 1;
        curtime->hour = (uint8) localtime.wHour;
        curtime->minute = (uint8) localtime.wMinute;
        curtime->second = (uint8) localtime.wSecond;
        curtime->ms = localtime.wMilliseconds;
    #elif defined(__unix)
            struct tm *ttm;
            struct timeval ttv;
        time_t    ti;

            gettimeofday( &ttv ,NULL);
        time((time_t *)&ti);
        ttm = localtime(&ti);

            curtime->ms = (uint16) (ttv.tv_usec/1000);
            curtime->second = (uint8) ttm->tm_sec;
            curtime->minute = (uint8) ttm->tm_min;
            curtime->hour = (uint8) ttm->tm_hour;
            curtime->week = (uint8) ttm->tm_wday;
            curtime->day = (uint8) ttm->tm_mday;
            curtime->month = (uint8) ttm->tm_mon+1;
            curtime->year = (uint16) ttm->tm_year+1900;
    #endif

}

/********************************************************************************
*        描述: 取得当前系统时间,写入curtime.
*        参数: 无；
*        返回: 无.
********************************************************************************/
void Protocol::GetCurSec(TIME *curtime)
{
        #if defined (_WIN32)
        time_t    ti;
        SYSTEMTIME  localtime;

        time(&ti);
        curtime->Sec = (uint32)ti;
        GetLocalTime( &localtime );
        curtime->Ms = localtime.wMilliseconds;

        #elif defined(__unix)
        struct timeval ttv;
        gettimeofday( &ttv ,NULL);
        curtime->Sec = ttv.tv_sec;
        curtime->Ms = ttv.tv_usec/1000;
        #endif

}

/********************************************************************************
*        描述: 取得statrtime偏移diffms毫秒后的时间..
********************************************************************************/
SYSTIME Protocol::GetTime(SYSTIME starttime,uint64 diffms)
{
  int seconds = diffms/1000;
  int millis = diffms%1000;

    return GetTime( starttime, seconds, millis );
}

/********************************************************************************
*        描述: 取得statrtime偏移diffsec秒diffms毫秒后的时间..
********************************************************************************/
SYSTIME Protocol::GetTime(SYSTIME starttime,int diffsec,int diffms)
{
    SYSTIME    curtime;
    struct    tm ttm,*ttmp;
    time_t    t;
    unsigned short    ms;

    ttm.tm_year = starttime.year-1900;
    ttm.tm_mon = starttime.month-1;
    ttm.tm_mday = starttime.day;
    ttm.tm_hour = starttime.hour;
    ttm.tm_min = starttime.minute;
    ttm.tm_sec = starttime.second;
    ttm.tm_isdst = 0;
    
    t = mktime(&ttm);
    t += diffsec;
    t += (diffms+starttime.ms)/1000;
    ms = (diffms+starttime.ms)%1000;

    ttmp = localtime(&t);
    curtime.year = ttmp->tm_year + 1900;
    curtime.month = ttmp->tm_mon + 1;
    curtime.day = ttmp->tm_mday;
    curtime.hour = ttmp->tm_hour;
    curtime.minute = ttmp->tm_min;
    curtime.second = ttmp->tm_sec;
    curtime.ms = ms;

    return    curtime;
}

/********************************************************************************
*        描述: 取curtime与basetime的时间间隔，按毫秒计．
*        返回: 时间间隔(毫秒).
********************************************************************************/
uint64 Protocol::GetMilliSeconds(SYSTIME curtime,SYSTIME basetime)
{
        struct  tm ttm,*ttmp;
        time_t  t,t1;
        uint64  ms, lt, lt1;

        ttm.tm_year = basetime.year-1900;
        ttm.tm_mon = basetime.month-1;
        ttm.tm_mday = basetime.day;
        ttm.tm_hour = basetime.hour;
        ttm.tm_min = basetime.minute;
        ttm.tm_sec = basetime.second;
        ttm.tm_isdst = 0;
        
        t = mktime(&ttm);

    ttm.tm_year = curtime.year-1900;
        ttm.tm_mon = curtime.month-1;
        ttm.tm_mday = curtime.day;
        ttm.tm_hour = curtime.hour;
        ttm.tm_min = curtime.minute;
        ttm.tm_sec = curtime.second;
        ttm.tm_isdst = 0;
    t1 = mktime(&ttm);

    lt1 = t1;
    lt = t;

    ms = (lt1-lt)*1000+curtime.ms-basetime.ms;

    return ms;
}

/*********************************************************************************/
/****获得当日逝去的秒数及毫秒数                                   ****************/          
/****返回值：0:success,1:failure                                  ****************/
/*********************************************************************************/

int Protocol::GetTimeOfDay(struct timeval *ptv)
{
#ifdef _WIN32
    struct _timeb timebuf;

    if(ptv==NULL)
    {
        return -1;
    }

    _ftime(&timebuf);

    ptv->tv_sec  = timebuf.time;
    ptv->tv_usec = timebuf.millitm*1000;
    return 0;

#else  /*end of NT version*/

    int ret_val; 
    struct timezone tz; 
    
    if(ptv==NULL)
    {
        return -1;
    }

    ret_val = gettimeofday((struct timeval*)ptv, &tz);
    return ret_val;

#endif /*end of Unix version*/
}



/********************************************************************
*    描述: 记录规约处理中的特殊事件,存于protocol.log中,该文件路径
*          由环境变量PROTO_LOG_PATH设定.
*    参数: evedesc: 事件描述信息;
*          buf: 记录数据缓冲区;
*          num: buf中的字节数.
********************************************************************/
void Protocol::Log_Event( uint8 *buf, uint32 num, const char *format, ... )
{
    char logfile[128]="", *logfile_path;
    FILE *fp;
    struct stat logfile_stat;
    va_list argp;

    logfile_path = getenv( "PROTO_LOG_PATH" );
    if( logfile_path )
        sprintf( logfile, "%s", logfile_path );

    if( logfile[ strlen(logfile)-1 ] != '/' )
        strcat( logfile, "/" );    
    strcat( logfile, "protocol.log" );

    if( stat( logfile, &logfile_stat ) == -1 )
    {
        fp = fopen( logfile, "w" );
        //return;
    }
    else
    {
        if( logfile_stat.st_size>= 512*1024 )    //over 512k bytes
            fp = fopen( logfile, "w" );
        else fp = fopen( logfile, "a" );
    }

    if( fp == NULL ) 
        return;

    time_t curtime;    
    time( &curtime );

    fprintf(fp, "%s", ctime( &curtime ) );

    va_start( argp, format );
    vfprintf(fp, format, argp );
    va_end( argp );
    fprintf(fp, "\n");

    if( buf )
    {
        fprintf( fp, "buffer data:\n" );
        for( int i=0; i<num; i++ )
        {
            fprintf( fp, "%02x ", buf[i] );
            if( i%16 == 15 )
                fprintf( fp, "\n" );
        }
        fprintf( fp, "\n" );
    }
    fprintf(fp,"\n\n");       

    fclose( fp );
}


uint16 Protocol::BinToBcd(uint16 sdata)
{
        uint16 ddata = 0;
        uint16 temp = 0;

        for(int i = 0;i < 4;i++)
        {
                temp = sdata%10;
                temp <<= i*4;
                ddata |= temp;
                sdata /= 10;
        }
        return ddata;
}


/************************************************************************
作者：   yay     
创建日期：2010.10.10  
函数功能说明： 解析规约配置参数设置函数
输入参数：
             RtuId: 厂站号(输入)；
             char *filename 文件名(输入)；
             uint8 Flag 标志；
输出参数：  
             无

返回: 无.
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void Protocol::SetTransProtocolConfig( const QString &rtuId, char *filename, uint8 Flag )
{
    TransFlag = Flag;
}

/************************************************************************
作者：   yay     
创建日期：2010.10.10  
函数功能说明： 规约翻译的中间部分内容写入到翻译结果缓冲区TransOutBuf
输入参数：
             const char *buf  规约翻译的中间部分内容
输出参数：  
             无
            
返回: >0 成功  <0 失败.
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
int Protocol::WriteTransOutBuf( const char *buf )
{
    int addlen = TRANS_PROC_PROTOCOL_BUFSIZE;

    if( buf == NULL )
    {
        printf("\nProtocol::WriteTransOutBuf buf=NULL\n");
        return -1;
    }

    int buflength = strlen(buf);
    if( (buflength+1) > TRANS_PROC_PROTOCOL_BUFSIZE )
        addlen = buflength + 1;

    int translength = 0;
    if( TransOutBuf )
        translength = strlen(TransOutBuf);
    else
    {
        TransOutBuf = (char *)calloc( 1, TRANS_PROC_PROTOCOL_BUFSIZE );
        if( TransOutBuf == NULL )
        {
            printf("\nProtocol::WriteTransOutBuf calloc fail\n");
            TransBufLen = 0;
            return -2;
        }
        
        TransBufLen = TRANS_PROC_PROTOCOL_BUFSIZE;
        translength = 0;
    }

    if( translength + buflength >= TransBufLen )
    {
        if( TransOutBuf )
            TransOutBuf = (char *)realloc( TransOutBuf, TransBufLen+addlen );
        else
            TransOutBuf = (char *)calloc( 1, TransBufLen+addlen );

        if( TransOutBuf == NULL )
        {
            printf("\nProtocol::WriteTransOutBuf calloc or realloc fail\n");
            TransBufLen = 0;
            return -3;
        }

        TransBufLen += addlen;
    }

    strcat( TransOutBuf, buf );
    return 1;
}

bool Protocol::get_rfid_flag( )
{
    return rfid_flag;
}

void Protocol::clr_rfid_flag( )
{
    rfid_flag = false;
}

uint8 * Protocol::get_rfid_code( )
{
    return rfid_code;
}

int Protocol::WriteOneRawData(TIME_SEQ_VAL OneRawData,int max_rawdata_num)
{
    return 1;
}

int Protocol::WriteAllRawData(TIME_SEQ_VAL_ALL OneRawData)
{
    return 1;
}

/************************************************************************
作者：
创建日期：
函数功能说明： Protocol构造函数
             无
输出参数：  
             无
            
返回: = 1, 解析成功返回;
      =-1, 解析未成功返回.
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
Protocol::Protocol( )
{
    pRxBuf = new DataBuffer();
    pRxBuf->create(SCN_SYNCH_BUFF_LEN);
    restore_flag = false;
    TransOutBuf = NULL;
    TransBufLen = 0;
    RawDataType = 0;
    RawDatano = -1;
    pRawDataBuf = NULL;
    RawDataNum = 0;
    pAllRawDataBuf = NULL;
    TransFlag = 0;
}

/************************************************************************
作者：
创建日期：
函数功能说明： Protocol析构函数
             无
输出参数：  
             无
            
返回: = 1, 解析成功返回;
      =-1, 解析未成功返回.
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
Protocol::~Protocol( )
{
    if(TransOutBuf)
    {
        free(TransOutBuf);
    }

    TransOutBuf = NULL;
    TransBufLen = 0;

    if(pRxBuf)
    {
        pRxBuf->clear();
        delete pRxBuf;
        pRxBuf= NULL;
    }
}

bool Protocol::FeedProtocolBytes(std::string bytes )
{
    pRxBuf->write((uint8 *)bytes.c_str(), bytes.size());
    return true;
}

bool Protocol::FeedProtocolBytes(const QByteArray &data)
{
    pRxBuf->write((uint8 *)data.data(), data.size());
    return true;
}


bool Protocol::AddNeedSendFrame(std::string frame)
{
    QByteArray dataArr(frame.c_str(), frame.length());
    LOG_INFO(pRouteInf->GetChnId(), "["+ pRouteInf->GetChnId() +"] \n<Send> " + StringUtil::toHexQString(dataArr, " "));

    if(m_protoInterface)
    {
        emit m_protoInterface->sendToDeviceSig(dataArr);
    }
    return true;
}

bool Protocol::AddNeedSendFrame(uint8 *val, int size)
{
    std::string frame = std::string((char *)val, size);
    return AddNeedSendFrame(frame);
}
