#pragma once
#include <sqlite_orm/sqlite_orm.h>
#include <mutex>
#include "node/zczh/ZhIQAcquire.pb.h"
#include "Miscellaneous.h"
#include <functional>

using namespace sqlite_orm;

namespace persistence
{
    struct IQAcquireDataItem {
        int dataSeq;
        std::string taskuuid;
        uint64_t numSequence;
        uint64_t timeStamp;
        int64 centerFreq;
        int64 sampleRate;
        std::vector<char> header;
        std::vector<char> iqData;
    };

    struct IQAcquireTaskItem {
        int taskSeq;
        std::string uuid;
        uint32_t nodeId;
        uint32_t deviceId;
        uint32_t numSegment;
        uint32_t numTransfer;
        uint32_t numBlocks;
        uint32_t sizeofSampleInBytes;
        uint64_t recordCount;
        uint64_t startTime;
        uint64_t stopTime;
        int64 startFreq;
        int64 stopFreq;
        std::vector<char> taskParam;
    };

    inline auto initIQAcquireTaskStorage(const std::string& path)
    {
        return make_storage(
            path,
            make_table("IQAcquireTask",
                make_column("seq",                  &IQAcquireTaskItem::taskSeq, primary_key()),
                make_column("uuid",                 &IQAcquireTaskItem::uuid),
                make_column("nodeId",               &IQAcquireTaskItem::nodeId),
                make_column("deviceId",             &IQAcquireTaskItem::deviceId),
                make_column("numSegment",           &IQAcquireTaskItem::numSegment),
                make_column("numTransfer",          &IQAcquireTaskItem::numTransfer),
                make_column("numBlocks",            &IQAcquireTaskItem::numBlocks),
                make_column("sizeofSampleInBytes",  &IQAcquireTaskItem::sizeofSampleInBytes),
                make_column("recordCount",          &IQAcquireTaskItem::recordCount),
                make_column("startTime",            &IQAcquireTaskItem::startTime),
                make_column("stopTime",             &IQAcquireTaskItem::stopTime),
                make_column("startFreq",            &IQAcquireTaskItem::startFreq),
                make_column("stopFreq",             &IQAcquireTaskItem::stopFreq),
                make_column("taskParam",            &IQAcquireTaskItem::taskParam)
            ),
            make_table("IQAcquireData",
                make_column("seq",              &IQAcquireDataItem::dataSeq, primary_key()),
                make_column("taskuuid",         &IQAcquireDataItem::taskuuid),
                make_column("numSequence",      &IQAcquireDataItem::numSequence),
                make_column("timeStamp",        &IQAcquireDataItem::timeStamp),
                make_column("centerFreq",       &IQAcquireDataItem::centerFreq),
                make_column("sampleRate",       &IQAcquireDataItem::sampleRate),
                make_column("rawHeader",        &IQAcquireDataItem::header),
                make_column("iqData",           &IQAcquireDataItem::iqData)
            )
        );
    }

    using IQAcquireDataItemHandler = std::function<void(const IQAcquireDataItem&)>;

    class IQAcquireDBAccessor
    {
    public:
        IQAcquireDBAccessor(const std::string& path, std::mutex& dblock)
            :storage(initIQAcquireTaskStorage(path)),
            lock(dblock)
        {
            storage.sync_schema();
        }
    public:
        int insertTask(const IQAcquireTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(taskItem);
        }

        void updateTask(const IQAcquireTaskItem& taskItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            storage.update(taskItem);
        }

        int insertData(const IQAcquireDataItem& dataItem)
        {
            std::lock_guard<std::mutex> lg(lock);
            return storage.insert(dataItem);
        }

        std::vector<IQAcquireTaskItem> findTask(const spectrum::FrequencySpan* freqSpan, uint32_t segmentCount, const TimeSpan* timeSpan)
        {
            std::vector<IQAcquireTaskItem> taskItems;
            std::lock_guard<std::mutex> lg(lock);
            if (freqSpan && timeSpan)
            {
                auto freqStart = static_cast<int64_t>(freqSpan->start_freq());
                auto freqStop = static_cast<int64_t>(freqSpan->stop_freq());
                auto timeStart = timestampToUINT64(timeSpan->start_time());
                auto timeStop = timestampToUINT64(timeSpan->stop_time());
                taskItems = storage.get_all<IQAcquireTaskItem>(
                    where(
                        c(&IQAcquireTaskItem::startFreq) >= freqStart and
                        c(&IQAcquireTaskItem::stopFreq) <= freqStop and
                        c(&IQAcquireTaskItem::startTime) > timeStart and
                        c(&IQAcquireTaskItem::stopTime) < timeStop
                        )
                    );
            }
            else
            {
                if (freqSpan != nullptr)
                {
                    auto freqStart = static_cast<int64_t>(freqSpan->start_freq());
                    auto freqStop = static_cast<int64_t>(freqSpan->stop_freq());
                    taskItems = storage.get_all<IQAcquireTaskItem>(
                        where(
                            c(&IQAcquireTaskItem::startFreq) >= freqStart and
                            c(&IQAcquireTaskItem::stopFreq) <= freqStop and
                            c(&IQAcquireTaskItem::numSegment) == segmentCount
                            )
                        );
                }
                else
                {
                    auto timeStart = timestampToUINT64(timeSpan->start_time());
                    auto timeStop = timestampToUINT64(timeSpan->stop_time());
                    taskItems = storage.get_all<IQAcquireTaskItem>(
                        where(
                        c(&IQAcquireTaskItem::startTime) > timeStart and
                        c(&IQAcquireTaskItem::stopTime) < timeStop
                        )
                    );
                }
            }
            return taskItems;
        }

        std::unique_ptr<IQAcquireTaskItem> getTaskItem(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            auto all = storage.get_all<IQAcquireTaskItem>(where(c(&IQAcquireTaskItem::uuid) == taskuuid));
            if (all.empty())
                return nullptr;
            else
                return std::make_unique<IQAcquireTaskItem>(all[0]);
        }

        std::unique_ptr<IQAcquireDataItem> getDataItem(int64_t dataSeq)
        {
            std::lock_guard<std::mutex> lg(lock);
            auto all = storage.get_all<IQAcquireDataItem>(where(c(&IQAcquireDataItem::dataSeq) == dataSeq));
            if (all.empty())
                return nullptr;
            else
                return std::make_unique<IQAcquireDataItem>(all[0]);
        }

        static void toTaskDescriptor(const IQAcquireTaskItem& dbItem, zczh::zhIQAcquire::RecordDescriptor& desc)
        {
            NodeDevice nd;
            nd.mutable_node_id()->set_value(dbItem.nodeId);
            nd.mutable_device_id()->set_value(dbItem.nodeId);
            desc.set_record_id(dbItem.uuid);
            *desc.mutable_from() = nd;
            *desc.mutable_time_span()->mutable_start_time() = UINT64ToTimestamp(dbItem.startTime);
            *desc.mutable_time_span()->mutable_stop_time() = UINT64ToTimestamp(dbItem.stopTime);
            desc.set_record_count(dbItem.recordCount);
            zczh::zhIQAcquire::TaskParam taskParam;
            taskParam.ParseFromArray(dbItem.taskParam.data(), dbItem.taskParam.size());
            //for (auto& segParam : taskParam.segment_param())
            //{
            //    auto seg = desc.mutable_bands()->Add();
            //    seg->set_center_frequency(segParam.center_freq());
            //    seg->set_sample_rate(segParam.sample_rate());
            //}
        }

        std::vector<int> getTaskAllDataItemId(const std::string& taskuuid)
        {
            std::lock_guard<std::mutex> lg(lock);
            auto ids = storage.select(&IQAcquireDataItem::dataSeq, where(
                c(&IQAcquireDataItem::taskuuid) == taskuuid),
                order_by(&IQAcquireDataItem::dataSeq)
                );
            return ids;
        }
    private:
        using Storage = decltype(initIQAcquireTaskStorage(""));
        Storage storage;
        std::mutex& lock;
    };
}


