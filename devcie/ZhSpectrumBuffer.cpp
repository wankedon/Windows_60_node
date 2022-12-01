#include "pch.h"
#include "ZhSpectrumBuffer.h"
namespace ZBDevice
{
	ZhSpectrumBuffer::ZhSpectrumBuffer()
	{
	}

	ZhSpectrumBuffer::~ZhSpectrumBuffer()
	{
	}

	void ZhSpectrumBuffer::input(const SpectrumData& spectrumData)
	{
		if (spectrumData.amplitude_size() == 0)
			return;
		std::lock_guard<std::mutex> lg(lock);
		if (m_buffer == nullptr)
			m_buffer = std::make_unique<std::list<senVecFloat>>();
		senVecFloat senSpretrum;
		senSpretrum.assign(spectrumData.amplitude().begin(), spectrumData.amplitude().end());
		m_buffer->emplace_back(senSpretrum);
	}

	std::vector<senVecFloat> ZhSpectrumBuffer::output()
	{
		std::vector<senVecFloat> spretrum;
		if (m_buffer == nullptr)
			return spretrum;
		std::lock_guard<std::mutex> lg(lock);
		std::unique_ptr<std::list<senVecFloat>> result;
		result.swap(m_buffer);
		if (!result->empty())
		{
			spretrum.assign(result->begin(), result->end());
		}
		return spretrum;
	}
}