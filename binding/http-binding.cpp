//
//  http-binding.cpp
//  mkxp-z
//
//  Created by ゾロアーク on 12/29/20.
//

#include <stdio.h>

#include "binding-util.h"
#include "json.h"

#include "net/net.h"

VALUE stringMap2hash(mkxp_net::StringMap &map) {
    VALUE ret = rb_hash_new();
    for (auto const &item : map) {
        VALUE key = rb_utf8_str_new_cstr(item.first.c_str());
        VALUE val = rb_utf8_str_new_cstr(item.second.c_str());
        rb_hash_aset(ret, key, val);
    }
    return ret;
}

mkxp_net::StringMap hash2StringMap(VALUE hash) {
    mkxp_net::StringMap ret;
    Check_Type(hash, T_HASH);
    
    VALUE keys = rb_funcall(hash, rb_intern("keys"), 0);
    for (int i = 0; i < RARRAY_LEN(keys); i++) {
        VALUE key = rb_ary_entry(keys, i);
        VALUE val = rb_hash_aref(hash, key);
        SafeStringValue(key);
        SafeStringValue(val);
        
        ret.emplace(RSTRING_PTR(key), RSTRING_PTR(val));
    }
    return ret;
}

bool strContainsStr(std::string &first, std::string second) {
    return first.find(second) != std::string::npos;
}

VALUE getResponseBody(mkxp_net::HTTPResponse &res) {
#if RAPI_FULL >= 190
    auto it = res.headers().find("Content-Type");
    if (it == res.headers().end())
        return rb_str_new(res.body().c_str(), res.body().length());
    
    std::string &ctype = it->second;
    
    if (strContainsStr(ctype, "text/plain")        || strContainsStr(ctype, "application/json") ||
        strContainsStr(ctype, "application/xml")   || strContainsStr(ctype, "text/html")        ||
        strContainsStr(ctype, "text/css")          || strContainsStr(ctype, "text/javascript")  ||
        strContainsStr(ctype, "application/x-sh")  || strContainsStr(ctype, "image/svg+xml")    ||
        strContainsStr(ctype, "application/x-httpd-php"))
        return rb_utf8_str_new(res.body().c_str(), res.body().length());
    
#endif
    return rb_str_new(res.body().c_str(), res.body().length());
}

VALUE formResponse(mkxp_net::HTTPResponse &res) {
    VALUE ret = rb_hash_new();
    
    rb_hash_aset(ret, ID2SYM(rb_intern("status")), INT2NUM(res.status()));
    rb_hash_aset(ret, ID2SYM(rb_intern("body")), getResponseBody(res));
    rb_hash_aset(ret, ID2SYM(rb_intern("headers")), stringMap2hash(res.headers()));
    return ret;
}

#if RAPI_MAJOR >= 2
void* httpGetInternal(void *req) {
    VALUE ret;
    
    mkxp_net::HTTPResponse res = ((mkxp_net::HTTPRequest*)req)->get();
    ret = formResponse(res);
    
    return (void*)ret;
}
#endif

RB_METHOD_GUARD(httpGet) {
    RB_UNUSED_PARAM;
    
    VALUE path, rheaders, redirect;
    rb_scan_args(argc, argv, "12", &path, &rheaders, &redirect);
    SafeStringValue(path);
    
    bool rd;
    rb_bool_arg(redirect, &rd);
    mkxp_net::HTTPRequest req(RSTRING_PTR(path), rd);
    if (rheaders != Qnil) {
        auto headers = hash2StringMap(rheaders);
        req.headers().insert(headers.begin(), headers.end());
    }
#if RAPI_MAJOR >= 2
    return (VALUE)drop_gvl_guard(httpGetInternal, &req, 0, 0);
#else
    return (VALUE)httpGetInternal(&req);
#endif
}
RB_METHOD_GUARD_END

#if RAPI_MAJOR >= 2

typedef struct {
    mkxp_net::HTTPRequest *req;
    mkxp_net::StringMap *postData;
} httpPostInternalArgs;

void* httpPostInternal(void *args) {
    VALUE ret;
    
    mkxp_net::HTTPRequest *req = ((httpPostInternalArgs*)args)->req;
    mkxp_net::StringMap *postData = ((httpPostInternalArgs*)args)->postData;
    
    mkxp_net::HTTPResponse res = req->post(*postData);
    ret = formResponse(res);
    
    return (void*)ret;
}
#endif

RB_METHOD_GUARD(httpPost) {
    RB_UNUSED_PARAM;
    
    VALUE path, postDataHash, rheaders, redirect;
    rb_scan_args(argc, argv, "22", &path, &postDataHash, &rheaders, &redirect);
    SafeStringValue(path);
    
    bool rd;
    rb_bool_arg(redirect, &rd);
    mkxp_net::HTTPRequest req(RSTRING_PTR(path), rd);
    if (rheaders != Qnil) {
        auto headers = hash2StringMap(rheaders);
        req.headers().insert(headers.begin(), headers.end());
    }
    
    mkxp_net::StringMap postData = hash2StringMap(postDataHash);
    httpPostInternalArgs args {&req, &postData};
#if RAPI_MAJOR >= 2
    return (VALUE)drop_gvl_guard(httpPostInternal, &args, 0, 0);
#else
    return httpPostInternal(&args);
#endif
}
RB_METHOD_GUARD_END

#if RAPI_MAJOR >= 2
typedef struct {
    mkxp_net::HTTPRequest *req;
    const char *body;
    const char *ctype;
} httpPostBodyInternalArgs;

void* httpPostBodyInternal(void *args) {
    VALUE ret;
    
    mkxp_net::HTTPRequest *req = ((httpPostBodyInternalArgs*)args)->req;
    const char *reqbody = ((httpPostBodyInternalArgs*)args)->body;
    const char *reqctype = ((httpPostBodyInternalArgs*)args)->ctype;
    
    mkxp_net::HTTPResponse res = req->post(reqbody, reqctype);
    ret = formResponse(res);
    
    return (void*)ret;
}
#endif

RB_METHOD_GUARD(httpPostBody) {
    RB_UNUSED_PARAM;
    
    VALUE path, body, ctype, rheaders;
    rb_scan_args(argc, argv, "31", &path, &body, &ctype, &rheaders);
    SafeStringValue(path);
    SafeStringValue(body);
    SafeStringValue(ctype);
    
    
    mkxp_net::HTTPRequest req(RSTRING_PTR(path));
    if (rheaders != Qnil) {
        auto headers = hash2StringMap(rheaders);
        req.headers().insert(headers.begin(), headers.end());
    }
    
    httpPostBodyInternalArgs args {&req, RSTRING_PTR(body), RSTRING_PTR(ctype)};
#if RAPI_MAJOR >= 2
    return (VALUE)drop_gvl_guard(httpPostBodyInternal, &args, 0, 0);
#else
    return httpPostBodyInternal(&args);
#endif
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(httpJsonParse) {
    RB_UNUSED_PARAM;
    
    VALUE jsonv;
    rb_scan_args(argc, argv, "1", &jsonv);
    SafeStringValue(jsonv);
    
    json v;
    try {
        v = json::parse(RSTRING_PTR(jsonv), nullptr, true, true, true);
    }
    catch (const std::exception &e) {
        throw Exception(Exception::MKXPError, "Failed to parse JSON: %s", e.what());
    }
    
    return json2rb(v);
}
RB_METHOD_GUARD_END

RB_METHOD_GUARD(httpJsonStringify) {
    RB_UNUSED_PARAM;
    
    VALUE obj;
    rb_scan_args(argc, argv, "1", &obj);

    const json v = rb2json(obj);
    return rb_utf8_str_new_cstr(v.dump(4).c_str());
}
RB_METHOD_GUARD_END

void httpBindingInit() {
    VALUE mNet = rb_define_module("HTTPLite");
    _rb_define_module_function(mNet, "get", httpGet);
    _rb_define_module_function(mNet, "post", httpPost);
    _rb_define_module_function(mNet, "post_body", httpPostBody);
    
    VALUE mNetJSON = rb_define_module_under(mNet, "JSON");
    _rb_define_module_function(mNetJSON, "stringify", httpJsonStringify);
    _rb_define_module_function(mNetJSON, "parse", httpJsonParse);
}
