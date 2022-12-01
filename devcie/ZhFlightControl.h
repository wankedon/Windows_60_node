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
		double longitude;     //经度
		double latitude;      //纬度
		double altitude;      //海拔高度
	};

	struct FlightCtrlCode
	{
		uint8_t syncCodeFirst;    //同步码  
		uint8_t syncCodeSecond;   //同步码  
		uint8_t year;             //年 2000年为0
		uint8_t month;            //月
		uint8_t day;              //日
		uint8_t hour;             //时
		uint8_t minute;           //分
		uint16_t msec;            //毫秒
		int longitude;            //经度
		int latitude;             //纬度
		int16_t altitude;         //海拔高度
		int16_t pitch;            //俯仰角
		int16_t roll;		      //滚转角
		uint16_t yaw;		      //偏航角
		uint16_t horSpeed;        //水平速度
		int16_t verSpeed;          //向上速度
		uint16_t trackDir;         //航迹方向
		uint8_t GPSFlag;           //GPS有效标志
		unsigned int GPSSec;       //GPS周内秒
		int16_t relaAltitude;       //相对起飞点高度
		int16_t ewSpeed;            //东西速度
		int16_t snSpeed;            //南北速度
		int16_t baroAltitude;       //气压高度
		uint8_t reserved[54];       //备用
		uint8_t  xorCheck;          //异或校验
		uint8_t  sumCheck;          //和校验
	};

	struct FlightCtrlInfo
	{
		SYSTEMTIME systime;
		FlightPosition pos;
		double pitch;         //俯仰角
		double roll;		  //滚转角
		double yaw;		      //偏航角
		double horSpeed;      //水平速度
		double verSpeed;      //向上速度
		double trackDir;      //航迹方向
		double GPSFlag;       //GPS有效标志
		double GPSSec;        //GPS周内秒
		double relaAltitude;  //相对起飞点高度
		double ewSpeed;       //东西速度
		double snSpeed;       //南北速度
		double baroAltitude;  //气压高度
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