#ifndef PLUGIN104_STRUCT_H
#define PLUGIN104_STRUCT_H

#define BYTE uchar
#define WORD ushort
#define FALSE 0
#define TRUE 1
#define DWORD ulong

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

#define  MMS_LITE
#include <QVector>

struct SYNCHRONIZETIME
{
    WORD wMilSec;            //包括了秒
    BYTE byMin        :6;
    BYTE byRes1        :1;
    BYTE byIV        :1;
    BYTE byHour        :5;
    BYTE byRes2        :2;
    BYTE bySU        :1;
    BYTE byDayM        :5;
    BYTE byDayW        :3;
    BYTE byMonth    :4;
    BYTE byRes3        :4;
    BYTE byYear        :7;
    BYTE byRes4        :1;
} ;    //时钟同步信息元素

/*typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;*/

struct VSQ
{
    union
    {
        struct
        {
            BYTE byNum    :7; //信息体或元素的数目,最大127个
            BYTE bySQ    :1; //表示信息体是顺序的还是非顺序的
        };
        BYTE byValue;
    };
    VSQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }
} ;    //可变结构限定词

struct DCORCO
{
    BYTE byDCS    :2;
    BYTE byQU    :5;
    BYTE bySE    :1;
    operator BYTE()
    {
        return *((BYTE*)this);
    }
    DCORCO(BYTE v)
    {
        *((BYTE*)this) = v;
    }
    DCORCO(){}
};    //遥控、升降命令码

#pragma pack(push,8)//
struct DEVICE_STRUCT {
    BYTE            byNo;                    //编号
    char            szLineName[40];            //线路名称
    WORD            wDeviceAdress;            //装置地址
    char            szDeviceName[40];        //装置名称
    char            szDeviceDesc[60];        //装置说明
    char            szChannelName[12][40];    //通道名称
    char            szDeviceVersion[8];        //装置版本信息
    WORD            wAcqNum;                //每周期采样点数//PT变比
    DWORD            dwPt;                    //PT变比
    DWORD            dwCt;                    //CT变比
    DWORD            dwRes;                    //保留字段
    char            szRes;                    //保留字段
};
#pragma pack(pop)

typedef QVector<DEVICE_STRUCT> ARY_DEVICE_STRUCT;

struct NVAQOS
{
    BYTE byQL    :7;
    BYTE bySE    :1;
    operator BYTE()
    {
        return *((BYTE*)this);
    }
    NVAQOS(BYTE v)
    {
        *((BYTE*)this) = v;
    }
    NVAQOS(){}
};    //遥控、升降命令码

struct SIQ
{
    BYTE bySPI        :1;        //分合位        0开，         1合
    BYTE byRES        :3;
    BYTE byBL        :1;        //封锁位        0未封，       1封
    BYTE bySB        :1;        //取代位        0未被取代，   1被取代
    BYTE byNT        :1;        //当前值        0当前值，     1非当前值
    BYTE byIV        :1;        //有效位        0有效，       1无效
};    //带品质描述的单点信息

struct DIQ
{
    BYTE byDPI        :2;        //分合位 0，3中间状态不确定，1确定状态开，2确定状态合
    BYTE byRES        :2;
    BYTE byBL        :1;        //封锁位        0未封，       1封
    BYTE bySB        :1;        //取代位        0未被取代，   1被取代
    BYTE byNT        :1;        //当前值        0当前值，     1非当前值
    BYTE byIV        :1;        //有效位        0有效，       1无效
};    //带品质描述的双点信息

struct CP24Time2a
{
    BYTE byMilSecLow;
    BYTE byMilSecHigh;
    BYTE byMin    :6;
    BYTE byRes    :1;
    BYTE byIV    :1;
    WORD ReadMilSec()
    {
        return (WORD)(byMilSecHigh*0x100+byMilSecLow);
    }
    void WriteMilSec(WORD wMilSec)
    {
        byMilSecLow = (BYTE)wMilSec ;
        byMilSecHigh = (BYTE)(wMilSec >> 8);
    }
};

/*日历时钟数据结构,用来处理带时标的单点和双点信息*/
struct CP56Time2a
{
    BYTE    byMilSecLow;                        //毫秒的低位
    BYTE    byMilSecHigh;                        //毫秒的高位,毫秒值范围为:0--59999
    BYTE    byMin        :6;                        //分,占该字节的前6位
    BYTE    byRes1        :1;                        //保留1
    BYTE    byIV        :1;                        //有效位,0:=有效,1:=无效
    BYTE    byHour        :5;                        //小时数:0--23
    BYTE    byRes2        :2;                        //保留2
    BYTE    bySU        :1;                        //SU
    BYTE    byDayofMonth:5;                        //号数,1--31
    BYTE    byDayofWeek    :3;                        //星期数,1--7
    BYTE    byMonth        :4;                        //月份,1--12
    BYTE    byRes3        :4;                        //保留3
    BYTE    byYear        :7;                        //年份0--99
    BYTE    byRes4        :1;                        //保留4
    WORD  ReadMilSec()                            //读出本结构中的毫秒数
    {
        return (WORD)(byMilSecHigh*0x100+byMilSecLow);
    }
    void  WriteMilSec(WORD wMilSec)                //写毫秒到本结构对应的字段
    {
        byMilSecLow = (BYTE)wMilSec ;            //先得到低位字节值
        byMilSecHigh = (BYTE)(wMilSec >> 8);    //得到高位字节值
    }
};
struct MENASTRUCT
{
    short wValue;
    BYTE byOV        :1;
    BYTE byRes        :3;
    BYTE byBL        :1;
    BYTE bySB        :1;
    BYTE byNT        :1;
    BYTE byIV        :1;
};    //带品质描述的遥测信息元素

struct QCC
{
    BYTE byRQT    :6;    //RQT,0未用6--31为配套标准保留
    BYTE byFRZ    :2;    //FRZ，0请求计数量，1冻结不带复位，2冻结带复位，3计数器复位
    operator BYTE()
    {
        return *((BYTE*)this);
    }
};//电能计数量召唤命令的限定词QCC

#pragma pack(push, 1)  //以下各结构内存按1个字节分配 结束#pragma pack(pop)

struct CTRLFIELD
{
    union
    {
        struct S_NS
        {
            WORD    wIsSF        :1;        //0：I幀，1：S幀或U幀
            WORD    wNS            :15;    //I幀时表示发送序号
        };
        struct S_UF
        {
            WORD    wIsSF        :1;        //0：I幀，1：S幀或U幀
            WORD    wIsUF        :1;        //如果wIsSF为1,则0: S幀, 1: U幀
            WORD    wStartAct    :1;        //U幀时: 启动传输数据
            WORD    wStartCon    :1;        //启动确认
            WORD    wStopAct    :1;        //停止传输数据
            WORD    wStopCon    :1;        //停止确认
            WORD    wTestAct    :1;        //激活测试
            WORD    wTestCon    :1;        //测试确认
            WORD    wPad1        :8;
        };
    };
    WORD        wPad2    :1;
    WORD        wNR        :15;
    CTRLFIELD()
    {
        S_NS.wIsSF= S_UF.wIsSF = FALSE;
        S_NS.wNS = wPad2 = wNR = 0;
    }

};

struct INFOADDR
{
    BYTE byAddrByte1;
    BYTE byAddrByte2;
    BYTE byAddrByte3;
    DWORD Addr()
    {
        return byAddrByte3 * 0x100 * 0x100 + byAddrByte2 * 0x100 + byAddrByte1;
    }
    INFOADDR& operator = (DWORD dwAddr)
    {
        byAddrByte1 = (BYTE)dwAddr;
        byAddrByte2 = (BYTE)(dwAddr>>8);
        byAddrByte3 = (BYTE)(dwAddr>>16);
        return *this;
    }
};

struct COT
{
    union
    {
        struct
        {
            BYTE byCause:6;    //传送原因（0未定义，1--63其中1--43为配套保留）
            BYTE byPN    :1;    //肯定、否定认可(0肯定、1否定认可)
            BYTE byTest    :1;    //测试、未测试(1试验、0未试验)
        };
        BYTE byValue;
    };
    COT& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

    BYTE byOriginator;
    COT& operator = (WORD v)
    {
        byValue = LOBYTE(v);
        byOriginator = HIBYTE(v);
        return *this;
    }

};    //传送原因

/*ASDU(应用数据单元)报文头*/
struct ASDUHEAD
{
    BYTE    byTypeId;            //类型标识（其中120到127为文件传输的类型标识）
    VSQ        Vsq;                //可变结构限定词
    COT        Cot;                //传送原因
    WORD    byAppAddr;            //应用单元公共地址???在104中为BYTE类型

    BYTE    VsqVal()            //返回N(信息元素/信息体的数量)的值
    {
        return Vsq.byValue;
    }

    WORD CotVal()
    {
        return Cot.byValue + Cot.byOriginator * 256;
    }
    WORD AppAddr()
    {
        return byAppAddr;
    }
    /*
    DWORD InfoAddr()
    {
        return *((WORD*)(this + 1));
    }
    */
} ;

/*104规约数据帧格式结构*/

union FRAME104                    //除去byStart和byLength外就剩下APDU
{
    struct SFrame104Data
    {
        BYTE        byStart;      //START 68H
        BYTE        byLength;     //Length of APDU
        CTRLFIELD    CtrlField;    //APCI            四个字节的应用规约控制信息
        ASDUHEAD    AsduHead;     //ASDU HEAD       ASDU固定的头部分
        BYTE        AsduData[2048];//ASDU           ASDU信息体部分//AsduData[247] wyh 2007-8-23 修改为2K 传输保护定值扩长度：68的低3位
    };
    struct SFrame104ErrInf
    {
        BYTE byFrameType;
        BYTE byLength1;
        WORD wErrCode;
        char szErr[2048];    //szErr[253] wyh 2007-8-23 修改为 2K 传输保护定值扩长度：68的低3位
    };

    WORD AppAddr(){
        return AsduHead.byAppAddr;
    };

    DWORD FirstInfoAddr(){
        DWORD dwInfAddr;
        memcpy(&dwInfAddr, this->AsduData, 3);
        return dwInfAddr;
    };

    WORD CotVal(){
        WORD Val = AsduHead.CotVal();
        return Val;
    };

};

//--File-Transmission-2003-10-21------------------------------------------------------------

//选择和召唤限定词
struct SCQ
{
    union
    {
        struct
        {
            BYTE byCause       :4;//0未用，
                                  //1选择文件
                                  //2请求文件
                                  //3停止激活文件
                                  //4删除文件
                                  //5选择节
                                  //6请求节
                                  //7停止激活节
                                  //8--10为配套标准保留（兼用范围）
                                  //11--15为特殊用途保留（专用范围）
            BYTE byUI          :4;//0缺损
                                  //1无被请求的存储空间
                                  //2校验和错
                                  //3非期望的通信服务
                                  //4非期望的文件名称
                                  //5非期望的节名称
                                  //6-10为配套标准保留（兼用范围）
                                  //11--15为特殊用途保留（专用范围）
        };

        BYTE byValue;
    };
    SCQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//文件或节确认
struct AFQ
{
    union
    {
        struct
        {
            BYTE byCon         :4;//0未用，
                                  //1文件传输的正确认可
                                  //2文件传输的否定认可
                                  //3节传输正确认可
                                  //4节传输的否定认可
                                  //5--10为配套标准保留（兼用范围）
                                  //11--15为特殊用途保留（专用范围）
            BYTE byUI          :4;//0缺省
                                  //1无被请求的存储空间
                                  //2校验和错
                                  //3非期望的通信服务
                                  //4非期望的文件名称
                                  //5非期望的节名称
                                  //6-10为配套标准保留（兼用范围）
                                  //11--15为特殊用途保留（专用范围）
        };

        BYTE byValue;
    };
    AFQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//文件准备好
struct SRQ
{
    union
    {
        struct
        {
            BYTE byUI          :7;//0未用，1--63为配套标准保留（兼用范围）64--127为特殊用途保留（专用范围）
            BYTE byReady       :1;//<0>:=节准备好去装载<1>:=节还没准备好去装载
        };

        BYTE byValue;
    };
    SRQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//文件准备好
struct FRQ
{
    union
    {
        struct
        {
            BYTE byUI          :7;//0未用，1--63为配套标准保留（兼用范围）64--127为特殊用途保留（专用范围）
            BYTE byCon         :1;//0为选择，请求，停止激活，删除肯定确认
                                  //1为选择，请求，停止激活，删除否定确认
        };

        BYTE byValue;
    };
    FRQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//文件状态
struct SOF
{
    union
    {
        struct
        {
            BYTE byStatus   :5;    //0未用，1--15为配套标准保留（兼用范围）16--32为特殊用途保留（专用范围）
            BYTE byRes1     :1;    //为配套标准保留（兼用范围）
            BYTE byFor      :1;    //0定义是文件名，1定义的是子目录名
            BYTE byFa       :1; //0文件等待传输，1此文件已经激活
        };
        BYTE byValue;
    };
    SOF& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }
};

union LENGTH{
    DWORD dwLenth;
    char cCh[4];
};

class DIRECTORY_FILE_OBJ
{
public:
    DIRECTORY_FILE_OBJ()
    {
        pbyFileBuffer = NULL;
        pbySectionBuffer = NULL;
        wFileName = 0;
        bySectionName = 0;
        dwSectionLength = 0;
        dwFileLength = 0;
        dwCurrSectionCopyLenth = 0;
        dwCurFileCopyLength = 0;
    };
    ~DIRECTORY_FILE_OBJ()
    {
        FreeMem(FALSE);
    };
        //BOOL bIsSection
        // TRUE  表示清空节缓冲区
        // FALSE 表示清空文件缓冲区
    void FreeMem(bool bIsSection)
    {
        if(bIsSection)
        {
            if(NULL != pbySectionBuffer){delete []pbySectionBuffer;pbySectionBuffer = NULL;}return;}
            if(NULL != pbyFileBuffer){delete []pbyFileBuffer;    pbyFileBuffer = NULL;}
            if(NULL != pbySectionBuffer){delete []pbySectionBuffer;pbySectionBuffer = NULL;}
        };
        bool AllocSectionBuffer(DWORD dwSectionLeng)
        {
            pbySectionBuffer = new BYTE[dwSectionLeng];
            if(NULL != pbySectionBuffer)
            return TRUE;
            return FALSE;
        };
        bool AllocFileBuffer(DWORD dwFileLeng)
        {
            pbyFileBuffer = new BYTE[dwFileLeng];
            if(NULL != pbyFileBuffer)
            return TRUE;
            return FALSE;
        };

    public:

        BYTE m_byServerCode;        //指明当前服务码

        QString IntNameToStringName(WORD wChannelNo,BYTE byApp);
        WORD             wAppAddr;                          //公共地址
        WORD             wFileName;                         //文件名称
        BYTE             bySectionName;                     //节名称
        DWORD            dwFileLength;                      //文件长度
        DWORD            dwSectionLength;                   //节长度
        DWORD            dwCurFileCopyLength;               //目前传输文件长度

        DWORD            dwCurrSectionCopyLenth;            //目前传输节长度
        BYTE*            pbyFileBuffer;                     //存放文件缓冲区
        BYTE*            pbySectionBuffer;                  //存放节缓冲区
        SOF              bySof;                             //文件状态
        CP56Time2a       byTime;                            //日历时钟
        BYTE             byTranStatus;                      //当前该文件的传输状态
};

struct __POSITION { };
typedef __POSITION* POSITION;

#endif // C104STRUCT_H
