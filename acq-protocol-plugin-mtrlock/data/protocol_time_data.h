/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  protocol_time_data.h
 *生成日期：2015-07-29 
 *作者：    yay
 *功能说明：定义采集时标数据类型.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2015-07-29, yay, 检查和调整代码 
*******************************************************************************/

#ifndef __PROTOCOLTIMEDATA_H
#define __PROTOCOLTIMEDATA_H
#include <iostream>
//#include "iprotocolbase.h"

class ProtocolTimeData
{
public:
    ProtocolTimeData( );
    //获取当前时间15位字符串，格式：yymmddHHMMSSsss
    std::string GetCurrTimeString15();
    void GetCurrTimeData( );
    long long CaculateElapsedMsecs( );
    int GetYear( );
    int GetMonth( );
    int GetDay( );
    int GetHour( );
    int GetMinute( );
    int GetSecond( );
    int GetMsecond( );

private:
    int _year = 0;     /* 日历年 */
    int _month = 0;    /* 日历月 */
    int _day = 0;      /* 日历日 */
    int _hour = 0;     /* 日历时 */
    int _minute = 0;   /* 日历分 */
    int _second = 0;   /* EPOCH秒 */
    int _msecond = 0;  /* 毫秒 */
};

#endif
