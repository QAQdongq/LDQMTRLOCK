/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_rtuinf.cxx
 *生成日期：2012-01-01
 *作者：    yay
 *功能说明：实现与规约库的接口类SCN_RtuInf. 该类为规约库提供所需的Rtu参数信息.
 *其它说明：
 *修改记录：date, maintainer
 *          2012-01-01, yay, 建立代码
*******************************************************************************/
#include <string.h>
#include <string>
#include <map>
#include "scn_rtuinf.h"

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的遥测总数
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的遥测总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/


uint32 SCN_RtuInf::GetYcNum( const QString &rtuId )
{
    if(0 == m_rtuList.size())
    {
        return 0;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuId == iter->rtuId)
        {
            return iter->ycCounts;
        }
    }
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的遥信总数
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的遥信总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetYxNum( const QString &rtuId )
{
    if(0 == m_rtuList.size())
    {
        return 0;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuId == iter->rtuId)
        {
            return iter->yxCounts;
        }
    }
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的遥脉(电度量) 总数()
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的遥脉(电度量) 总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetKwhNum(const QString &rtuId )
{
#if 1
    return 1;
#else
    if(0 == m_rtuList.size())
    {
        return 0;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuId == iter->rtuId)
        {
            return iter->ddCounts;
        }
    }
    return 0;
#endif
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的设备总数
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的设备总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetDevNum( const QString &rtuId )
{
    return 1;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的遥控总数
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的遥控总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetYkNum( const QString &rtuId )
{
    if(0 == m_rtuList.size())
    {
        return 0;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuId == iter->rtuId)
        {
            return iter->ykCounts;
        }
    }
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的遥调总数
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的遥调总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetYtNum( const QString &rtuId )
{
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的SOE总数
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的SOE总数
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetSoeNum( const QString &rtuId )
{
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu的对钟时间间隔
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu的对钟时间间隔
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetSyncTimeInt( const QString &rtuId )
{
    if(this->m_protocolCfgParam.syncTime <=0)
    {
        /*chenfuqing 20230914 modified 轨道需求：校时周期（秒）为 0时 ，不要校时命令*/
        return 0;//300;//秒
    }
    return this->m_protocolCfgParam.syncTime;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu地址
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu地址
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetRtuAddr( const QString &rtuId )
{
    if(0 == m_rtuList.size())
    {
        return 0;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuId == iter->rtuId)
        {
            return iter->rtuAddr;
        }
    }
    return 0;
}

QString SCN_RtuInf::GetRtuIdByAddr( int rtuaddr )
{
    if(0 == m_rtuList.size())
    {
        return 0;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuaddr == iter->rtuAddr)
        {
            return iter->rtuId;
        }
    }
    return 0;
}

void SCN_RtuInf::SetRtuList(std::list<Rtu_S> rtuList)
{
    this->m_rtuList = rtuList;
}


/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtuId号Rtu对应的子站地址
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtuId号Rtu对应的子站地址
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetSubAddr( const QString &rtuId )
{
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取本系统的系统站址
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    本系统的系统站址
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetSysAddr( void )
{
    return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtu的类型
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtu的类型
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint32 SCN_RtuInf::GetRtuType( const QString &rtuId )
{
        return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取保护地址
输入参数：
             const QString &rtuId        Rtu号
             uint8 *num
             STRU_PROT_ADDR *addr
输出参数：  
             无
            
返回值
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8 SCN_RtuInf::GetProtAddr(const QString &rtuId ,uint8 *num,STRU_PROT_ADDR *addr)
{        
        return 1;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtu的转发组号
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtu的转发组号
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8  SCN_RtuInf::GetGroupNum( const QString &rtuId )
{
        return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtu对应的转发rtu号
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     >=0    rtu对应的转发rtu号
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
uint8  SCN_RtuInf::GetHopRtuId( const QString &rtuId )
{
        return 0;
}

/************************************************************************
作者：   lee     
创建日期：2009.6.3   
函数功能说明： 取rtu描述
输入参数：
             const QString &rtuId        Rtu号
输出参数：  
             无
            
返回值：     无
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
void  SCN_RtuInf::GetRtuDesc(const QString &rtuId,char *rtu_desc)
{
    if(0 == m_rtuList.size())
    {
        return;
    }
    std::list<Rtu_S>::const_iterator iter = m_rtuList.begin();
    for(; iter != m_rtuList.end(); iter++)
    {
        if(rtuId == iter->rtuId)
        {
            int len = iter->rtuDesc.length();
            if(!rtu_desc)
            {
                rtu_desc = new char[len+1];
                memset(rtu_desc, 0, len+1);
            }
            strcpy(rtu_desc, iter->rtuDesc.toStdString().c_str());
            return;
        }
    }
}

/*
自学数据中对rfid进行排序的数据结构
*/
struct lock_temp {
    uint64 val;
    uint32 devCode;
    uint32 lockcode_no;//    若都为uint32，在对齐中结构体的大小要增加一半
};

static lock_temp *pTemp;

/*
    功能：锁码排序
*/
static int cmp_lockcode(const void *p1, const void *p2)
{
    //if( 1 == rtuinf_debug_flag )
        printf("p1====%d  p2====%d\n",*(uint16 *)p1,*(uint16 *)p2);
    return pTemp[*(uint16 *)p1].val > pTemp[*(uint16 *)p2].val ? -1 : 1;
}

/*
    功能：通道号排序
*/
static int cmp_channelno(const void* p1, const void* p2)
{
    return *(int*)p1>*(int*)p2 ? 1 : -1;
}
/*
    功能：传票时按scada内存库中的opsheet排序
*/
static int cmp_opsheetid(const void *p1, const void *p2)
{
    return *(uint32 *)p1 > *(uint32 *)p2 ? 1 : -1;
}

typedef struct DlryElementTableStruct
{
   int workgroupid;
   char workgroupname[16];
   char username[12];
   char password[10];
   char rfidcode[16];
   char permit[16];
   int station;
}DlryElementTable;

static int cmp_dlrydata(const void *p1, const void *p2)
{
    return ((DlryElementTable *)p1)->workgroupid > ((DlryElementTable *)p2)->workgroupid ? 1 : -1;
}
int cmp_strap_data( const void *elem1, const void *elem2 )
{
    STRAP_CONFIG_DATA *strap_data1, *strap_data2;

    strap_data1 = (STRAP_CONFIG_DATA *)elem1;
    strap_data2 = (STRAP_CONFIG_DATA *)elem2;

    if( strap_data1->sub_addr > strap_data2->sub_addr )
        return 1;
    else if( strap_data1->sub_addr == strap_data2->sub_addr )
    {
        if( strap_data1->strap_x > strap_data2->strap_x )
            return 1;
        else if( strap_data1->strap_x == strap_data2->strap_x )
        {
            if( strap_data1->strap_y > strap_data2->strap_y )
                return 1;
            else if( strap_data1->strap_y == strap_data2->strap_y )
                return 0;
            else
                return -1;
        }
        else
            return -1;
    }
    else
        return -1;
}

uint8 SCN_RtuInf::transtohex(uint8 ch1, uint8 ch2)
{
    if (isxdigit(ch1) && isxdigit(ch2))
    {
        int tmp1 = ch1 > '9' ? tolower(ch1) - 'a' + 10 : ch1 - '0';
        int tmp2 = ch2 > '9' ? tolower(ch2) - 'a' + 10 : ch2 - '0';

        return (tmp1 << 4) | tmp2;
    }
    else
    {
        printf("GQJG+++++ ch1=%02X ch2=%02Xd\n", ch1, ch2);
        return 0;
    }
}

void SCN_RtuInf::SetProtocolcfgParam(const ProtocolCfgParam_S &protocolCfgParam)
{
    this->m_protocolCfgParam= protocolCfgParam;
}
