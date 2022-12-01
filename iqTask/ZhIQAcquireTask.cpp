#include "pch.h"
#include "ZhIQAcquireTask.h"
#include "IQRecorder.h"
#include "IQAcquireDef.h"
#include "Logger.h"

using namespace std;

ZhIQAcquireTask::ZhIQAcquireTask(const TaskInitEntry& entry, std::shared_ptr<persistence::IQAcquireDBAccessor> accessor)
	:NodeTask(entry),
	 totalBlockCount(0),
	 recorder(make_unique<IQAcquireRecorder>(accessor))
{

}

ZhIQAcquireTask::~ZhIQAcquireTask()
{
	stop();
	updateTaskInfo();
}

void ZhIQAcquireTask::config(TaskRequestContext& context)
{
	context.messageExtractor().deserialize(taskParam);
}

ErrorType ZhIQAcquireTask::start()
{
	return realtimeSetup();
}

void ZhIQAcquireTask::onCmd(TaskRequestContext& context)
{
	IQAcquireCmdType cmdType;
	context.messageExtractor().extract(cmdType);
	if (cmdType == IQAcquireCmdType::RECORD_OPEN)
	{
		recorder->enable();
	}
	if (cmdType == IQAcquireCmdType::RECORD_CLOSE)
	{
		recorder->disable();
	}
}

ErrorType ZhIQAcquireTask::realtimeSetup()
{
	auto sensor = downcast<ZBDevice::ZhSensor>();
	if (sensor == nullptr)
		return ERR_INVALID_HANDLE;
	iqSweeper = make_unique<ZBDevice::IQSweeper>(sensor);
	//config sweeper
	iqSweeper->configBandParam(
		taskParam.span().start_freq(),
		taskParam.span().stop_freq(),
		taskParam.if_bandwidth()
	);
	iqSweeper->configWorkMode(
		taskParam.attenuation_gain(),
		(taskParam.antenna() == 0) ? senAntenna_1 : senAntenna_2,
		(taskParam.receive_mode() == 1) ? senReceiver_noise : senReceiver_normal,
		taskParam.interval()
	);
	iqSweeper->configOther(taskParam.points());
	//start
	if (taskParam.is_record())
		recorder->enable();
	auto iqHandler = [this](const senIQSegmentData& header, const std::vector<float>& rawIQ) {onAcq(header, rawIQ); };
	if (!iqSweeper->start(iqHandler))
		return ERR_PARAM;
	return ERR_NONE;
}

void ZhIQAcquireTask::onAcq(const senIQSegmentData& header, const std::vector<float>& iq)
{
	zczh::zhIQAcquire::Result result;
	auto resultHeader = result.mutable_header();
	*resultHeader->mutable_result_from() = taskRunner();
	resultHeader->set_sequence_number(header.sequenceNumber);
	resultHeader->set_center_frequency(header.centerFrequency);
	resultHeader->set_sample_rate(header.sampleRate);
	lastTS = makeTimestamp(header.timestampSeconds, header.timestampNSeconds);
	*resultHeader->mutable_time_stamp() = lastTS;
	auto pos = makePosition(header.longitude, header.latitude, header.elevation);
	*resultHeader->mutable_position() = pos;
	//resultHeader->set_sweep_index(header.sweepIndex);
	//resultHeader->set_segment_index(header.segmentIndex);
	//resultHeader->set_scale_to_volts(header.scaleToVolts);
	//resultHeader->set_truncate_bits(header.shlBits);
	*result.mutable_body()->mutable_iq_data() = { iq.data(), iq.data() + header.numSamples};
	if (header.sequenceNumber == 0)	//first IQ block
	{
		writeTaskInfo();
	}
	if (header.sequenceNumber + 1 == totalBlockCount)	// last IQ block
	{
		resultHeader->set_is_last_one(true);
	}
	recorder->onResultReport(result);
	if (taskParam.header_only())
	{
		result.clear_body();
	}
	//send result
	MessageBuilder builder;
	builder.serializeToTail(result);
	sendTaskData(builder);
}

void ZhIQAcquireTask::writeTaskInfo()
{
	persistence::IQAcquireTaskItem taskItem{};
	taskItem.taskSeq = -1;
	taskItem.deviceId = taskRunner().device_id().value();
	taskItem.nodeId = taskRunner().node_id().value();
	//taskItem.numSegment = taskParam.segment_param_size();
	//taskItem.numTransfer = taskParam.num_transfer_samples();
	//taskItem.numBlocks = taskParam.num_blocks();
	//taskItem.sizeofSampleInBytes = taskParam.data_type() == sensor::IQAcquire::DataType::cplx_16 ? 4 : 8;
	taskItem.startTime = timestampToUINT64(lastTS);
	taskItem.startFreq = static_cast<int64_t>(taskParam.span().start_freq());
	taskItem.stopFreq = static_cast<int64_t>(taskParam.span().stop_freq());
	vector<char> buffer(taskParam.ByteSizeLong());
	taskParam.SerializeToArray(buffer.data(), buffer.size());
	taskItem.taskParam = std::move(buffer);
	recorder->writeTaskInfo(taskItem);
}

void ZhIQAcquireTask::updateTaskInfo()
{
	recorder->rewriteTaskInfo(lastTS);
}

void ZhIQAcquireTask::stop()
{
	recorder->enable();
	iqSweeper.reset();
}