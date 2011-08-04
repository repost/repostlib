#include "rpdebug.h"
#include "networkuiops.h"
#include "acct.h"
#include "link.h"

NetworkUiOps::NetworkUiOps(const NetworkUiOps& nuiops)
{
    notifyaddedcb_ = nuiops.notifyaddedcb_;
    requestaddcb_ = nuiops.requestaddcb_;
    requestauthorizeaddedcb_ = nuiops.requestauthorizeaddedcb_;
    statuschangedcb_ = nuiops.statuschangedcb_;
    linkstatuschangedcb_ = nuiops.linkstatuschangedcb_;
    accountdisconnectcb_ = nuiops.accountdisconnectcb_;
}

void NetworkUiOps::operator=(const NetworkUiOps& nuiops)
{
    notifyaddedcb_ = nuiops.notifyaddedcb_;
    requestaddcb_ = nuiops.requestaddcb_;
    requestauthorizeaddedcb_ = nuiops.requestauthorizeaddedcb_;
    statuschangedcb_ = nuiops.statuschangedcb_;
    linkstatuschangedcb_ = nuiops.linkstatuschangedcb_;
    accountdisconnectcb_ = nuiops.accountdisconnectcb_;
}

void NetworkUiOps::NotifyAdded(Account acct, Link link, std::string msg)
{
    LOG_IF(WARNING, !notifyaddedcb_) << "NetworkUiOp::NotifyAdded == NULL. \
        Throwing away callback.";
    if(notifyaddedcb_) notifyaddedcb_->Run(acct, link, msg);
}

void NetworkUiOps::RequestAdd(Account acct, Link link, std::string msg)
{
    LOG_IF(WARNING, !requestaddcb_) << "NetworkUiOp::RequestAdd == NULL. \
        Throwing away callback.";
    if(requestaddcb_) requestaddcb_->Run(acct, link, msg);
}

bool NetworkUiOps::RequestAuthorizeAdded(Account acct, Link link, std::string msg,
        bool onlist)
{
    LOG_IF(WARNING, !requestauthorizeaddedcb_) << "NetworkUiOp::RequestAuthorizedAdd == NULL. \
        Throwing away callback.";
    if(requestauthorizeaddedcb_) return requestauthorizeaddedcb_->Run(acct, link, msg, onlist);
    return false;
}

void NetworkUiOps::LinkStatusChanged(Link link)
{
    LOG_IF(WARNING, !linkstatuschangedcb_) << "NetworkUiOp::LinkStatusChanged == NULL. \
        Throwing away callback.";
    if(linkstatuschangedcb_) linkstatuschangedcb_->Run(link);
}

void NetworkUiOps::StatusChanged(Account acct)
{
    LOG_IF(WARNING, !statuschangedcb_) << "NetworkUiOp::StatusChanged == NULL. \
        Throwing away callback.";
    if(statuschangedcb_) statuschangedcb_->Run(acct);
}

void NetworkUiOps::AccountDisconnect(Account acct, std::string reason)
{
    LOG_IF(WARNING, !accountdisconnectcb_) << "NetworkUiOp::AccountDisconnect == NULL. \
        Throwing away callback.";
    if(accountdisconnectcb_) accountdisconnectcb_->Run(acct, reason);
}
