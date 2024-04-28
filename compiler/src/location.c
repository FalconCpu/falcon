#include <string.h>
#include <stdio.h>
#include "fpl.h"
#include "location.h"

// ========================================================================
//                     new_location
// ========================================================================
Location new_location(String file_name, int first_line, int first_column, int last_line, int last_column) {
    Location ret = new(struct Location);
    ret->file_name = file_name;
    ret->first_line = first_line;
    ret->first_column = first_column;
    ret->last_line = last_line;
    ret->last_column = last_column;
    return ret;
}

// ========================================================================
//                     combine_location
// ========================================================================

Location combine_location(Location loc1, Location loc2) {
    Location ret = new(struct Location);
    ret->file_name = loc1->file_name;
    ret->first_line = loc1->first_line;
    ret->first_column = loc1->first_column;
    ret->last_line = loc2->last_line;
    ret->last_column = loc2->last_column;
    return ret;
}

// ========================================================================
//                     location_toString
// ========================================================================

// String location_toString(Location location) {
//     char buf[80];
//     sprintf(buf,"%s %d.%d", location->file_name, location->first_line, location->last_line);
//     return strdup(buf);
// }