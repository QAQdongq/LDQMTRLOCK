/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  proto_def.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：定义规约类库共用的数据结构.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef PROTO_DEF_H
#define PROTO_DEF_H

//#include "data_type.h"
#include "../acq-protocol-plugin-utils/plugin_def.h"
extern "C"{
//#include "extslm.h"
}

//#define DIR_UP                            0
//#define DIR_DOWN                        1
//#define DIR_NONE                        2

#define CHAR_LENGTH_32   32
#define CHAR_LENGTH_64   64

#define PROTOCOL_SCISLOT_MAXNUM        16
#define PROTOCOL_U4FSLOT_MAXNUM        32
#define PROTOCOL_U4F_FACTOR_MAXNUM    8
#define PROTOCOL_U4F_SCANFRQ_MAXNUM    8

//时间结构
struct SYSTIME
{
    uint16    year;
    uint8    month;
    uint8    day;
    uint8    week;
    uint8    hour;
    uint8    minute;
    uint8    second;
    uint16    ms;
};
struct TIME
{
    uint32    Sec;
    uint32  Ms;
};

//事项数据
struct SOEDATA
{
    uint8    YxValue;
    SYSTIME soetime;
};

//sci配置
struct Sci_Config
{
    uint8    slot[PROTOCOL_SCISLOT_MAXNUM];        //插槽类型
    uint32    deadband;                            //死区值
    uint32    cpu_jumper;                            //CPU跳线
};

//u4f配置
struct U4f_Config
{
   uint8   slot[PROTOCOL_U4FSLOT_MAXNUM];            //插槽类型
   uint8   compfactor[PROTOCOL_U4F_FACTOR_MAXNUM];    //压缩因子
   uint8   scanfreq[PROTOCOL_U4F_SCANFRQ_MAXNUM];    //扫描频率
};

#endif //PROTO_DATADEF_H
