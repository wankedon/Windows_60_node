#pragma once
#include "NodeTask.h"
#include "ZhSensor.h"
#include "node/zczh/ZhDirection.pb.h"
//#include "ZhDFPersistence.h"

class DFTargetProcessor;
class DFSpectrumProcessor;
class DFStatusMonitor;

class ZhDFTask : public NodeTask
{
public:
	ZhDFTask(const TaskInitEntry& entry/*, std::shared_ptr<persistence::DFDBAccessor> dbAccessor*/);
	virtual ~ZhDFTask();
public:
	void config(TaskRequestContext& context) override;
	ErrorType start() override;
	void onCmd(TaskRequestContext& context) override;
	void stop() override;
private:
	ErrorType realtimeSetup();
	void onDFResult(const senSegmentData& header, const std::vector<senDFTarget>& target, const std::vector<senVecFloat>& dfSpectrum);
	void realtimeStop();
	void onDFThreadEntrance();
	void onDFThreadQuit();
	void processTaskResult();
	void processTaskStatus();
private:
	std::unique_ptr<ZBDevice::DirectionFinder> dFinder;
	std::unique_ptr<DFTargetProcessor> targetProcessor;
	std::unique_ptr<DFSpectrumProcessor> spectrumProcessor;
	std::unique_ptr<DFStatusMonitor> statusMonitor;
	//std::shared_ptr<persistence::DFDBAccessor> dbAccessor;
	zczh::zhdirection::TaskParam taskParam;
};

