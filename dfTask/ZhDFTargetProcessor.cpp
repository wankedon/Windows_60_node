#include "pch.h"
#include "ZhDFTargetProcessor.h"

using namespace std;

DFTargetProcessor::DFTargetProcessor()
{

}

DFTargetProcessor::~DFTargetProcessor()
{

}

void DFTargetProcessor::onRawTargetData(const std::vector<senDFTarget>& data)
{
	std::lock_guard<mutex> lg(mtx);
	if (buffer == nullptr)
		buffer = std::make_unique<std::list<senDFTarget>>();
	buffer->insert(buffer->begin(), data.begin(), data.end());
}

void DFTargetProcessor::fillResult(zczh::zhdirection::Result& result)
{
	std::lock_guard<mutex> lg(mtx);
	std::unique_ptr<std::list<senDFTarget>> targets;
	targets.swap(buffer);
	if (targets->size() == 0)
		return;
	//只将第一个目标发送到客户端 接口定义中只有一个目标
	zczh::zhdirection::TargetDirection target;
	auto tar = targets->front();
	target.set_center_frequency(tar.center_frequency);
	target.set_direction(tar.direction);
	target.set_confidence(tar.confidence);
	target.set_amplitude(tar.amplitude);
	*result.mutable_target_detection() = target;
}