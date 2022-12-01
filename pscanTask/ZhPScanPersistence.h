/**
 * @file PScanRecorder.h
 * @brief 频谱扫描数据记录
 * @author 装备事业部软件组 杨青 
 * @version 0.1
 * @date 2020-12-09
 * 
 * @copyright Copyright (c) 2020  中国电子科技集团公司第四十一研究所
 * 
 */
#pragma once

#include <sqlite_orm/sqlite_orm.h>
#include <mutex>
#include "node/zczh/ZhPScan.pb.h"
#include "dcts.pb.h"
#include "Miscellaneous.h"
#include <functional>

using namespace sqlite_orm;

namespace persistence
{
    struct PScanDataItem {
        int seq;
        std::string datauuid;
        std::string taskuuid;
        double longitude;
        double latitude;
        double altitude;
        uint64_t startTime;
        uint64_t stopTime;
        uint32_t sweepCount;
        std::vector<char> data;
        std::vector<char> hits;
    };

    struct PScanTaskItem {
        int seq;
        std::string uuid;
        uint32_t nodeId;
        uint32_t deviceId;
        uint32_t points;
        uint32_t sweepCount;
        uint32_t dataItemCount;
        uint64_t startTime;
        uint64_t stopTime;
        int64 startFreq;
        int64 stopFreq;
        std::vector<char> bandInfo;
    };

    inline auto initPScanTaskStorage(const std::string& path)
    {
        return make_storage(
            path,
            make_table("PScanTask",
                make_column("seq",           &PScanTaskItem::seq, primary_key()),
                make_column("uuid",         &PScanTaskItem::uuid),
                make_column("nodeId",       &PScanTaskItem::nodeId),
                make_column("deviceId",     &PScanTaskItem::deviceId),
                make_column("points",       &PScanTaskItem::points),
                make_column("sweepCount",   &PScanTaskItem::sweepCount),
                make_column("dataItemCount",&PScanTaskItem::dataItemCount),
                make_column("startTime",    &PScanTaskItem::startTime),
                make_column("stopTime",     &PScanTaskItem::stopTime),
                make_column("startFreq",    &PScanTaskItem::startFreq),
                make_column("stopFreq",     &PScanTaskItem::stopFreq),
                make_column("bandInfo",     &PScanTaskItem::bandInfo)
            ),
            make_table("PScanData",
                make_column("seq",           &PScanDataItem::seq, primary_key()),
                make_column("datauuid",         &PScanDataItem::datauuid),
                make_column("taskuuid",          &PScanDataItem::taskuuid),
                make_column("longitude",    &PScanDataItem::longitude),
                make_column("latitude",     &PScanDataItem::latitude),
                make_column("altitude",     &PScanDataItem::altitude),
                make_column("startTime",    &PScanDataItem::startTime),
                make_column("stopTime",     &PScanDataItem::stopTime),
                make_column("sweepCount",   &PScanDataItem::sweepCount),
                make_column("data",         &PScanDataItem::data),
                make_column("hits",         &PScanDataItem::hits)
            )
        );
    }

    using PScanDataItemHandler = std::function<void(const PScanDataItem&)>;

    class PScanDBAccessor
    {
    public:
        PScanDBAccessor(const std::string& path, std::mutex& dblock)
            :storage(initPScanTaskStorage(path)),
            lock(dblock)
        {
            storage.sync_schema();
        }
    public:
        int insertTask(const PScanTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(taskItem);
        }

        void updateTask(const PScanTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            storage.update(taskItem);
        }

        int insertData(const PScanDataItem& dataItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(dataItem);
        }

        std::vector<std::string> findMatchTask(int64 startFreq, int64 stopFreq, uint32_t points, uint64_t startTime = 0, uint64_t stopTime = 0)
        {
            std::vector<std::string> taskuuid;
            std::lock_guard<std::mutex> lg(lock);
            if (startTime == 0 || stopTime == 0)
            {
                auto taskItems = storage.iterate<PScanTaskItem>(
                    where(
                        c(&PScanTaskItem::startFreq) == startFreq and
                        c(&PScanTaskItem::stopFreq) == stopFreq and
                        c(&PScanTaskItem::points) == points
                        )
                    );
                for (auto& item : taskItems)
                {
                    taskuuid.push_back(item.uuid);
                }
            }
            else
            {
                auto taskItems = storage.iterate<PScanTaskItem>(
                    where(
                        c(&PScanTaskItem::startFreq) == startFreq and
                        c(&PScanTaskItem::stopFreq) == stopFreq and
                        c(&PScanTaskItem::points) == points and
                        c(&PScanTaskItem::startTime) > startTime and
                        c(&PScanTaskItem::stopTime) < stopTime
                        )
                    );
                for (auto& item : taskItems)
                {
                    taskuuid.push_back(item.uuid);
                }
            }
            return taskuuid;
        }

        std::unique_ptr<PScanTaskItem> getTaskItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            auto all = storage.get_all<PScanTaskItem>(where(c(&PScanTaskItem::uuid) == taskuuid));
            if (all.empty())
                return nullptr;
            else
                return std::make_unique<PScanTaskItem>(all[0]);
        }

        std::vector<PScanTaskItem> findTask(const spectrum::FrequencySpan* freqSpan, const TimeSpan* timeSpan)
        {
            if (freqSpan && timeSpan)
            {
                auto freqStart = static_cast<int64_t>(freqSpan->start_freq());
                auto freqStop = static_cast<int64_t>(freqSpan->stop_freq());
                auto timeStart = timestampToUINT64(timeSpan->start_time());
                auto timeStop = timestampToUINT64(timeSpan->stop_time());
                return storage.get_all<PScanTaskItem>(where(
                    c(&PScanTaskItem::startFreq) > freqStart and
                    c(&PScanTaskItem::stopFreq) < freqStop and
                    c(&PScanTaskItem::startTime) > timeStart and
                    c(&PScanTaskItem::stopTime) < timeStop)
                );   
            }
            else
            {
                if (freqSpan)
                {
                    auto freqStart = static_cast<int64_t>(freqSpan->start_freq());
                    auto freqStop = static_cast<int64_t>(freqSpan->stop_freq());
                    return storage.get_all<PScanTaskItem>(where(
                        c(&PScanTaskItem::startFreq) > freqStart and
                        c(&PScanTaskItem::stopFreq) < freqStop)
                        );
                }
                else
                {
                    auto timeStart = timestampToUINT64(timeSpan->start_time());
                    auto timeStop = timestampToUINT64(timeSpan->stop_time());
                    return storage.get_all<PScanTaskItem>(where(
                        c(&PScanTaskItem::startTime) > timeStart and
                        c(&PScanTaskItem::stopTime) < timeStop)
                        );
                }
            }
        }

        static void toTaskDescriptor(const PScanTaskItem& dbItem, zczh::zhpscan::RecordDescriptor& desc)
        {
            NodeDevice nd;
            nd.mutable_node_id()->set_value(dbItem.nodeId);
            nd.mutable_device_id()->set_value(dbItem.nodeId);
            desc.set_record_id(dbItem.uuid);
            *desc.mutable_from() = nd;
            *desc.mutable_time_span()->mutable_start_time() = UINT64ToTimestamp(dbItem.startTime);
            *desc.mutable_time_span()->mutable_stop_time() = UINT64ToTimestamp(dbItem.stopTime);
            desc.mutable_freq_span()->set_start_freq(dbItem.startFreq);
            desc.mutable_freq_span()->set_stop_freq(dbItem.stopFreq);
            desc.set_sweep_points(dbItem.points);
            desc.set_sweep_count(dbItem.sweepCount);
            desc.set_record_count(dbItem.dataItemCount);
        }

        std::vector<PScanDataItem> getTaskAllDataItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.get_all<PScanDataItem>(where(
                c(&PScanDataItem::taskuuid) == taskuuid),
                order_by(&PScanDataItem::seq)
                );
            return std::vector<PScanDataItem>();
        }

        int useDataInSpan(const std::vector<std::string>& taskuuId, const zb::dcts::TimeSpan& span, int maxRecordCount, PScanDataItemHandler handler)
        {
            std::lock_guard<std::mutex> lg(lock);
            int loadedCount = 0;
            uint64_t startTime = timestampToUINT64(span.start_time());
            uint64_t stopTime = timestampToUINT64(span.stop_time());
            uint64_t timeStep = (stopTime - startTime) / maxRecordCount;
            for (int i = 0; i < maxRecordCount; i++)
            {
                auto dataItems = storage.iterate<PScanDataItem>(
                    where(
                        in(&PScanDataItem::taskuuid, taskuuId) and
                        c(&PScanDataItem::startTime) >= startTime and
                        c(&PScanDataItem::stopTime)  <= (startTime + timeStep)
                        )
                    );
                startTime += timeStep;
                if (!dataItems.empty())
                {
                    for (auto& dataItem : dataItems)
                    {
                        if (!dataItem.data.empty())
                        {
                            handler(dataItem);
                            loadedCount++;
                            break;  //只加载一条
                        }
                    }
                }
            }
            return loadedCount;
        }
    private:
        using Storage = decltype(initPScanTaskStorage(""));
        Storage storage;
        std::mutex& lock;
    };
}


