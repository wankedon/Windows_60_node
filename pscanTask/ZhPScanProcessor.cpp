#include "pch.h"
#include "ZhPScanProcessor.h"
#include "SpectrumDataHolder.h"
#include "SpectrumThreshold.h"
#include "FrequencyScale.h"
#include "ZhPScanRecorder.h"
#include "ZhPScanDef.h"
#include "ZhPScanHistoryThreshold.h"
#include "SignalDetection.h"

using namespace std;

ZhPScanProcessor::ZhPScanProcessor(const spectrum::FrequencyBand& band, int32_t outputPoints, std::shared_ptr<persistence::PScanDBAccessor> accessor)
	:expectedPoints(outputPoints),
	 scale(make_unique<FrequencyScale>(band)),
	 dbAccessor(accessor)
{
}

ZhPScanProcessor::~ZhPScanProcessor(void) = default;

void ZhPScanProcessor::onTaskBegin(const NodeDevice& nd)
{
	dataHolder[spectrum::NO_HOLD] = make_shared<SpectrumDataHolder>(spectrum::NO_HOLD);	//默认仅创建当前迹线
	//历史门限对象创建函数
	auto htCreator = [this](const spectrum::FrequencyBand& band) {return make_shared<PScanHistoryThreshold>(band, dbAccessor); };
	thresholdManager = make_unique<ThresholdManager>(scale->wholeBand(), htCreator);
	recorder = make_unique<PScanRecorder>(dbAccessor);
	//信号检测
	signalDetector = make_unique<SignalDetection>();
	//建立数据流
	*dataHolder[spectrum::NO_HOLD] >> recorder->accumulatedSpectrum;
	*dataHolder[spectrum::NO_HOLD] >> *thresholdManager >> recorder->accumulatedHits;
	recorder->writeTaskInfo(makePScanTaskItem(nd));
}

void ZhPScanProcessor::onTaskEnd(const zczh::zhpscan::OperationStatus& status)
{
	recorder->rewriteTaskInfo(status);
}

persistence::PScanTaskItem ZhPScanProcessor::makePScanTaskItem(const NodeDevice& nd)
{
	persistence::PScanTaskItem taskItem{};
	taskItem.seq = -1;
	taskItem.deviceId = nd.device_id().value();
	taskItem.nodeId = nd.node_id().value();
	taskItem.points = scale->totalPoints();
	taskItem.startFreq = static_cast<int64_t>(scale->wholeSpan().start_freq());
	taskItem.stopFreq = static_cast<int64_t>(scale->wholeSpan().stop_freq());
	auto wholeBand = scale->wholeBand();
	vector<char> buffer(wholeBand.ByteSizeLong());
	wholeBand.SerializePartialToArray(buffer.data(), buffer.size());
	taskItem.bandInfo = std::move(buffer);
	return taskItem;
}

void ZhPScanProcessor::onSegUpdate(int segIdx, const float* data, size_t count)
{
	for (auto& pr : dataHolder)
	{
		pr.second->inputSegData(segIdx, data, count);
	}
}

void ZhPScanProcessor::onSweepBegin(std::list<MessageExtractor>& pendingCmd)
{
	for (auto& extractor : pendingCmd)
	{
		PScanCmdType type;
		if (extractor.extract(type))
		{
			switch (type)
			{
			case PScanCmdType::DATAHOLD_OPEN:
				onDataHoldOpen(extractor);
				break;
			case PScanCmdType::DATAHOLD_RESET:
				onDataHoldReset(extractor);
				break;
			case PScanCmdType::DATAHOLD_CLOSE:
				onDataHoldClose(extractor);
				break;
			case PScanCmdType::DETECT_OPEN:
				onDetectOpen(extractor);
				break;
			case PScanCmdType::DETECT_CLOSE:
				onDetectClose(extractor);
				break;
			case PScanCmdType::ZOOMIN_OPEN:
				onZoominOpen(extractor);
				break;
			case PScanCmdType::ZOOMIN_CLOSE:
				onZoominClose(extractor);
				break;
			case PScanCmdType::RECORD_OPEN:
				onRecordOpen(extractor);
				break;
			case PScanCmdType::RECORD_CLOSE:
				onRecordClose(extractor);
				break;
			default:
				break;
			}
		}
	}
}

void ZhPScanProcessor::onSweepEnd()
{
	dataHolder[spectrum::NO_HOLD]->forwardData();
}

void ZhPScanProcessor::onDataHoldOpen(MessageExtractor& extractor)
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

void ZhPScanProcessor::onDataHoldClose(MessageExtractor& extractor)
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

void ZhPScanProcessor::onDataHoldReset(MessageExtractor& extractor)
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

void ZhPScanProcessor::onDetectOpen(MessageExtractor& extractor)
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

void ZhPScanProcessor::onDetectClose(MessageExtractor& extractor)
{
	thresholdManager->onCloseRequest();
}

void ZhPScanProcessor::onZoominOpen(MessageExtractor& extractor)
{
	spectrum::FrequencySegment newZoominPart;
	if (!extractor.deserialize(newZoominPart))
		return;
	zoominParam = make_unique<FrequencySegment>(newZoominPart);
	scale->onSpanChange(zoominParam->freq_span());
}

void ZhPScanProcessor::onZoominClose(MessageExtractor& extractor)
{
	zoominParam.reset();
}

void ZhPScanProcessor::onRecordOpen(MessageExtractor& extractor)
{
	int timesOfResultPeriod;
	if (!extractor.extract(timesOfResultPeriod))
		return;
	recorder->enable(timesOfResultPeriod);
}

void ZhPScanProcessor::onRecordClose(MessageExtractor& extractor)
{
	recorder->disable();
}

void ZhPScanProcessor::onDetailKeeping(MessageExtractor& extractor)
{
	detection::DetailKeeping detailkeeping;
	if (!extractor.extract(detailkeeping))
		return;
	if (signalDetector == nullptr)
		return;
	signalDetector->setDetailKeeping(detailkeeping);
}

void ZhPScanProcessor::fillResult(zczh::zhpscan::Result& result)
{
	fillDataHoldResult(result);
	fillDetectResult(result);
	recorder->onResultReport(result.header());	//驱动记录器
}

void ZhPScanProcessor::fillStatus(zczh::zhpscan::OperationStatus& status)
{
	auto& lines = *status.mutable_threshold_lines();
	thresholdManager->fillThresholdLine(lines);
	status.set_record_count(recorder->recordCount());
}

void ZhPScanProcessor::fillDataHoldResult(zczh::zhpscan::Result& result)
{
	{
		auto view = result.mutable_panorama_view();
		*view->mutable_freq_span() = scale->wholeSpan();
		fillOneTrace(spectrum::DataHoldType::NO_HOLD, view->mutable_cur_trace(), true);
		fillOneTrace(spectrum::DataHoldType::MAX_HOLD, view->mutable_maxhold_trace(), true);
		fillOneTrace(spectrum::DataHoldType::MIN_HOLD, view->mutable_minhold_trace(), true);
	}
	if (zoominParam != nullptr)
	{
		auto view = result.mutable_closeshot_view();
		*view->mutable_freq_span() = scale->currentSpan();
		fillOneTrace(spectrum::DataHoldType::NO_HOLD, view->mutable_cur_trace(), false);
		fillOneTrace(spectrum::DataHoldType::MAX_HOLD, view->mutable_maxhold_trace(), false);
		fillOneTrace(spectrum::DataHoldType::MIN_HOLD, view->mutable_minhold_trace(), false);
	}
}

void ZhPScanProcessor::fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama)
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

void ZhPScanProcessor::fillDetectResult(zczh::zhpscan::Result& result)
{
	if (!thresholdManager->isOpened())
		return;
	{
		auto dst = result.mutable_panorama_view()->mutable_over_threshold_hits();
		auto src = thresholdManager->getPanorama(expectedPoints);
		*dst = { src.begin(), src.end() };
	}
	if (zoominParam != nullptr)
	{
		auto dst = result.mutable_closeshot_view()->mutable_over_threshold_hits();
		auto src = thresholdManager->getCloseshot(expectedPoints, *scale);
		*dst = { src.begin(), src.end() };
	}
	signalDetect(result);		//信号检测
	thresholdManager->sendHits();			//发送超限次数给下游
	thresholdManager->resetToDefault();		//拷贝完后重置最大保持线和超限计数（门限线不重置）
	signalDetector->onCommitDetectedSignal(*result.mutable_signal_list());	//提交检测结果
}

void ZhPScanProcessor::signalDetect(const zczh::zhpscan::Result& result)
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