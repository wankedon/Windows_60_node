#include "pch.h"
#include "IQReplayTask.h"
#include "IQAcquireDef.h"

using namespace std;

IQReplayTask::IQReplayTask(const TaskInitEntry& entry, std::shared_ptr<persistence::IQAcquireDBAccessor> accessor)
	:NodeTask(entry),
	dbAccessor(accessor)
{

}

IQReplayTask::~IQReplayTask()
{
	stop();
}

void IQReplayTask::config(TaskRequestContext& context)
{
	
}

ErrorType IQReplayTask::start()
{
	return ERR_NONE;
}

void IQReplayTask::onCmd(TaskRequestContext& context)
{
	IQAcquireCmdType cmdType;
	auto& extractor = context.messageExtractor();
	extractor.extract(cmdType);
	ErrorType err = ERR_NONE;
	switch (cmdType)
	{
	case IQAcquireCmdType::REPLAY_ADJUST:
		err = onReplayAdjustCmd(extractor);
		break;
	case IQAcquireCmdType::REPLAY_START:
		err = onReplayStartCmd(extractor);
		break;
	case IQAcquireCmdType::REPLAY_STOP:
		err = onReplayStopCmd(extractor);
		break;
	case IQAcquireCmdType::TASK_QUERY:
		err = onTaskQueryCmd(extractor);
		break;
	default:
		break;
	}
	context.setErrorCode(err);
}

void IQReplayTask::stop()
{
	controller.reset();
}

ErrorType IQReplayTask::onReplayAdjustCmd(MessageExtractor& extractor)
{
	replay::AdjustRequest request;
	if (extractor.deserialize(request))
	{
		if (controller != nullptr)
		{
			controller->onAdjust(request.option());
			return ERR_NONE;
		}
	}
	return ERR_PARAM;
}

ErrorType IQReplayTask::onReplayStartCmd(MessageExtractor& extractor)
{
	if (controller != nullptr)
		return ERR_BUSY;
	zczh::zhIQAcquire::ReplayStartRequest request;
	if (!extractor.deserialize(request))
		return ERR_PARAM;
	auto taskInfo = dbAccessor->getTaskItem(request.replay_id().record_id());
	if (taskInfo == nullptr)
		return ERR_PARAM;
	auto itemIds = dbAccessor->getTaskAllDataItemId(request.replay_id().record_id());
	if (itemIds.empty())
		return ERR_NO_DATA_AVAILABLE;
	controller = make_unique<ReplayController<int>>(request.result_interval(), std::move(itemIds));
	auto dataHandler = [this](int idx, int id) {onDataItem(id); };
	return controller->start(dataHandler) ? ERR_NONE : ERR_ABORTED;
}

ErrorType IQReplayTask::onReplayStopCmd(MessageExtractor& extractor)
{
	if (controller == nullptr)
		return ERR_PARAM;
	stop();
	return ERR_NONE;
}

ErrorType IQReplayTask::onTaskQueryCmd(MessageExtractor& extractor)
{
	zczh::zhIQAcquire::QueryRecordRequest request;
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
	auto taskRecords = dbAccessor->findTask(freqSpan, request.segment_count(), timeSpan);
	if (taskRecords.empty())
		return ERR_NO_DATA_AVAILABLE;
	zczh::zhIQAcquire::QueryRecordReply reply;
	for (auto& record : taskRecords)
	{
		auto* newItem = reply.mutable_matching_records()->Add();
		dbAccessor->toTaskDescriptor(record, *newItem);
	}
	MessageBuilder mb;
	mb.add(IQAcquireSteamType::RECORD_DESCRIPTOR).serializeToTail(reply);
	sendTaskData(mb);
	return ERR_NONE;
}

zczh::zhIQAcquire::Result IQReplayTask::extractFromDataItem(const persistence::IQAcquireDataItem& item)
{
	zczh::zhIQAcquire::Result result;
	bool success = result.mutable_header()->ParseFromArray(item.header.data(), item.header.size());
	success |= result.mutable_body()->ParseFromArray(item.iqData.data(), item.iqData.size());
	result.mutable_header()->set_record_id(item.taskuuid);
	return result;
}

void IQReplayTask::onDataItem(int dataSeq)
{
	auto dataItem = dbAccessor->getDataItem(dataSeq);
	if (dataItem)
	{
		//send result
		MessageBuilder builder;
		builder.add(IQAcquireSteamType::IQ_RESULT).serializeToTail(extractFromDataItem(*dataItem));
		sendTaskData(builder);
	}
	
}
