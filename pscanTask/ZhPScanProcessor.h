#pragma once

#include "node/zczh/ZhPScan.pb.h"
#include "ZMsgWrap.h"
#include "ZhPScanRecorder.h"

class SpectrumDataHolder;
class FrequencyScale;
class ThresholdManager;
class SignalDetection;

class ZhPScanProcessor
{
public:
	ZhPScanProcessor(const spectrum::FrequencyBand& band, int32_t outputPoints, std::shared_ptr<persistence::PScanDBAccessor> dbAccessor);
	~ZhPScanProcessor(void);
public:
	void onTaskBegin(const NodeDevice& nd);
	void onTaskEnd(const zczh::zhpscan::OperationStatus& status);
	void onSegUpdate(int segIdx, const float* data, size_t count);
	void onSweepEnd();
	void onSweepBegin(std::list<MessageExtractor>& pendingCmd);
	void fillResult(zczh::zhpscan::Result& result);
	void fillStatus(zczh::zhpscan::OperationStatus& status);
private:
	void onDataHoldOpen(MessageExtractor& extractor);
	void onDataHoldClose(MessageExtractor& extractor);
	void onDataHoldReset(MessageExtractor& extractor);
	void onDetectOpen(MessageExtractor& extractor);
	void onDetectClose(MessageExtractor& extractor);
	void onZoominOpen(MessageExtractor& extractor);
	void onZoominClose(MessageExtractor& extractor);
	void onRecordOpen(MessageExtractor& extractor);
	void onRecordClose(MessageExtractor& extractor);
	void onDetailKeeping(MessageExtractor& extractor);
	void fillDataHoldResult(zczh::zhpscan::Result& result);
	void fillDetectResult(zczh::zhpscan::Result& result);
	void fillOneTrace(spectrum::DataHoldType type, google::protobuf::RepeatedField<float>* protoRepeatedField, bool isPanorama);
	persistence::PScanTaskItem makePScanTaskItem(const NodeDevice& nd);
	void signalDetect(const zczh::zhpscan::Result& result);
private:
	const int32_t expectedPoints;
	std::unique_ptr<spectrum::FrequencySegment> zoominParam;
	std::unique_ptr<FrequencyScale> scale;								//近景频率范围控制
	std::map<spectrum::DataHoldType, std::shared_ptr<SpectrumDataHolder>> dataHolder;	//数据持有
	std::unique_ptr<ThresholdManager> thresholdManager;						//门限
	std::shared_ptr<persistence::PScanDBAccessor> dbAccessor;
	std::unique_ptr<PScanRecorder> recorder;
	std::unique_ptr<SignalDetection> signalDetector;
};