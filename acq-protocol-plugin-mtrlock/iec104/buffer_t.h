/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  buffer_t.h
 *生成日期：2011-04-07 
 *作者：    yay
 *功能说明：为规约库提供统一缓存机制
 *其它说明：description 
 *修改记录：date, maintainer 
 *          2011-04-07, yay, 检查和调整代码 
 *          2012-02-06, yay, 完成代码编写 
*******************************************************************************/

#ifndef PBUFFER_H
#define PBUFFER_H

//#include "data_type.h"
#include "../acq-protocol-plugin-utils/plugin_def.h"

class Buffer_T
{
    uint8 *pBuff, DataSize;    //缓冲区指针, 数据单元大小
    uint32 ReadPt, WritePt;    //读写指针
    uint32 Capacity;        //缓冲区数据单元容量
public:
    Buffer_T( void );
    ~Buffer_T( void );

    //创建能容纳capacity个数据大小为datasize的数据缓冲区.
    int Create( const uint16 capacity, const uint8 datasize );
    //从缓冲区中读取一个数据,要求data数据类型严格符合 Create()中datasize要求.
    int Read( void *data );
    //向缓冲区写入一个数据,要求data数据类型严格符合 Create()中datasize要求.
    int Write( const void *data );
    //取得缓冲区中未读数据的个数
    uint16 GetDataNum( void );
    //取得缓冲区的数据容量
    uint16 GetCapacity( void );
    //将缓冲区的读指针移回num个数据.
    void Rewind( uint16 num );
    //清空缓冲区.
    void Clear( );
};

#endif
