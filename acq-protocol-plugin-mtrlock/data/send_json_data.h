#ifndef __SENDJSONDATA_H
#define __SENDJSONDATA_H
#include "plugin104_define.h"

#include <QString>
#include <QVariantList>
//发送数据JSON结构信息
/**
示例：
下发遥控命令：
{
"code":"YKReq",                    //下发遥控命令
"time":"1630564973670",            //当前命令生成的时间点(毫秒)
"nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
"data":[                        //遥控数据
{
"rtuId":1,
"cchId":1,         // 通道ID
"no":1,          // 遥控点序号
"value":0,            // 遥控值
"type":1            // 遥控类型：1：执行、2：预置
}
]
}
下发总召命令：
{
"code":"CallReq",                //下发总召命令
"time":"1630564973670",            //当前命令生成的时间点(毫秒)
"nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
"data":{                        //通道数据
"rtuId":1,
"cchId":1,         // 通道ID
}
}
下发校时命令：
{
"code":"SyncClockReq",                //下发校时命令
"time":"1630564973670",            //当前命令生成的时间点(毫秒)
"nodeKey":"192.168.80.100:1",        //当前采集服务的MQ子主题tag关键字
"data":{                        //通道数据
"rtuId":1,
"cchId":1,         // 通道ID
}
}

*/
typedef struct SendCmdData
{
    QString code;//命令类型：YXCYC:-周期遥信，YXCHG:变位遥信, yc-遥测 ,yk-遥控, mc-脉冲, calldata-总召, calldataend-总召结束
    QString time;//时间戳(秒+毫秒，相对于UTC时间)
    QString nodeKey;//当前采集服务的MQ子主题tag关键字
    std::list<std::shared_ptr<BaseParam_S>> param;
}SendCmdData_S;

#endif // __SENDJSONDATA_H
