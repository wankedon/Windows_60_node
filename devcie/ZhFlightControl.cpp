#include "pch.h"
#include "ThreadLoop.h"
#include "ZhFlightControl.h"
#include "ZhGeomagTools.h"
#include "Logger.h"
#include <time.h>
#ifdef PLATFROM_WINDOWS
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

namespace ZBDevice
{
#ifdef PLATFROM_WINDOWS
    int readTimes=1;
    int openSerial() { return 1; }
    int readSerial(const int fd, char* buf, int bufSize)
    {
        FlightCtrlCode code;
        SYSTEMTIME wtm;
        GetLocalTime(&wtm);
        code.syncCodeFirst = 0xEE;
        code.syncCodeSecond = 0X90;
        code.year = wtm.wYear - 2000;
        code.month = wtm.wMonth;
        code.day = wtm.wDay;
        code.hour = wtm.wHour;
        code.minute = wtm.wMinute;
        code.msec = wtm.wSecond * 1000 + wtm.wMilliseconds;  
		code.longitude = 118.0530 / (180 / (pow(2, 31) - 1));// 118.1887 120.1691
		code.latitude = 32.7320 / (90 / (pow(2, 31) - 1));//32.7264 35.97
        code.altitude = 87.4 /0.25;
        code.pitch = 1.26/0.01;
        code.roll = -3.03/0.01;
        code.yaw = 284.95/0.01;
        code.horSpeed = 350/0.1;
        code.verSpeed = 360/0.1;
        code.trackDir = 194.96 /0.01;
        code.GPSFlag = 1;
        code.GPSSec = wtm.wSecond;
        code.relaAltitude = 100/0.25;
        code.ewSpeed = 1000;
        code.snSpeed = 1100;
        code.baroAltitude = 25;
        code.xorCheck = 55;
        code.sumCheck = 66;
#ifdef SEG_SEND
		std::vector<char> codebuffer(sizeof(FlightCtrlCode), 0);
		memcpy(codebuffer.data(), &code, sizeof(FlightCtrlCode));
		int offset = (readTimes - 1) * 8;
		readTimes++;
		if (offset + 8 < sizeof(FlightCtrlCode))
		{
			memcpy(buf, codebuffer.data() + offset, 8);
			return 8;
		}
        else
		{
			readTimes = 1;
			memcpy(buf, codebuffer.data() + offset, offset + 8 - sizeof(FlightCtrlCode));
			return offset + 8 - sizeof(FlightCtrlCode);
		}
#else
        memcpy(buf, &code, sizeof(FlightCtrlCode));
        return sizeof(FlightCtrlCode);
#endif	
    }
	int closeSerialport(const int fd) { return 1; }
#else
	int openSerial()
	{
		char* path = "/dev/ttyS1";
		int fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
		if (fd < 0) 
        { 
            LOG("OpenSerial Failed");
            return -1; 
        }
        LOG("OpenSerial Succeed");
		struct termios opt;
		tcflush(fd, TCIOFLUSH);    //清空串口接收缓冲区	
		tcgetattr(fd, &opt);       //获取串口参数opt	
		cfsetospeed(&opt, B115200);//设置串口输出波特率
		cfsetispeed(&opt, B115200);//设置串口输入波特率
		//设置数据位数
		opt.c_cflag &= ~CSIZE;
		opt.c_cflag |= CS8;
		//校验位
		opt.c_cflag &= ~PARENB;
		opt.c_iflag &= ~INPCK;
		opt.c_cflag &= ~CSTOPB;      //设置停止位
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE); //增加配置，串口助手不用强制加换行符
        //解决0x03 0x13不显示问题
        opt.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        opt.c_oflag &= ~OPOST;
        opt.c_cflag |= CLOCAL | CREAD;
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		tcsetattr(fd, TCSANOW, &opt);//更新配置
		return fd;
	}
    int readSerial(const int fd, char* buf, int bufSize)
    {
        return read(fd, buf, bufSize);
    }
    int closeSerialport(const int fd)
    {
        return close(fd);
    }
#endif	

    ZhFlightControl::ZhFlightControl()
        :m_fd(-1),
        m_flightCtrlInfo(FlightCtrlInfo{}),
        count(0)
    {
        m_flightCtrlInfo.pos.longitude = 120.1691;
        m_flightCtrlInfo.pos.latitude = 35.9700;
        m_flightCtrlInfo.pos.altitude = 87.4;
        openSerialPort();
    }

	ZhFlightControl::~ZhFlightControl()
	{
		if (m_fd >= 0)
			closeSerialport(m_fd);
		if (m_thread)
			m_thread->stop();
	}

    void ZhFlightControl::openSerialPort()
    {
		if (m_fd < 0)
		{
            m_fd = openSerial();
		}
		if (m_fd >= 0 && m_thread == nullptr)
		{
			m_thread = std::make_unique<ThreadLoop>([this]() {getInfoLoop(); });
			LOG("create getFlightControlInfo Thread");
		}
    }

    //获取飞控信息
	void ZhFlightControl::getInfoLoop()
	{
		LOG("Start GetFlightControlInfo");
        std::vector<char> totalBuffer;
        FlightCtrlCode flightCtrlCode;
		while (m_thread && m_thread->isRuning())
		{
            std::vector<char> curBuffer(1024);
            if (m_fd > 0)
            {
                auto readSize = readSerial(m_fd, curBuffer.data(), curBuffer.size());
                if (readSize > 0)
                {
                   // LOG("curBuffer Size: {}", readSize);
                    //for (int i = 0; i < readSize; i++)
                    //{
                    //    LOG("Read data {:X}", curBuffer[i]);
                    //}
                    totalBuffer.insert(totalBuffer.end(), curBuffer.begin(), curBuffer.begin() + readSize);
                    //LOG("totalBuffer Size: {}", totalBuffer.size());
                    if (extractFlightCtrlCode(flightCtrlCode, totalBuffer))
                    {
                        //LOG("extractFlightCtrlCode succeed");
                        parserFlightCtrlCode(flightCtrlCode);
                    }
                }
            }
#ifdef PLATFROM_WINDOWS
            Sleep(1000);
#else
            sleep(1);
#endif	
		}
		LOG("Stop GetFlightControlInfo");
	}

    bool ZhFlightControl::extractFlightCtrlCode(FlightCtrlCode& flightCtrlCode,std::vector<char>& buffer)
    {
        auto headerIter = std::find_if(buffer.begin(), buffer.end(), std::bind2nd(std::equal_to<char>(), 0xee));
        if (headerIter == buffer.end())
            return false;
        buffer.erase(buffer.begin(), headerIter);
        if (buffer.size() < sizeof(FlightCtrlCode))
            return false;
        memcpy(&flightCtrlCode, buffer.data(), sizeof(FlightCtrlCode));
        //for (int i = 0; i < sizeof(FlightCtrlCode); i++)
        //{
        //    LOG("code {} 0x{:X}",i,buffer[i]);
        //}
        buffer.erase(buffer.begin(), buffer.begin() + sizeof(FlightCtrlCode));
        return true;
    }

    void ZhFlightControl::parserFlightCtrlCode(const FlightCtrlCode& flightCtrlCode)
    {
        SYSTEMTIME time;
        time.wYear = flightCtrlCode.year + 2000;
        time.wMonth = flightCtrlCode.month;
        time.wDay = flightCtrlCode.day;
        time.wHour = flightCtrlCode.hour;
        time.wMinute = flightCtrlCode.minute;
        time.wSecond = flightCtrlCode.msec / 1000;
        time.wMilliseconds = flightCtrlCode.msec % 1000;
        FlightPosition pos;
        pos.longitude = (double)flightCtrlCode.longitude * (180 / ((pow(2, 31) - 1)));
        pos.latitude = (double)flightCtrlCode.latitude * (90 / ((pow(2, 31) - 1)));
        pos.altitude = flightCtrlCode.altitude * 0.25;
        FlightCtrlInfo flightCtrlInfo;
        flightCtrlInfo.systime = time;
        flightCtrlInfo.pos = pos;
        flightCtrlInfo.pitch = flightCtrlCode.pitch * 0.01;
        flightCtrlInfo.roll = flightCtrlCode.roll * 0.01;
        flightCtrlInfo.yaw = flightCtrlCode.yaw * 0.01;
        flightCtrlInfo.horSpeed = flightCtrlCode.horSpeed * 0.1;
        flightCtrlInfo.verSpeed = flightCtrlCode.verSpeed * 0.1;
        flightCtrlInfo.trackDir = flightCtrlCode.trackDir * 0.01;
        flightCtrlInfo.GPSFlag = flightCtrlCode.GPSFlag;
        flightCtrlInfo.GPSSec = flightCtrlCode.GPSSec;
        flightCtrlInfo.relaAltitude = flightCtrlCode.relaAltitude * 0.25;
        flightCtrlInfo.ewSpeed = flightCtrlCode.ewSpeed;
        flightCtrlInfo.snSpeed = flightCtrlCode.snSpeed;
        flightCtrlInfo.baroAltitude = flightCtrlCode.baroAltitude;
        std::lock_guard<std::mutex> lg(lock);
        m_flightCtrlInfo = flightCtrlInfo;
#ifdef PLATFROM_WINDOWS
#else
        LOG("----FlightControlInfo----");
        LOG("CodeFirst {:X}", flightCtrlCode.syncCodeFirst);
        LOG("CodeSecond {:X}", flightCtrlCode.syncCodeSecond);
        LOG("Time {}:{}:{}:{}:{}:{}:{}", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
        LOG("Pos {}_{}_{}", pos.longitude, pos.latitude, pos.altitude);
        LOG("Pitch:{}_Roll:{}_Yaw:{}", m_flightCtrlInfo.pitch, m_flightCtrlInfo.roll, m_flightCtrlInfo.yaw);
        LOG("HorSpeed:{}_VerSpeed:{}_TrackDir:{}", m_flightCtrlInfo.horSpeed, m_flightCtrlInfo.verSpeed, m_flightCtrlInfo.trackDir);
        LOG("GPSFlag:{}_GPSSec:{}", m_flightCtrlInfo.GPSFlag, m_flightCtrlInfo.GPSSec);
        LOG("relaAltitude:{}", m_flightCtrlInfo.relaAltitude);
#endif
    }

    //获取时间timeval
    struct timeval ZhFlightControl::getTimeVal()
    {
		struct timeval tv;
		auto wtm = systemTime();
#ifdef PLATFROM_WINDOWS
#else     
		if (wtm.wYear == 0)
		{
			struct timezone tz;
			gettimeofday(&tv, &tz);
			return tv;
		}
#endif 
        struct tm tm;
        tm.tm_year = wtm.wYear - 1900;
        tm.tm_mon = wtm.wMonth - 1;
        tm.tm_mday = wtm.wDay;
        tm.tm_hour = wtm.wHour;
        tm.tm_min = wtm.wMinute;
        tm.tm_sec = wtm.wSecond;
        tm.tm_isdst = -1;
        tv.tv_sec = mktime(&tm);
        tv.tv_usec = wtm.wMilliseconds * 1000;
        return tv;
    }

    SYSTEMTIME ZhFlightControl::systemTime()
    {
        return m_flightCtrlInfo.systime;
    }

    //获取位置
    FlightPosition ZhFlightControl::getPosition()
    {
#ifdef MOVE_TARGET
        //定位测试
		if (count < 100)
		{
			m_flightCtrlInfo.pos.longitude = 120.1691;
		}
		else
		{
			m_flightCtrlInfo.pos.longitude = 120.1891;
		}
		m_flightCtrlInfo.pos.latitude = 35.9700;
		m_flightCtrlInfo.pos.altitude = 87.4;
		if (count == 200)
			count = 0;
        count++;
#endif 
        return m_flightCtrlInfo.pos;
    }

    //获取方向（解耦后)
    double ZhFlightControl::getDirection(const double azi, const double ele)
    {
		auto pos = getPosition();
		auto systime = m_flightCtrlInfo.systime;
        auto magDec = decCalculator(0.001*pos.altitude, pos.latitude, pos.longitude, systime.wYear, systime.wMonth, systime.wDay);
        std::vector<float> result(2);
        directionDecouple(result.data(), azi * ANGLE_TO_PI, ele * ANGLE_TO_PI, m_flightCtrlInfo.roll * ANGLE_TO_PI, m_flightCtrlInfo.pitch * ANGLE_TO_PI, (m_flightCtrlInfo.yaw + magDec) * ANGLE_TO_PI);
        return result[0];
    }

    /**
     * @brief 方向解耦
     * @param result 解耦后方向 数组类型
     * @param fAzi_origin 设备上报角度
     * @param fEle_origin 设备上报的俯仰角，默认0
     * @param fRoll 电子罗盘横滚角度
     * @param fPitch 电子罗盘俯仰角
     * @param fYaw 电子罗盘方向角+磁偏角
     */
     //directionDecouple(result, targetDirect* angle_to_pi, 0, deviceRoll* angle_to_pi, devicePitch* angle_to_pi, (deviceYaw + direction)* angle_to_pi);
    void ZhFlightControl::directionDecouple(float* result, float fAzi_origin, float fEle_origin, float fRoll, float fPitch, float fYaw)
    {
		float fCos_val[3] = { cos(fRoll),cos(-fPitch),cos(fYaw) };
		float fSin_val[3] = { sin(fRoll),sin(-fPitch),sin(fYaw) };
		float fAzi_Ele_matrix[3] = { cos(fEle_origin) * cos(-fAzi_origin),cos(fEle_origin) * sin(-fAzi_origin),sin(fEle_origin) };
		float fTxyz_Matrix[3][3] = { 0 };
		float fBoll_Matrix[3] = { 0 };
		float AZI_val;
        fTxyz_Matrix[0][0] = fCos_val[1] * fCos_val[2];//12
        fTxyz_Matrix[0][1] = fCos_val[0] * fSin_val[2] + fSin_val[0] * fSin_val[1] * fCos_val[2];//02 + 012
        fTxyz_Matrix[0][2] = fSin_val[0] * fSin_val[2] - fCos_val[0] * fSin_val[1] * fCos_val[2];//02 - 012
        fTxyz_Matrix[1][0] = -fCos_val[1] * fSin_val[2];//12
        fTxyz_Matrix[1][1] = fCos_val[0] * fCos_val[2] - fSin_val[0] * fSin_val[1] * fSin_val[2];//02 - 012
        fTxyz_Matrix[1][2] = fSin_val[0] * fCos_val[2] + fCos_val[0] * fSin_val[1] * fSin_val[2];//02+012
        fTxyz_Matrix[2][0] = fSin_val[1];//1
        fTxyz_Matrix[2][1] = -fSin_val[0] * fCos_val[1];//01
        fTxyz_Matrix[2][2] = fCos_val[0] * fCos_val[1];//01
        fBoll_Matrix[0] = fTxyz_Matrix[0][0] * fAzi_Ele_matrix[0] + fTxyz_Matrix[0][1] * fAzi_Ele_matrix[1] + fTxyz_Matrix[0][2] * fAzi_Ele_matrix[2];
        fBoll_Matrix[1] = fTxyz_Matrix[1][0] * fAzi_Ele_matrix[0] + fTxyz_Matrix[1][1] * fAzi_Ele_matrix[1] + fTxyz_Matrix[1][2] * fAzi_Ele_matrix[2];
        fBoll_Matrix[2] = fTxyz_Matrix[2][0] * fAzi_Ele_matrix[0] + fTxyz_Matrix[2][1] * fAzi_Ele_matrix[1] + fTxyz_Matrix[2][2] * fAzi_Ele_matrix[2];
        AZI_val = -atan2(fBoll_Matrix[1], fBoll_Matrix[0]) * PI_TO_ANGLE;
		result[0] = (AZI_val < 0) ? AZI_val + 360 : AZI_val;
		result[1] = asin(fBoll_Matrix[2]) * PI_TO_ANGLE;;
    }
}
