#ifndef  ACQ_CAIJIPROTOCOL104_H
#define  ACQ_CAIJIPROTOCOL104_H
#include <QVariantHash>
#include <QObject>
#include <QString>
#include <QSharedPointer>
#include <QTime>
#include <QFile>
#include <QTimer>
#include <QPluginLoader>
#include "acq_protocol_interface.h"
#include "proc_thread.h"
#include "iec104/protocol.h"
#include "protocol_session.h"

#ifdef ACQ_PROTOCOL104_PLUGIN_INTERFACE_LIB
#define Q_ACQ_PROTOCOL104_PLUGIN_INTERFACE_EXPORT Q_DECL_EXPORT
#else
#define Q_ACQ_PROTOCOL104_PLUGIN_INTERFACE_EXPORT Q_DECL_IMPORT
#endif

class AcqProtocolFactory;

class Protocol104Factory:public QObject, public AcqProtocolFactory
{
    Q_OBJECT
    Q_INTERFACES(AcqProtocolFactory)
    Q_PLUGIN_METADATA(IID "tools.acqprotocolfactory.Protocol104Factory")

public:
    virtual ~Protocol104Factory(){}
    virtual QSharedPointer<AcqProtocolInterface> createOneDataProtocolInstance(const QVariantHash &parameter);
};


class Q_ACQ_PROTOCOL104_PLUGIN_INTERFACE_EXPORT Protocol104Plugin :public AcqProtocolInterface
{
    Q_OBJECT
public:
    Protocol104Plugin(const QVariantHash &channelHash);
    virtual ~Protocol104Plugin();

    /**
     * @brief 初始化
     * @return true->成功，false->失败
     */
    virtual bool init() override;

public slots:
    virtual void recieveFromDeviceSlot(const QByteArray& data) override;//接收通讯装置消息
    virtual void recieveFromGatewaySlot(const QVariantHash& data) override;//接收网关机消息
    virtual void recieveNetStateChangedSlot(int state) override;//接收装置连接状态变化事件
private:

    /**
     * @brief 读取输入参数
     * @param parameter 输入参数
     */
    bool readParameter(const QVariantHash &channelHash);

    /**
     * @brief 创建规约会话(注意：必须在连接成功后，才创建)
     * @return
     */
    std::shared_ptr<AcqProtocolSession> createSession();

    /**
     * @brief 清空会话
     */
    void clearSession();


    /**
     * @brief 重置会话
     */
    void resetSession();
 private:
    InitData_S m_initData;
    std::shared_ptr<AcqProtocolSession> m_session;
    QString m_cchId;
    bool m_isInitSuccess;// 是否初始化成功
};


#endif
