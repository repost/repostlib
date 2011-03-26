#ifndef LINK_H_
#define LINK_H_

#include <string>

class Link{
public:
    Link(){};

    std::string name() const {return name_;};

    void set_name(std::string constr){name_ = constr;};

private:
    std::string name_;
};   

#endif
