#pragma once
#include "NodeTask.h"
#include "node/zczh/ZhPScan.pb.h"
#include "ZhPScanPersistence.h"

class ZhPScanReplayer;
class ZhPScanDataLoader;
class ZhPScanReplayTask : public NodeTask
{
public:
	
public:
	ZhPScanReplayTask(const TaskInitEntry& entry, std::shared_ptr<persistence::PScanDBAccessor> dbAccessor);
	virtual ~ZhPScanReplayTask();
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
	void onDataItem(const persistence::PScanDataItem& dataItem);
private:
	std::unique_ptr<ZhPScanDataLoader> dataLoader;		//���ݼ�������������������
	std::unique_ptr<ZhPScanReplayer> replayer;			//�ط����ݵĴ���Ԫ
	std::shared_ptr<persistence::PScanDBAccessor> dbAccessor;
	std::list<MessageExtractor> pendingCmd;		//��ִ�е�����
	std::mutex mtx;
};
