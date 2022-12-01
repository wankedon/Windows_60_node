#pragma once
#include "NodeTask.h"
#include "ZhSensor.h"
#include "node/zczh/ZhIQAcquire.pb.h"
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "Miscellaneous.h"
#include "ZhRecognition.h"

class ZhRecognition;
class ZhIFSubIQTask
{
public:
	ZhIFSubIQTask(std::shared_ptr<ZBDevice::ZhSensor> sensor);
	virtual ~ZhIFSubIQTask();
public:
	bool config(zczh::zhIFAnalysis::TaskParam taskParam);
	ErrorType start(std::function<void(MessageBuilder& mb)> sendTaskData);
	void stop();
	void setLastresult(zczh::zhIFAnalysis::Result result)
	{
		m_result = result;
	}
private:
	void setRecogniseType(zczh::zhIFAnalysis::TaskParam taskParam);
	ErrorType realtimeSetup();
	void onAcq(const senIQSegmentData& header, const std::vector<float>& iq);
	void processTaskResultLoop();
	void processTaskResult();
private:
	std::unique_ptr<ZBDevice::IQSweeper> iqSweeper;
	std::shared_ptr<ZBDevice::ZhSensor> m_sensor;
	std::unique_ptr<ZhRecognition> m_recognise;
	std::function<void(const RecognizeResult& rr)> m_sendRecognizeResult;
	std::function<void(MessageBuilder& mb)> m_sendTaskData;
	zczh::zhIFAnalysis::Result m_result;
	bool m_isRuning;
	ThreadWrap m_Thread;
};