#include "pch.h"
#include "ZhIFSpectrumProcessor.h"
#include "ZhIFRecorder.h"
#include "ZhIFDef.h"
#include "SpectrumDataHolder.h"
#include "SpectrumThreshold.h"
#include "FrequencyScale.h"
#include "SignalDetection.h"
#include "ZhIFHistoryThreshold.h"

using namespace std;

IFSpectrumProcessor::IFSpectrumProcessor(const spectrum::FrequencyBand& band, int32_t outputPoints, std::shared_ptr<persistence::IFDBAccessor> accessor)
	:expectedPoints(outputPoints),
	 scale(make_unique<FrequencyScale>(band)),
	 dbAccessor(accessor)
{
}

IFSpectrumProcessor::~IFSpectrumProcessor(void) = default;

ErrorType IFSpectrumProcessor::onCmd(MessageExtractor& extractor)
{
	lock_guard<mutex> lg(mtx);
	pendingCmd.push_back(std::move(extractor));
	return ERR_NONE;
}

void IFSpectrumProcessor::onTaskBegin(const NodeDevice& nd)
{
	dataHolder[spectrum::NO_HOLD] = make_shared<SpectrumDataHolder>(spectrum::NO_HOLD);	//默认仅创建当前迹线
	//历史门限对象创建函数
	auto htCreator = [this](const spectrum::FrequencyBand& band) {return make_shared<IFHistoryThreshold>(band, dbAccessor); };
	//历史门限对象创建函数
	thresholdManager = make_unique<ThresholdManager>(scale->wholeBand(), htCreator);
	recorder = make_unique<IFRecorder>(dbAccessor);
	//信号检测
	signalDetector = make_unique<SignalDetection>();
	//建立数据流
	*dataHolder[spectrum::NO_HOLD] >> recorder->accumulatedSpectrum;
	*dataHolder[spectrum::NO_HOLD] >> *thresholdManager >> recorder->accumulatedHits;
	recorder->writeTaskInfo(makeTaskItem(nd));
}

persistence::IFTaskItem IFSpectrumProcessor::makeTaskItem(const NodeDevice& nd)
{
	persistence::IFTaskItem taskItem{};
	taskItem.seq = -1;
	taskItem.deviceId = nd.device_id().value();
	taskItem.nodeId = nd.node_id().value();
	taskItem.points = scale->totalPoints();
	auto startFreq = scale->wholeSpan().start_freq();
	auto stopFreq = scale->wholeSpan().stop_freq();
	taskItem.centerFreq = static_cast<int64_t>(startFreq + stopFreq) / 2;
	taskItem.bandWidth = static_cast<int64_t>(stopFreq - startFreq);
	return taskItem;
}

void IFSpectrumProcessor::onTaskEnd(const zczh::zhIFAnalysis::OperationStatus& status)
{
	recorder->rewriteTaskInfo(status);
}

void IFSpectrumProcessor::onSpectrum(const senSegmentData& header, const std::vector<float>& segmentData)
{
	if (!pendingCmd.empty())
	{
		std::list<MessageExtractor> copy;
		{
			lock_guard<mutex> lg(mtx);
			copy.swap(pendingCmd);
		}
		processPendingCmd(copy);	//在扫描之初执行悬置命令
	}
	for (auto& pr : dataHolder)
	{
		pr.second->inputSegData(0, segmentData.data(), header.numPoints);
	}
	dataHolder[spectrum::NO_HOLD]->forwardData();
}

void IFSpectrumProcessor::processPendingCmd(std::list<MessageExtractor>& cmdCopy)
{
	for (auto& extractor : cmdCopy)
	{
		IFCmdType type;
		if (extractor.extract(type))
		{
			switch (type)
			{
			case IFCmdType::DATAHOLD_OPEN:
				onDataHoldOpen(extractor);
				break;
			case IFCmdType::DATAHOLD_RESET:
				onDataHoldReset(extractor);
				break;
			case IFCmdType::DATAHOLD_CLOSE:
				onDataHoldClose(extractor);
				break;
			case IFCmdType::DETECT_OPEN:
				onDetectOpen(extractor);
				break;
			case IFCmdType::DETECT_CLOSE:
				onDetectClose(extractor);
				break;
			case IFCmdType::ZOOMIN_OPEN:
				onZoominOpen(extractor);
				break;
			case IFCmdType::ZOOMIN_CLOSE:
				onZoominClose(extractor);
				break;
			case IFCmdType::RECORD_OPEN:
				onRecordOpen(extractor);
				break;
			case IFCmdType::RECORD_CLOSE:
				onRecordClose(extractor);
				break;
			case IFCmdType::DETAIL_KEEPING:
				onDetailKeeping(extractor);
				break;
			default:
				break;
			}
		}
	}
}

void IFSpectrumProcessor::onDataHoldOpen(MessageExtractor& extractor)
{
	spectrum::DataHoldType holdType;
	if (extractor.extract(holdType))
	{
		if (dataHolder.find(holdType) == dataHolder.end())
		{
			dataHolder[holdType] = make_shared<SpectrumDataHolder>(holdType);
		}
	}
}

void IFSpectrumProcessor::onDataHoldClose(MessageExtractor& extractor)
{
	spectrum::DataHoldType holdType;
	if (extractor.extract(holdType))
	{
		if (holdType != spectrum::NO_HOLD)	//当前迹线不允许关闭
		{
			auto iter = dataHolder.find(holdType);
			if (iter != dataHolder.end())
			{
				dataHolder.erase(iter);
			}
		}
	}
}

void IFSpectrumProcessor::onDataHoldReset(MessageExtractor& extractor)
{
	spectrum::DataHoldType holdType;
	if (extractor.extract(holdType))
	{
		auto iter = dataHolder.find(holdType);
		if (iter != dataHolder.end())
		{
			iter->second->clear();
		}
	}
}

void IFSpectrumProcessor::onDetectOpen(MessageExtractor& extractor)
{
	detection::DetectionParam detectParam;
	if (!extractor.deserialize(detectParam))
		return;
	if (detectParam.has_energy_detect_param())
	{
		thresholdManager->onDetectionRequest(detectParam.energy_detect_param());
	}
	signalDetector->setDetectionParam(detectParam);
}

void IFSpectrumProcessor::onDetectClose(MessageExtractor& extractor)
{
	thresholdManager->onCloseRequest();
}

void IFSpectrumProcessor::onZoominOpen(MessageExtractor& extractor)
{
	spectrum::FrequencySegment newZoominPart;
	if (!extractor.deserialize(newZoominPart))
		return;
	zoominParam = make_unique<FrequencySegment>(newZoominPart);
	scale->onSpanChange(zoominParam->freq_span());
}

void IFSpectrumProcessor::onZoominClose(MessageExtractor& extractor)
{
	zoominParam.reset();
}

void IFSpectrumProcessor::onRecordOpen(MessageExtractor& extractor)
{
	recorder->enable();
}

void IFSpectrumProcessor::onRecordClose(MessageExtractor& extractor)
{
	recorder->disable();
}

void IFSpectrumProcessor::onDetailKeeping(MessageExtractor& extractor)
{
	detection::DetailKeeping detailkeeping;
	if (!extractor.extract(detailkeeping))
		return;
	if (signalDetector == nullptr)
		return;
	signalDetector->setDetailKeeping(detailkeeping);
}

void IFSpectrumProcessor::fillResult(zczh::zhIFAnalysis::Result& result)
{
	fillDataHoldResult(result);
	fillDetectResult(result);
	recorder->onResultReport(result);	//驱动记录器
}

void IFSpectrumProcessor::fillStatus(zczh::zhIFAnalysis::OperationStatus& status)
{
	auto& lines = *status.mutable_threshold_lines();
	thresholdManager->fillThresholdLine(lines);
	status.set_record_count(recorder->recordCount());
}

void IFSpectrumProcessor::fillDataHoldResult(zczh::zhIFAnalysis::Result& result)
{
	auto view = result.mutable_spectrum();
	*view->mutable_freq_span() = scale->wholeSpan();
	fillOneTrace(spectrum::DataHoldType::NO_HOLD, view->mutable_cur_trace(), true);
	fillOneTrace(spectrum::DataHoldType::MAX_HOLD, view->mutable_maxhold_trace(), true);
	fillOneTrace(spectrum::DataHoldType::MIN_HOLD, view->mutable_minhold_trace(), true);
}

void IFSpectrumProcessor::fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama)
{
	auto iter = dataHolder.find(type);
	if (iter != dataHolder.end())
	{
		if (isPanorama)
		{
			auto traceData = iter->second->getPanorama(expectedPoints);
			*protoRepeatedField = { traceData.begin(), traceData.end() };
		}
		else
		{
			auto traceData = iter->second->getCloseshot(zoominParam->points(), *scale);
			*protoRepeatedField = { traceData.begin(), traceData.end() };
		}
	}
}

void IFSpectrumProcessor::fillDetectResult(zczh::zhIFAnalysis::Result& result)
{
	if (!thresholdManager->isOpened())
		return;
	auto dst = result.mutable_spectrum()->mutable_over_threshold_hits();
	auto src = thresholdManager->getPanorama(expectedPoints);
	*dst = { src.begin(), src.end() };
	signalDetect(result);		//信号检测
	thresholdManager->sendHits();			//发送超限次数给下游
	thresholdManager->resetToDefault();		//拷贝完后重置最大保持线和超限计数（门限线不重置）
	signalDetector->onCommitDetectedSignal(*result.mutable_signal_list());	//提交检测结果
}

void IFSpectrumProcessor::signalDetect(const zczh::zhIFAnalysis::Result& result)
{
	detection::RawData rawData;
	auto rawHeader = rawData.mutable_header();
	*rawHeader->mutable_time_span() = result.header().time_span();
	rawHeader->set_sweep_count(result.header().sweep_count());
	*rawHeader->mutable_device_position() = result.header().device_position();
	auto rawBody = rawData.mutable_body();
	*rawBody->mutable_freq_span() = scale->wholeSpan();
	auto src = dataHolder[spectrum::DataHoldType::NO_HOLD]->getPanorama(0);	//取得全景的所有原始数据
	*rawBody->mutable_cur_trace() = { src.begin(), src.end() };
	src = thresholdManager->getPanorama(0);	//取得hits值的所有原始数据
	*rawBody->mutable_over_threshold_hits() = { src.begin(), src.end() };
	signalDetector->detect(rawData);
}