/**
 * @file AnalogDemodPersistence.h
 * @brief 模拟解调数据记录
 * @author 装备事业部软件组 杨青 
 * @version 0.1
 * @date 2022-6-29
 * 
 * @copyright Copyright (c) 2022  中国电子科技集团公司第四十一研究所
 * 
 */
#pragma once

#include <sqlite_orm/sqlite_orm.h>
#include <mutex>
#include "node/zczh/ZhDirection.pb.h"
#include "Miscellaneous.h"
#include <functional>

using namespace sqlite_orm;

namespace persistence
{
    struct DFDataItem {
        int seq;
        std::string datauuid;
        std::string taskuuid;
        double longitude;
        double latitude;
        double altitude;
        uint64_t startTime;
        uint64_t stopTime;
        uint32_t sweepCount;
        std::vector<char> spectrum;
        std::vector<char> hits;
        std::vector<char> target;
    };

    struct DFTaskItem {
        int seq;
        std::string uuid;
        uint32_t nodeId;
        uint32_t deviceId;
        uint32_t points;
        uint32_t sweepCount;
        uint32_t dataItemCount;
        uint64_t startTime;
        uint64_t stopTime;
        int64 centerFreq;
        int64 bandWidth;
    };

    inline auto initDFTaskStorage(const std::string& path)
    {
        return make_storage(
            path,
            make_table("DFTask",
                make_column("seq",           &DFTaskItem::seq, primary_key()),
                make_column("uuid",         &DFTaskItem::uuid),
                make_column("nodeId",       &DFTaskItem::nodeId),
                make_column("deviceId",     &DFTaskItem::deviceId),
                make_column("points",       &DFTaskItem::points),
                make_column("sweepCount",   &DFTaskItem::sweepCount),
                make_column("dataItemCount",&DFTaskItem::dataItemCount),
                make_column("startTime",    &DFTaskItem::startTime),
                make_column("stopTime",     &DFTaskItem::stopTime),
                make_column("centerFreq",    &DFTaskItem::centerFreq),
                make_column("bandWidth",     &DFTaskItem::bandWidth)
            ),
            make_table("DFData",
                make_column("seq",           &DFDataItem::seq, primary_key()),
                make_column("datauuid",      &DFDataItem::datauuid),
                make_column("taskuuid",      &DFDataItem::taskuuid),
                make_column("longitude",    &DFDataItem::longitude),
                make_column("latitude",     &DFDataItem::latitude),
                make_column("altitude",     &DFDataItem::altitude),
                make_column("startTime",    &DFDataItem::startTime),
                make_column("stopTime",     &DFDataItem::stopTime),
                make_column("sweepCount",   &DFDataItem::sweepCount),
                make_column("spectrum",     &DFDataItem::spectrum),
                make_column("hits",         &DFDataItem::hits),
                make_column("target",        &DFDataItem::target)
            )
        );
    }

    using DFDataItemHandler = std::function<void(const DFDataItem&)>;

    class DFDBAccessor
    {
    public:
        DFDBAccessor(const std::string& path, std::mutex& dblock)
            :storage(initDFTaskStorage(path)),
            lock(dblock)
        {
            storage.sync_schema();
        }
    public:
        int insertTask(const DFTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(taskItem);
        }

        void updateTask(const DFTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            storage.update(taskItem);
        }

        int insertData(const DFDataItem& dataItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(dataItem);
        }

        std::vector<std::string> findMatchTask(int64 centerFreq, int64 bandWidth, uint32_t points, uint64_t startTime = 0, uint64_t stopTime = 0)
        {
            std::vector<std::string> taskuuid;
            std::lock_guard<std::mutex> lg(lock);
            if (startTime == 0 || stopTime == 0)
            {
                auto taskItems = storage.iterate<DFTaskItem>(
                    where(
                        c(&DFTaskItem::centerFreq) == centerFreq and
                        c(&DFTaskItem::bandWidth) == bandWidth and
                        c(&DFTaskItem::points) == points
                        )
                    );
                for (auto& item : taskItems)
                {
                    taskuuid.push_back(item.uuid);
                }
            }
            else
            {
                auto taskItems = storage.iterate<DFTaskItem>(
                    where(
                        c(&DFTaskItem::centerFreq) == centerFreq and
                        c(&DFTaskItem::bandWidth) == bandWidth and
                        c(&DFTaskItem::points) == points and
                        c(&DFTaskItem::startTime) > startTime and
                        c(&DFTaskItem::stopTime) < stopTime
                        )
                    );
                for (auto& item : taskItems)
                {
                    taskuuid.push_back(item.uuid);
                }
            }
            return taskuuid;
        }

        std::unique_ptr<DFTaskItem> getTaskItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            auto all = storage.get_all<DFTaskItem>(where(c(&DFTaskItem::uuid) == taskuuid));
            if (all.empty())
                return nullptr;
            else
                return std::make_unique<DFTaskItem>(all[0]);
        }

        std::vector<DFTaskItem> findTask(const spectrum::SignalBand* band, const TimeSpan* timeSpan)
        {
            if (band && timeSpan)
            {
                auto center = static_cast<int64_t>(band->center_frequency());
                auto bandwidth = static_cast<int64_t>(band->band_width());
                auto timeStart = timestampToUINT64(timeSpan->start_time());
                auto timeStop = timestampToUINT64(timeSpan->stop_time());
                return storage.get_all<DFTaskItem>(where(
                    c(&DFTaskItem::centerFreq) == center and
                    c(&DFTaskItem::bandWidth) == bandwidth and
                    c(&DFTaskItem::startTime) > timeStart and
                    c(&DFTaskItem::stopTime) < timeStop)
                );   
            }
            else
            {
                if (band)
                {
                    auto center = static_cast<int64_t>(band->center_frequency());
                    auto bandwidth = static_cast<int64_t>(band->band_width());
                    return storage.get_all<DFTaskItem>(where(
                        c(&DFTaskItem::centerFreq) == center and
                        c(&DFTaskItem::bandWidth) == bandwidth)
                        );
                }
                else
                {
                    auto timeStart = timestampToUINT64(timeSpan->start_time());
                    auto timeStop = timestampToUINT64(timeSpan->stop_time());
                    return storage.get_all<DFTaskItem>(where(
                        c(&DFTaskItem::startTime) > timeStart and
                        c(&DFTaskItem::stopTime) < timeStop)
                        );
                }
            }
        }

        static void toTaskDescriptor(const DFTaskItem& dbItem, zczh::zhdirection::RecordDescriptor& desc)
        {
            NodeDevice nd;
            nd.mutable_node_id()->set_value(dbItem.nodeId);
            nd.mutable_device_id()->set_value(dbItem.nodeId);
            desc.set_record_id(dbItem.uuid);
            *desc.mutable_from() = nd;
            *desc.mutable_time_span()->mutable_start_time() = UINT64ToTimestamp(dbItem.startTime);
            *desc.mutable_time_span()->mutable_stop_time() = UINT64ToTimestamp(dbItem.stopTime);
            desc.mutable_analysis_band()->set_center_frequency(dbItem.centerFreq);
            desc.mutable_analysis_band()->set_band_width(dbItem.bandWidth);
            desc.set_record_count(dbItem.dataItemCount);
        }

        std::vector<DFDataItem> getTaskAllDataItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.get_all<DFDataItem>(where(
                c(&DFDataItem::taskuuid) == taskuuid),
                order_by(&DFDataItem::seq)
                );
            return std::vector<DFDataItem>();
        }

        int useDataInSpan(const std::vector<std::string>& taskuuId, const zb::dcts::TimeSpan& span, int maxRecordCount, DFDataItemHandler handler)
        {
            std::lock_guard<std::mutex> lg(lock);
            int loadedCount = 0;
            uint64_t startTime = timestampToUINT64(span.start_time());
            uint64_t stopTime = timestampToUINT64(span.stop_time());
            uint64_t timeStep = (stopTime - startTime) / maxRecordCount;
            for (int i = 0; i < maxRecordCount; i++)
            {
                auto dataItems = storage.iterate<DFDataItem>(
                    where(
                        in(&DFDataItem::taskuuid, taskuuId) and
                        c(&DFDataItem::startTime) >= startTime and
                        c(&DFDataItem::stopTime)  <= (startTime + timeStep)
                        )
                    );
                startTime += timeStep;
                for(auto& item : dataItems)
                {
                    handler(item);
                    loadedCount++;
                    break;
                }
            }
            return loadedCount;
        }
    private:
        using Storage = decltype(initDFTaskStorage(""));
        Storage storage;
        std::mutex& lock;
    };
}


