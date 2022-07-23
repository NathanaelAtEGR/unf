#ifndef NOTICE_BROKER_MERGER_H
#define NOTICE_BROKER_MERGER_H

#include "notice.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>

#include <functional>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

using NoticeCaturePredicateFunc =
    std::function<bool (const UsdBrokerNotice::StageNotice &)>;

class NoticeMerger {
public:
    NoticeMerger(const NoticeCaturePredicateFunc& predicate=nullptr)
        : _predicate(predicate) {}

    void Capture(const UsdBrokerNotice::StageNoticeRefPtr&);
    void Join(NoticeMerger&);
    void MergeAndSend(const UsdStageWeakPtr&);

private:
    using _StageNoticePtrList =
        std::vector<UsdBrokerNotice::StageNoticeRefPtr>;

    std::unordered_map<std::string, _StageNoticePtrList> _noticeMap;
    NoticeCaturePredicateFunc _predicate = nullptr;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // NOTICE_BROKER_MERGER_H
