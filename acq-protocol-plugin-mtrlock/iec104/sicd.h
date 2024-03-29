/*从/utplat/include/sicd.h中修改过来的*/

#ifndef sicd_H
#define sicd_H

//#include "drs.h"

#ifdef DRSGEN_EXPORT
#ifdef WIN32
#define DRS_PROTO __declspec(dllexport)
#else
#define DRS_PROTO
#endif
#else
#ifdef WIN32
#define DRS_PROTO __declspec(dllimport)
#else
#define DRS_PROTO extern
#endif
#endif

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SICD_H_RPCGEN
#define _SICD_H_RPCGEN



#ifdef __cplusplus
extern "C" {
#endif


/* Digital input point status */

#define SICD_M_DI_STS_ON_LINE 0x00000001
#define SICD_M_DI_STS_RESTART 0x00000002
#define SICD_M_DI_STS_COMM_LOST 0x00000004
#define SICD_M_DI_STS_REMOTE_FORCED 0x00000008
#define SICD_M_DI_STS_LOCAL_FORCED 0x00000010
#define SICD_M_DI_STS_CHATTER_FILTER 0x00000020
#define SICD_M_DI_STS_RESERVED 0x00000040
#define SICD_M_DI_STS_STATE 0x00000080
#define SICD_M_DI_STS_TRANSITONE 0x00000100
#define SICD_M_DI_STS_TRANSITTWO 0x00000200
#define SICD_M_DI_STS_OV 0x00000400
#define SICD_M_DI_STS_BL 0x00000800
#define SICD_M_DI_STS_SB 0x00001000
#define SICD_M_DI_STS_NT 0x00002000
#define SICD_M_DI_STS_UNLOCK 0x00004000

/* Analog input point status */

#define SICD_M_AI_STS_ON_LINE 0x00000001
#define SICD_M_AI_STS_RESTART 0x00000002
#define SICD_M_AI_STS_COMM_LOST 0x00000004
#define SICD_M_AI_STS_REMOTE_FORCED 0x00000008
#define SICD_M_AI_STS_LOCAL_FORCED 0x00000010
#define SICD_M_AI_STS_OVER_RANGE 0x00000020
#define SICD_M_AI_STS_REF_CHECK 0x00000040
#define SICD_M_AI_STS_RESERVED 0x00000080
#define SICD_M_AI_STS_OV 0x00000100
#define SICD_M_AI_STS_BL 0x00000200
#define SICD_M_AI_STS_SB 0x00000400
#define SICD_M_AI_STS_NT 0x00000800


/*WF input point status */

#define SICD_M_WF_AVC_ON_LINE 0x00000080
#define SICD_M_WF_STS_ON_LINE 0x00000040
#define SICD_M_WF_YKBS_ON_LINE 0x00000020
#define SICD_M_WF_STS_BS 0x00000001
#define SICD_M_WF_STS_YX 0x00000003


/*STRAP input point status */

#define SICD_M_STRAP_STS_ON_LINE 0x00000001 /* 压板值有效位 */
#define SICD_M_STRAP_STS_ENABLE 0x00000100 /* 压板值=投 */
#define SICD_M_STRAP_STS_DISABLE 0x00000200 /* 压板值=退 */
#define SICD_M_STRAP_STS_HALF_ENABLE 0x00000400 /* 压板值=半投 */
#define SICD_M_STRAP_STS_ERROR_DISABLE 0x00000800 /* 压板值=异常退 */
#define SICD_M_STRAP_STS_ERROR_ENABLE 0x00001000 /* 压板值=异常投 */
#define SICD_M_STRAP_STS_ERROR_OFFLINE 0x00002000 /* 压板值=离线异常 */
#define SICD_M_STRAP_STS_RESERVED 0x00004000 /* 压板值=保留 */
#define SICD_M_STRAP_STS_ERROR_TOTAL 0x00008000 /* 压板值=总异常 */


/* Counter point status */

#define SICD_M_CO_STS_ON_LINE 0x00000001
#define SICD_M_CO_STS_RESTART 0x00000002
#define SICD_M_CO_STS_COMM_LOST 0x00000004
#define SICD_M_CO_STS_REMOTE_FORCED 0x00000008
#define SICD_M_CO_STS_LOCAL_FORCED 0x00000010
#define SICD_M_CO_STS_ROLLOVER 0x00000020
#define SICD_M_CO_STS_RESERVED1 0x00000040
#define SICD_M_CO_STS_RESERVED2 0x00000080

/* rtu state */
#define SICD_M_RTU_STS_STATE 0x00000080
#define SICD_M_RTU_STS_DSBL 0x00000100

#ifdef __cplusplus
}
#endif

#endif /* !_SICD_H_RPCGEN */

#endif  /* sicd_H */
