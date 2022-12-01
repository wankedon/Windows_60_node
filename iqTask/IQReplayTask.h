#pragma once
#include "NodeTask.h"
#include "node/zczh/ZhIQAcquire.pb.h"
#include "IQAcquirePersistence.h"
#include "ReplayController.h"

class IQReplayTask : public NodeTask
{
public:
	IQReplayTask(const TaskInitEntry& entry, std::shared_ptr<persistence::IQAcquireDBAccessor> dbAccessor);
	virtual ~IQReplayTask();
public:
	void config(TaskRequestContext& context) override;
	ErrorType start() override;
	void onCmd(TaskRequestContext& context) override;
	void stop() override;
private:
	ErrorType onReplayAdjustCmd(MessageExtractor& extractor);
	ErrorType onReplayStartCmd(MessageExtractor& extractor);
	ErrorType onReplayStopCmd(MessageExtractor& extractor);
	ErrorType onTaskQueryCmd(MessageExtractor& extractor);
	void onDataItem(int dataSeq);
	zczh::zhIQAcquire::Result extractFromDataItem(const persistence::IQAcquireDataItem& item);
private:
	std::unique_ptr<ReplayController<int>> controller;		//回放控制器，负责数据推送
	std::shared_ptr<persistence::IQAcquireDBAccessor> dbAccessor;
	std::mutex mtx;
};