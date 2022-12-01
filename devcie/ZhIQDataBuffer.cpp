#include "pch.h"
#include "ZhIQDataBuffer.h"

namespace ZBDevice
{
	ZhIQDataBuffer::ZhIQDataBuffer()
	{
	}

	ZhIQDataBuffer::~ZhIQDataBuffer()
	{
		m_buffer.reset();
	}

	void ZhIQDataBuffer::input(const senVecFloat& rawIQData,const IQDataDescribe iqDataDescribe)
	{
		if (rawIQData.size() == 0)
			return;
		m_iqDataDescribe = iqDataDescribe;
		std::lock_guard<std::mutex> lg(lock);
		if (m_buffer == nullptr)
			m_buffer = std::make_unique<std::list<senVecFloat>>();
		for (size_t i = 0; i < rawIQData.size() / FRAME_SIZE; i++)
		{
			senVecFloat oneFrame(FRAME_SIZE);
			memcpy(oneFrame.data(), rawIQData.data() + i * FRAME_SIZE, FRAME_SIZE * sizeof(float));
			m_buffer->emplace_back(oneFrame);
		}
	}

	std::pair<senVecFloat, IQDataDescribe> ZhIQDataBuffer::output()
	{
		std::lock_guard<std::mutex> lg(lock);
		senVecFloat oneFrame;
		if (m_buffer->size())
		{
			oneFrame = m_buffer->front();
			m_buffer->pop_front();
		}
		return std::make_pair(oneFrame, m_iqDataDescribe);
	}
}