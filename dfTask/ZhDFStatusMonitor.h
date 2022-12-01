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
	zczh::zhdirection::Header header;		//���ͷ��Ϣ
	zczh::zhdirection::OperationStatus status;
	PeriodTimer resultTimer;	//�ش������ʱ
	PeriodTimer statusTimer;	//�ش�״̬��ʱ
};

