#include "pch.h"
#include "ZhPScanProcessor.h"
#include "TraceDataHolder.h"
#include "Logger.h"
using namespace std;

PScanDataAsyncWriter::PScanDataAsyncWriter(std::shared_ptr<persistence::PScanDBAccessor> dbAccessor)
	:accessor(dbAccessor),
	writeItemCounter(0)
{

}

PScanDataAsyncWriter::~PScanDataAsyncWriter()
{
}

bool PScanDataAsyncWriter::isReady()
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
	return true;
}

bool PScanDataAsyncWriter::writeItem(const persistence::PScanDataItem& item)
{
	int itemId = accessor->insertData(item);
	if (itemId >= 0)
	{
		writeItemCounter++;
		CLOG("{} data item have been writen", writeItemCounter);
		return true;
	}
	else
	{
		return false;
	}
}

int PScanDataAsyncWriter::recordCount() const
{
	return writeItemCounter;
}

PScanRecorder::PScanRecorder(std::shared_ptr<persistence::PScanDBAccessor> pda)
	:accessor(pda),
	timesOfResultPeriod(1),
	dataWriter(pda)
{

}

PScanRecorder::~PScanRecorder()
{
}

void PScanRecorder::enable(int period)
{
	timesOfResultPeriod = period > 1 ? period : timesOfResultPeriod;
	accumulatedSpectrum.enable();
	accumulatedHits.enable();
}

void PScanRecorder::disable()
{
	accumulatedSpectrum.disable();
	accumulatedHits.disable();
	accumulatedStatus.reset();
}

bool PScanRecorder::isEnabled() const
{
	return accumulatedSpectrum.isEnabled();
}

bool PScanRecorder::isNeedRecord(int sequenceNum) const
{
	if (!isEnabled())
		return false;
	return sequenceNum % timesOfResultPeriod == 0;
}

void PScanRecorder::writeTaskInfo(const persistence::PScanTaskItem& taskItem)
{
	if (taskInfo != nullptr)
		return;
	taskInfo = make_unique<persistence::PScanTaskItem>(taskItem);
	taskInfo->seq = -1;
	taskInfo->uuid = genUUID();
	taskInfo->seq = accessor->insertTask(*taskInfo);
}

void PScanRecorder::onResultReport(const spectrum::Header& header)
{
	if (accumulatedStatus == nullptr)
	{
		accumulatedStatus = make_unique<spectrum::Header>(header);
	}
	else
	{
		*(accumulatedStatus->mutable_time_span()->mutable_stop_time()) = header.time_span().stop_time();
		accumulatedStatus->set_sweep_count(accumulatedStatus->sweep_count() + header.sweep_count());
	}
	int accumulatedResultCount = header.sequence_number() + 1 - accumulatedStatus->sequence_number();	//根据序列号的差值得到累积的数据量
	if (isNeedRecord(accumulatedResultCount))
	{
		if (dataWriter.isReady())
		{
			makeDataItem();	//构造dataItem
			resetAcc();		//重置累积的结果
			dataWriter.writeItem(dataItem); //写入数据库耗时较长，异步写入，不妨碍下次的数据累积
		}
	}
}

void PScanRecorder::rewriteTaskInfo(const zczh::zhpscan::OperationStatus& lastStatus)
{
	if (taskInfo == nullptr || taskInfo->seq < 0)
		return;
	taskInfo->startTime = timestampToUINT64(lastStatus.time_span().start_time());
	taskInfo->stopTime = timestampToUINT64(lastStatus.time_span().stop_time());
	taskInfo->sweepCount = lastStatus.total_sweep_count();
	taskInfo->dataItemCount = recordCount();
	accessor->updateTask(*taskInfo);
}

void PScanRecorder::makeDataItem()
{
	dataItem.seq = -1;
	dataItem.datauuid = genUUID();
	dataItem.taskuuid = taskInfo->uuid;
	dataItem.startTime = timestampToUINT64(accumulatedStatus->time_span().start_time());
	dataItem.stopTime = timestampToUINT64(accumulatedStatus->time_span().stop_time());
	dataItem.sweepCount = accumulatedStatus->sweep_count();
	auto pos = accumulatedStatus->device_position();
	dataItem.altitude = pos.altitude();
	dataItem.longitude = pos.longitude();
	dataItem.latitude = pos.latitude();
	accumulatedSpectrum.fillDataBuffer(dataItem.data);
	accumulatedHits.fillDataBuffer(dataItem.hits);
}

void PScanRecorder::resetAcc()
{
	accumulatedSpectrum.reset();
	accumulatedHits.reset();
	accumulatedStatus.reset();
}