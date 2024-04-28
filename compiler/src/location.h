#include "fpl.h"

// ===========================================================================
//                          Locations
// ===========================================================================
// Describes a location in an input file

struct Location {
    String   file_name;
    int      first_line;
    int      first_column;
    int      last_line;
    int      last_column;
};

Location new_location(String file_name, int first_line, int first_column, int last_line, int last_column);

Location combine_location(Location loc1, Location loc2);