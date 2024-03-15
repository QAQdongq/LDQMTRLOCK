#include "command_util.h"
#include "converter_util.h"
#include "plugin104_define.h"
#include "iec104/scn_tasklist.h"

/**
 * @brief 生成遥控命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeYkCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeYkCmd:No data need to be sent!";
        return false;
    }
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter = data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        //添加到任务队列
        std::shared_ptr<YKReqParam_S> param = std::static_pointer_cast<YKReqParam_S>(*iter);
        TaskInfo taskInfo(data->code, data->taskId, param->no);
        if(pTaskList->exist(data->code, param->no))
        {
            errMsg ="makeYkCmd: YK cmd has already exist! skip!!! code:" + data->code +", pointNo:" + QString::number(param->no);
            continue;
        }
        pTaskList->addTask(taskInfo);

        //注意：遥控命令必须一个一个添加，而不能合成一个命令下发，因为遥控命令需要确认后，才能再发下一个命令，需要用到队列
        COMMAND command;
        command.cmdType = CMD_TYPE_YK;
        command.taskId = data->taskId;
        command.dataList.push_back(*iter);
        cmdList.push_back(command);

    }
    return true;
}

/**
 * @brief 生成遥控结果命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeYkResultCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeYkResultCmd:No data need to be sent!";
        return false;
    }
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        //注意：遥控命令必须一个一个添加，而不能合成一个命令下发，因为遥控命令需要确认后，才能再发下一个命令，需要用到队列
        COMMAND command;
        command.cmdType = CMD_TYPE_YKRESULT;
        command.taskId = data->taskId;
        command.dataList.push_back(*iter);
        cmdList.push_back(command);
    }
    return true;
}

/**
 * @brief 生成遥调命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeYTCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeYTCmd:No data need to be sent!";
        return false;
    }
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter = data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        //添加到任务队列
        std::shared_ptr<YKReqParam_S> param = std::static_pointer_cast<YKReqParam_S>(*iter);
        TaskInfo taskInfo(data->code, data->taskId, param->no);
        if(pTaskList->exist(data->code, param->no))
        {
            errMsg ="makeYkCmd: YT cmd has already exist! skip!!! code:" + data->code +", pointNo:" + QString::number(param->no);
            continue;
        }
        pTaskList->addTask(taskInfo);

        //注意：遥调命令必须一个一个添加，而不能合成一个命令下发，因为遥调命令需要确认后，才能再发下一个命令，需要用到队列
        COMMAND command;
        command.cmdType = CMD_TYPE_YT;
        command.taskId = data->taskId;
        command.dataList.push_back(*iter);
        cmdList.push_back(command);
    }
    return true;
}

/**
 * @brief 生成防误命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeWFCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeWFCmd:No data need to be sent!";
        return false;
    }
    //添加到任务队列
    COMMAND command;
    command.cmdType = CMD_TYPE_WF;
    command.taskId = data->taskId;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter = data->param.begin();
    QList<std::shared_ptr<BaseParam_S>> dataList;
    int type= -1;
    for(; iter != data->param.end(); iter++)
    {
        std::shared_ptr<FWReqParam_S> tmpReq = std::static_pointer_cast<FWReqParam_S>(*iter);
        if(-1 == type)
        {
            type = tmpReq->type;
        }
        else
        {
            if(type != tmpReq->type)
            {
                //同一个五防命令下，类型必须是同一类，要么是：1-防误校验， 要么是：2-应急模式。不能混在一起。因为它们的类型标识不同，一个是0xC8,一个是：0xC9
                errMsg = QString("All type must be same in one 'FW' command! SKIP!!!! type:%1, no:%2").arg(tmpReq->type).arg(tmpReq->no);
                //return -1;
                continue;
            }
        }

        dataList.append(*iter);
        command.dataList.push_back(*iter);
    }

    int pointNo=-1;//防误命令，每次只能有一个防误命令正在执行（防误可一批点进行校验），不用细分到点级别
    if(pTaskList->exist(data->code, pointNo))
    {
        errMsg ="makeWFCmd: WF cmd has already exist! skip!!! code:" + data->code +", pointNo:" + QString::number(pointNo);
        return false;
    }
    QVariantHash otherParamHash;
    TaskInfo taskInfo(data->code, data->taskId, pointNo, otherParamHash, dataList);
    pTaskList->addTask(taskInfo);

    //加到命令列表
    cmdList.push_back(command);
    return true;
}

/**
 * @brief 生成遥调结果命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeYTResultCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeYTResultCmd:No data need to be sent!";
        return false;
    }
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        //注意：遥调命令必须一个一个添加，而不能合成一个命令下发，因为遥调命令需要确认后，才能再发下一个命令，需要用到队列
        COMMAND command;
        command.cmdType = CMD_TYPE_YTRESULT;
        command.taskId = data->taskId;
        command.dataList.push_back(*iter);
        cmdList.push_back(command);
    }
    return true;
}

/**
 * @brief 生成设置点命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeSPCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeSPCmd:No data need to be sent!";
        return false;
    }
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter = data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        //添加到任务队列
        std::shared_ptr<SPReqParam_S> param = std::static_pointer_cast<SPReqParam_S>(*iter);
        QVariantHash otherParamHash;
        otherParamHash.insert(KEY_TYPE, param->type);
        otherParamHash.insert(KEY_DECVALUE, param->decValue);
        otherParamHash.insert(KEY_FLOATVALUE, param->floatValue);
        otherParamHash.insert(KEY_INTVALUE, param->intValue);
        otherParamHash.insert(KEY_STRVALUE, param->strValue);

        TaskInfo taskInfo(data->code, data->taskId, param->no, otherParamHash);
        if(pTaskList->exist(data->code, param->no))
        {
            errMsg ="makeYkCmd: SP cmd has already exist! skip!!! code:" + data->code +", pointNo:" + QString::number(param->no);
            continue;
        }
        pTaskList->addTask(taskInfo);

        //注意：设值点命令必须一个一个添加，而不能合成一个命令下发，因为设值点命令需要确认后，才能再发下一个命令，需要用到队列
        COMMAND command;
        command.cmdType = CMD_TYPE_SP;
        command.taskId = data->taskId;
        command.dataList.push_back(*iter);
        cmdList.push_back(command);
    }
    return true;
}

/**
 * @brief 生成总召命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeCallDataCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeCallDataCmd: No data need to be sent!";
        return false;
    }
    COMMAND command;
    command.cmdType = CMD_TYPE_CALLALLDATA;
    command.taskId = data->taskId;

    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        command.dataList.push_back(*iter);
    }
    cmdList.push_back(command);
    return true;
}

/**
 * @brief 生成总召结束命令
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeCallDataEndCmd(std::list<COMMAND> &cmdList, QString &errMsg)
{
    COMMAND command;
    command.cmdType = CMD_TYPE_CALLALLDATAEND;
    cmdList.push_back(command);
    return true;
}


/**
 * @brief 生成遥信命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeYxCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    return makeYxYcCmd(CMD_TYPE_YX, data, cmdList, errMsg);
}

/**
 * @brief 生成遥测命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeYcCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    return makeYxYcCmd(CMD_TYPE_YC, data, cmdList, errMsg);
}

bool CommandUtil::makeYxYcCmd(int cmdType, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    std::size_t dataSize = data->param.size();
    if(0 == dataSize)
    {
        errMsg ="makeYxYcCmd:No data need to be sent!";
        return false;
    }
    COMMAND command;
    command.cmdType = cmdType;
    command.taskId = data->taskId;
    std::list<std::shared_ptr<BaseParam_S>>::const_iterator iter =  data->param.begin();
    for(; iter != data->param.end(); iter++)
    {
        command.dataList.push_back(*iter);
    }
    cmdList.push_back(command);
    return true;
}


/**
 * @brief 生成lalala命令
 * @param data 待发送数据
 * @param cmdList [out] 命令列表
 * @param errMsg [out] 执行错误详细信息
 * @return 执行结果：true--成功，false--失败
 */
bool CommandUtil::makeSetPasswordRegCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg)
{
    //在此处生成一个command对象并cmdList.push_back(command);

    return true;
}
