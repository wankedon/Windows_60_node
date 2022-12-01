#pragma once
#include "NodeTask.h"
#include "ZhSensor.h"
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "ZhIFPersistence.h"

class ZhIFSubScanTask;
class ZhIFSubIQTask;
class ZhIFTask : public NodeTask
{
public:
	ZhIFTask(const TaskInitEntry& entry, std::shared_ptr<persistence::IFDBAccessor> dbAccessor);
	virtual ~ZhIFTask();
public:
	void config(TaskRequestContext& context) override;
	ErrorType start() override;
	void onCmd(TaskRequestContext& context) override;
	void stop() override;
private:
	ErrorType realtimeScanStart();
	ErrorType realtimeIqStart();
private:
	zczh::zhIFAnalysis::TaskParam taskParam;
	std::unique_ptr<ZhIFSubScanTask> m_scanTask;
	std::unique_ptr<ZhIFSubIQTask> m_iqTask;
	std::shared_ptr<persistence::IFDBAccessor> dbAccessor;

};