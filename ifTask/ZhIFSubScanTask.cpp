#include "pch.h"
#include "ZhIFSubScanTask.h"
#include "ZhIFSpectrumProcessor.h"
#include "ZhIFStatusMonitor.h"
#include "Logger.h"
#include "ZhIFDef.h"

using namespace std;

ZhIFSubScanTask::ZhIFSubScanTask(std::shared_ptr<ZBDevice::ZhSensor> sensor,std::shared_ptr<persistence::IFDBAccessor> accessor)
	:m_sensor(sensor),
	dbAccessor(accessor),
	m_recognise(make_unique<ZhRecognition>())
{

}

ZhIFSubScanTask::~ZhIFSubScanTask()
{
	stop();
}

bool ZhIFSubScanTask::config(zczh::zhIFAnalysis::TaskParam taskParam,const NodeDevice& nd)
{
	m_taskParam= taskParam;
	m_taskRunner = nd;
	ifAna = make_unique<ZBDevice::IfAnalyst>(m_sensor);
	//config IfAnalyst
	ifAna->configIFBandParam(
		m_taskParam.span().start_freq(),
		m_taskParam.span().stop_freq(),
		m_taskParam.if_bandwidth()
	);
	ifAna->configIFWorkMode(
		m_taskParam.ref_level(),
		(m_taskParam.antenna() == 0) ? senAntenna_1 : senAntenna_2,
		(m_taskParam.receive_mode() == 1) ? senReceiver_noise : senReceiver_normal,
		m_taskParam.result_interval()
	);
	ifAna->configIFThreshold(m_taskParam.mode(), m_taskParam.value(), m_taskParam.average_count());
	//setRecogniseType
	setRecogniseType(ZhRecognitionType(m_taskParam.type() + 1));
	return true;
}

void ZhIFSubScanTask::setRecogniseType(ZhRecognitionType type)
{
	if (m_recognise)
		m_recognise->resetRecogniseType(type);
	if (type == ZhRecognitionType::MOD)
		LOG("MOD_Recognition");
	if (type == ZhRecognitionType::TRANS)
		LOG("TRANS_Recognition");
	if (type == ZhRecognitionType::DEMOD)
	{
		m_recognise->resetDemodType(ZhDemodType(m_taskParam.demod_type()));
		LOG("DEMOD");
	}
}

ErrorType ZhIFSubScanTask::onCmd(MessageExtractor& extractor)
{
	if (spectrumProcessor)
		return spectrumProcessor->onCmd(extractor);
	else
		return ERR_NOTIMPLEMENTED;
}

ErrorType ZhIFSubScanTask::start(std::function<void(MessageBuilder& mb)> sendTaskData)
{
	m_sendTaskData = sendTaskData;
	return realtimeSetup();
}

void ZhIFSubScanTask::stop()
{
	realtimeStop();
}

ErrorType ZhIFSubScanTask::realtimeSetup()
{
	auto ifSpectrumHandler = [this](const senSegmentData& header, const senVecFloat& ifSpectrum) {onIFSpectrumResult(header, ifSpectrum); };
	auto ifIQHandler = [this](const senIQSegmentData& header, const std::vector<float>& iqdata) {onIFIQResult(header, iqdata); };
	auto entranceHandler = [this]() {onIFThreadEntrance(); };
	auto quitHandler = [this]() {onIFThreadQuit(); };
	ifAna->setThreadEventHandler(entranceHandler, quitHandler);
	if (!ifAna->start(ifSpectrumHandler, ifIQHandler))
		return ERR_PARAM;
	return ERR_NONE;
}

void ZhIFSubScanTask::onIFThreadEntrance()
{
	statusMonitor = make_unique<IFStatusMonitor>(m_taskParam.result_interval(), m_taskParam.status_interval());
	spectrumProcessor = make_unique<IFSpectrumProcessor>(ifAna->frequencyBand(), m_taskParam.expected_points(), dbAccessor);	//创建频谱数据处理单元
	spectrumProcessor->onTaskBegin(m_taskRunner);	//完成Processor的剩余成员初始化
}

void ZhIFSubScanTask::onIFThreadQuit()
{
	spectrumProcessor->onTaskEnd(statusMonitor->getStatus());
}

void ZhIFSubScanTask::onIFSpectrumResult(const senSegmentData& header, const senVecFloat& ifSpectrum)
{
	Timestamp ts = ZBDevice::extractSensorTS(header);
	Position pos = ZBDevice::extractSensorPos(header.location);
	//频谱处理
	spectrumProcessor->onSpectrum(header, ifSpectrum);
	statusMonitor->onSweepEnd(ts, pos);
	processTaskResult();
	processTaskStatus();
}

void ZhIFSubScanTask::onIFIQResult(const senIQSegmentData& header, const std::vector<float>& iqdata)
{
	m_recognise->Recognise(header, iqdata);
}

void ZhIFSubScanTask::processTaskResult()
{
	if (!statusMonitor->needResultReport())	//生成结果并发送
		return;
	zczh::zhIFAnalysis::Result result;
	*result.mutable_header() = statusMonitor->getHeader();
	spectrumProcessor->fillResult(result);
	m_recognise->fillResult(result);
	statusMonitor->setupForNextResult();
	MessageBuilder mb;
	mb.add(IFSteamType::IF_RESULT_T);
	mb.serializeToTail(result);
	m_sendTaskData(mb);
}

void ZhIFSubScanTask::processTaskStatus()
{
	if (!statusMonitor->needStatusReport()) //生成状态并发送
		return;
	auto status = statusMonitor->getStatus();
	statusMonitor->restartStatusTimer();	//重新开始状态计时
	spectrumProcessor->fillStatus(status);
	MessageBuilder mb;
	mb.add(IFSteamType::IF_STATUS_T);
	mb.serializeToTail(status);
	m_sendTaskData(mb);
}

void ZhIFSubScanTask::realtimeStop()
{
	if (ifAna)
	{
		ifAna.reset();
	}
}