#pragma once

#include "node/zczh/ZhPScan.pb.h"
#include "PeriodTimer.h"

class ZhPScanStatusMonitor
{
public:
	ZhPScanStatusMonitor(int32_t resultInterval, int32_t statusInterval);
	~ZhPScanStatusMonitor();
public:
	void onSweepBegin(const Timestamp& ts);
	void onSweepEnd(const Timestamp& ts, const Position& pos);
	bool needResultReport() const;
	bool needStatusReport() const;
	zczh::zhpscan::OperationStatus getStatus() const { return status; }
	spectrum::Header getHeader() const { return header; }
	void restartStatusTimer();
	void setupForNextResult();
public:
	void restartResultTimer();
private:
	const std::chrono::milliseconds resultInterval;
	const std::chrono::milliseconds statusInterval;
	spectrum::Header header;		//���ͷ��Ϣ
	zczh::zhpscan::OperationStatus status;
	PeriodTimer resultTimer;	//�ش������ʱ
	PeriodTimer statusTimer;	//�ش�״̬��ʱ
};

