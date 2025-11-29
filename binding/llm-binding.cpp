//
//  llm-binding.cpp
//  mkxp-z
//
//  Created by Joni Savolainen on 20.6.2025.
//

#include "binding-util.h"
#include "util/debugwriter.h"
#include <ruby/thread.h>
#include "json.h"

#include "llm/ollama.h"

static mkxp_llm::Ollama* getOllamaClient() {
    static mkxp_llm::Ollama client;
    return &client;
}

static mkxp_llm::Ollama::Request hash2Request(const VALUE hash) {
    mkxp_llm::Ollama::Request ret;
    Check_Type(hash, T_HASH);

    VALUE model = rb_hash_aref(hash, rb_str_new_cstr("model"));
    if (NIL_P(model)) {
        rb_raise(rb_eArgError, "missing required key 'model'");
    }
    SafeStringValue(model);
    
    ret.model = rb_string_value_cstr(&model);
    
    const VALUE messages = rb_hash_aref(hash, rb_str_new_cstr("messages"));
    Check_Type(messages, T_ARRAY);
    
    for (int i = 0; i < RARRAY_LEN(messages); i++) {
        const VALUE message = rb_ary_entry(messages, i);
        Check_Type(message, T_HASH);
        VALUE role = rb_hash_aref(message, rb_str_new_cstr("role"));
        SafeStringValue(role);
        
        VALUE content = rb_hash_aref(message, rb_str_new_cstr("content"));
        SafeStringValue(content);
        
        const std::string role_str = rb_string_value_cstr(&role);
        const std::string content_str = rb_string_value_cstr(&content);
        
        ret.messages.emplace_back(mkxp_llm::Ollama::Message(role_str, content_str));
    }
    return ret;
}

struct ThreadArgs {
    mkxp_llm::Ollama::Request request;
    VALUE rb_callback;
    std::atomic<bool> callback_called{false};
    std::mutex callback_mutex;

    ThreadArgs(const mkxp_llm::Ollama::Request& req, VALUE callback)
        : request(req), rb_callback(callback) {
        rb_gc_register_address(&rb_callback);
    }

    ~ThreadArgs() {
        rb_gc_unregister_address(&rb_callback);
    }
};

struct OllamaWaitData {
    std::mutex* mutex;
    std::condition_variable* cv;
    bool* completed;
};

static void* ollama_wait_without_gvl(void* data) {
    const auto wait_data = static_cast<OllamaWaitData*>(data);
    
    // Wait for completion without holding the GVL
    std::unique_lock<std::mutex> lock(*wait_data->mutex);
    wait_data->cv->wait(lock, [&] { return *wait_data->completed; });
    
    return nullptr;
}

static void safe_callback_call(ThreadArgs* args, const VALUE response, const VALUE error_flag) {
    std::lock_guard<std::mutex> lock(args->callback_mutex);

    if (args->callback_called.exchange(true)) {
        return; // Already called
    }

    // Verify the callback is still valid before calling
    if (args->rb_callback != Qnil && TYPE(args->rb_callback) == T_DATA) {
        try {
            rb_funcall(args->rb_callback, rb_intern("call"), 2, response, error_flag);
        } catch (...) {
            // Log error but don't throw - we're in cleanup phase
            Debug() << "Error calling Ruby callback";
        }
    }
}

static VALUE ruby_thread_func(void* args_ptr) {
    const auto args = static_cast<ThreadArgs*>(args_ptr);

	std::string response_body;
    
    try {
        const auto client = getOllamaClient();

        std::mutex mutex;
        std::condition_variable cv;
        bool completed = false;
        bool is_error = false;
        
        Debug() << "Starting Ollama chat...";
        
        client->chat(args->request, [&](const std::string& body, const bool error) {
            std::lock_guard<std::mutex> lock(mutex);
            response_body = body;
            is_error = error;
            completed = true;
            cv.notify_one();
        });
        
        Debug() << "Started Ollama chat. Waiting...";
        
        OllamaWaitData wait_data{&mutex, &cv, &completed};
        rb_thread_call_without_gvl(ollama_wait_without_gvl, &wait_data, nullptr, nullptr);
        
        Debug() << "Waited.";
        
        try {
            // FIXME: errors do not yield JSON strings
            const auto json = json::parse(response_body);
            const VALUE rb_response = json2rb(json);
            const VALUE rb_error = is_error ? Qtrue : Qfalse;

            safe_callback_call(args, rb_response, rb_error);
        } catch (...) {
            const VALUE error_hash = rb_hash_new();
            rb_hash_aset(error_hash, rb_str_new_cstr("error"), rb_str_new_cstr("JSON parsing error"));
            safe_callback_call(args, error_hash, Qtrue);
        }
    } catch (...) {
        const VALUE error_hash = rb_hash_new();
        rb_hash_aset(error_hash, rb_str_new_cstr("error"), rb_str_new_cstr("Internal error occurred"));
        safe_callback_call(args, error_hash, Qtrue);
    }

    delete args;
    
    return Qnil;
}

RB_METHOD_GUARD(ollamaChat) {
    RB_UNUSED_PARAM;

    VALUE request;
    rb_scan_args(argc, argv, "1", &request);
    
    const VALUE callback = rb_block_given_p() ? rb_block_proc() : Qnil;
    
    if (request == Qnil) {
        rb_raise(rb_eArgError, "arg must be a Hash");
    }

	if (callback == Qnil) {
		rb_raise(rb_eArgError, "no block given");
	}
    
    const mkxp_llm::Ollama::Request req = hash2Request(request);
    const auto args = new ThreadArgs(req, callback);
    
    // Create a new Ruby thread to handle the async operation
    rb_thread_create(ruby_thread_func, args);
    
    rb_thread_schedule();
    
    return Qnil;
}
RB_METHOD_GUARD_END

void llmBindingInit() {
    const VALUE mOllama = rb_define_module("Ollama");
    _rb_define_module_function(mOllama, "chat", ollamaChat);
}
