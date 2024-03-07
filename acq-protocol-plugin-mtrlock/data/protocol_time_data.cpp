/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  protocol_time_data.cpp
 *生成日期：2015-07-29 
 *作者：    yay
 *功能说明：实现采集时标数据类型.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2015-07-29, yay, 检查和调整代码 
*******************************************************************************/

//#include "dfdatagather/internal/osutilpackage.h"
#include "protocol_time_data.h"

//using namespace datagatherservice_plugin;
//using namespace datagathergeneralbusiness;
#include <QDateTime>

ProtocolTimeData::ProtocolTimeData( )
{
    //OsUtilPackage::GetCurrTimeMsecs(_msecond);
    //OsUtilPackage::GetCurrTimeCalender(_year, _month, _day, _hour, _minute, _second);
    GetCurrTimeData();
}

void ProtocolTimeData::GetCurrTimeData( )
{
    //OsUtilPackage::GetCurrTimeMsecs(_msecond);
    //OsUtilPackage::GetCurrTimeCalender(_year, _month, _day, _hour, _minute, _second);
    QDateTime curDateTime = QDateTime::currentDateTime();
    QDate curDate = curDateTime.date();
    _year=curDate.year();
    _month = curDate.month();
    _day = curDate.day();
    QTime curTime = curDateTime.time();
    _hour = curTime.hour();
    _minute = curTime.minute();
    _second = curTime.second();
    _msecond= curTime.msec();
}

long long ProtocolTimeData::CaculateElapsedMsecs( )
{
    ///QDateTime::
    //return OsUtilPackage::GetElapsedMsecs(_second,_msecond);
    return 0;//@todo cfq
}

int ProtocolTimeData::GetYear( )
{
    return _year;
}

int ProtocolTimeData::GetMonth( )
{
    return _month;
}

int ProtocolTimeData::GetDay( )
{
    return _day;
}

int ProtocolTimeData::GetHour( )
{
    return _hour;
}

int ProtocolTimeData::GetMinute( )
{
    return _minute;
}

int ProtocolTimeData::GetSecond( )
{
    return _second;
}

int ProtocolTimeData::GetMsecond( )
{
    return _msecond;
}

std::string ProtocolTimeData::GetCurrTimeString15()
{
    char timestr[64] = { 0 };
    sprintf(timestr, "%02d%02d%02d%02d%02d%02d%03d",
        (GetYear()) % 100, GetMonth(), GetDay(),GetHour(), GetMinute(), GetSecond() % 60, GetMsecond());
    return timestr;
}

