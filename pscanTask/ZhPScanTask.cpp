#include "pch.h"
#include "ZhPScanTask.h"
#include "ZhPScanStatusMonitor.h"
#include "ZhPScanProcessor.h"
#include "Logger.h"
#include "ZhPScanDef.h"
#include "ZhPScanSpectrumAverage.h"
using namespace std;

ZhPScanTask::ZhPScanTask(const TaskInitEntry& entry, std::shared_ptr<persistence::PScanDBAccessor> accessor)
	:NodeTask(entry),
	dbAccessor(accessor)
{

}

ZhPScanTask::~ZhPScanTask()
{
	stop();
}

void ZhPScanTask::config(TaskRequestContext& context)
{
	context.messageExtractor().deserialize(taskParam);
}

ErrorType ZhPScanTask::start()
{
	if (m_average == nullptr)
		m_average = std::make_unique<ZhPScanSpectrumAverage>(taskParam.average_count());
	return realtimeSetup();
}

void ZhPScanTask::onCmd(TaskRequestContext& context)
{
	lock_guard<mutex> lg(mtx);
	pendingCmd.emplace_back(std::move(context.messageExtractor()));	//MessageExtractor只支持右值拷贝构造
}

void ZhPScanTask::stop()
{
	if (m_average == nullptr)
		m_average.reset();
	realtimeStop();
}

ErrorType ZhPScanTask::realtimeSetup()
{
	auto sensor = downcast<ZBDevice::ZhSensor>();
	if (sensor == nullptr)
		return ERR_INVALID_HANDLE;
	sweeper = make_unique<ZBDevice::SpectrumSweeper>(sensor);
	//config PanoBandParam
	sweeper->configPanoBandParam(
		taskParam.freq_span().start_freq(),
		taskParam.freq_span().stop_freq(),
		taskParam.if_bandwidth()
	);
	//config PanoWorkMode
	sweeper->configPanoWorkMode(
		taskParam.ref_level(),
		(taskParam.antenna() == 0) ? senAntenna_1 : senAntenna_2,
		(taskParam.receive_mode() == 1) ? senReceiver_noise : senReceiver_normal,
		taskParam.result_interval(), 
		taskParam.average_count()
	);

	//config calibparam
	if (taskParam.has_calib_param())
	{
		CLOG("Zh Calib Model");
		senCalibParam senCalibParam;
		auto calibParam = taskParam.calib_param();
		senCalibParam.antenna_mode = calibParam.antenna_mode();
		senCalibParam.gain = calibParam.gain();
		senCalibParam.downconverter_mode = calibParam.downconverter_mode();
		senCalibParam.downconverter_if_attenuation = calibParam.downconverter_if_attenuation();
		senCalibParam.receive_mode = calibParam.receive_mode();
		senCalibParam.receive_rf_attenuation = calibParam.receive_rf_attenuation();
		senCalibParam.receive_if_attenuation = calibParam.receive_if_attenuation();
		senCalibParam.attenuation_mode = calibParam.attenuation_mode();
		sweeper->configCalibParam(&senCalibParam);
	}
	sweeper->setFirstSegHandler([this](const senSegmentData& header) {onFirstSeg(header); });
	sweeper->setLastSegHandler([this](const senSegmentData& header) {onLastSeg(header); });
	sweeper->setThreadEventHandler([this]() {onAcqThreadEntrance(); }, [this]() {onAcqThreadQuit(); });
	if (sweeper->start([this](const senSegmentData& header, const std::vector<float>& segmentData) {onAcq(header, segmentData); }))
		return ERR_NONE;
	else
		return ERR_PARAM;
}

void ZhPScanTask::onAcqThreadEntrance()
{
	processor = make_unique<ZhPScanProcessor>(sweeper->frequencyBand(), taskParam.expected_points(), dbAccessor);	//创建数据处理单元
	statusMonitor = make_unique<ZhPScanStatusMonitor>(taskParam.result_interval(), taskParam.status_interval());
	processor->onTaskBegin(taskRunner());	//完成Processor的剩余成员初始化
}

void ZhPScanTask::onAcqThreadQuit()
{
	processor->onTaskEnd(statusMonitor->getStatus());
	CLOG("Zhpscan quit");
}

void ZhPScanTask::onAcq(const senSegmentData& header, const std::vector<float>& segmentData)
{
	processor->onSegUpdate(header.segmentIndex, segmentData.data(), header.numPoints);
}

void ZhPScanTask::onFirstSeg(const senSegmentData& header)
{
	statusMonitor->onSweepBegin(ZBDevice::extractSensorTS(header));
	if (!pendingCmd.empty())
	{
		std::list<MessageExtractor> copy;
		{
			lock_guard<mutex> lg(mtx);
			copy.swap(pendingCmd);
		}
		processor->onSweepBegin(copy);	//在扫描之初执行悬置命令
	}
}

void ZhPScanTask::onLastSeg(const senSegmentData& header)
{
	Timestamp ts = ZBDevice::extractSensorTS(header);
	Position pos = ZBDevice::extractSensorPos(header.location);
	statusMonitor->onSweepEnd(ts, pos);
	processor->onSweepEnd();	
	if (statusMonitor->needResultReport())	//生成结果并发送
	{
		zczh::zhpscan::Result result;
		*result.mutable_header() = statusMonitor->getHeader();
		processor->fillResult(result);
		senVecFloat spectrum(result.panorama_view().cur_trace().begin(), result.panorama_view().cur_trace().end());
		auto averageData = m_average->getAverageData(spectrum);
		if (averageData.empty())
			return;
		result.mutable_panorama_view()->mutable_cur_trace()->CopyFrom({ averageData.begin(),averageData.end() });
		statusMonitor->setupForNextResult();
		MessageBuilder mb;
		mb.add(PScanSteamType::SPECTRUM_RESULT);
		mb.serializeToTail(result);
		sendTaskData(mb);	
	}
	if (statusMonitor->needStatusReport()) //生成状态并发送
	{
		auto status = statusMonitor->getStatus();
		statusMonitor->restartStatusTimer();	//重新开始状态计时
		processor->fillStatus(status);
		MessageBuilder mb;
		mb.add(PScanSteamType::OPERATION_STATUS);
		mb.serializeToTail(status);
		sendTaskData(mb);
	}
}

void ZhPScanTask::realtimeStop()
{
	sweeper.reset();
}