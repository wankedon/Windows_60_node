#include "pch.h"
#include "ZhDFRecorder.h"
#include "TraceDataHolder.h"
#include "Logger.h"

using namespace std;

DFDataAsyncWriter::DFDataAsyncWriter(std::shared_ptr<persistence::DFDBAccessor> dbAccessor)
	:accessor(dbAccessor),
	writeItemCounter(0)
{

}

DFDataAsyncWriter::~DFDataAsyncWriter()
{
}

bool DFDataAsyncWriter::isReady()
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

bool DFDataAsyncWriter::writeItem(const persistence::DFDataItem& item)
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

int DFDataAsyncWriter::recordCount() const
{
	return writeItemCounter;
}

DFRecorder::DFRecorder(std::shared_ptr<persistence::DFDBAccessor> pda)
	:accessor(pda),
	dataWriter(pda)
{

}

DFRecorder::~DFRecorder()
{
}

void DFRecorder::enable()
{
	accumulatedSpectrum.enable();
	accumulatedHits.enable();
}

void DFRecorder::disable()
{
	accumulatedSpectrum.disable();
	accumulatedHits.disable();
}

bool DFRecorder::isEnabled() const
{
	return accumulatedSpectrum.isEnabled();
}

void DFRecorder::writeTaskInfo(const persistence::DFTaskItem& taskItem)
{
	if (taskInfo != nullptr)
		return;
	taskInfo = make_unique<persistence::DFTaskItem>(taskItem);
	taskInfo->seq = -1;
	taskInfo->uuid = genUUID();
	taskInfo->seq = accessor->insertTask(*taskInfo);
}

void DFRecorder::onResultReport(const zczh::zhdirection::Result& result)
{
	if (!isEnabled())
		return;
	if (dataWriter.isReady())
	{
		makeDataItem(result);	//构造dataItem
		resetAcc();		//重置累积的结果
		dataWriter.writeItem(dataItem); //写入数据库耗时较长，异步写入，不妨碍下次的数据累积
	}
}

void DFRecorder::rewriteTaskInfo(const zczh::zhdirection::OperationStatus& lastStatus)
{
	if (taskInfo == nullptr || taskInfo->seq < 0)
		return;
	taskInfo->startTime = timestampToUINT64(lastStatus.time_span().start_time());
	taskInfo->stopTime = timestampToUINT64(lastStatus.time_span().stop_time());
	taskInfo->sweepCount = lastStatus.total_sweep_count();
	taskInfo->dataItemCount = recordCount();
	accessor->updateTask(*taskInfo);
}

void DFRecorder::makeDataItem(const zczh::zhdirection::Result& result)
{
	dataItem.seq = -1;
	dataItem.datauuid = genUUID();
	dataItem.taskuuid = taskInfo->uuid;
	dataItem.startTime = timestampToUINT64(result.header().time_span().start_time());
	dataItem.stopTime = timestampToUINT64(result.header().time_span().stop_time());
	dataItem.sweepCount = result.header().sweep_count();
	auto pos = result.header().device_position();
	dataItem.altitude = pos.altitude();
	dataItem.longitude = pos.longitude();
	dataItem.latitude = pos.latitude();
	accumulatedSpectrum.fillDataBuffer(dataItem.spectrum);
	accumulatedHits.fillDataBuffer(dataItem.hits);
	dataItem.target.resize(result.target_detection().ByteSizeLong());
	result.target_detection().SerializeToArray(dataItem.target.data(), (int)dataItem.target.size());
}

void DFRecorder::resetAcc()
{
	accumulatedSpectrum.reset();
	accumulatedHits.reset();
}