/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  poll.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：定义问答式规约的基类Poll.
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef POLL_H
#define POLL_H

#include "protocol.h"

class Poll : public Protocol
{
private:
    static     uint16  CRC16[256];
    static     uint8   CRC8[128];
    static     uint8     change_h_to_l(uint8 temp);
    void    MakeCRC16Table(void);
    void    MakeCRC8Table(void);
protected:
    uint8     AckFinished;
    uint8   MakeCRC8(uint8 *,int);
    uint16  MakeCRC16(uint8 *,int);
    void    float_to_char(float a,char *trg);
    float char_to_float(char *src);
public:
    Poll() {}
    virtual ~Poll() {}
    virtual int RxProc()=0;
    virtual int TxProc()=0;
    virtual void SetProtocolConfig( PROTOCOL_CONFIG protocol_config );
    virtual int  TransProc(uint8 dir,uint8 *inbuf,int inlen,char* &outbuf,uint8 frametype = 0xff);

    virtual void SetConfigParam( ProtocolCfgParam_S &protocolCfg)=0;
};


#endif //POLL_H
