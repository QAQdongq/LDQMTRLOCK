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
    WORD wMilSec;            //��������
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
} ;    //ʱ��ͬ����ϢԪ��

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
            BYTE byNum    :7; //��Ϣ���Ԫ�ص���Ŀ,���127��
            BYTE bySQ    :1; //��ʾ��Ϣ����˳��Ļ��Ƿ�˳���
        };
        BYTE byValue;
    };
    VSQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }
} ;    //�ɱ�ṹ�޶���

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
};    //ң�ء�����������

#pragma pack(push,8)//
struct DEVICE_STRUCT {
    BYTE            byNo;                    //���
    char            szLineName[40];            //��·����
    WORD            wDeviceAdress;            //װ�õ�ַ
    char            szDeviceName[40];        //װ������
    char            szDeviceDesc[60];        //װ��˵��
    char            szChannelName[12][40];    //ͨ������
    char            szDeviceVersion[8];        //װ�ð汾��Ϣ
    WORD            wAcqNum;                //ÿ���ڲ�������//PT���
    DWORD            dwPt;                    //PT���
    DWORD            dwCt;                    //CT���
    DWORD            dwRes;                    //�����ֶ�
    char            szRes;                    //�����ֶ�
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
};    //ң�ء�����������

struct SIQ
{
    BYTE bySPI        :1;        //�ֺ�λ        0����         1��
    BYTE byRES        :3;
    BYTE byBL        :1;        //����λ        0δ�⣬       1��
    BYTE bySB        :1;        //ȡ��λ        0δ��ȡ����   1��ȡ��
    BYTE byNT        :1;        //��ǰֵ        0��ǰֵ��     1�ǵ�ǰֵ
    BYTE byIV        :1;        //��Чλ        0��Ч��       1��Ч
};    //��Ʒ�������ĵ�����Ϣ

struct DIQ
{
    BYTE byDPI        :2;        //�ֺ�λ 0��3�м�״̬��ȷ����1ȷ��״̬����2ȷ��״̬��
    BYTE byRES        :2;
    BYTE byBL        :1;        //����λ        0δ�⣬       1��
    BYTE bySB        :1;        //ȡ��λ        0δ��ȡ����   1��ȡ��
    BYTE byNT        :1;        //��ǰֵ        0��ǰֵ��     1�ǵ�ǰֵ
    BYTE byIV        :1;        //��Чλ        0��Ч��       1��Ч
};    //��Ʒ��������˫����Ϣ

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

/*����ʱ�����ݽṹ,���������ʱ��ĵ����˫����Ϣ*/
struct CP56Time2a
{
    BYTE    byMilSecLow;                        //����ĵ�λ
    BYTE    byMilSecHigh;                        //����ĸ�λ,����ֵ��ΧΪ:0--59999
    BYTE    byMin        :6;                        //��,ռ���ֽڵ�ǰ6λ
    BYTE    byRes1        :1;                        //����1
    BYTE    byIV        :1;                        //��Чλ,0:=��Ч,1:=��Ч
    BYTE    byHour        :5;                        //Сʱ��:0--23
    BYTE    byRes2        :2;                        //����2
    BYTE    bySU        :1;                        //SU
    BYTE    byDayofMonth:5;                        //����,1--31
    BYTE    byDayofWeek    :3;                        //������,1--7
    BYTE    byMonth        :4;                        //�·�,1--12
    BYTE    byRes3        :4;                        //����3
    BYTE    byYear        :7;                        //���0--99
    BYTE    byRes4        :1;                        //����4
    WORD  ReadMilSec()                            //�������ṹ�еĺ�����
    {
        return (WORD)(byMilSecHigh*0x100+byMilSecLow);
    }
    void  WriteMilSec(WORD wMilSec)                //д���뵽���ṹ��Ӧ���ֶ�
    {
        byMilSecLow = (BYTE)wMilSec ;            //�ȵõ���λ�ֽ�ֵ
        byMilSecHigh = (BYTE)(wMilSec >> 8);    //�õ���λ�ֽ�ֵ
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
};    //��Ʒ��������ң����ϢԪ��

struct QCC
{
    BYTE byRQT    :6;    //RQT,0δ��6--31Ϊ���ױ�׼����
    BYTE byFRZ    :2;    //FRZ��0�����������1���᲻����λ��2�������λ��3��������λ
    operator BYTE()
    {
        return *((BYTE*)this);
    }
};//���ܼ������ٻ�������޶���QCC

#pragma pack(push, 1)  //���¸��ṹ�ڴ水1���ֽڷ��� ����#pragma pack(pop)

struct CTRLFIELD
{
    union
    {
        struct S_NS
        {
            WORD    wIsSF        :1;        //0��I����1��S����U��
            WORD    wNS            :15;    //I��ʱ��ʾ�������
        };
        struct S_UF
        {
            WORD    wIsSF        :1;        //0��I����1��S����U��
            WORD    wIsUF        :1;        //���wIsSFΪ1,��0: S��, 1: U��
            WORD    wStartAct    :1;        //U��ʱ: ������������
            WORD    wStartCon    :1;        //����ȷ��
            WORD    wStopAct    :1;        //ֹͣ��������
            WORD    wStopCon    :1;        //ֹͣȷ��
            WORD    wTestAct    :1;        //�������
            WORD    wTestCon    :1;        //����ȷ��
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
            BYTE byCause:6;    //����ԭ��0δ���壬1--63����1--43Ϊ���ױ�����
            BYTE byPN    :1;    //�϶������Ͽ�(0�϶���1���Ͽ�)
            BYTE byTest    :1;    //���ԡ�δ����(1���顢0δ����)
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

};    //����ԭ��

/*ASDU(Ӧ�����ݵ�Ԫ)����ͷ*/
struct ASDUHEAD
{
    BYTE    byTypeId;            //���ͱ�ʶ������120��127Ϊ�ļ���������ͱ�ʶ��
    VSQ        Vsq;                //�ɱ�ṹ�޶���
    COT        Cot;                //����ԭ��
    WORD    byAppAddr;            //Ӧ�õ�Ԫ������ַ???��104��ΪBYTE����

    BYTE    VsqVal()            //����N(��ϢԪ��/��Ϣ�������)��ֵ
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

/*104��Լ����֡��ʽ�ṹ*/

union FRAME104                    //��ȥbyStart��byLength���ʣ��APDU
{
    struct SFrame104Data
    {
        BYTE        byStart;      //START 68H
        BYTE        byLength;     //Length of APDU
        CTRLFIELD    CtrlField;    //APCI            �ĸ��ֽڵ�Ӧ�ù�Լ������Ϣ
        ASDUHEAD    AsduHead;     //ASDU HEAD       ASDU�̶���ͷ����
        BYTE        AsduData[2048];//ASDU           ASDU��Ϣ�岿��//AsduData[247] wyh 2007-8-23 �޸�Ϊ2K ���䱣����ֵ�����ȣ�68�ĵ�3λ
    };
    struct SFrame104ErrInf
    {
        BYTE byFrameType;
        BYTE byLength1;
        WORD wErrCode;
        char szErr[2048];    //szErr[253] wyh 2007-8-23 �޸�Ϊ 2K ���䱣����ֵ�����ȣ�68�ĵ�3λ
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

//ѡ����ٻ��޶���
struct SCQ
{
    union
    {
        struct
        {
            BYTE byCause       :4;//0δ�ã�
                                  //1ѡ���ļ�
                                  //2�����ļ�
                                  //3ֹͣ�����ļ�
                                  //4ɾ���ļ�
                                  //5ѡ���
                                  //6�����
                                  //7ֹͣ�����
                                  //8--10Ϊ���ױ�׼���������÷�Χ��
                                  //11--15Ϊ������;������ר�÷�Χ��
            BYTE byUI          :4;//0ȱ��
                                  //1�ޱ�����Ĵ洢�ռ�
                                  //2У��ʹ�
                                  //3��������ͨ�ŷ���
                                  //4���������ļ�����
                                  //5�������Ľ�����
                                  //6-10Ϊ���ױ�׼���������÷�Χ��
                                  //11--15Ϊ������;������ר�÷�Χ��
        };

        BYTE byValue;
    };
    SCQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//�ļ����ȷ��
struct AFQ
{
    union
    {
        struct
        {
            BYTE byCon         :4;//0δ�ã�
                                  //1�ļ��������ȷ�Ͽ�
                                  //2�ļ�����ķ��Ͽ�
                                  //3�ڴ�����ȷ�Ͽ�
                                  //4�ڴ���ķ��Ͽ�
                                  //5--10Ϊ���ױ�׼���������÷�Χ��
                                  //11--15Ϊ������;������ר�÷�Χ��
            BYTE byUI          :4;//0ȱʡ
                                  //1�ޱ�����Ĵ洢�ռ�
                                  //2У��ʹ�
                                  //3��������ͨ�ŷ���
                                  //4���������ļ�����
                                  //5�������Ľ�����
                                  //6-10Ϊ���ױ�׼���������÷�Χ��
                                  //11--15Ϊ������;������ר�÷�Χ��
        };

        BYTE byValue;
    };
    AFQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//�ļ�׼����
struct SRQ
{
    union
    {
        struct
        {
            BYTE byUI          :7;//0δ�ã�1--63Ϊ���ױ�׼���������÷�Χ��64--127Ϊ������;������ר�÷�Χ��
            BYTE byReady       :1;//<0>:=��׼����ȥװ��<1>:=�ڻ�û׼����ȥװ��
        };

        BYTE byValue;
    };
    SRQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//�ļ�׼����
struct FRQ
{
    union
    {
        struct
        {
            BYTE byUI          :7;//0δ�ã�1--63Ϊ���ױ�׼���������÷�Χ��64--127Ϊ������;������ר�÷�Χ��
            BYTE byCon         :1;//0Ϊѡ������ֹͣ���ɾ���϶�ȷ��
                                  //1Ϊѡ������ֹͣ���ɾ����ȷ��
        };

        BYTE byValue;
    };
    FRQ& operator = (BYTE v)
    {
        byValue = v;
        return *this;
    }

};
//�ļ�״̬
struct SOF
{
    union
    {
        struct
        {
            BYTE byStatus   :5;    //0δ�ã�1--15Ϊ���ױ�׼���������÷�Χ��16--32Ϊ������;������ר�÷�Χ��
            BYTE byRes1     :1;    //Ϊ���ױ�׼���������÷�Χ��
            BYTE byFor      :1;    //0�������ļ�����1���������Ŀ¼��
            BYTE byFa       :1; //0�ļ��ȴ����䣬1���ļ��Ѿ�����
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
        // TRUE  ��ʾ��սڻ�����
        // FALSE ��ʾ����ļ�������
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

        BYTE m_byServerCode;        //ָ����ǰ������

        QString IntNameToStringName(WORD wChannelNo,BYTE byApp);
        WORD             wAppAddr;                          //������ַ
        WORD             wFileName;                         //�ļ�����
        BYTE             bySectionName;                     //������
        DWORD            dwFileLength;                      //�ļ�����
        DWORD            dwSectionLength;                   //�ڳ���
        DWORD            dwCurFileCopyLength;               //Ŀǰ�����ļ�����

        DWORD            dwCurrSectionCopyLenth;            //Ŀǰ����ڳ���
        BYTE*            pbyFileBuffer;                     //����ļ�������
        BYTE*            pbySectionBuffer;                  //��Žڻ�����
        SOF              bySof;                             //�ļ�״̬
        CP56Time2a       byTime;                            //����ʱ��
        BYTE             byTranStatus;                      //��ǰ���ļ��Ĵ���״̬
};

struct __POSITION { };
typedef __POSITION* POSITION;

#endif // C104STRUCT_H
