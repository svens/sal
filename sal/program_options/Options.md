<!--- \defgroup program_options Program Options -->

# Overview

Generic program options module. It offers classes to describe, parse and store
options for programs. It allows to read options with their arguments from
multiple sources and merge or look up individual values from option/argument
map.

There are three major participants configuring program options:
  * `sal::program_options::option_set_t`: describes program options (i.e.
    whether argument is optional/required, help text etc). Once all options
    are described, it can generate full help page.
  * `sal::program_options::argument_map_t`: generic readonly map, storing
    program options and corresponding arugments.
  * parser: read and return option/argument pairs from specific source (for
    example, `sal::program_options::command_line_t` for command line etc).

All read options/arguments are stored in list. It is up to specific
application to handle multiple values (i.e. merge, use first or let next to
override previous etc).


# Usage

TODO
