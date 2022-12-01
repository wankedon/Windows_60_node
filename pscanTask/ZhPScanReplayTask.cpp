#include "pch.h"
#include "ZhPScanReplayTask.h"
#include "ZhPScanReplayer.h"
#include "ZhPScanDef.h"

using namespace std;

ZhPScanReplayTask::ZhPScanReplayTask(const TaskInitEntry& entry, std::shared_ptr<persistence::PScanDBAccessor> accessor)
	:NodeTask(entry),
	dbAccessor(accessor)
{
}


ZhPScanReplayTask::~ZhPScanReplayTask()
{
	stop();
}

void ZhPScanReplayTask::config(TaskRequestContext& context)
{
	
}

ErrorType ZhPScanReplayTask::start()
{
	return ERR_NONE;
}

void ZhPScanReplayTask::onCmd(TaskRequestContext& context)
{
	ErrorType err = ERR_NONE;
	PScanCmdType type;
	auto& extractor = context.messageExtractor();
	if (extractor.extract(type, false))	//不删除消息帧
	{
		switch (type)
		{
		case PScanCmdType::DATAHOLD_OPEN:
		case PScanCmdType::DATAHOLD_RESET:
		case PScanCmdType::DATAHOLD_CLOSE:
		case PScanCmdType::ZOOMIN_OPEN:
		case PScanCmdType::ZOOMIN_CLOSE:
		case PScanCmdType::DETECT_OPEN:
		case PScanCmdType::DETECT_CLOSE:
		case PScanCmdType::DETAIL_KEEPING:
		{
			lock_guard<mutex> lg(mtx);
			pendingCmd.emplace_back(std::move(extractor));	//MessageExtractor只支持右值拷贝构造
			break;
		}
		case PScanCmdType::REPLAY_ADJUST:
			err = onReplayAdjustCmd(extractor);
			break;
		case PScanCmdType::REPLAY_START:
			err = onReplayStartCmd(extractor);
			break;
		case PScanCmdType::REPLAY_STOP:
			err = onReplayStopCmd(extractor);
			break;
		case PScanCmdType::TASK_QUERY:
			err = onTaskQueryCmd(extractor);
			break;
		default:
			break;
		}
	}
	context.setErrorCode(err);
}

ErrorType ZhPScanReplayTask::onReplayAdjustCmd(MessageExtractor& extractor)
{
	PScanCmdType type;
	extractor.extract(type);
	assert(type == PScanCmdType::REPLAY_ADJUST);
	replay::AdjustRequest request;
	if (extractor.deserialize(request))
	{
		if (dataLoader != nullptr)
		{
			dataLoader->onAdjust(request.option());
			return ERR_NONE;
		}
	}
	return ERR_PARAM;
}

ErrorType ZhPScanReplayTask::onReplayStartCmd(MessageExtractor& extractor)
{
	PScanCmdType type;
	extractor.extract(type);
	assert(type == PScanCmdType::REPLAY_START);
	if (dataLoader != nullptr || replayer != nullptr)
		return ERR_BUSY;
	zczh::zhpscan::ReplayStartRequest request;
	if (!extractor.deserialize(request))
		return ERR_PARAM;
	auto taskInfo = dbAccessor->getTaskItem(request.replay_id().record_id());
	if (taskInfo == nullptr)
		return ERR_PARAM;
	auto items = dbAccessor->getTaskAllDataItem(request.replay_id().record_id());
	if (items.empty())
		return ERR_NO_DATA_AVAILABLE;
	dataLoader = make_unique<ZhPScanDataLoader>(request.result_interval(), *taskInfo, std::move(items));
	replayer = make_unique<ZhPScanReplayer>(*taskInfo, request.expected_points());
	auto dataHandler = [this](const persistence::PScanDataItem& dataItem) {onDataItem(dataItem); };
	return dataLoader->start(dataHandler) ? ERR_NONE : ERR_ABORTED;
}

void ZhPScanReplayTask::onDataItem(const persistence::PScanDataItem& dataItem)
{
	if (!pendingCmd.empty())
	{
		std::list<MessageExtractor> cmdCopy;
		{
			std::lock_guard<mutex> lg(mtx);
			cmdCopy.swap(pendingCmd);
		}
		replayer->onCmd(cmdCopy);
	}
	MessageBuilder mb;
	mb.add(PScanSteamType::SPECTRUM_RESULT).serializeToTail(replayer->onDataItem(dataItem));
	sendTaskData(mb);
}

ErrorType ZhPScanReplayTask::onReplayStopCmd(MessageExtractor& extractor)
{
	PScanCmdType type;
	extractor.extract(type);
	assert(type == PScanCmdType::REPLAY_STOP);
	if (dataLoader == nullptr || replayer == nullptr)
		return ERR_PARAM;
	stop();
	return ERR_NONE;
}

ErrorType ZhPScanReplayTask::onTaskQueryCmd(MessageExtractor& extractor)
{
	PScanCmdType type;
	extractor.extract(type);
	assert(type == PScanCmdType::TASK_QUERY);
	zczh::zhpscan::QueryRecordRequest request;
	if (!extractor.deserialize(request))
		return ERR_PARAM;
	const spectrum::FrequencySpan* freqSpan = nullptr;
	const TimeSpan* timeSpan = nullptr;
	if (request.has_freq_span())
	{
		freqSpan = &request.freq_span();
	}
	if (request.has_time_span())
	{
		timeSpan = &request.time_span();
	}
	if (timeSpan == nullptr && freqSpan == nullptr)
		return ERR_PARAM;
	auto taskRecords = dbAccessor->findTask(freqSpan, timeSpan);
	if (taskRecords.empty())
		return ERR_NO_DATA_AVAILABLE;
	zczh::zhpscan::QueryRecordReply reply;
	for (auto& record : taskRecords)
	{
		auto* newItem = reply.mutable_matching_records()->Add();
		dbAccessor->toTaskDescriptor(record, *newItem);
	}
	MessageBuilder mb;
	mb.add(PScanSteamType::RECORD_DESCRIPTOR).serializeToTail(reply);
	sendTaskData(mb);
	return ERR_NONE;
}

void ZhPScanReplayTask::stop()
{
	dataLoader.reset();
	replayer.reset();
}