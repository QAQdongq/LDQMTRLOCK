/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  poll.cxx
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：问答式规约的基类Poll的函数实现.
 *其它说明：description
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

//#include "stdafx.h"
#include "poll.h"
#include <string.h>

uint8 Poll::CRC8[128];
uint16 Poll::CRC16[256];

/********************************************************************************
*        描述: 规约参数设置函数,负责初始化规约接口对象和规约
*              对象内部数据.
*        参数: protocol_config: 规约配置参数(输入)；
*        返回: 无.
********************************************************************************/
void Poll::SetProtocolConfig( PROTOCOL_CONFIG protocol_config )
{
    
    Protocol::SetProtocolConfig( protocol_config );
    AckFinished = 0;
    MakeCRC16Table();
    MakeCRC8Table();
}

/********************************************************************************
*        描述: 生成16位校验参考码表.
*        参数: 无.
*        返回: 无.
********************************************************************************/
void Poll::MakeCRC16Table(void)
{
     int    i,j;
     uint16 v;

        for(i=0;i<256;i++)
        {
                v = i;
                for(j=0;j<8;j++)
                {
                        if(v & 0x01)
                        {
                                v >>= 1;
                                v ^= 0xa001;
                        }
                        else    v >>= 1;
                }
                CRC16[i] = v;
        }

}

/********************************************************************************
*        描述: 生成8位校验参考码表.
*        参数: 无.
*        返回: 无.
********************************************************************************/
void Poll::MakeCRC8Table(void)
{
 int i,j;
 uint8  v;

        for(i=0;i<128;i++)
        {
                for(j=0;j<7;j++)
                {
                        v = i;
                        if(v & 0x01)
                        {
                                v >>= 1;
                                v ^= 0x44;
                        }
                        else    v >>= 1;
                }
                v &= 0x7f;
                CRC8[i] = v;
        }

}

/********************************************************************************
*        描述: 生成8位校验码.
********************************************************************************/
uint8 Poll::MakeCRC8( uint8 *buf,int len )
{
 uint8  tempbuf[128];
 uint8  temp;
 uint8  k[128];
 int    i,j;

        temp = 0;
        memcpy(tempbuf,buf,128*sizeof(uint8));

        for(i=0;i<len;i++) temp ^= tempbuf[i];

        temp &= 0x80;
        for(i=0;i<len;i++)
        {
                tempbuf[i] <<= 1;
                tempbuf[i] = change_h_to_l(tempbuf[i]);
        }
        for(i=0;i<128;i++) k[i] = 0;
        for(i=1;i<len;i++)
        {
                tempbuf[i]<<=1;
                for(j=0;j<7;j++)
                {
                        if(tempbuf[i] & 0x80) k[(i-1)*7+j] = 1;
                        else    k[(i-1)*7+j] = 0;
                        tempbuf[i] <<= 1;
                }
        }
        j = tempbuf[0];
        for(i=0;i<len*7;i++)
        {
                j <<= 1;
                if(k[i]!=0) j ^= 0x01;
                if(j>=0x80) j ^= 0x91;
        }
        j = change_h_to_l(j);
        j >>= 1;
        j ^= temp;

        return((uint8)j);
}

/********************************************************************************
*        描述: 生成16位校验码.
********************************************************************************/
uint16 Poll::MakeCRC16(uint8 *buf,int len)
{
  uint16        crcdata;
  uint8         bval;
  uint8         cnt;

        bval = buf[0];
        cnt = 0;
        crcdata = buf[1] + buf[2]*256;
        len -= 3;
        cnt += 3;
        while(len>=0)
        {
                crcdata ^= CRC16[bval];
                if(len<=0) break;
                bval = LOBYTE(crcdata);
                crcdata = HIBYTE(crcdata) + buf[cnt]*256;
                len--;
                cnt++;
        }
        bval = LOBYTE(crcdata);
        crcdata = (uint16)HIBYTE(crcdata);
        crcdata ^= CRC16[bval];
        bval = LOBYTE(crcdata);
        crcdata = (uint16)HIBYTE(crcdata);
        crcdata ^= CRC16[bval];
        return(crcdata);
}

uint8 Poll::change_h_to_l(uint8 temp)
{
 int i,k;
 uint8 t;

        k = temp;
        temp = 0;
        for(i=0;i<8;i++)
        {
                if((k%2)==1)
                {
                        t =1;
                        t <<= 7-i;
                        temp |= t;
                }
                k >>= 1;
        }
        return(temp);
}

void Poll::float_to_char(float a,char *trg)
{
        char *tmp = (char*)&a;
#if BYTE_ORDER == BIG_ENDIAN
        //LOG_INFO(pRouteInf->GetChanNo(), QString("Poll::float_to_char BYTE_ORDER == BIG_ENDIAN.....\n"));
        trg[3] = tmp[0];
        trg[2] = tmp[1];
        trg[1] = tmp[2];
        trg[0] = tmp[3];
#else
        //LOG_INFO(pRouteInf->GetChanNo(), QString("Poll::float_to_char BYTE_ORDER != BIG_ENDIAN.....\n"));
        trg[0] = tmp[0];
        trg[1] = tmp[1];
        trg[2] = tmp[2];
        trg[3] = tmp[3];

#endif
}

float Poll::char_to_float(char *src)
{
        float32 trg;
        char *tmp = (char *)&trg;

#if BYTE_ORDER == BIG_ENDIAN
        //LOG_INFO(pRouteInf->GetChanNo(), QString("Poll::char_to_float BYTE_ORDER == BIG_ENDIAN.....\n"));
        tmp[0] = src[3];
        tmp[1] = src[2];
        tmp[2] = src[1];
        tmp[3] = src[0];
#else
        //LOG_INFO(pRouteInf->GetChanNo(), QString("Poll::char_to_float BYTE_ORDER != BIG_ENDIAN.....\n"));
        tmp[0] = src[0];
        tmp[1] = src[1];
        tmp[2] = src[2];
        tmp[3] = src[3];
#endif
        return trg;
}


/************************************************************************
作者：   yay     
创建日期：2010.10.10  
函数功能说明： 规约解析处理函数
输入参数：
             uint8 dir,    报文上下行方向
             uint8 *inbuf,   报文首地址
             int inlen,      报文长度
             char* &outbuf,    翻译结果首地址
             uint8 frametype    需要翻译的帧类型
输出参数：  
             无
            
返回: = 1, 解析成功返回;
      =-1, 解析未成功返回.
            
函数扇出清单：    
函数扇入清单：
函数体中用到的公共资源：
更新后的上述资源：    
*************************************************************************/
int Poll::TransProc(uint8 dir,uint8 *inbuf,int inlen,char* &outbuf,uint8 frametype)
{
    outbuf = NULL;
    return 1;
}
