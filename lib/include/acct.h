#ifndef ACCOUNT_H_
#define ACCOUNT_H_

#include <string>

class Account{
public:
    Account(){};

    std::string user() const {return user_;};
    std::string pass() const {return pass_;};
    std::string type() const {return type_;};

    void set_user(std::string constr){user_ = constr;};
    void set_pass(std::string constr){pass_ = constr;};
    void set_type(std::string constr){type_ = constr;};

private:
    std::string user_;
    std::string pass_;
    std::string type_;
};   

#endif
