#ifndef LINK_H_
#define LINK_H_

#include <string>

class Link{
public:
    Link(){};

    std::string name() const {return name_;};
    std::string host() const {return host_;};

    void set_name(std::string constr){name_ = constr;};
    void set_host(std::string constr){host_ = constr;};

private:
    std::string name_;
    std::string host_;
};   

#endif
