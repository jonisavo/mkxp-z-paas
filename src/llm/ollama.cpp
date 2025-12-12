//
//  ollama.cpp
//  mkxp-z
//
//  Created by Joni Savolainen on 20.6.2025.
//

#include "ollama.h"
#include "util/debugwriter.h"

namespace mkxp_llm {
static std::string truncateForLog(const std::string& text, const size_t limit = 5192) {
    if (text.size() <= limit) {
        return text;
    }

    return text.substr(0, limit) + "...(truncated)";
}

json Ollama::Message::to_object() const {
    json obj = json::object();
    obj["role"] = this->role;

    if (this->has_content) {
        obj["content"] = this->content;
    }

    if (this->has_tool_name) {
        obj["tool_name"] = this->tool_name;
    }

    if (this->has_tool_call_id) {
        obj["tool_call_id"] = this->tool_call_id;
    }

    if (this->has_tool_calls) {
        obj["tool_calls"] = this->tool_calls;
    }

    return obj;
}

void Ollama::chat(const Request &request, const Callback& callback) {
    this->thread_pool_.enqueue([request, callback] {
        auto client = httplib::Client("127.0.0.1", 11434);

        if (!client.is_valid()) {
            Debug() << "HTTP client is not valid";
            callback("Failed to create HTTP client", true);
            return;
        }
        
        client.set_read_timeout(300);
        client.set_connection_timeout(60);
        
        auto messages = json::array();
        
        for (const auto& msg : request.messages) {
            messages.emplace_back(msg.to_object());
        }

        json chat_payload = json::object();
        chat_payload["model"] = request.model;
        chat_payload["messages"] = messages;
        chat_payload["stream"] = false;
        chat_payload["think"] = false;

        if (request.has_tools) {
            chat_payload["tools"] = request.tools;
        }
        
        const auto json_str = chat_payload.dump();
        
        Debug() << "Sending to Ollama:" << truncateForLog(json_str);
        
        const auto result = client.Post("/api/chat", json_str.data(), json_str.size(), "application/json");
        
        if (result) {
            Debug() << "Got response with status" << result->status << "and body" << truncateForLog(result->body);
            callback(result->body, false);
        } else {
            const std::string error = httplib::to_string(result.error());
            Debug() << "Got error:" << error;
            callback(error, true);
        }
    });
}
}
