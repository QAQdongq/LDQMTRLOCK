#ifndef __CONVERTERUTIL_H
#define __CONVERTERUTIL_H
#include <string>
#include <QString>
#include <QVariantHash>
#include "plugin104_define.h"

static const int POINT_INDEX_NO_DIFF = 1;//点号与下标差值

/**
 * @brief 采集服务与采集网关服务之间交互约定数据转换工具
 */
class ConverterUtil
{
public:

    /**
     * @brief 将字符串命令类型转换成整形
     * @param cmdType 字符串命令类型
     * @return 命令类型 CMD_TYPE
     */
    static int cmdTypeStrToInt(const std::string &cmdType);

    /**
     * @brief 将遥控命令类型转换功能码
     * @param ykCmd 遥控命令 遥控命令: 0-遥控选择，1-遥控执行，2-遥控 撤销，3-遥调选择，4-遥调执行，5-遥调撤销，6-设置点值
     * @return 功能码 CTRL_FUNC
     */
    static int ykCmdToCtrlFunc(int ykCmd);

    /**
     * @brief 将遥控命令类型转换功能码
     * @param ykCmd 遥控命令 遥控命令: 0-遥控选择，1-遥控执行，2-遥控 撤销，3-遥调选择，4-遥调执行，5-遥调撤销，6-设置点值
     * @return 功能码 CTRL_FUNC
     */
    static int wfCmdToCtrlFunc(int ykCmd);

    /**
     * @brief 生成遥信交互数据
     * @param isCycCallAll 是否周期总召数据
     * @param dataList 参数数据
     * @return 交互数据
     */
    static QVariantHash toYXHash(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S>> &dataList);

    /**
     * @brief 生成遥信SOE交互数据
     * @param dataList 参数数据
     * @return 交互数据
     */
    static QVariantHash toSOEHash(std::list<std::shared_ptr<BaseParam_S>> &dataList);

    /**
     * @brief 生成遥测交互数据
     * @param isCycCallAll 是否周期总召数据
     * @param dataList 参数数据
     * @return 交互数据
     */
    static QVariantHash toYCHash(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S> > &dataList);

    /**
     * @brief 生成脉冲（电度）交互数据
     * @param isCycCallAll 是否周期总召数据
     * @param dataList 参数数据
     * @return 交互数据
     */
    static QVariantHash toDDHash(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S> > &dataList);

    /**
     * @brief 生成总召请求交互数据
     * @param data 参数数据
     * @return 交互数据
     */
    static QVariantHash toCallDataHash(const CallReqParam_S &data);

    /**
     * @brief 生成总召结束（应答）交互数据
     * @param data 参数数据
     * @return 交互数据
     */
    static QVariantHash toCallDataEndHash(const CallRspParam_S &data);

    /**
     * @brief 生成遥控请求交互数据
     * @param data 参数数据
     * @return 交互数据
     */
    static QVariantHash toYKReqHash(const YKReqParam_S &data);

    /**
     * @brief 生成遥控应答交互数据
     * @param cmdType 命令类型
     * @param data 参数数据
     * @param taskId 任务号
     * @return 交互数据
     */
    static QVariantHash toYKRspHash(int cmdType, const YKRspParam_S &data, const QString &taskId);

    /**
     * @brief 生成防误应答交互数据
     * @param dataList 参数数据
     * @return 交互数据
     */
    static QVariantHash toFwRspHash(const QList<FwRspParam_S> &dataList, const QString &taskId);

    /**
     * @brief 生成设置点请求交互数据
     * @param data 参数数据
     * @return 交互数据
     */
    static QVariantHash toSPReqHash(const SPReqParam_S &data);

    /**
     * @brief 生成设置点应答交互数据
     * @param data 参数数据
     * @param taskId 任务号
     * @return 交互数据
     */
    static QVariantHash toSPRspHash(const SPRspParam &data, const QString &taskId);

    /**
     * @brief 交互数据转换成命令数据
     * @param data
     * @return 命令数据
     */
    static std::shared_ptr<BaseCmdData_S> toCmdData(const QString &code, const QVariantHash &data);

    /**
     * @brief 将点下标(从0开始)转换成点号(从1开始)
     * @param pointIdx 点下标
     * @return
     */
    static int pointIndexToNo(int pointIdx);

    /**
     * @brief 将点号(从1开始)转换成下标(从0开始)
     * @param pointIdx 点下标
     * @return
     */
    static int noToIndex(int no);

    /**
     * @brief 将防误的返回错误码转换成错误原因
     * @param errCode
     * @return
     */
    static QString toFwErrMsg(int errCode);
};

#endif // __CONVERTER_H
