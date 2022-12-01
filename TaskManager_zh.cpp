#include "pch.h"
#include "TaskManager.h"
#include "PushPull.h"
#include "TaskCreator.h"
#include "NodeTask.h"
#include "Logger.h"
#include "node/nodeInternal.pb.h"
#include "AbnormalMsgInfo.h"
#include "TaskRequestContext.h"
#include "ZhTaskFactory.h"

using namespace std;

TaskManager::TaskManager(const ZeromqLinkCfg& sourceCfg, const std::string& path, DeviceRequestFunc deviceRequest)
	:dataSender(make_unique<StreamSource>(sourceCfg))
{
	// ��ʼ���������ķ�����
	TaskDataSendFunc func = [this](MessageBuilder& builder)
	{
		return dataSender->send(builder);
	};
	string dbPath = path + "/node.db";
	taskCreator = make_unique<TaskCreator>(deviceRequest, func);
	taskCreator->addFactory(make_shared<ZhTaskFactory>(dbPath));
}

TaskManager::~TaskManager()
{
}

MessageBuilder TaskManager::onRemoteRequest(MessageExtractor&& extractor)
{
	TaskRequestContext context(std::move(extractor));
	if (context.parsed())
	{
		auto type = context.taskCmdType();
		if (type == T_START)
		{
			taskStart(context);
		}
		else if (type == T_STOP)
		{
			taskStop(context);
		}
		else if (type == T_MODIFY)
		{
			taskCmd(context);
		}
		else
		{
			context.setErrorCode(ERR_PARAM);
		}
	}
	return context.makeReply();
}

void TaskManager::taskStart(TaskRequestContext& context)
{
	TaskType type;
	context.messageExtractor().deserialize(type);
	auto newTask = taskCreator->createTask(context.cmdHeader(), type);
	if (newTask == nullptr)
	{
		context.setErrorCode(ERR_NODE_TASK_ASSIGN);	//������Ӧ�Ĵ�����
		return;
	}
	newTask->config(context);//��������
	if (!context.isGood())
		return;
	auto error = newTask->start();	//���óɹ�������
	if (error == ERR_NONE)
	{
		context.setDeviceId(newTask->deviceId().value());
		tasks[context.getKey()] = newTask;	//���������б�
	}
	context.setErrorCode(error);
	LOG("task start :{}", context.toString());
	LOG("cur task count : {}", tasks.size());
}

void TaskManager::taskCmd(TaskRequestContext& context)
{
	auto iter = tasks.find(context.getKey());	//����nodeDevice����task
	if (iter != tasks.end())
	{
		iter->second->onCmd(context);	//������ȥִ������
	}
	else
	{
		context.setErrorCode(ERR_TASK_NOT_FOUND);
	}
}

void TaskManager::taskStop(TaskRequestContext& context)
{
	auto iter = tasks.find(context.getKey());

	if (iter != tasks.end())
	{
		iter->second->stop();	//����ֹͣ��һ���ֹͣ��������ɺ�̨�̣߳����ܻ�����ֱ��������������߳��˳�

		tasks.erase(iter);	//�Ӽ��������������
		context.setErrorCode(ERR_NONE);
	}
	else
	{
		context.setErrorCode(ERR_TASK_NOT_FOUND);
	}
	LOG("task stop :{}", context.toString());
	LOG("cur task count : {}", tasks.size());
}

void TaskManager::taskAbortAll()
{
	for (auto t : tasks)
	{
		t.second->stop();
	}
	tasks.clear();
}

void TaskManager::onTimer(AbnormalMsgInfoClient& abnormalMsgInfo)
{
	auto record = abnormalMsgInfo.receivedRecord();
	for (auto iter = tasks.begin(); iter != tasks.end();)
	{
		if(iter->second->canErase(record))
		{
			LOG("task {:x} abnormal deleted, cur task count:{}", iter->second->taskId().value(), tasks.size() - 1);
			iter->second->stop();
			iter = tasks.erase(iter);	//��������
		}
		else
		{
			iter++;
		}
	}
}

void TaskManager::updateInfo(NodeInfo& info) const
{
	auto* ts = info.mutable_tasks();
	for (auto& t : tasks)
	{
		*ts->Add() = t.second->getSummary();
	}
}
	 
