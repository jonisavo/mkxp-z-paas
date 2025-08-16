//
//  json.h
//  mkxp-z
//
//  Created by Joni Savolainen on 21.6.2025.
//


#ifndef json_h
#define json_h

#include "binding-util.h"
#include "util/json5pp.hpp"

VALUE json2rb(json5pp::value const &v);
json5pp::value rb2json(VALUE v);

#endif
