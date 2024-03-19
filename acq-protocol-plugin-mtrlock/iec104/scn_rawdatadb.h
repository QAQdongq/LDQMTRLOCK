/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  scn_rawdatadb.h
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：定义与规约库的接口类SCN_RawdataDb. 该类为规约库提供所需的系统数据存储接口.
 *其它说明：
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#ifndef SCN_RAWDATADB_H
#define SCN_RAWDATADB_H
//#include "proto_rawdb.h"
#include "proto_def.h"
#include "proto_cmdmem.h"
#include "acq_protocol_interface.h"

#define VAR_YX_BAK_TIME_LIMITE 60 //缓存变位遥信时间限

class SCN_RawdataDb//: public PROTO_RawdataDb
{
public:

    virtual void   PutSciConfig( const QString &rtuId, const QString &ChnId, uint8 *buf );
    virtual void   PutAYc( const QString &rtuId, int YcNo, float32 YcValue, uint32 YcBin, uint16 YcStatus );
    virtual void   PutAYx( const QString &rtuId, int YxNo, uint8 YxValue, uint16 YxStatus );
    virtual void   PutAWf( const QString &rtuId, const QString &ChnId, char *bh, sint16 yxVal, sint16 bsVal,int ykbs );
    //virtual void   PutAStrap( const QString &rtuId, const QString &ChnId, sint16 StrapBoxID, sint16 StrapAddr, uint16 StrapValue );
    virtual void   PutAKwh( const QString &rtuId, const QString &ChnId, int KwhNo, float32 KwhValue, uint16 KwhStatus );
    virtual void   PutASoe( const QString &rtuId, const QString &ChnId, int YxNo, SOEDATA Soe, uint16 SoeStatus );
    //virtual void   PutFrequence( const QString &ChnId, int freq );
    virtual uint8  GetAYc( const QString &rtuId, int YcNo, float32 *YcValue, uint8 *Flag=NULL );
    //virtual uint8  GetADw( const QString &rtuId, int DwNo, float32 *DwValue, uint8 *Flag=NULL );
    virtual uint8  GetAYx( const QString &rtuId, int YxNo, uint8 *YxValue, uint8 *Flag=NULL );
    virtual uint8  GetAVarYx( const QString &rtuId, const QString &ChnId, int *YxNo, uint8 *YxValue, uint8 *Flag=NULL );
    virtual uint32  GetVarYxNum( const QString &rtuId, const QString &ChnId );
    virtual uint8  GetAKwh( const QString &rtuId, int KwhNo, float32 *KwhValue, uint8 *Flag=NULL );
    virtual uint8  GetASoe( const QString &rtuId, int *YxNo, SOEDATA *Soe );
    virtual uint8  GetAVarSoe( const QString &rtuId, const QString &ChnId, int *YxNo, SOEDATA *Soe );
    virtual uint32  GetVarSoeNum( const QString &rtuId, const QString &ChnId );
    virtual bool IsOutRtuDataValid( const QString &rtuId );

    /**
     * @brief 批量上送遥信状态
     * @param isCycCallAll 是否周期总召数据
     * @param dataList
     */
    virtual void PutYxList(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S>> &dataList);

    /**
     * @brief 批量上送遥测值
     * @param isCycCallAll 是否周期总召数据
     * @param dataList
     */
    virtual void PutYcList(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S>> &dataList);


    /**
     * @brief 批量上送脉冲（电度）状态
     * @param isCycCallAll 是否周期总召数据
     * @param dataList
     */
    virtual void PutKwhList(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S>> &dataList);

    /**
     * @brief 上送总召命令到上层应用
     */
    virtual void SendCmdCallData();

    /**
     * @brief 上送总召应答(结束)命令到上层应用
     */
    virtual void SendCmdCallDataEnd(QString rtuId, QString cchId, int result, const QString &reason);

    /**
     * @brief 上送遥控命令到上层应用
     */
    virtual void SendCmdYk(const YKReqParam_S &data);

    /**
    * @brief 上送密码信息命令设置反馈到上层应用
    */
    virtual void SendSetPassword(const SetPasswordReqParam_S &data);

    /**
     * @brief 设置所连接的设备IP和端口(发送源头端)
     * @param source 所连接的设备IP和端口
     */
    virtual void SetSource(const std::string &source);


    inline void SetRtuId(const QString &rtuId)
    {
        this->m_rtuId = rtuId;
    }

    inline void SetChnId(const QString &chnId)
    {
        this->m_chnId = chnId;
    }

    inline void setProtocolInterface(AcqProtocolInterface *protoInterface){m_protoInterface=protoInterface;}

private:
    /**
     * @brief 生成发送JSON命令
     * @param cmdType 命令类型
     * @param source 源端
     * @param dataListStr 数据列表
     * @return
     */
    /*std::string MakeSendDataJson(const std::string &cmdType, const std::string &source, const std::string &dataListStr);*/
private:
    std::string m_source;//所连接的设备IP和端口(发送源头端)

    AcqProtocolInterface *m_protoInterface;

    QString m_rtuId;
    QString m_chnId;
};

#endif //SCN_RAWDATADB_H
