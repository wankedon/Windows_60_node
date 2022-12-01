#include "pch.h"
#include "ZhIFSubIQTask.h"
#include "ZhIQDataBuffer.h"
#include "Logger.h"
#include "ZhIFDef.h"

using namespace std;

ZhIFSubIQTask::ZhIFSubIQTask(std::shared_ptr<ZBDevice::ZhSensor> sensor)
	 :m_sensor(sensor),
	m_recognise(make_unique<ZhRecognition>()),
	m_isRuning(false)
{
	
}

ZhIFSubIQTask::~ZhIFSubIQTask()
{
	stop();
}

bool ZhIFSubIQTask::config(zczh::zhIFAnalysis::TaskParam taskParam)
{
	iqSweeper = make_unique<ZBDevice::IQSweeper>(m_sensor);
	iqSweeper->configBandParam(taskParam.span().start_freq(),
		taskParam.span().stop_freq(),
		taskParam.if_bandwidth()
	);
	iqSweeper->configWorkMode(taskParam.ref_level(),
		(taskParam.antenna() == 0) ? senAntenna_1 : senAntenna_2,
		(taskParam.receive_mode() == 1) ? senReceiver_noise : senReceiver_normal,
		taskParam.result_interval()
	);
	iqSweeper->configOther(taskParam.iq_sweep_count());
	setRecogniseType(taskParam);
	return true;
}

void ZhIFSubIQTask::setRecogniseType(zczh::zhIFAnalysis::TaskParam taskParam)
{
	auto type = ZhRecognitionType(taskParam.type() + 1);
	if (m_recognise)
		m_recognise->resetRecogniseType(type);
	if (type == ZhRecognitionType::DEMOD)
	{
		m_recognise->resetDemodType(ZhDemodType(taskParam.demod_type()));
		LOG("DEMOD");
	}
}

ErrorType ZhIFSubIQTask::start(std::function<void(MessageBuilder& mb)> sendTaskData)
{
	m_sendTaskData = sendTaskData;
	return realtimeSetup();
}

ErrorType ZhIFSubIQTask::realtimeSetup()
{
	auto iqHandler = [this](const senIQSegmentData& header, const std::vector<float>& rawIQ) {onAcq(header, rawIQ); };
	if (!iqSweeper->start(iqHandler))
		return ERR_PARAM;
	return ERR_NONE;
}

void ZhIFSubIQTask::onAcq(const senIQSegmentData& header, const std::vector<float>& iqdata)
{
	//CLOG("ZhIFSubIQTask Acq IQData");
	m_recognise->Recognise(header, iqdata);
	if (m_isRuning)
		return;
	std::function<void()> startFunc = [this]() {processTaskResultLoop(); };
	std::function<void()> stopFunc = [this]() {m_isRuning = false; };
	m_Thread.start(startFunc, &stopFunc);
}

void ZhIFSubIQTask::processTaskResultLoop()
{
	m_isRuning = true;
	while (m_isRuning)
	{
		processTaskResult();
#ifdef PLATFROM_WINDOWS
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#else
		sleep(1);
#endif 
	}
}

void ZhIFSubIQTask::processTaskResult()
{
	m_recognise->fillResult(m_result);
	MessageBuilder mb;
	mb.add(IFSteamType::IF_RESULT_T);
	mb.serializeToTail(m_result);
	m_sendTaskData(mb);
}

void ZhIFSubIQTask::stop()
{
	if (iqSweeper)
		iqSweeper.reset();
	m_Thread.stop();
}