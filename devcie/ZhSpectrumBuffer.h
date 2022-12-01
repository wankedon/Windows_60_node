#pragma once
#include "ZhSensorDef.h"
#include "node/module/spatialSpectrumInternal.grpc.pb.h"
#include "node/module/spatialSpectrumInternal.pb.h"

namespace ZBDevice
{
	using namespace zb::dcts::node::module::spatailSpectrum::internal;
	class ZhSpectrumBuffer
	{
	public:
		ZhSpectrumBuffer();
		~ZhSpectrumBuffer();
	public:
		void input(const SpectrumData& spectrumData);
		std::vector<senVecFloat> output();
	private:
		mutable std::mutex lock;
		std::unique_ptr<std::list<senVecFloat>> m_buffer;
	};
}