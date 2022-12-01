#pragma once
#include "node/zczh/ZhDirection.pb.h"
#include "PeriodTimer.h"

class DFStatusMonitor
{
public:
	DFStatusMonitor(int32_t resultInterval, int32_t statusInterval);
	~DFStatusMonitor();
public:
	void onSweepEnd(const Timestamp& ts, const Position& pos);
	bool needResultReport() const;
	bool needStatusReport() const;
	zczh::zhdirection::OperationStatus getStatus() const { return status; }
	zczh::zhdirection::Header getHeader() const { return header; }
	void restartStatusTimer();
	void setupForNextResult();
public:
	void restartResultTimer();
private:
	const std::chrono::milliseconds resultInterval;
	const std::chrono::milliseconds statusInterval;
	zczh::zhdirection::Header header;		//结果头信息
	zczh::zhdirection::OperationStatus status;
	PeriodTimer resultTimer;	//回传结果计时
	PeriodTimer statusTimer;	//回传状态计时
};

