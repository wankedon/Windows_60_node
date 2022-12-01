#pragma once
#include "node/zczh/ZhIFAnalysis.pb.h"
#include "ZMsgWrap.h"
#include "ZhSensor.h"

class IFSignalProcessor
{
public:
	IFSignalProcessor();
	~IFSignalProcessor();
public:
	void onRawTargetData(const std::vector<senSignal>& target);
	void fillResult(zczh::zhIFAnalysis::Result& result);

private:
	std::mutex mtx;
	std::unique_ptr<std::list<senSignal>> buffer;
};