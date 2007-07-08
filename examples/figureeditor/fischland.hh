#ifndef _FISCHLAND_FISCHLAND_HH
#define _FISCHLAND_FISCHLAND_HH

#include <string>
#include <toad/cursor.hh>

std::string resource(const std::string &file);
#define RESOURCE(file) resource(file)

namespace fischland {
  extern toad::TCursor *cursor[16];
}

#endif
