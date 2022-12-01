#pragma once
#include "ZhSensorDef.h"
namespace ZBDevice
{
	class ZhIQDataBuffer
	{
	public:
		ZhIQDataBuffer();
		~ZhIQDataBuffer();
	public:
		void input(const senVecFloat& iqData, const IQDataDescribe iqDataDescribe);
		std::pair<senVecFloat, IQDataDescribe> output();
	private:
		mutable std::mutex lock;
		std::unique_ptr<std::list<senVecFloat>> m_buffer;
		IQDataDescribe m_iqDataDescribe;
		const int FRAME_SIZE = 2048;
	};
}