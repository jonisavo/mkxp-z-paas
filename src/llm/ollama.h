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
#include "util/json.hpp"

using json = nlohmann::json;

namespace mkxp_llm {
class Ollama {
    httplib::ThreadPool thread_pool_;
public:
    Ollama() : thread_pool_(1) {}
    
    ~Ollama() {
        this->thread_pool_.shutdown();
    }
    
    struct Message {
        std::string role;
        std::string content;
        bool has_content{false};

        std::string tool_name;
        bool has_tool_name{false};

        std::string tool_call_id;
        bool has_tool_call_id{false};

        json tool_calls;
        bool has_tool_calls{false};
        
        Message(const std::string& role, const std::string& content) {
            this->role = role;
            this->content = content;
            this->has_content = true;
        }

        explicit Message(const std::string& role) {
            this->role = role;
        }
        
        json to_object() const;
    };
    
    struct Request {
        std::string model;
        std::vector<Message> messages;
        json tools;
        bool has_tools{false};
        
        Request() : model(""), messages({}) {}
    };
    
    using Callback = std::function<void(const std::string& content, bool error)>;
    
    void chat(const Request& request, const Callback& callback);
};
}

#endif
