//
//  ollama.cpp
//  mkxp-z
//
//  Created by Joni Savolainen on 20.6.2025.
//

#include "ollama.h"
#include "util/debugwriter.h"

namespace mkxp_llm {
json5pp::value Ollama::Message::to_object() const {
    return json5pp::object({
        {"role", this->role},
        {"content", this->content}
    });
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
        
        auto messages = json5pp::array({});
        auto& messages_array = messages.as_array();
        
        for (const auto& msg : request.messages) {
            messages_array.emplace_back(msg.to_object());
        }
        
        const auto json_str = json5pp::stringify(json5pp::object({
            {"model", request.model},
            {"messages", messages},
            {"stream", false},
            {"think", false}
        }));
        
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
