#pragma once
#include "ThreadWrap.h"

namespace ZBDevice
{
	enum senThreadType
	{
		DEVSTATUS = 0,
		PSCAN = 1,
		DF_SPECTRUM = 2,
		DF_TARGET = 3,
		IQ_ACQUIRE = 4,
		IF_SPECTRUM = 5,
		IF_IQ = 6,
		IF_SIGNAL = 7,
		IQ_SEND = 8,
	};

	class ThreadLoop
	{
	public:
		ThreadLoop(std::function<void()> startFunc)
			:m_isRuning(false)
		{
			std::function<void()> stopFunc = [this]() {m_isRuning = false; };
			m_Thread.start(startFunc, &stopFunc);
			m_isRuning = true;
		}

		~ThreadLoop()
		{
			m_Thread.stop();
		}

		void stop()
		{
			m_Thread.stop();
		}
	public:
		bool isRuning() { return m_isRuning; }
	private:
		bool m_isRuning;
		ThreadWrap m_Thread;
	};
}