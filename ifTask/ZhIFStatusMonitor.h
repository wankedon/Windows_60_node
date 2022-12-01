#pragma once
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "PeriodTimer.h"

class IFStatusMonitor
{
public:
	IFStatusMonitor(int32_t resultInterval, int32_t statusInterval);
	~IFStatusMonitor();
public:
	void onSweepEnd(const Timestamp& ts, const Position& pos);
	bool needResultReport() const;
	bool needStatusReport() const;
	zczh::zhIFAnalysis::OperationStatus getStatus() const { return status; }
	zczh::zhIFAnalysis::Header getHeader() const { return header; }
	void restartStatusTimer();
	void setupForNextResult();
public:
	void restartResultTimer();
private:
	const std::chrono::milliseconds resultInterval;
	const std::chrono::milliseconds statusInterval;
	zczh::zhIFAnalysis::Header header;		//结果头信息
	zczh::zhIFAnalysis::OperationStatus status;
	PeriodTimer resultTimer;	//回传结果计时
	PeriodTimer statusTimer;	//回传状态计时
};

