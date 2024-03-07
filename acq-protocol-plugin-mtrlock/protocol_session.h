#ifndef ACQ_PROTOCOL_SESSION_H
#define ACQ_PROTOCOL_SESSION_H

#include "proc_thread.h"
#include "task_thread.h"
#include "plugin104_define.h"
#include "data/init_json_data.h"
#include "iec104/protocol.h"
#include "acq_protocol_interface.h"

/**
 * @brief 采集规约会话
 */
class AcqProtocolSession: QObject
{
    Q_OBJECT
public:
    explicit AcqProtocolSession(InitData_S *initData);
    virtual ~AcqProtocolSession();

    /**
     * @brief 初始化
     */
    virtual void init(AcqProtocolInterface *protoInterface);

    /**
     * @brief 释放
     */
    virtual void finalize();

    /**
     * @brief 初始化规约
     */
    bool initProtocol();

    /**
     * @brief 释放规约配置
     */
    void finalizeProtocol();


    /**
     * @brief 获取当前规约对象
     * @return
     */
    virtual Protocol* getProtocolObj();

    /**
     * @brief 下发送命令数据
     * @param data 待发送数据
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    virtual void sendCmdData(const QVariantHash &data);

    /**
     * @brief 退出规约处理线程
     */
    void exitProcThread();

    /**
     * @brief 退出任务处理线程
     */
    void exitTaskThread();

    /**
     * @brief 获取通道号字符串格式
     * @return
     */
    virtual std::string getChnIdStr();

    /**
     * @brief 通过回调上送遥信变位数据到智能分析网关机
     * @param data 上送遥信变位数据
     */
    virtual void slotPutData(std::string data);

    /**
     * @brief 处理从socket取的数据，放到规约接收缓存中
     * @param data 接收到的数据
     */
    virtual void slotRead(const std::string &data);

    /**
     * @brief 断开连接
     */
    //virtual void disconnect();

    /**
     * @brief 重连
     */
    virtual void reconnect();


    /**
     * @brief 是否已连接
     * @return true-已连接, false-未连接
     */
    virtual bool isConnected();

    /**
     * @brief 设置连接标志
     * @param isConnected 是否已连接
     */
    virtual void setIsConnected(bool isConnected);

    /**
     * @brief 将数据通过socket发送
     * @param data 待发送数据
     * @return true-成功, false-失败
     */
    virtual bool slotWrite(const std::string &data);

    void setProtocolInterface(AcqProtocolInterface *protoInterface);
private:
    bool m_isConnected = false;//是否已连接
    ProcThread *m_procThread = NULL;//规约处理线程
    TaskThread *m_taskThread = NULL;//任务处理线程


    PROTOCOL_CONFIG  m_protoconfig;//规约配置信息
    InitData *m_initData = NULL;//实始化时JSON结构信息
    std::string m_chnIdStr;//通道号

    bool m_isInit;//是否初始化
    QString m_cchId;
    QString m_rtuId;
protected:
  Protocol *m_protocolObj = NULL;//规约对象

  AcqProtocolInterface *m_protoInterface;
};
#endif // AcqProtocolSession_H
