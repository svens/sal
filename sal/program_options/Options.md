<!--- \defgroup program_options Program Options -->

\tableofcontents

# Overview {#sal_program_options_overview}

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
* parser: read and return option/argument pairs from specific source. SAL
  library has pre-defined parsers:
  * `sal::program_options::command_line_t` for command line
  * `sal::program_options::config_reader_t` for configuration file

All read options/arguments are stored in list. It is up to specific
application to handle multiple values (i.e. merge, use first or let next to
override previous etc).


# Usage {#sal_program_options_usage}

In following examples, it's assumed implicit `using namespace sal::program_options;`


## Help screen {#sal_program_options_help_screen}

Create command line configuration that supports only `--help` and `--version`
options and print help screen:
```{.cpp}
option_set_t options;
options
  .add({"h", "help"},
    help("display this help and exit")
  )
  .add({"v", "version"},
    help("output version information and exit")
  )
;
std::cout << "usage:\n  "
  << argv[0] << " [options]\n\n"
  << options
  << std::endl
;
```


## Help screen on demand {#sal_program_options_help_screen_on_demand}

This example uses `options` from previous example and parses command line from
specified `argc`/`argv`. If user has passed either `-h` or `--help`, print
help screen:
```{.cpp}
// parse command line interface
auto cli = options.parse<command_line_t>(argc, argv);
if (cli.has("help"))
{
  std::cout << "usage:\n  "
    << argv[0] << " [options]\n\n"
    << "options:"
    << options
    << std::endl
  ;
  return EXIT_SUCCESS;
}
```
Note how code checks for `"help"` only and it works if either `-h` or `--help`
is used.


## Combine command line and config file {#sal_program_options_combine}

Following examples that allows to specify input file, either on command line
or from configuration file. Confguration file name itself can be given on
command line:

First, create options description set:
```{.cpp}
option_set_t options;
options
  .add({"c", "config"}, help("config file"),
    requires_argument("FILE", "my-app.conf")
  )
  .add({"f", "file"}, help("input file"),
    requires_argument("FILE")
  )
;
```
It allows to pass config file name to load (using `-c` or `--config`). If none
it given, `my-app.conf` is used as default.

Next, parse command line to get configuration file to load (in case there are
multiple config files listed, use only last):
```{.cpp}
auto cli = options.parse<command_line_t>(argc, argv);
auto config_file_name = options.back_or_default("config", { cli });
```

Read and parse configuration file (file open error handling is skipped for
clarity):
```{.cpp}
std::ifstream config_file{config_file_name};
auto config = options.parse<config_reader_t>(config_file);
```

Following examples show different ways to combine values from command line
and/or configuration file.


### Use all given file names

Iterate over all files give both command line or configuration file:
```{.cpp}
for (auto &file_name: options.merge("file", { cli, config }))
{
  std::cout << file_name << std::endl;
}
```


### Use first occurrence, prefer command line over configuration file

```{.cpp}
auto &file = options.front_or_default("file", { cli, config });
std::cout << file << std::endl;
```


### Use first occurrence, prefer configuration file over command line

```{.cpp}
auto &file = options.front_or_default("file", { config, cli });
std::cout << file << std::endl;
```


### Use last occurrence, prefer command line over configuration file

```{.cpp}
auto &file = options.back_or_default("file", { config, cli });
std::cout << file << std::endl;
```


### Use last occurrence, prefer configuration file over command line

```{.cpp}
auto &file = options.back_or_default("file", { cli, config });
std::cout << file << std::endl;
```


# Config file format {#sal_program_options_config}

Different syntaxes used to mix SAL configuration file format:
* JSON http://json.org
* HJSON http://hjson.org/syntax.html
* SJSON http://help.autodesk.com/cloudhelp/ENU/Stingray-Help/stingray_help/managing_content/sjson.html
* TOML https://github.com/toml-lang/toml

Note: although SAL config file format parser can parse JSON input, it does not
support arrays that contain other arrays or objects.


## Example {#sal_program_options_example}

```
/**
 * You can use block comments
 * /* that can be even recursive */
 */

one = 1 two = 2 // comment that lasts until newline

"assignment that continues"
  =
  "on next line", another = value

nested = {
  trailing: with_comma,         // nested.trailing = with_comma
  key = value                   // nested.key = value
  deeper = { key = value }      // nested.deeper.key = value
}

// multiline string
text = '''
  can use
    text
  that preserves indentation
'''

'list of values' = [
 one,
 two,
 three,
]
```


## Spec {#sal_program_options_spec}

* Case sensitive
* UTF-8 only
* Whitespace is defined as `std::isspace()`
* Newline is either `\n` or `\r\n`


## Comment {#sal_program_options_comment}

C++ style comments:
* Line comment marked with `//`: everything is ignored until newline
* Block comment is between `/*` and `*/`. Block comment inside other block
  comment are allowed. Block ends until it's beginnings and endings are
  equally paired.


## Key/value pairs {#sal_program_options_key_value}

Both colon `:` and assignment `=` can be used to create key/value pairs and
they can be freely placed (even on separate lines or on same line). Also,
consecutive pairs can be separated with commas but don't have to:
```
one = 1
two: 2
three
 = 3, four = 4 five : 5
```

Keys can be bare or quoted:
* bare key names may contain letters, numbers, underscores and dashes
* quoted keys follow same rules as basic or literal strings (described below)

```
key = value
bare-key = value
bare_key = value
123 = value

"key with spaces" = value
'' = value // key with no name
```


## String {#sal_program_options_string}

There are multiple string types:
* basic string (marked with double quote `"`)
* literal string (marked with single quote `'`)
* basic multiline string (marked with triple double quotes `"`)
* literal multiline string (marked with triple single quotes `'`)
* quoteless string

In case of basic strings it can contain escaped characters that will be
unescaped:
* `\b` backspace (8)
* `\t` tab (9)
* `\n` linefeed (10))
* `\f` formfeed (12)
* `\r` carriage return (13)
* `\"` double quote (34)
* `\\` backslash (92)

All other escapes will produce error. In literal strings, no such unescaping is done.

Multiline variants of those strings can span over multiple lines and
whitespaces are preserved in returned values. Only newline immediately
following opening delimiters will be trimmed.

Quoteless strings can contain any characters and continue until comma or
whitespace.


```
//
key = "continuing\non\nnext line"
regex = '\s+'
quoteless = \s+

// "text\n on\nmultiple lines\n"
key = """
text
 on
multiple lines
"""
```


## Object {#sal_program_options_object}

Object is structured list of key/value pairs that begins and ends with curly
braces (begin with `{` and end with `}`). Objects can contain also nested
objects. Also, whole document can be nested inside object.

```
{
  nested = {
    deeper = {
      key = 1
      array = [ one, two ]
    }
  }
}
```


## Array {#sal_program_options_array}

Arrays are delimited with square brackets with values inside (or can be
empty). Whitespaces between values are ignored. Elements can be separated with
comma but don't have to. Final element can have also trailing comma.

Array elements can't be another arrays or objects.

```
empty-array = []
array-with-commas = [ 1, 2, ]
array-with-whitespaces = [ 1 2
 3
 4
]
```
