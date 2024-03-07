/*******************************************************************************
 *版权所有：珠海优特电力科技
 *版本号：  1.0.0
 *文件名：  buffer.cxx
 *生成日期：2011-09-22
 *作者：    yay
 *功能说明：为规约库提供统一缓存机制
 *其它说明：description
 *修改记录：date, maintainer
 *          2011-10-12, yay, 检查和调整代码
 *          2012-01-22, yay, 完成代码编写
*******************************************************************************/

#include <string.h>
#include "buffer_t.h"

Buffer_T::Buffer_T( void )
{
    pBuff = NULL;
    ReadPt = WritePt = 0;
    Capacity = 0;
    DataSize = 0;
}

Buffer_T::~Buffer_T( void )
{
    if( pBuff )
    {
        delete []pBuff;
        pBuff = NULL;
    }

    ReadPt = WritePt = 0;
    Capacity = 0;
    DataSize = 0;
}

/*****************************************************************
*    描述: 创建能容纳capacity个数据大小为datasize的数据缓冲区.
*   参数: capacity: 缓冲区数据容量;
*          datasize: 缓冲区数据单元大小.
*    返回: = 1, 创建成功;
*          =-1, 创建失败.
******************************************************************/
int Buffer_T::Create( const uint16 capacity, const uint8 datasize )
{
    if( pBuff )
    {
        if( Capacity == capacity && DataSize == datasize )
            return 1;

        delete []pBuff;
        pBuff = NULL;
        Capacity = 0;
        DataSize = 0;
    } 

    pBuff = new uint8[capacity*datasize];
    if( !pBuff )
        return -1;

    Capacity = capacity;
    DataSize = datasize;
    ReadPt = WritePt = 0;
    
    return 1;
}

/*******************************************************************
*    描述: 从缓冲区中读取一个数据.
*    参数: 
*          data: 用于存储取得的大小为DataSize的数据.
*    返回: = 1, 读取成功.
*          =-1, 读取出错.
*    注意: data数据类型必须严格符合 Create()中datasize要求.
*******************************************************************/
int Buffer_T::Read( void *data )
{
    if( GetDataNum() < 1 || !pBuff ) 
        return -1;
    else 
    {
        memcpy( data, &pBuff[ReadPt], DataSize );
        ReadPt = (ReadPt+DataSize)%(Capacity*DataSize);
    }

    return 1;
}

/*******************************************************************
*    描述: 将缓冲区的读指针移回num个数据.
*    参数: num, 指针移动的数据个数.
*******************************************************************/
void Buffer_T::Rewind( uint16 num )
{
    if( !pBuff ) 
        return;
    else
    {
        ReadPt = (ReadPt+Capacity*DataSize-num*DataSize)%(Capacity*DataSize);
    }
}

/******************************************************************
*    描述: 取得缓冲区中未读数据的个数.
*******************************************************************/
uint16 Buffer_T::GetDataNum( void )
{
    if( Capacity == 0 || !pBuff )
        return 0;

    return ( (WritePt+Capacity*DataSize-ReadPt)/DataSize)%Capacity;
}

/******************************************************************
*    描述: 向缓冲区写入一个数据
*    参数: 
*          data: 用于存储大小为DataSize的数据.
*    返回: = 1, 写成功.
*          =-1, 写出错.
*    注意: data数据类型必须严格符合 Create()中datasize要求.
******************************************************************/
int Buffer_T::Write( const void *data )
{
    if( !pBuff )
        return -1;

    memcpy( &pBuff[WritePt], data, DataSize );
    WritePt = (WritePt+DataSize)%(Capacity*DataSize);

    return 1;
}

/******************************************************************
*    描述: 取得缓冲区的数据容量.
*******************************************************************/
uint16 Buffer_T::GetCapacity( void )
{
    return Capacity;
}

/******************************************************************
*    描述: 清空缓冲区.
*******************************************************************/
void Buffer_T::Clear( )
{
    ReadPt = WritePt;
}
