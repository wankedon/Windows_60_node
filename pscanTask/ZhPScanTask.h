#pragma once
#include "NodeTask.h"
#include "ZhSensor.h"
#include "ZhPScanPersistence.h"

class ZhPScanProcessor;
class ZhPScanStatusMonitor;
class PScanRecorder;
class ZhPScanSpectrumAverage;
class ZhPScanTask : public NodeTask
{
public:
	ZhPScanTask(const TaskInitEntry& entry, std::shared_ptr<persistence::PScanDBAccessor> dbAccessor);
	virtual ~ZhPScanTask();
public:
	void config(TaskRequestContext& context) override;
	ErrorType start() override;
	void onCmd(TaskRequestContext& context) override;
	void stop() override;
private:
	ErrorType realtimeSetup();
	void onAcq(const senSegmentData&, const std::vector<float>&);
	void realtimeStop();
	void onAcqThreadEntrance();
	void onAcqThreadQuit();
	void onFirstSeg(const senSegmentData& header);
	void onLastSeg(const senSegmentData& header);
private:
	std::unique_ptr<ZBDevice::SpectrumSweeper> sweeper;	//数据采集
	std::unique_ptr<ZhPScanStatusMonitor> statusMonitor;	//任务状态监控
	std::unique_ptr<ZhPScanProcessor> processor;			//扫描数据的处理单元
	zczh::zhpscan::TaskParam taskParam;
	std::list<MessageExtractor> pendingCmd;		//待执行的命令
	std::shared_ptr<persistence::PScanDBAccessor> dbAccessor;
	std::unique_ptr<ZhPScanSpectrumAverage> m_average;
	std::mutex mtx;
};