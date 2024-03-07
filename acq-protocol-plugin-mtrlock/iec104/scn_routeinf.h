/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_routeinf.h
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：定义与规约库的接口类SCN_RouteInf. 该类为规约库提供所需的Route参数信息.
 *其它说明：
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#ifndef SCN_ROUTEINF_H
#define SCN_ROUTEINF_H
#include "proto_def.h"
#include "data/init_json_data.h"
#include "acq_protocol_interface.h"

//许可请求
enum PermissionRequest
{
    Ordinarily_Send,
    Command_Send,
    Query_Send
};

class SCN_RouteInf: QObject
{
    Q_OBJECT
private:
    //uint32 RouteNo;
    QString m_chnId = ""; //通道号
    QString m_rtuId = "";//-1; //当前RTU号
    //uint32 m_allScan; //全数据召唤周期(分钟)
    uint32 m_rtuAddr = 0;//-1; //当前RTU地址
    ProtocolCfgParam_S m_protocolCfgParam;
public:
    void                 Init( uint32 routeno );
    virtual QString      GetRtuId( void );
    virtual sint32         GetRtuType( void );
    virtual QString      GetChnId( void );
    virtual sint32      GetRouteNo( void );
    virtual uint8        ReqPermission( PermissionRequest req );
    virtual sint32      GetRetryTimes( void );
    virtual sint32      GetRxTimeouts( void );    
    virtual uint32         GetScanPulseTime( void );

    virtual uint8       GetDebugFlag( void );
    virtual uint32      GetAllScan( void );
    virtual void         UpdateRouteDev();
//    virtual void         UpdatePeerRouteDev();

    virtual void         SetRtuId(const QString &rtuId );
    virtual void         SetRtuAddr( uint32 rtuAddr);
    virtual uint32         GetRtuAddr( void );
    virtual void        SetChnId(const QString &chnId);

    virtual void         SetProtocolcfgParam(const ProtocolCfgParam_S &protocolCfgParam);

    inline void setProtocolInterface(AcqProtocolInterface *protoInterface){m_protoInterface=protoInterface;}
protected:
    AcqProtocolInterface *m_protoInterface;
};

#endif //SCN_ROUTEINF_H
