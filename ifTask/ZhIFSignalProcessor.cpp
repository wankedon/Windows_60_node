#include "pch.h"
#include "ZhIFSignalProcessor.h"

using namespace std;
using namespace zb::dcts::node::modulation;

IFSignalProcessor::IFSignalProcessor()
{

}

IFSignalProcessor::~IFSignalProcessor()
{

}

void IFSignalProcessor::onRawTargetData(const std::vector<senSignal>& data)
{
	std::lock_guard<mutex> lg(mtx);
	if (buffer == nullptr)
		buffer = std::make_unique<std::list<senSignal>>();
	buffer->insert(buffer->begin(), data.begin(), data.end());
}

void IFSignalProcessor::fillResult(zczh::zhIFAnalysis::Result& result)
{
	std::lock_guard<mutex> lg(mtx);
	std::unique_ptr<std::list<senSignal>> targets;
	targets.swap(buffer);
	if (targets->size() == 0)
		return;
	//只将第一个目标发送到客户端
	//zczh::zhIFAnalysis::SignalDescribe signal;
	//auto sig = targets->front();
	//signal.mutable_signal_band()->set_center_frequency(sig.center_frequency);
	//signal.mutable_signal_band()->set_band_width(sig.band_width);
	//signal.set_modulation_type(ModType(sig.modulation_type));
	//signal.set_amplitude(sig.amplitude);
	//*result.mutable_signal_describe() = signal;
}