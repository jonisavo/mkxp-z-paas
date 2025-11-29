//
//  ollama.cpp
//  mkxp-z
//
//  Created by Joni Savolainen on 20.6.2025.
//

#include "ollama.h"
#include "util/debugwriter.h"

namespace mkxp_llm {
json Ollama::Message::to_object() const {
    json obj = json::object();
    obj["role"] = this->role;
    obj["content"] = this->content;
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
        
        const auto json_str = chat_payload.dump();
        
        Debug() << "Sending to Ollama:" << json_str;
        
        const auto result = client.Post("/api/chat", json_str.data(), json_str.size(), "application/json");
        
        if (result) {
            Debug() << "Got response with status" << result->status << "and body" << result->body;
            callback(result->body, false);
        } else {
            const std::string error = httplib::to_string(result.error());
            Debug() << "Got error:" << error;
            callback(error, true);
        }
    });
}
}
