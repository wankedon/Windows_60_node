#pragma once

#include <asio.hpp>
#include "ZhPScanPersistence.h"
#include "StreamConnect.h"
#include "node/replay.pb.h"
#include "ZMsgWrap.h"

class ZhPScanDataLoader
{
	using DataItemHandler = std::function<void(const persistence::PScanDataItem&)>;
	using ThreadEventHandler = std::function<void()>;
public:
	ZhPScanDataLoader(int intervalInMs, const persistence::PScanTaskItem& taskItem, std::vector<persistence::PScanDataItem>&& dataItems);
	~ZhPScanDataLoader();
public:
	bool start(DataItemHandler handler);
	void onAdjust(const replay::AdjustOption& option);
	void abort();
public:
	void setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit);
private:
	void onTimer();
	void adjustPos();
private:
	struct State
	{
		std::atomic<double> progress;
		std::atomic<float> interval_factor;
		std::atomic_bool pause;
	};
	struct UserHandler
	{
		DataItemHandler dataHandler;
		ThreadEventHandler threadEntranceHandler;
		ThreadEventHandler threadQuitHandler;
	};
private:
	const persistence::PScanTaskItem taskItem;
	const int intervalInMs;
	std::vector<persistence::PScanDataItem>::iterator curPos;
	std::vector<persistence::PScanDataItem> dataItems;
	std::future<void> replayFuture;
	asio::io_context context;
	asio::steady_timer timer;
	State state;
	UserHandler userHandlers;
};


class SpectrumDataHolder;
class FrequencyScale;
class SignalDetection;
class ZhPScanReplayer
{
public:
	ZhPScanReplayer(const persistence::PScanTaskItem& taskItem, const int32_t outputPoints);
	~ZhPScanReplayer();
public:
	void onCmd(std::list<MessageExtractor>& pendingCmd);
	zczh::zhpscan::Result onDataItem(const persistence::PScanDataItem&);
private:
	void onDataHoldOpen(MessageExtractor& extractor);
	void onDataHoldClose(MessageExtractor& extractor);
	void onDataHoldReset(MessageExtractor& extractor);
	void onZoominOpen(MessageExtractor& extractor);
	void onZoominClose(MessageExtractor& extractor);
	void onDetectOpen(MessageExtractor& extractor);
	void onDetectClose(MessageExtractor& extractor);
	void preUpdate(const persistence::PScanDataItem&);
	void update(const persistence::PScanDataItem&);
	zczh::zhpscan::Result makeResult(const persistence::PScanDataItem&);
	void updateHeader(const persistence::PScanDataItem&);
	void fillDataHoldResult(zczh::zhpscan::Result& result);
	void fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama);
	void fillHits(zczh::zhpscan::Result& result);
	void fillDetectResult(zczh::zhpscan::Result& result);
	void signalDetect(const zczh::zhpscan::Result& result);
private:
	const int32_t expectedPoints;
	spectrum::FrequencyBand band;
	spectrum::Header resultHeader;
	std::unique_ptr<spectrum::FrequencySegment> zoominParam;
	std::unique_ptr<FrequencyScale> scale;								//近景频率范围控制
	std::map<spectrum::DataHoldType, std::shared_ptr<SpectrumDataHolder>> dataHolder;	//数据持有
	std::unique_ptr<SpectrumDataHolder> hits;
	std::unique_ptr<SignalDetection> signalDetector;
};