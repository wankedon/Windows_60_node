#pragma once
#include "node/zczh/ZhDirection.pb.h"
#include "ZMsgWrap.h"
#include "ZhSensor.h"

class DFTargetProcessor
{
public:
	DFTargetProcessor();
	~DFTargetProcessor();
public:
	void onRawTargetData(const std::vector<senDFTarget>& target);
	void fillResult(zczh::zhdirection::Result& result);

private:
	std::mutex mtx;
	std::unique_ptr<std::list<senDFTarget>> buffer;
};