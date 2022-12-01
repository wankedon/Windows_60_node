#pragma once
#include "NodeTask.h"
#include "ZhSensor.h"
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "ZhIFPersistence.h"
#include "ZhRecognition.h"

class IFSpectrumProcessor;
class IFStatusMonitor;
class ZhRecognition;
class ZhIFSubScanTask
{
public:
	ZhIFSubScanTask(std::shared_ptr<ZBDevice::ZhSensor> sensor,std::shared_ptr<persistence::IFDBAccessor> accessor);
	virtual ~ZhIFSubScanTask();
public:
	bool config(zczh::zhIFAnalysis::TaskParam,const NodeDevice& nd);
	ErrorType onCmd(MessageExtractor& extractor);
	ErrorType start(std::function<void(MessageBuilder& mb)> sendTaskData);
	void stop();
	zczh::zhIFAnalysis::Result lastResult() { return zczh::zhIFAnalysis::Result{}; }
	void setRecogniseType(ZhRecognitionType type);

private:
	ErrorType realtimeSetup();
	void onIFSpectrumResult(const senSegmentData& header, const senVecFloat& ifSpectrum);
	void onIFIQResult(const senIQSegmentData& header, const std::vector<float>& iqdata);
	void realtimeStop();
	void onIFThreadEntrance();
	void onIFThreadQuit();
	void processTaskResult();
	void processTaskStatus();
private:
	std::unique_ptr<ZBDevice::IfAnalyst> ifAna;	
	std::unique_ptr<IFSpectrumProcessor> spectrumProcessor;
	std::unique_ptr<ZhRecognition> m_recognise;
	std::unique_ptr<IFStatusMonitor> statusMonitor;
	std::shared_ptr<persistence::IFDBAccessor> dbAccessor;
	zczh::zhIFAnalysis::TaskParam m_taskParam;
	std::shared_ptr<ZBDevice::ZhSensor> m_sensor;
	NodeDevice m_taskRunner;
	std::function<void(MessageBuilder& mb)> m_sendTaskData;
};

