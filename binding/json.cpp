//
//  json.cpp
//  mkxp-z
//
//  Created by Joni Savolainen on 21.6.2025.
//

#include "json.h"

VALUE json2rb(json const &v) {
    if (v.is_null())
        return Qnil;
    
    if (v.is_number_float())
        return rb_float_new(v.get<double>());
    
    if (v.is_string())
        return rb_utf8_str_new_cstr(v.get<std::string>().c_str());
    
    if (v.is_boolean())
        return rb_bool_new(v.get<bool>());
    
    if (v.is_number())
        return LL2NUM(v.get<int64_t>());
    
    if (v.is_array()) {
        const VALUE ret = rb_ary_new();
        for (auto& item : v) {
            rb_ary_push(ret, json2rb(item));
        }
        return ret;
    }
    
    if (v.is_object()) {
        const VALUE ret = rb_hash_new();
        for (auto const &pair : v.items()) {
            rb_hash_aset(ret, rb_utf8_str_new_cstr(pair.key().c_str()), json2rb(pair.value()));
        }
        return ret;
    }
    
    // This should be unreachable
    return Qnil;
}

json rb2json(const VALUE v) {
    if (v == Qnil)
        return json(nullptr);
    
    if (RB_TYPE_P(v, RUBY_T_FLOAT))
        return json(RFLOAT_VALUE(v));
    
    if (RB_TYPE_P(v, RUBY_T_STRING))
        return json(RSTRING_PTR(v));
    
    if (v == Qtrue || v == Qfalse)
        return json(RTEST(v));
    
    if (RB_TYPE_P(v, RUBY_T_FIXNUM))
        return json(NUM2DBL(v));
    
    if (RB_TYPE_P(v, RUBY_T_ARRAY)) {
        json ret_value = json::array({});
        for (int i = 0; i < RARRAY_LEN(v); i++) {
            ret_value.push_back(rb2json(rb_ary_entry(v, i)));
        }
        return ret_value;
    }
    
    if (RTEST(rb_funcall(v, rb_intern("is_a?"), 1, rb_cHash))) {
        json ret_value = json::object();
        
        const VALUE keys = rb_funcall(v, rb_intern("keys"), 0);
        
        for (int i = 0; i < RARRAY_LEN(keys); i++) {
            VALUE key = rb_ary_entry(keys, i); SafeStringValue(key);
            const VALUE val = rb_hash_aref(v, key);
            ret_value.emplace(RSTRING_PTR(key), rb2json(val));
        }
        
        return ret_value;
    }
    
    throw Exception(Exception::MKXPError, "Invalid value for JSON: %s", RSTRING_PTR(rb_inspect(v)));
}
