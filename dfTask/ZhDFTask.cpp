#include "pch.h"
#include "ZhDFTask.h"
#include "ZhDFSpectrumProcessor.h"
#include "ZhDFTargetProcessor.h"
#include "ZhDFStatusMonitor.h"
#include "Logger.h"
#include "ZhDFDef.h"

using namespace std;

ZhDFTask::ZhDFTask(const TaskInitEntry& entry/*, std::shared_ptr<persistence::DFDBAccessor> accessor*/)
	:NodeTask(entry)
	//dbAccessor(accessor)
{

}

ZhDFTask::~ZhDFTask()
{
	stop();
}

void ZhDFTask::config(TaskRequestContext& context)
{
	context.messageExtractor().deserialize(taskParam);
}

ErrorType ZhDFTask::start()
{
	return realtimeSetup();
}

void ZhDFTask::onCmd(TaskRequestContext& context)
{
	
}

void ZhDFTask::stop()
{
	realtimeStop();
}

ErrorType ZhDFTask::realtimeSetup()
{
	auto sensor = downcast<ZBDevice::ZhSensor>();
	if (sensor == nullptr)
		return ERR_INVALID_HANDLE;
	dFinder = make_unique<ZBDevice::DirectionFinder>(sensor);
	//config dFinder
	dFinder->configDFBandParam(
		taskParam.center_frequency(),
		taskParam.if_bandwidth(),
		taskParam.df_bandwidth()
	);
	dFinder->configDFWorkMode(
		taskParam.ref_level(),
		(taskParam.antenna() == 0) ? senAntenna_1 : senAntenna_2,
		(taskParam.receive_mode() == 1) ? senReceiver_noise : senReceiver_normal,
		taskParam.result_interval()
	);
	dFinder->configDFThreshold(taskParam.mode(),
		taskParam.value(),
		taskParam.target_count()
	);
	//start
	auto dfHandler = [this](const senSegmentData& header, const std::vector<senDFTarget>& dfTarget, const std::vector<senVecFloat>& dfSpectrum) {onDFResult(header, dfTarget, dfSpectrum); };
	auto entranceHandler = [this]() {onDFThreadEntrance(); };
	auto quitHandler = [this]() {onDFThreadQuit(); };
	dFinder->setThreadEventHandler(entranceHandler, quitHandler);
	if (!dFinder->start(dfHandler))
		return ERR_PARAM;
	return ERR_NONE;
}

void ZhDFTask::onDFThreadEntrance()
{
	statusMonitor = make_unique<DFStatusMonitor>(taskParam.result_interval(), taskParam.status_interval());
	targetProcessor = make_unique<DFTargetProcessor>();
	spectrumProcessor = make_unique<DFSpectrumProcessor>(dFinder->frequencyBand(), taskParam.expected_points()/*, dbAccessor*/);//创建频谱数据处理单元
	spectrumProcessor->onTaskBegin(taskRunner());//完成Processor的剩余成员初始化
}

void ZhDFTask::onDFThreadQuit()
{
	spectrumProcessor->onTaskEnd(statusMonitor->getStatus());
}

void ZhDFTask::onDFResult(const senSegmentData& header, const std::vector<senDFTarget>& target, const std::vector<senVecFloat>& dfSpectrum)
{
	Timestamp ts = ZBDevice::extractSensorTS(header);
	Position pos = ZBDevice::extractSensorPos(header.location);
	//目标处理
	targetProcessor->onRawTargetData(target);
	//频谱处理
	for (auto segmentData : dfSpectrum)
	{
		spectrumProcessor->onSpectrum(header, segmentData);
	}
	statusMonitor->onSweepEnd(ts, pos);
	processTaskResult();
	processTaskStatus();
}

void ZhDFTask::processTaskResult()
{
	if (!statusMonitor->needResultReport())	//生成结果并发送
		return;
	zczh::zhdirection::Result result;
	*result.mutable_header() = statusMonitor->getHeader();
	spectrumProcessor->fillResult(result);
	targetProcessor->fillResult(result);
	statusMonitor->setupForNextResult();
	MessageBuilder mb;
	mb.add(DFSteamType::DF_RESULT);
	mb.serializeToTail(result);
	sendTaskData(mb);
}

void ZhDFTask::processTaskStatus()
{
	if (!statusMonitor->needStatusReport()) //生成状态并发送
		return;
	auto status = statusMonitor->getStatus();
	statusMonitor->restartStatusTimer();	//重新开始状态计时
	spectrumProcessor->fillStatus(status);
	MessageBuilder mb;
	mb.add(DFSteamType::DF_STATUS);
	mb.serializeToTail(status);
	sendTaskData(mb);
}

void ZhDFTask::realtimeStop()
{
	dFinder.reset();
}