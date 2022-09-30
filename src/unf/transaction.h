#ifndef USD_NOTICE_FRAMEWORK_TRANSACTION_H
#define USD_NOTICE_FRAMEWORK_TRANSACTION_H

#include "unf/broker.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/common.h>

namespace unf {

class NoticeTransaction {
  public:
    NoticeTransaction(
        const BrokerPtr&,
        const NoticeCaturePredicateFunc& predicate=nullptr);

    NoticeTransaction(
        const PXR_NS::UsdStageRefPtr&,
        const NoticeCaturePredicateFunc& predicate=nullptr);

    ~NoticeTransaction();

    // Don't allow copies
    NoticeTransaction(const NoticeTransaction &) = delete;
    NoticeTransaction &operator=(const NoticeTransaction &) = delete;

    BrokerPtr GetBroker() { return _broker; }

  private:
    BrokerPtr _broker;
};

}  // namespace unf

#endif  // USD_NOTICE_FRAMEWORK_TRANSACTION_H
