//
//  ollama.h
//  mkxp-z
//
//  Created by Joni Savolainen on 20.6.2025.
//

#ifndef ollama_h
#define ollama_h

#include <vector>
#include <string>
#include <functional>
#include "net/httplib.h"
#include "util/json5pp.hpp"

namespace mkxp_llm {
class Ollama {
private:
    std::string base_url_;
    httplib::ThreadPool thread_pool_;
public:
    Ollama(const std::string& base_url)
    : base_url_(base_url), thread_pool_(1) {}
    
    ~Ollama() {
        this->thread_pool_.shutdown();
    }
    
    struct Message {
        std::string role;
        std::string content;
        
        Message(const std::string& role, const std::string& content) {
            this->role = role;
            this->content = content;
        }
        
        json5pp::value to_object() const;
    };
    
    struct Request {
        std::string model;
        std::vector<Message> messages;
        
        Request() : model(""), messages({}) {}
    };
    
    using Callback = std::function<void(const std::string& content, bool error)>;
    
    void chat(const Request& request, Callback callback);
};
}

#endif
