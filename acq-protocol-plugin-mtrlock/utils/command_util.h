#ifndef __COMMANDUTIL_H
#define __COMMANDUTIL_H
#include "data/send_json_data.h"
#include "iec104/scn_commandmem.h"


/**
 * @brief 用于操作组合下发命令工具类
 */
class SCN_TaskList;
class CommandUtil
{
public:
    /**
     * @brief 生成遥控命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYkCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成遥控结果命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYkResultCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成遥调命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYTCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成遥调结果命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYTResultCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成防误操作命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     * liujie add 20230525
     */
    static bool makeWFCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成防误操作结果命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     * liujie add 20230525
     */
    static bool makeWFResultCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成设值点命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeSPCmd(SCN_TaskList *pTaskList, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成总召命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeCallDataCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);


    /**
     * @brief 生成总召结束命令
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeCallDataEndCmd(std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成遥信命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYxCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成遥测命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYcCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

    /**
     * @brief 生成设置密码锁密码命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeSetPasswordRegCmd(const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);

private:

    /**
     * @brief 生成遥信\遥测命令
     * @param data 待发送数据
     * @param cmdList [out] 命令列表
     * @param errMsg [out] 执行错误详细信息
     * @return 执行结果：true--成功，false--失败
     */
    static bool makeYxYcCmd(int cmdType, const std::shared_ptr<BaseCmdData_S> &data, std::list<COMMAND> &cmdList, QString &errMsg);
};

#endif // __COMMANDUTIL_H
