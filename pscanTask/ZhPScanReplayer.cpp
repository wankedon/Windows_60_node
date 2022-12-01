#include "pch.h"
#include "ZhPScanReplayer.h"
#include "ProtoSerializer.h"
#include "ZhPScanReplayTask.h"
#include "FrequencyScale.h"
#include "SpectrumDataHolder.h"
#include "SignalDetection.h"
#include "ZhPScanDef.h"

using namespace std;

ZhPScanDataLoader::ZhPScanDataLoader(int interval, const persistence::PScanTaskItem& ti, std::vector<persistence::PScanDataItem>&& di)
	:intervalInMs(interval),
	 taskItem(ti),
	 dataItems(std::move(di)),
	 timer(context)
{
	curPos = dataItems.begin();
	state.progress = -1;
	state.interval_factor = 1;
	state.pause = false;
	int seq = 0;
	for (auto& item : dataItems)
	{
		item.seq = seq++;	//让item的id从零开始递增排列
	}
}

ZhPScanDataLoader::~ZhPScanDataLoader()
{
	abort();
}

void ZhPScanDataLoader::setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit)
{
	userHandlers.threadEntranceHandler = entrance;
	userHandlers.threadQuitHandler = quit;
}

bool ZhPScanDataLoader::start(DataItemHandler handler)
{
	if (dataItems.empty())
		return false;
	userHandlers.dataHandler = handler;
	replayFuture = std::async([this]() {
		if (userHandlers.threadEntranceHandler != nullptr)
		{
			userHandlers.threadEntranceHandler();
		}
		onTimer();
		context.run(); 
		if (userHandlers.threadQuitHandler != nullptr)
		{
			userHandlers.threadQuitHandler();
		}
	});
	return true;
}

void ZhPScanDataLoader::onAdjust(const replay::AdjustOption& adjustOption)
{
	if (adjustOption.has_progress())
	{
		auto val = adjustOption.progress().value();
		if (val <= 1 && val >= 0)
		{
			state.progress = val;
		}
	}
	if (adjustOption.has_interval_factor())
	{
		auto val = adjustOption.interval_factor().value();
		if (val >= 0)
		{
			state.interval_factor = val;
		}
	}
	if (adjustOption.has_pause_resume())
	{
		state.pause = adjustOption.pause_resume().value();
	}
}

void ZhPScanDataLoader::adjustPos()
{
	if (state.progress < 0)	//规定小于零为没有调整进度的请求
		return;
	auto offset = static_cast<int>(dataItems.size() * state.progress);
	curPos = dataItems.begin() + offset;
	state.progress = -1;	//设置完后位置后把state.progress置为-1
}

void ZhPScanDataLoader::onTimer()
{
	auto delay = std::chrono::milliseconds((int)(intervalInMs * state.interval_factor));
	timer.expires_from_now(delay);
	timer.async_wait([this](const asio::error_code& e) {
		if (e.value() == asio::error::operation_aborted) { return; }
		if (!state.pause){
			adjustPos();	//可能发生的位置调整
			if (curPos < dataItems.end()){
				//发送结果
				userHandlers.dataHandler(*curPos);	//用户代码在此线程执行数据处理
				curPos++;
			}
		}
		onTimer();
	});
}

void ZhPScanDataLoader::abort()
{
	if (replayFuture.valid())
	{
		context.stop();
		replayFuture.get();
	}
}


ZhPScanReplayer::ZhPScanReplayer(const persistence::PScanTaskItem& taskItem, const int32_t outputPoints)
	:expectedPoints(outputPoints)
{
	if (band.ParseFromArray(taskItem.bandInfo.data(), taskItem.bandInfo.size()))
	{
		scale = make_unique<FrequencyScale>(band);
		resultHeader.mutable_result_from()->mutable_node_id()->set_value(taskItem.nodeId);
		resultHeader.mutable_result_from()->mutable_device_id()->set_value(taskItem.deviceId);
		resultHeader.set_record_id(taskItem.uuid);
		dataHolder[spectrum::NO_HOLD] = make_shared<SpectrumDataHolder>(spectrum::NO_HOLD);	//默认仅创建当前迹线
		hits = make_unique<SpectrumDataHolder>(spectrum::MAX_HOLD);
	}
}

ZhPScanReplayer::~ZhPScanReplayer()
{

}

void ZhPScanReplayer::preUpdate(const persistence::PScanDataItem& item)
{
	hits->clear();
	if (resultHeader.sequence_number() + 1 == item.seq)
		return;
	//进度发生跳转, 重置
	auto iter = dataHolder.find(spectrum::DataHoldType::MAX_HOLD);
	if (iter != dataHolder.end())
	{
		iter->second->clear();
	}
	iter = dataHolder.find(spectrum::DataHoldType::MIN_HOLD);
	if (iter != dataHolder.end())
	{
		iter->second->clear();
	}
}

void ZhPScanReplayer::update(const persistence::PScanDataItem& item)
{
	for (auto& pr : dataHolder)
	{
		pr.second->loadSweepData(scale->wholeBand(), item.data);
	}

	if (!item.hits.empty())
	{
		hits->loadSweepData(scale->wholeBand(), item.hits);
	}
}

void ZhPScanReplayer::updateHeader(const persistence::PScanDataItem& item)
{
	resultHeader.set_sequence_number(item.seq);
	resultHeader.set_sweep_count(item.sweepCount);
	*resultHeader.mutable_time_span()->mutable_start_time() = UINT64ToTimestamp(item.startTime);
	*resultHeader.mutable_time_span()->mutable_stop_time() = UINT64ToTimestamp(item.stopTime);
	*resultHeader.mutable_device_position() = makePosition(item.longitude, item.latitude, item.altitude);
}

zczh::zhpscan::Result ZhPScanReplayer::makeResult(const persistence::PScanDataItem& item)
{
	zczh::zhpscan::Result result;
	updateHeader(item);
	*result.mutable_header() = resultHeader;
	fillDataHoldResult(result);
	fillHits(result);
	fillDetectResult(result);
	return result;
}

zczh::zhpscan::Result ZhPScanReplayer::onDataItem(const persistence::PScanDataItem& dataItem)
{
	preUpdate(dataItem);
	update(dataItem);
	return makeResult(dataItem);
}

void ZhPScanReplayer::onCmd(std::list<MessageExtractor>& pendingCmd)
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
			case PScanCmdType::ZOOMIN_OPEN:
				onZoominOpen(extractor);
				break;
			case PScanCmdType::ZOOMIN_CLOSE:
				onZoominClose(extractor);
				break;
			case PScanCmdType::DETECT_OPEN:
				onDetectOpen(extractor);
				break;
			case PScanCmdType::DETECT_CLOSE:
				onDetectClose(extractor);
				break;
			default:
				break;
			}
		}
	}
}

void ZhPScanReplayer::onDataHoldOpen(MessageExtractor& extractor)
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

void ZhPScanReplayer::onDataHoldClose(MessageExtractor& extractor)
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

void ZhPScanReplayer::onDataHoldReset(MessageExtractor& extractor)
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

void ZhPScanReplayer::onZoominOpen(MessageExtractor& extractor)
{
	spectrum::FrequencySegment newZoominPart;
	if (!extractor.deserialize(newZoominPart))
		return;
	zoominParam = make_unique<spectrum::FrequencySegment>(newZoominPart);
	scale->onSpanChange(zoominParam->freq_span());
}

void ZhPScanReplayer::onZoominClose(MessageExtractor& extractor)
{
	zoominParam.reset();
}

void ZhPScanReplayer::onDetectOpen(MessageExtractor& extractor)
{
	detection::DetectionParam detectParam;
	if (!extractor.deserialize(detectParam))
		return;
	if (detectParam.has_energy_detect_param())
	{
		if (signalDetector == nullptr)
		{
			signalDetector = make_unique<SignalDetection>(detectParam);
		}
		else
		{
			signalDetector->setDetectionParam(detectParam);
		}
	}
}

void ZhPScanReplayer::onDetectClose(MessageExtractor& extractor)
{
	signalDetector.reset();
}

void ZhPScanReplayer::fillDataHoldResult(zczh::zhpscan::Result& result)
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

void ZhPScanReplayer::fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama)
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

void ZhPScanReplayer::fillHits(zczh::zhpscan::Result& result)
{
	if (hits->segmentCount() == 0)
		return;
	auto panoramaData = hits->getPanorama(expectedPoints);
	*result.mutable_panorama_view()->mutable_over_threshold_hits() = { panoramaData.begin(), panoramaData.end() };
	if (zoominParam != nullptr)
	{
		auto closeshotData = hits->getCloseshot(zoominParam->points(), *scale);
		*result.mutable_closeshot_view()->mutable_over_threshold_hits() = { closeshotData.begin(), closeshotData.end() };
	}
}

void ZhPScanReplayer::signalDetect(const zczh::zhpscan::Result& result)
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
	src = hits->getPanorama(0);	//取得hits值的所有原始数据
	*rawBody->mutable_over_threshold_hits() = { src.begin(), src.end() };
	signalDetector->detect(rawData);
}

void ZhPScanReplayer::fillDetectResult(zczh::zhpscan::Result& result)
{
	if (signalDetector == nullptr)
		return;
	if (hits->segmentCount() == 0)
		return;
	signalDetect(result);
	signalDetector->onCommitDetectedSignal(*result.mutable_signal_list());	//提交检测结果
}