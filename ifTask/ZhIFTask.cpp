#include "pch.h"
#include "ZhIFTask.h"
#include "ZhIFSubScanTask.h"
#include "ZhIFSubIQTask.h"
using namespace std;

ZhIFTask::ZhIFTask(const TaskInitEntry& entry, std::shared_ptr<persistence::IFDBAccessor> accessor)
	:NodeTask(entry),
	dbAccessor(accessor)
{
}

ZhIFTask::~ZhIFTask()
{
	stop();
}

void ZhIFTask::config(TaskRequestContext& context)
{
	context.messageExtractor().deserialize(taskParam);
}

ErrorType ZhIFTask::start()
{
	auto sensor = downcast<ZBDevice::ZhSensor>();
	if (sensor == nullptr)
		return ERR_INVALID_HANDLE;
	if (taskParam.type() == zczh::zhIFAnalysis::RecogniseType::DEMOD)
	{
		m_iqTask = make_unique<ZhIFSubIQTask>(sensor);
		return realtimeIqStart();
	}
	else
	{
		m_scanTask = make_unique<ZhIFSubScanTask>(sensor, dbAccessor);
		return realtimeScanStart();
	}
}

void ZhIFTask::onCmd(TaskRequestContext& context)
{	
	//干预指令 切换识别类型
	//zczh::zhIFAnalysis::RecogniseParam recogParam;
	//if (!extractor.deserialize(recogParam))
	//	return ERR_PARAM;
	//设置识别类型
	//重置任务
	//if (m_scanTask)
		//m_scanTask->setRecogniseType(ZhRecognitionType(recogParam.type() + 1));
	if (m_scanTask)
		m_scanTask->onCmd(context.messageExtractor());//std::move(context.messageExtractor())
}

void ZhIFTask::stop()
{
	if (m_scanTask)
		m_scanTask->stop();
	if (m_iqTask)
		m_iqTask->stop();
}

ErrorType ZhIFTask::realtimeScanStart()
{
	m_scanTask->config(taskParam, taskRunner());
	auto sendHandler = [this](MessageBuilder& mb) 
	{
		sendTaskData(mb); 
	};
	return m_scanTask->start(sendHandler);
}

ErrorType ZhIFTask::realtimeIqStart()
{
	m_iqTask->config(taskParam);
	auto sendHandler = [this](MessageBuilder& mb)
	{
		sendTaskData(mb);
	};
	return m_iqTask->start(sendHandler);
}