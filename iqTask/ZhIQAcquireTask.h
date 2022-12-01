#pragma once
#include "NodeTask.h"
#include "ZhSensor.h"
#include "node/zczh/ZhIQAcquire.pb.h"
#include "IQAcquirePersistence.h"
#include "Miscellaneous.h"

class IQAcquireRecorder;
class ZhIQAcquireTask : public NodeTask
{
public:
	ZhIQAcquireTask(const TaskInitEntry& entry, std::shared_ptr<persistence::IQAcquireDBAccessor> accessor);
	virtual ~ZhIQAcquireTask();
public:
	void config(TaskRequestContext& context) override;
	ErrorType start() override;
	void onCmd(TaskRequestContext& context) override;
	void stop() override;
private:
	ErrorType realtimeSetup();
	void onAcq(const senIQSegmentData& header, const std::vector<float>& iq);
	void writeTaskInfo();
	void updateTaskInfo();
private:
	zczh::zhIQAcquire::TaskParam taskParam;
	uint64_t totalBlockCount;
	std::unique_ptr<ZBDevice::IQSweeper> iqSweeper;
	std::unique_ptr<IQAcquireRecorder> recorder;
	Timestamp lastTS;
};