#include "pch.h"
#include "IQRecorder.h"
#include "Logger.h"

using namespace std;

IQDataAsyncWriter::IQDataAsyncWriter(std::shared_ptr<persistence::IQAcquireDBAccessor> dbAccessor)
	:accessor(dbAccessor),
	writeItemCounter(0)
{

}

IQDataAsyncWriter::~IQDataAsyncWriter()
{
}

bool IQDataAsyncWriter::isReady()
{
	if (asyncWriteProc.valid())	//上次的
	{
		auto status = asyncWriteProc.wait_for(chrono::seconds(0));
		if (status == std::future_status::ready)
		{
			asyncWriteProc.get();
		}
		else
		{
			return false;	//上次写入还没有结束，则取消本次写入
		}
	}
	return true;	//同步写入
}

bool IQDataAsyncWriter::writeItem(const string& taskid, const zczh::zhIQAcquire::Result& result)
{
	int itemId = accessor->insertData(makeDataItem(taskid, result));
	if (itemId >= 0)
	{
		writeItemCounter++;
		CLOG("{} iqdata item have been writen", writeItemCounter);
		return true;
	}
	else
	{
		return false;
	}
}

int IQDataAsyncWriter::recordCount() const
{
	return writeItemCounter;
}

persistence::IQAcquireDataItem IQDataAsyncWriter::makeDataItem(const string& taskId, const zczh::zhIQAcquire::Result& result)
{
	auto& header = result.header();
	persistence::IQAcquireDataItem item;
	item.dataSeq = -1;
	item.taskuuid = taskId;
	item.numSequence = header.sequence_number();
	item.timeStamp = timestampToUINT64(header.time_stamp());
	item.centerFreq = header.center_frequency();
	item.sampleRate = header.sample_rate();
	item.header.resize(result.header().ByteSizeLong());
	result.header().SerializeToArray(item.header.data(), item.header.size());
	item.iqData.resize(result.body().ByteSizeLong());
	result.body().SerializeToArray(item.iqData.data(), item.iqData.size());
	return item;
}

IQAcquireRecorder::IQAcquireRecorder(std::shared_ptr<persistence::IQAcquireDBAccessor> pda)
	:accessor(pda),
	dataWriter(pda),
	enableFlag(false)
{

}

IQAcquireRecorder::~IQAcquireRecorder()
{
}

void IQAcquireRecorder::enable()
{
	enableFlag = true;
}

void IQAcquireRecorder::disable()
{
	enableFlag = false;
}

bool IQAcquireRecorder::isEnabled() const
{
	return enableFlag;
}

void IQAcquireRecorder::writeTaskInfo(const persistence::IQAcquireTaskItem& taskItem)
{
	if (taskInfo != nullptr)
		return;
	taskInfo = make_unique<persistence::IQAcquireTaskItem>(taskItem);
	taskInfo->taskSeq = -1;
	taskInfo->uuid = genUUID();
	taskInfo->taskSeq = accessor->insertTask(*taskInfo);
}

void IQAcquireRecorder::onResultReport(const zczh::zhIQAcquire::Result& result)
{
	if (!isEnabled())
		return;
	if (dataWriter.isReady())
	{
		dataWriter.writeItem(taskInfo->uuid, result); 
	}
}

void IQAcquireRecorder::rewriteTaskInfo(const Timestamp& ts)
{
	if (taskInfo == nullptr || taskInfo->taskSeq < 0)
		return;
	taskInfo->stopTime = timestampToUINT64(ts);
	taskInfo->recordCount = recordCount();
	accessor->updateTask(*taskInfo);
}