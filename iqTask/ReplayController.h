#pragma once

#include <asio.hpp>
#include "node/replay.pb.h"

template <class IDType>
class ReplayController
{
	using DataItemHandler = std::function<void(int idx, const IDType& dataItemId)>;
	using ThreadEventHandler = std::function<void()>;
public:
	ReplayController(int interval, std::vector<IDType>&& ids)
		:intervalInMs(interval),
		dataItemIds(std::move(ids)),
		timer(context)
	{
		curPos = dataItemIds.begin();
		state.progress = -1;
		state.interval_factor = 1;
		state.pause = false;
	}
	~ReplayController()
	{
		abort();
	}
public:
	bool start(DataItemHandler handler)
	{
		if (dataItemIds.empty())
			return false;
		userHandlers.dataHandler = handler;
		replayFuture = std::async([this]() {
			if (userHandlers.threadEntranceHandler != nullptr)
			{
				userHandlers.threadEntranceHandler();
			}
			onTimer();
			context.run();
			if (userHandlers.threadQuitHandler != nullptr)
			{
				userHandlers.threadQuitHandler();
			}
			});
		return true;
	}
	void onAdjust(const node::replay::AdjustOption& adjustOption)
	{
		if (adjustOption.has_progress())
		{
			auto val = adjustOption.progress().value();
			if (val <= 1 && val >= 0)
			{
				state.progress = val;
			}
		}
		if (adjustOption.has_interval_factor())
		{
			auto val = adjustOption.interval_factor().value();
			if (val >= 0)
			{
				state.interval_factor = val;
			}
		}
		if (adjustOption.has_pause_resume())
		{
			state.pause = adjustOption.pause_resume().value();
		}
	}
	void abort()
	{
		if (replayFuture.valid())
		{
			context.stop();
			replayFuture.get();
		}
	}
public:
	void setThreadEventHandler(ThreadEventHandler entrance, ThreadEventHandler quit)
	{
		userHandlers.threadEntranceHandler = entrance;
		userHandlers.threadQuitHandler = quit;
	}
private:
	void onTimer()
	{
		auto delay = std::chrono::milliseconds((int)(intervalInMs * state.interval_factor));
		timer.expires_from_now(delay);
		timer.async_wait([this](const asio::error_code& e) {
			if (e.value() == asio::error::operation_aborted) { return; }
			if (!state.pause) {
				adjustPos();	//���ܷ�����λ�õ���
				if (curPos < dataItemIds.end()) {
					//���ͽ��
					userHandlers.dataHandler(curPos - dataItemIds.begin(), *curPos);	//�û������ڴ��߳�ִ�����ݴ���
					curPos++;
				}
			}
			onTimer();
			});
	}
	void adjustPos()
	{
		if (state.progress < 0)	//�涨С����Ϊû�е������ȵ�����
			return;
		auto offset = static_cast<int>(dataItemIds.size() * state.progress);
		curPos = dataItemIds.begin() + offset;
		state.progress = -1;	//�������λ�ú��state.progress��Ϊ-1
	}
private:
	struct State
	{
		std::atomic<double> progress;
		std::atomic<float> interval_factor;
		std::atomic_bool pause;
	};
	struct UserHandler
	{
		DataItemHandler dataHandler;
		ThreadEventHandler threadEntranceHandler;
		ThreadEventHandler threadQuitHandler;
	};
private:
	const int intervalInMs;
	typename std::vector<IDType>::iterator curPos;
	std::vector<IDType> dataItemIds;
	std::future<void> replayFuture;
	asio::io_context context;
	asio::steady_timer timer;
	State state;
	UserHandler userHandlers;
};