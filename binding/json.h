//
//  json.h
//  mkxp-z
//
//  Created by Joni Savolainen on 21.6.2025.
//


#ifndef json_h
#define json_h

#include "binding-util.h"
#include "util/json.hpp"

using json = nlohmann::json;

VALUE json2rb(json const &v);
json rb2json(VALUE v);

#endif
