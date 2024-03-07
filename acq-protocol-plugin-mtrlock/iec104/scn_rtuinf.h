/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_rtuinf.h
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：定义与规约库的接口类SCN_RtuInf. 该类为规约库提供所需的Rtu参数信息.
 *其它说明：
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#ifndef SCN_RTUINF_H
#define SCN_RTUINF_H

#include "proto_def.h"
#include "data/init_json_data.h"

#define ANTI_STRAP_DEVTYPE 101

typedef struct
{
    uint8 prot_addr;
    uint8    cpu_num;
    uint8    cpu_no[10];
}STRU_PROT_ADDR;

typedef struct
{
    char strap_name[CHAR_LENGTH_32];
    uint8 sub_addr;
    uint8    strap_x;
    uint8    strap_y;
}STRAP_CONFIG_DATA;

typedef struct
{
    uint32 device_id;
    char device_name[CHAR_LENGTH_32];
    char device_desc[CHAR_LENGTH_32];
}JXGL_DEVICE;

typedef struct
{
    sint16 taskno;
    char   taskname[CHAR_LENGTH_32];
    char   permiter[CHAR_LENGTH_32];
    char   charger[CHAR_LENGTH_32];
    int    planstart;
    int    planstop;
    int    personum;
    char   data[1];
}JXGL_TICKET_DATA;

typedef struct
{
    uint8 Rs485No;
    uint8 DevType;
    uint8 NodeNum;
    uint8 DevPoints;
    uint8 StartAddr;
    uint32 BakField;
}MNP5G_RS485DevCon;

typedef struct
{
    uint8 step_number;
    uint8 bh_length;
    char device_bh[CHAR_LENGTH_32];
    uint8 state_before;
    uint8 state_after;
}INTERWF_TICKET_DATA;
class SCN_RtuInf//: public PROTO_RtuInf
{
public:
    virtual uint32  GetYcNum( const QString &rtuId );
    virtual uint32  GetYxNum( const QString &rtuId );
    virtual uint32  GetKwhNum( const QString &rtuId );
    virtual uint32  GetDevNum( const QString &rtuId );
    virtual uint32  GetYkNum( const QString &rtuId );
    virtual uint32  GetYtNum( const QString &rtuId );
    virtual uint32  GetSoeNum( const QString &rtuId );
    virtual uint32  GetSyncTimeInt( const QString &rtuId );
    virtual uint32  GetRtuAddr( const QString &rtuId );
    virtual void  SetRtuList(std::list<Rtu_S> rtuList);//cfq add 20200528
    virtual uint32  GetSubAddr( const QString &rtuId );
    virtual uint32  GetSysAddr( void );
    virtual uint32  GetRtuType(const QString &rtuId );
    virtual uint8   GetProtAddr(const QString &rtuId ,uint8 *num,STRU_PROT_ADDR *addr);
    virtual uint8  GetGroupNum( const QString &rtuId );
    virtual uint8  GetHopRtuId( const QString &rtuId );
    virtual void  GetRtuDesc(const QString &rtuId,char *rtu_desc);
    virtual QString    GetRtuIdByAddr( int rtuaddr );

    virtual void SetProtocolcfgParam(const ProtocolCfgParam_S &protocolCfgParam);
private:
    uint8 transtohex(uint8 ch1, uint8 ch2);

    QString m_rtuId = 0;
    std::list<Rtu_S> m_rtuList;
    ProtocolCfgParam_S m_protocolCfgParam;
};

#endif //SCN_RTUINF_H
