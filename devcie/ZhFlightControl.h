#pragma once
#include "ThreadLoop.h"

#pragma pack(1)
#ifdef PLATFROM_WINDOWS
#else
struct SYSTEMTIME 
{
	int wYear;
	int wMonth;
	int wDay;
	int wHour;
	int wMinute;
	int wSecond;
	int wMilliseconds;
};
#endif 

namespace ZBDevice
{
	struct FlightPosition
	{
		double longitude;     //����
		double latitude;      //γ��
		double altitude;      //���θ߶�
	};

	struct FlightCtrlCode
	{
		uint8_t syncCodeFirst;    //ͬ����  
		uint8_t syncCodeSecond;   //ͬ����  
		uint8_t year;             //�� 2000��Ϊ0
		uint8_t month;            //��
		uint8_t day;              //��
		uint8_t hour;             //ʱ
		uint8_t minute;           //��
		uint16_t msec;            //����
		int longitude;            //����
		int latitude;             //γ��
		int16_t altitude;         //���θ߶�
		int16_t pitch;            //������
		int16_t roll;		      //��ת��
		uint16_t yaw;		      //ƫ����
		uint16_t horSpeed;        //ˮƽ�ٶ�
		int16_t verSpeed;          //�����ٶ�
		uint16_t trackDir;         //��������
		uint8_t GPSFlag;           //GPS��Ч��־
		unsigned int GPSSec;       //GPS������
		int16_t relaAltitude;       //�����ɵ�߶�
		int16_t ewSpeed;            //�����ٶ�
		int16_t snSpeed;            //�ϱ��ٶ�
		int16_t baroAltitude;       //��ѹ�߶�
		uint8_t reserved[54];       //����
		uint8_t  xorCheck;          //���У��
		uint8_t  sumCheck;          //��У��
	};

	struct FlightCtrlInfo
	{
		SYSTEMTIME systime;
		FlightPosition pos;
		double pitch;         //������
		double roll;		  //��ת��
		double yaw;		      //ƫ����
		double horSpeed;      //ˮƽ�ٶ�
		double verSpeed;      //�����ٶ�
		double trackDir;      //��������
		double GPSFlag;       //GPS��Ч��־
		double GPSSec;        //GPS������
		double relaAltitude;  //�����ɵ�߶�
		double ewSpeed;       //�����ٶ�
		double snSpeed;       //�ϱ��ٶ�
		double baroAltitude;  //��ѹ�߶�
	};

#pragma pack()

	class ThreadLoop;
	class ZhFlightControl
	{
	public:
		ZhFlightControl();
		~ZhFlightControl();
	public:
		void openSerialPort();
		struct timeval getTimeVal();
		FlightPosition getPosition();
		double getDirection(const double azi, const double ele);
		SYSTEMTIME systemTime();
	private:
		void getInfoLoop();
		bool extractFlightCtrlCode(FlightCtrlCode& flightCtrlCode, std::vector<char>& totalBuffer);
		void parserFlightCtrlCode(const FlightCtrlCode& flightCtrlCode);
		void directionDecouple(float* result, float fAzi_origin, float fEle_origin, float fRoll, float fPitch, float fYaw);
	private:
		int m_fd;
		mutable std::mutex lock;
		std::unique_ptr<ThreadLoop> m_thread;
		std::vector<char> m_buffer;
		FlightCtrlInfo m_flightCtrlInfo;
	private:
		const double PI_TO_ANGLE = 57.2957795;
		const double ANGLE_TO_PI = 0.0174533;
		int count;
	};
	int openSerial();
	int readSerial(const int fd, char* buf, int bufSize);
	int closeSerialport(const int fd);
}