#ifndef NETWORKUIOPS_H_
#define NETWORKUIOPS_H_

#include <stddef.h>
#include <string>

class Link;
class Account;

class NetworkUiOps{
public:
    NetworkUiOps(): notifyaddedcb_(NULL),
                    requestaddcb_(NULL),
                    requestauthorizeaddedcb_(NULL),
                    statuschangedcb_(NULL),
                    accountdisconnectcb_(NULL){};

    NetworkUiOps(const NetworkUiOps& nuiops);
    void operator=(const NetworkUiOps& nuiops);

    /* Buddy List Callbacks */
    class NotifyAddedCB{
    public:
        virtual void Run(const Account &acct, const Link &link, 
            const std::string &msg)=0;
    };
    void NotifyAdded(Account acct, Link link, std::string msg);   
    void set_notifyaddedcb(NotifyAddedCB* nacb){notifyaddedcb_ = nacb;};

    class RequestAddCB{
    public:
        virtual void Run(const Account &acct, const Link &link, 
            const std::string &msg)=0;
    };
    void RequestAdd(Account acct, Link link, std::string msg);   
    void set_requestaddcb(RequestAddCB* racb){requestaddcb_ = racb;};

    class RequestAuthorizeAddedCB{
    public:
        virtual void Run(const Account &acct, const Link &link, 
            const std::string &msg, const bool onlist)=0;
    };
    void RequestAuthorizeAdded(Account acct, Link link, std::string msg,
        bool onlist);   
    void set_requestauthorizeaddedcb(RequestAuthorizeAddedCB* racb)
        {requestauthorizeaddedcb_ = racb;};

    class StatusChangedCB{
    public:
        virtual void Run(const Account &acct)=0;
    };
    void StatusChanged(Account acct);   
    void set_statuschangedcb(StatusChangedCB* sccb){statuschangedcb_ = sccb;};

    /* Connection Callbacks */
    class AccountDisconnectCB{
    public:
        virtual void Run(const Account &acct, const std::string &reason)=0;
    };
    void AccountDisconnect(Account acct, std::string reason);   
    void set_accountdisconnectcb(AccountDisconnectCB* adcb){accountdisconnectcb_ = adcb;};

private:
    /* Buddy List Callbacks */
    NotifyAddedCB *notifyaddedcb_;
    RequestAddCB *requestaddcb_;
    RequestAuthorizeAddedCB *requestauthorizeaddedcb_;
    StatusChangedCB *statuschangedcb_;

    /* Connection Callbacks */
    AccountDisconnectCB *accountdisconnectcb_;
};

#endif
