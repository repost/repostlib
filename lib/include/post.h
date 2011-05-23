#ifndef POST_H_
#define POST_H_

#include <string>

/**
 * @brief The generic repost post class. 
 */
class Post
{
public:
    /**
     * @brief Create an empty post.
     */   
    Post(){uuid_ = gen_uuid();};
    Post(std::string u, std::string c, int m, std::string ct, std::string cm, std::string t)
           {uuid_ = u; content_ = c; metric_ = m; certs_ = ct; comments_ = cm; tags_  = t; };

    std::string uuid() const {return uuid_;};
    std::string content()const{return content_;};
    int metric()const{return metric_;};
    std::string certs()const{return certs_;};
    std::string comments()const{return comments_;};
    std::string tags()const{return tags_;};

    void set_uuid(std::string constr){uuid_ = constr;};
    void set_content(std::string constr){content_ = constr;};
    void set_metric(int constr){metric_ = constr;};
    void set_certs(std::string constr){certs_ = constr;};
    void set_comments(std::string constr){comments_ = constr;};
    void set_tags(std::string constr){tags_ = constr;};

private:
    std::string uuid_;
    std::string content_;
    int metric_;
    std::string certs_;
    std::string comments_;
    std::string tags_;
    std::string gen_uuid();
};

#endif 
