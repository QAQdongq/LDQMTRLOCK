/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  prottype_def.h
 *生成日期：2011-07-22
 *作者：    yay
 *功能说明：保护类型定义头文件
 *其它说明：description
 *修改记录：date, maintainer
 *          2011-07-22, yay, 检查和调整代码
 *          2012-03-22, yay, 完成代码编写
*******************************************************************************/

#ifndef PROTTYPE_DEF_H
#define PROTTYPE_DEF_H

//定义保护类型
#define PROT_LFP            1
#define PROT_ISA            2
#define PROT_SEL            3
#define PROT_DFP            4
#define PROT_LSA_P            5
#define PROT_SBC            6
#define PROT_YH3003            7
#define PROT_FWB            8
#define PROT_CAN2000            9
#define PROT_IEC103            10
#define PROT_LOW_CURRENT        11
#define PROT_WATER            12
#define PROT_IEC103_NR            13
#define PROT_UT_61850            14

#define PROT_MODBUS            15

#define PROT_HJLM_300_310            30


#define PROT_IEC103_EQU_CONFIG    100     /*专用于IEC103保护，不能与IEC103保护的其他保护命令冲突*/

#define PROT_CANCEL_COMMAND 0xff


#endif
