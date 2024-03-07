/*******************************************************************************
 *��Ȩ���У��麣���ص����Ƽ�
 *�汾�ţ�  1.0.0
 *�ļ�����  protocol_time_data.h
 *�������ڣ�2015-07-29 
 *���ߣ�    yay
 *����˵��������ɼ�ʱ����������.
 *����˵����description 
 *�޸ļ�¼��date, maintainer 
 *          2015-07-29, yay, ���͵������� 
*******************************************************************************/

#ifndef __PROTOCOLTIMEDATA_H
#define __PROTOCOLTIMEDATA_H
#include <iostream>
//#include "iprotocolbase.h"

class ProtocolTimeData
{
public:
    ProtocolTimeData( );
    //��ȡ��ǰʱ��15λ�ַ�������ʽ��yymmddHHMMSSsss
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
    int _year = 0;     /* ������ */
    int _month = 0;    /* ������ */
    int _day = 0;      /* ������ */
    int _hour = 0;     /* ����ʱ */
    int _minute = 0;   /* ������ */
    int _second = 0;   /* EPOCH�� */
    int _msecond = 0;  /* ���� */
};

#endif
