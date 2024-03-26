#include "converter_util.h"
#include "iec104/proto_cmdmem.h"
#include "plugin104_define.h"
#include <QDateTime>

/**
 * @brief 将字符串命令类型转换成整形 CMD_TYPE
 * @param cmdType
 * @return 命令类型 CMD_TYPE
 */
int ConverterUtil::cmdTypeStrToInt(const std::string &cmdType)
{
    if(CMD_TYPE_STR_YK == cmdType)
    {
        return CMD_TYPE_YK;
    }
    else if(CMD_TYPE_STR_YKRESULT == cmdType)
    {
        return CMD_TYPE_YKRESULT;
    }
    else if(CMD_TYPE_STR_CALLDATA == cmdType)
    {
        return CMD_TYPE_CALLALLDATA;
    }
    else if(CMD_TYPE_STR_CALLDATAEND == cmdType)
    {
        return CMD_TYPE_CALLALLDATAEND;
    }
    else if(CMD_TYPE_STR_YXCHG == cmdType || CMD_TYPE_STR_YXCYC == cmdType)
    {
        return CMD_TYPE_YX;
    }
    else if(CMD_TYPE_STR_YCCHG == cmdType || CMD_TYPE_STR_YCCYC == cmdType)
    {
        return CMD_TYPE_YC;
    }
    else if(CMD_TYPE_STR_YT == cmdType)
    {
        return CMD_TYPE_YT;
    }
    else if(CMD_TYPE_STR_YTRESULT == cmdType)
    {
        return CMD_TYPE_YTRESULT;
    }
    else if(CMD_TYPE_STR_SP == cmdType)
    {
        return CMD_TYPE_SP;
    }
    else if(CMD_TYPE_STR_WF == cmdType)
    {
        return CMD_TYPE_WF;
    }
    else if(CMD_TYPE_STR_SetPasswordReg == cmdType)
    {
        return CMD_TYPE_SetPasswordReg;
    }
    else
    {
        return INVALID_CMDTYPE;
    }
}

/**
 * @brief 将遥控命令类型转换功能码
 * @param ykCmd 遥控命令: 0-撤销，1-选择，2-执行
 * @return 功能码 CTRL_FUNC
 */
int ConverterUtil::ykCmdToCtrlFunc(int ykCmd)
{
    int ctrlFunc = INVALID_CTRLFUNC;
    switch(ykCmd)
    {
        case YK_CMD_CANCEL:
        {
            ctrlFunc = CTRL_FUNC_CANCEL;
            break;
        }
        case YK_CMD_SELECT:
        {
            ctrlFunc = CTRL_FUNC_SELECT;
            break;
        }
        case YK_CMD_EXECUTE:
        {
            ctrlFunc = CTRL_FUNC_EXECUTE;
            break;
        }
    }
    return ctrlFunc;
}

/**
  @note liujie add
 * @brief 将五防命令类型转换功能码
 * @param ykCmd 五防命令: 1-防误校验，2-应急模式
 * @return 功能码 CTRL_FUNC
 */
int ConverterUtil::wfCmdToCtrlFunc(int wfCmd)
{
    int ctrlFunc = INVALID_CTRLFUNC;
    switch(wfCmd)
    {
        //liujie add 20230525 添加五防操作
        case WF_CMD_EXECUTE://五防校验-1
        {
            ctrlFunc = CTRL_FUNC_WF;
            break;
        }
        case WF_CMD_YJMS://应急模式-2
        {
            ctrlFunc = CTRL_FUNC_YJMS;
            break;
        }
    }
    return ctrlFunc;
}

/**
 * @brief 生成遥信交互数据
 * @param isCycCallAll 是否周期总召数据
 * @param dataList 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toYXHash(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S> > &dataList)
{
    /*样例：
        {
        "code":"YXCHG",  //上送遥信命令 YXCHG-变化遥信；YXCYC-周期遥信
        "data":[           //遥信数据
        {
        "rtuId":1,
        "no":1,          // 遥信点号
        "value":1,        // 遥信值
        "quality":"1" ,        // 遥信质量码
        "time":1630564973670    // 时标
        }
        ]
        }
     */
    QVariantList paramList;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  dataList.begin();
    for(; iter != dataList.end(); iter++)
    {
        std::shared_ptr<YXParam_S> data = std::static_pointer_cast<YXParam_S>(*iter);

        QVariantHash oneParam;
        oneParam.insert(KEY_RTUID, data->rtuId);
        oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data->no));
        oneParam.insert(KEY_VALUE, data->value);
        oneParam.insert(KEY_QUALITY, data->quality);
        //oneParam.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
        oneParam.insert(KEY_TIME, 0);
        paramList.append(oneParam);
    }
    QVariantHash dataHash;
    if(isCycCallAll)
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YXCYC));//周期总召遥信
    }
    else
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YXCHG));//突发遥信
    }
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

/**
 * @brief 生成遥信SOE交互数据
 * @param dataList 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toSOEHash(std::list<std::shared_ptr<BaseParam_S> > &dataList)
{
    /*样例：
        {
        "code":"SOE",  //上送遥信命令 YXCHG-变化遥信；YXCYC-周期遥信, SOE-遥信SOE
        "data":[           //遥信数据
        {
        "rtuId":1,
        "no":1,          // 遥信点号
        "value":1,        // 遥信值
        "quality":"1" ,        // 遥信质量码
        "time":1630564973670    // 时标
        }
        ]
        }
     */
    QVariantList paramList;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  dataList.begin();
    for(; iter != dataList.end(); iter++)
    {
        std::shared_ptr<YXParam_S> data = std::static_pointer_cast<YXParam_S>(*iter);

        QVariantHash oneParam;
        oneParam.insert(KEY_RTUID, data->rtuId);
        oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data->no));
        oneParam.insert(KEY_VALUE, data->value);
        oneParam.insert(KEY_QUALITY, data->quality);
        oneParam.insert(KEY_TIME, data->time);
        paramList.append(oneParam);
    }
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_SOE));//SOE
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

/**
 * @brief 生成遥测交互数据
 * @param isCycCallAll 是否周期总召数据
 * @param dataList 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toYCHash(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S> > &dataList)
{
    /*样例：
        {
        "code":"YCCHG",  //上送遥测命令 YCCHG-变化遥测；YCCYC-周期遥测
        "data":[           //遥测数据
        {
        "rtuId":1,
        "no":1,          // 遥测点号
        "value":32.5,        // 遥测值
        "quality":"1" ,        // 遥测质量码
        "time":1630564973670    // 时标
        }
        ]
        }
     */
    QVariantList paramList;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  dataList.begin();
    for(; iter != dataList.end(); iter++)
    {
        std::shared_ptr<YCParam_S> data = std::static_pointer_cast<YCParam_S>(*iter);
        QVariantHash oneParam;
        oneParam.insert(KEY_RTUID, data->rtuId);
        oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data->no));
        oneParam.insert(KEY_VALUE, data->value);
        oneParam.insert(KEY_QUALITY, data->quality);
        //oneParam.insert(KEY_QUALITY, 1);
        //oneParam.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
        paramList.append(oneParam);
    }
    QVariantHash dataHash;
    if(isCycCallAll)
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YCCYC));//周期总召遥测
    }
    else
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YCCHG));//突发遥测
    }
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

/**
 * @brief 生成脉冲（电度）交互数据
 * @param isCycCallAll 是否周期总召数据
 * @param dataList 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toDDHash(bool isCycCallAll, std::list<std::shared_ptr<BaseParam_S> > &dataList)
{
    /*样例：
        {
        "code":"DDCHG",  //上送脉冲（电度）命令 DDCHG-变化脉冲（电度）；DDCYC-周期脉冲（电度）
        "data":[           //数据
        {
        "rtuId":1,
        "no":1,          // 点号
        "value":32.5        // 值
        }
        ]
        }
     */
    QVariantList paramList;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  dataList.begin();
    for(; iter != dataList.end(); iter++)
    {
        std::shared_ptr<DDParam_S> data = std::static_pointer_cast<DDParam_S>(*iter);
        QVariantHash oneParam;
        oneParam.insert(KEY_RTUID, data->rtuId);
        oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data->no));
        oneParam.insert(KEY_VALUE, data->value);
        paramList.append(oneParam);
    }
    QVariantHash dataHash;
    if(isCycCallAll)
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_DDCYC));//周期总召
    }
    else
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_DDCHG));//突发
    }
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

/**
 * @brief 生成总召请求交互数据
 * @param data 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toCallDataHash(const CallReqParam_S &data)
{
    /*
    {
    "code":"CallReq",                //下发总召命令
    "time":"1630564973670",            //当前命令生成的时间点(毫秒)
    "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
    "data":{                        //通道数据
    "rtuId":1,
    "cchId":1,         // 通道ID
    }
    }
    */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_CALLDATA));//遥测
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    dataHash.insert(KEY_DATA, oneParam);
    return dataHash;
}

/**
 * @brief 生成总召结束（应答）交互数据
 * @param data 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toCallDataEndHash(const CallRspParam_S &data)
{
    /*
        {
        "code":"CallRsp",                //总召应答
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
        "data":{                        //通道数据
        "rtuId":1,
        "cchId":1,                 // 通道ID
        "result":1,                    // 1:总召成功； -1：Json数据异常；-2：其他原因。。。
        "reason":""
        }
        }

    */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_CALLDATAEND));//遥测
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    oneParam.insert(KEY_RESULT, data.result);
    oneParam.insert(KEY_REASON, data.reason);
    dataHash.insert(KEY_DATA, oneParam);
    return dataHash;
}


/**
 * @brief 生成遥控请求交互数据
 * @param data 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toYKReqHash(const YKReqParam_S &data)
{
    /*样例：
        {
        "code":"YKReq",                    //下发遥控命令
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
        "data":[                        //遥控数据
        {
        "rtuId":1,
        "cchId":1,          // 通道ID
        "no":1,             // 遥控点号
        "value":0,          // 遥控值
        "type":1            // 遥控类型：0:取消、1：执行、2：预置
        }
        ]
        }

     */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YK));//遥控
    dataHash.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
    dataHash.insert(KEY_NODEKEY, "");
    QVariantList paramList;
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data.no));
    oneParam.insert(KEY_VALUE, data.value);
    oneParam.insert(KEY_TYPE, data.type);
    paramList.append(oneParam);
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

/**
 * @brief 生成密码信息命令设置反馈交互数据
 * @param data 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toSetPasswordHash(const SetPasswordReqParam_S &data)
{
    /*样例：
        {
        "code":"SetPasswordRegReq",                    //
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
        "data":[                        //遥控数据
        {
        "rtuId":1,
        "cchId":1,          // 通道ID
        "passwd":1,             // 密码
        "lockno":0,          // 锁号
        "value":1,            // 结果
        "message":""          //错误码+内容
        }
        ]
        }

     */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_SetPasswordReg));//信息命令设置反馈
    dataHash.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
    dataHash.insert(KEY_NODEKEY, "");
    QVariantList paramList;
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    oneParam.insert(KEY_PASSWD, data.passwd);
    oneParam.insert(KEY_LOCKNO, data.lockno);
    oneParam.insert(KEY_VALUE, data.value);
    paramList.append(oneParam);
    dataHash.insert(KEY_DATA, oneParam);
    //dataHash.insert(KEY_DATA, paramList);
    return dataHash;


}

/**
 * @brief 生成遥控应答交互数据
 * @param data 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toYKRspHash(int cmdType, const YKRspParam_S &data, const QString &taskId)
{
    /*样例：
        {
        "code":"YKRsp",                    //遥控应答
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
        "data":[                        //应答数据
        {
        "rtuId":1,
        "cchId":1,        // 通道ID
        "no":1,          // 遥控点序号
        "type":1            // 遥控类型：0:取消、1：执行、2：预置
        "value":0,            // 遥控值
        "result":1,          // 1:-遥控下发成功； -1：网络异常；-2：其他原因。。。
        "reason":"遥控失败原因"      // 遥控失败返回原因
        }
        ]
        }
     */
    QVariantHash dataHash;
    if(CMD_TYPE_YT == cmdType)
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YTRESULT));//遥调应答
    }
    else if(CMD_TYPE_YK == cmdType)
    {
        dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_YKRESULT));//遥控应答
    }
    dataHash.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
    dataHash.insert(KEY_NODEKEY, "");
    dataHash.insert(KEY_TASKID, taskId);
    QVariantList paramList;
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data.no));
    oneParam.insert(KEY_TYPE, data.type);
    oneParam.insert(KEY_VALUE, data.value);
    oneParam.insert(KEY_RESULT, data.result);//1:-遥控下发成功； -1：网络异常；-2：其他原因
    oneParam.insert(KEY_REASON, data.reason);
    paramList.append(oneParam);
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

/**
 * @brief 生成防误应答交互数据
 * @param data 参数数据
 * @return 交互数据
 */
QVariantHash ConverterUtil::toFwRspHash(const QList<FwRspParam_S> &dataList, const QString &taskId)
{
    /*样例：
        {
        "code":"FWRsp",                    //防误应答
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
        "data":[                        //应答数据
        {
        "rtuId":1,
        "cchId":1,        // 通道ID
        "no":1,          // 遥控点序号
        "type":1            // 遥控类型：0:取消、1：执行、2：预置
        "value":0,            // 遥控值
        "result":1,          // 1:-遥控下发成功； -1：网络异常；-2：其他原因。。。
        "reason":"遥控失败原因"      // 遥控失败返回原因
        }
        ]
        }
     */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_WFRESULT));//遥控应答
    dataHash.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
    dataHash.insert(KEY_NODEKEY, "");
    dataHash.insert(KEY_TASKID, taskId);
    QVariantList paramList;
    for(int i=0; i<dataList.size();i++)
    {
        QVariantHash oneParam;
        oneParam.insert(KEY_RTUID, dataList.at(i).rtuId);
        oneParam.insert(KEY_CHNID, dataList.at(i).cchId);
        oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(dataList.at(i).no));
        oneParam.insert(KEY_TYPE, dataList.at(i).type);
        oneParam.insert(KEY_VALUE, dataList.at(i).value);
        oneParam.insert(KEY_RESULT, dataList.at(i).result);//1:-遥控下发成功； -1：网络异常；-2：其他原因
        oneParam.insert(KEY_REASON, dataList.at(i).reason);
        paramList.append(oneParam);
    }
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}


QVariantHash ConverterUtil::toSPReqHash(const SPReqParam_S &data)
{
    /*样例：
        {
        "code":"SetReq",                    //设置点命令
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",      //当前采集服务的MQ子主题tag关键字
        "data":[                           //数据
        {
        "rtuId":1,
        "cchId":1,             // 通道ID
        "no":1,                // 设置点号
        "type":1,              // 设点类型：0: 小数、1：整数、2：浮点、3：位串
        "decValue":10.5,       // 设置点小数值
        "intValue":0,          // 设置点整数值
        "floatValue":0.0,      // 设置点浮点值
        "decValue":"",         // 设置点位串值
        }
        ]
        }

     */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_SP));
    dataHash.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
    dataHash.insert(KEY_NODEKEY, "");
    QVariantList paramList;
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data.no));
    oneParam.insert(KEY_DECVALUE, data.decValue);
    oneParam.insert(KEY_INTVALUE, data.intValue);
    oneParam.insert(KEY_FLOATVALUE, data.floatValue);
    oneParam.insert(KEY_STRVALUE, data.strValue);
    oneParam.insert(KEY_TYPE, data.type);
    paramList.append(oneParam);
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

QVariantHash ConverterUtil::toSPRspHash(const SPRspParam &data, const QString &taskId)
{
    /*样例：
        {
        "code":"SetRsp",                    //设置点应答
        "time":"1630564973670",            //当前命令生成的时间点(毫秒)
        "nodeKey":"192.168.80.100:1",      //当前采集服务的MQ子主题tag关键字
        "data":[                           //应答数据
        {
        "rtuId":1,
        "cchId":1,              // 通道ID
        "no":1,                 // 设置点序号
        "type":0,               // 设点类型：0: 小数、1：整数、2：浮点、3：位串
        "decValue":10.5,        // 设置点小数值
        "intValue":0,           // 设置点整数值
        "floatValue":0.0,       // 设置点浮点值
        "decValue":"",          // 设置点位串值
        "result":1,             // 1:-成功；0：失败。。。
        "reason":"失败原因"      // 失败原因
        }
        ]
        }
     */
    QVariantHash dataHash;
    dataHash.insert(KEY_CODE, QString::fromStdString(CMD_TYPE_STR_SPRESULT));//设置点应答
    dataHash.insert(KEY_TIME, QDateTime::currentMSecsSinceEpoch());
    dataHash.insert(KEY_NODEKEY, "");
    dataHash.insert(KEY_TASKID, taskId);
    QVariantList paramList;
    QVariantHash oneParam;
    oneParam.insert(KEY_RTUID, data.rtuId);
    oneParam.insert(KEY_CHNID, data.cchId);
    oneParam.insert(KEY_POINTNO, ConverterUtil::pointIndexToNo(data.no));
    oneParam.insert(KEY_TYPE, data.type);
    oneParam.insert(KEY_DECVALUE, data.decValue);
    oneParam.insert(KEY_INTVALUE, data.intValue);
    oneParam.insert(KEY_FLOATVALUE, data.floatValue);
    oneParam.insert(KEY_STRVALUE, data.strValue);
    oneParam.insert(KEY_RESULT, data.result);
    oneParam.insert(KEY_REASON, data.reason);
    paramList.append(oneParam);
    dataHash.insert(KEY_DATA, paramList);
    return dataHash;
}

//liujie note 交互数据转换为命令数据
std::shared_ptr<BaseCmdData_S> ConverterUtil::toCmdData(const QString &code, const QVariantHash &data)
{
    QString taskId;
    if(data.contains(KEY_TASKID))
    {
        taskId = data.value(KEY_TASKID).toString();
    }
    if(QString::fromStdString(CMD_TYPE_STR_YXCHG) == code || QString::fromStdString(CMD_TYPE_STR_YXCYC) == code)
    {
        /*样例：
        {
        "code":"YXCHG",          //上送遥信命令-变化遥信；YXCYC-周期遥信；YXCHG-变位遥信; SOE
        "data":[           //遥信数据
        {
        "rtuId":1,
        "no":1,          // 遥信点序号
        "value":0，            // 遥信值
        "quality":0，          // 质量码，有效位等
        "time":1630564973670    // SOE时标
        }
        ]
        }
         */
        std::shared_ptr<YXCmdData_S> cmdData = std::make_shared<YXCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
            std::shared_ptr<YXParam_S> yxParam = std::make_shared<YXParam_S>();
            QVariantHash oneParam=paramList.at(i).toHash();
            yxParam->rtuId = oneParam.value(KEY_RTUID).toString();
            yxParam->no = oneParam.value(KEY_POINTNO).toInt();
            if(yxParam->no > 0){
                yxParam->no = noToIndex(yxParam->no);
            }
            yxParam->value = oneParam.value(KEY_VALUE).toInt();
            yxParam->quality = oneParam.value(KEY_QUALITY).toInt();
            yxParam->time = oneParam.value(KEY_TIME).toLongLong();
            cmdData->param.push_back(yxParam);
        }
        return cmdData;
    }

    if(QString::fromStdString(CMD_TYPE_STR_YCCHG) == code || QString::fromStdString(CMD_TYPE_STR_YCCYC) == code)
    {
        /*样例：
            {
            "code":"YCCHG",  //上送遥测命令 YCCHG-变化遥测；YCCYC-周期遥测
            "data":[           //遥测数据
            {
            "rtuId":1,
            "no"":1,          // 遥测点号
            "value":32.5,        // 遥测值
            "quality":"1" ,        // 遥测质量码
            "time":1630564973670    // 时标
            }
            ]
            }
         */
        std::shared_ptr<YCCmdData_S> cmdData = std::make_shared<YCCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
          std::shared_ptr<YCParam_S> ycParam = std::make_shared<YCParam_S>();
          QVariantHash oneParam=paramList.at(i).toHash();
          ycParam->rtuId = oneParam.value(KEY_RTUID).toString();
          ycParam->no = oneParam.value(KEY_POINTNO).toInt();
          if(ycParam->no > 0){
              ycParam->no = noToIndex(ycParam->no);
          }
          ycParam->value = oneParam.value(KEY_VALUE).toFloat();
          ycParam->quality = oneParam.value(KEY_QUALITY).toInt();
          ycParam->time = oneParam.value(KEY_TIME).toLongLong();
          cmdData->param.push_back(ycParam);
        }
        return cmdData;
    }
    if(QString::fromStdString(CMD_TYPE_STR_YK) == code
            || QString::fromStdString(CMD_TYPE_STR_YT) == code)
    {
        /*样例：
            {
            "code":"YKReq",                    //下发遥控命令
            "time":"1630564973670",            //当前命令生成的时间点(毫秒)
            "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
            "data":[                        //遥控数据
            {
            "rtuId":1,
            "cchId":1,         // 通道ID
            "no":1,          // 遥控点号
            "value":0,            // 遥控值
            "type":1            // 遥控类型：0:遥控取消，1：遥控选择，2：遥控执行
            }
            ]
            }

         */
        std::shared_ptr<YKReqCmdData_S> cmdData = std::make_shared<YKReqCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        cmdData->time = data.value(KEY_TIME).toLongLong();
        cmdData->nodeKey = data.value(KEY_NODEKEY).toString();
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
            std::shared_ptr<YKReqParam_S> ykParam = std::make_shared<YKReqParam_S>();
            QVariantHash oneParam=paramList.at(i).toHash();
            ykParam->rtuId = oneParam.value(KEY_RTUID).toString();
            ykParam->cchId = oneParam.value(KEY_CHNID).toString();
            ykParam->no = oneParam.value(KEY_POINTNO).toInt();
            if(ykParam->no > 0){
                ykParam->no = noToIndex(ykParam->no);
            }
            ykParam->value = oneParam.value(KEY_VALUE).toInt();
            ykParam->type = oneParam.value(KEY_TYPE).toInt();
            cmdData->param.push_back(ykParam);
        }
        return cmdData;
    }
    if(QString::fromStdString(CMD_TYPE_STR_WF) == code)
    {
        /*样例：
            {
            "code":"FWReq",                    //下发遥控命令
            "time":"1630564973670",            //当前命令生成的时间点(毫秒)
            "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
            "data":[                        //遥控数据
            {
            "rtuId":1,
            "cchId":1,         // 通道ID
            "no":1,          // 遥控点号
            "value":0,            // 遥控值
            "type":1            // 遥控类型：0:遥控取消，1：遥控选择，2：遥控执行
            }
            ]
            }

         */
        std::shared_ptr<FWReqCmdData_S> cmdData = std::make_shared<FWReqCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        cmdData->time = data.value(KEY_TIME).toLongLong();
        cmdData->nodeKey = data.value(KEY_NODEKEY).toString();
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
            std::shared_ptr<FWReqParam_S> fwParam = std::make_shared<FWReqParam_S>();
            QVariantHash oneParam=paramList.at(i).toHash();
            fwParam->rtuId = oneParam.value(KEY_RTUID).toString();
            fwParam->cchId = oneParam.value(KEY_CHNID).toString();
            fwParam->no = oneParam.value(KEY_POINTNO).toInt();
            if(fwParam->no > 0){
                fwParam->no = noToIndex(fwParam->no);
            }
            fwParam->value = oneParam.value(KEY_VALUE).toInt();
            fwParam->type = oneParam.value(KEY_TYPE).toInt();
            cmdData->param.push_back(fwParam);
        }
        return cmdData;
    }
    if(QString::fromStdString(CMD_TYPE_STR_YKRESULT) == code
            || QString::fromStdString(CMD_TYPE_STR_YTRESULT) == code)
    {
        /*样例：
            {
            "code":"YKRsp",                    //遥控应答
            "time":"1630564973670",            //当前命令生成的时间点(毫秒)
            "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
            "data":[                        //应答数据
            {
            "rtuId":1,
            "cchId":1,        // 通道ID
            "no":1,          // 遥控点号
            "result":1,          // 1:-遥控下发成功； -1：网络异常；-2：其他原因。。。
            "reason":"遥控失败原因"      // 遥控失败返回原因
            }
            ]
            }
         */
        std::shared_ptr<YKRspCmdData_S> cmdData = std::make_shared<YKRspCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        cmdData->time = data.value(KEY_TIME).toLongLong();
        cmdData->nodeKey = data.value(KEY_NODEKEY).toString();
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
            std::shared_ptr<YKRspParam_S> ykresultParam = std::make_shared<YKRspParam_S>();
            QVariantHash oneParam=paramList.at(i).toHash();
            ykresultParam->rtuId = oneParam.value(KEY_RTUID).toString();
            ykresultParam->cchId = oneParam.value(KEY_CHNID).toString();
            ykresultParam->no = oneParam.value(KEY_POINTNO).toInt();
            ykresultParam->result = oneParam.value(KEY_RESULT).toInt();
            ykresultParam->reason = oneParam.value(KEY_REASON).toString();
            cmdData->param.push_back(ykresultParam);
        }
        return cmdData;
    }
    if(QString::fromStdString(CMD_TYPE_STR_CALLDATA) == code)
    {
        /*样例：
         {
            "code":"CallReq",                //下发总召命令
            "time":"1630564973670",            //当前命令生成的时间点(毫秒)
            "nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
            "data":{                        //通道数据
            "rtuId":1,
            "cchId":1,         // 通道ID
            }
         }
         */
        std::shared_ptr<CallReqCmdData_S> cmdData = std::make_shared<CallReqCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        cmdData->time = data.value(KEY_TIME).toLongLong();
        cmdData->nodeKey = data.value(KEY_NODEKEY).toString();
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
            std::shared_ptr<CallReqParam_S> callAllReqParam = std::make_shared<CallReqParam_S>();
            QVariantHash oneParam=paramList.at(i).toHash();
            callAllReqParam->rtuId = oneParam.value(KEY_RTUID).toString();
            callAllReqParam->cchId = oneParam.value(KEY_CHNID).toString();
            cmdData->param.push_back(callAllReqParam);
        }
        return cmdData;
    }
    if(QString::fromStdString(CMD_TYPE_STR_SP) == code)
    {
        /*样例：
         {
            "code":"SetReq",                    //设置点命令
            "time":"1630564973670",            //当前命令生成的时间点(毫秒)
            "nodeKey":"192.168.80.100:1",      //当前采集服务的MQ子主题tag关键字
            "data":[                           //数据
                {
                "rtuId":1,
                "cchId":1,             // 通道ID
                "no":1,                // 设置点号
                "type":1,              // 设点类型：0: 小数、1：整数、2：浮点、3：位串
                "decValue":10.5,       // 设置点小数值
                "intValue":0,          // 设置点整数值
                "floatValue":0.0,      // 设置点浮点值
                "decValue":"",         // 设置点位串值
                }
            ]
         }
         */
        std::shared_ptr<SPReqCmdData_S> cmdData = std::make_shared<SPReqCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        cmdData->time = data.value(KEY_TIME).toLongLong();
        cmdData->nodeKey = data.value(KEY_NODEKEY).toString();
        QVariantList paramList = data.value(KEY_DATA).toList();
        for(int i=0;i<paramList.size();++i)
        {
            std::shared_ptr<SPReqParam_S> spParam = std::make_shared<SPReqParam_S>();
            QVariantHash oneParam=paramList.at(i).toHash();
            spParam->rtuId = oneParam.value(KEY_RTUID).toString();
            spParam->cchId = oneParam.value(KEY_CHNID).toString();
            spParam->no = oneParam.value(KEY_POINTNO).toInt();
            if(spParam->no > 0){
                spParam->no = noToIndex(spParam->no);
            }
            spParam->type = oneParam.value(KEY_TYPE).toInt();
            spParam->decValue = oneParam.value(KEY_DECVALUE).toFloat();
            spParam->intValue = oneParam.value(KEY_INTVALUE).toInt();
            spParam->floatValue = oneParam.value(KEY_FLOATVALUE).toFloat();
            spParam->strValue = oneParam.value(KEY_STRVALUE).toString();
            cmdData->param.push_back(spParam);
        }
        return cmdData;
    }
    //ldq20240307在此处解析json数据为byte
    if(QString::fromStdString(CMD_TYPE_STR_SetPasswordReg) == code)
    {

        /*样例：
        {
            "code": "SetPasswordReg",
            "data":{
                "mrid":"FFXOXX12",
                "passwd":123456,
                "lockno":2945
            }
        }
         */
        std::shared_ptr<SetPasswordRegCmdData_S> cmdData = std::make_shared<SetPasswordRegCmdData_S>();
        cmdData->code = code;
        cmdData->taskId = taskId;
        cmdData->time = data.value(KEY_TIME).toLongLong();
        cmdData->nodeKey = data.value(KEY_NODEKEY).toString();
        QVariant paramList = data.value(KEY_DATA);//.toList();
        //for(int i=0;i<paramList.size();++i)
        //{
            std::shared_ptr<SetPasswordRegParam_S> setPasswordRegParam = std::make_shared<SetPasswordRegParam_S>();
            QVariantHash oneParam=paramList.toHash();
            setPasswordRegParam->passwd = oneParam.value(KEY_PASSWD).toInt();
            setPasswordRegParam->lockno = oneParam.value(KEY_LOCKNO).toInt();
            setPasswordRegParam->controlNum0x = oneParam.value(KEY_ControlNum0X).toString();
            cmdData->param.push_back(setPasswordRegParam);
        //}
        return cmdData;

    }

    return nullptr;
}

int ConverterUtil::pointIndexToNo(int pointIdx)
{
    return pointIdx + POINT_INDEX_NO_DIFF;
}

int ConverterUtil::noToIndex(int no)
{
    return no - POINT_INDEX_NO_DIFF;
}

QString ConverterUtil::toFwErrMsg(int errCode)
{
    /*错误码
    0x00正确；
    0x01-0x0A保留；
    0x10表示其他错误（例如：整票校核的时候，某一个设备不通过，后续的设备均不允许操作，附带的就是这个错误码）；
    0x11表示设备被其他任务闭锁；
    0x12表示操作状态余当前状态不符合；
    0x13表示校核逻辑不通过；
    0X14表示不存在该设备;
    0x15表示程并控信息不符合。
    */
    QString errMsg;
    switch(errCode)
    {
    case 0x00:
        errMsg="";//正确
        break;
    case 0x10:
        errMsg="其他错误！";
        break;
    case 0x11:
        errMsg="设备被其他任务闭锁！";
        break;
    case 0x12:
        errMsg="操作状态与当前状态不符合！";
        break;
    case 0x13:
        errMsg="校核逻辑不通过！";
        break;
    case 0x14:
        errMsg="不存在该设备！";
        break;
    case 0x15:
        errMsg="程并控信息不符合！";
        break;
    }

    return errMsg;
}
