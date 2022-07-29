#include "broker.h"
#include "notice.h"
#include "dispatcher.h"

#include <pxr/pxr.h>
#include <pxr/base/tf/weakPtr.h>
#include <pxr/usd/usd/common.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>

PXR_NAMESPACE_OPEN_SCOPE

// Initiate static registry.
std::unordered_map<size_t, NoticeBrokerPtr> NoticeBroker::Registry;

NoticeBroker::NoticeBroker(const UsdStageWeakPtr& stage)
    : _stage(stage)
{
    // Add default dispatcher.
    AddDispatcher<StageDispatcher>();

    // Discover dispatchers added via plugin to complete or override
    // default dispatcher.
    DiscoverDispatchers();

    // Register all dispatchers
    for (auto& d: _dispatcherMap) {
        d.second->Register();
    }
}

NoticeBrokerPtr NoticeBroker::Create(const UsdStageWeakPtr& stage)
{
    size_t stageHash = hash_value(stage);

    NoticeBroker::_CleanCache();

    // If there doesn't exist a broker for the given stage, create a new broker.
    if(Registry.find(stageHash) == Registry.end()) {
        Registry[stageHash] = TfCreateRefPtr(new NoticeBroker(stage));
    }

    return Registry[stageHash];
}

bool NoticeBroker::IsInTransaction()
{
    return _transactions.size() > 0;
}

void NoticeBroker::BeginTransaction(
    const NoticeCaturePredicateFunc& predicate)
{
    _TransactionHandler transaction;
    transaction.predicate = predicate;

    _transactions.push_back(std::move(transaction));
}

void NoticeBroker::EndTransaction()
{
    if (_transactions.size() == 0) {
        return;
    }

    _TransactionHandler& transaction = _transactions.back();

    // If there are only one transaction left, process all notices.
    if (_transactions.size() == 1) {
        _SendNotices(transaction);
    }
    // Otherwise, it means that we are in a nested transaction that should
    // not be processed yet. Join transaction data with next broker.
    else {
       (_transactions.end()-2)->Join(transaction);
    }

    _transactions.pop_back();
}

void NoticeBroker::Process(const UsdBrokerNotice::StageNoticeRefPtr notice)
{
    // Capture the notice to be processed later if a transaction is pending.
    if (_transactions.size() > 0) {
        _TransactionHandler& transaction = _transactions.back();

        // Indicate whether the notice needs to be captured.
        if (transaction.predicate && !transaction.predicate(*notice))
            return;

        // Store notices per type name, so that each type can be merged if
        // required.
        std::string name = notice->GetTypeId();
        transaction.noticeMap[name].push_back(notice);
    }
    // Otherwise, send the notice.
    else {
        notice->Send(_stage);
    }
}

void NoticeBroker::_SendNotices(_TransactionHandler& transaction)
{
    for (auto& element : transaction.noticeMap) {
        auto& notices = element.second;

        // If there are more than one notice for this type and
        // if the notices are mergeable, we only need to keep the
        // first notice, and all other can be pruned.
        if (notices.size() > 1 && notices[0]->IsMergeable()) {
            auto& notice = notices.at(0);
            auto it = std::next(notices.begin());

            while(it != notices.end()) {
                // Attempt to merge content of notice with first notice
                // if this is possible.
                notice->Merge(std::move(**it));
                it = notices.erase(it);
            }
        }

        // Send all remaining notices.
        for (const auto& notice: notices) {
            notice->Send(_stage);
        }
    }
}

void NoticeBroker::_CleanCache() {
    for (auto it = Registry.begin();
        it != Registry.end();)
    {
        // If the stage doesn't exist anymore, delete the corresponding
        // broker from the registry.
        if (it->second->GetStage().IsExpired()) {
            it = Registry.erase(it);
        }
        else {
            it++;
        }
    }
}

void NoticeBroker::_TransactionHandler::Join(
    _TransactionHandler& transaction)
{
    for (auto& element : transaction.noticeMap) {
        auto& source = element.second;
        auto& target = noticeMap[element.first];

        target.reserve(target.size() + source.size());
        std::move(
            std::begin(source),
            std::end(source),
            std::back_inserter(target));
        source.clear();
    }
    transaction.noticeMap.clear();
}

void NoticeBroker::DiscoverDispatchers()
{
    std::set<TfType> dispatcherTypes;
    PlugRegistry::GetAllDerivedTypes(
        TfType::Find<Dispatcher>(), &dispatcherTypes);

    auto self = TfCreateWeakPtr(this);

    for (const TfType& dispatcherType : dispatcherTypes) {
        const PlugPluginPtr plugin =
            PlugRegistry::GetInstance().GetPluginForType(dispatcherType);

        if (!plugin) {
            continue;
        }

        if (!plugin->Load()) {
            TF_CODING_ERROR("Failed to load plugin %s for %s",
                plugin->GetName().c_str(),
                dispatcherType.GetTypeName().c_str());
            continue;
        }

        DispatcherPtr dispatcher;
        DispatcherFactoryBase* factory =
            dispatcherType.GetFactory<DispatcherFactoryBase>();

        if (factory) {
            dispatcher = factory->New(self);
        }

        if (!dispatcher) {
            TF_CODING_ERROR(
                "Failed to manufacture dispatcher %s from plugin %s",
                dispatcherType.GetTypeName().c_str(),
                plugin->GetName().c_str());
        }

        _dispatcherMap[dispatcher->GetIdentifier()] = dispatcher;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
