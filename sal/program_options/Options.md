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


# Config file format {#sal_program_options_config}

Different syntaxes used to mix SAL configuration file format:
* JSON http://json.org
* HJSON http://hjson.org/syntax.html
* SJSON http://help.autodesk.com/cloudhelp/ENU/Stingray-Help/stingray_help/managing_content/sjson.html
* TOML https://github.com/toml-lang/toml

Note: although SAL config file format parser can parse JSON input, it does not
support arrays that contain other arrays or objects.


### Example

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


### Spec

* Case sensitive
* UTF-8 only
* Whitespace is defined as `std::isspace()`
* Newline is either `\n` or `\r\n`


### Comment

C++ style comments:
* Line comment marked with `//`: everything is ignored until newline
* Block comment is between `/*` and `*/`. Block comment inside other block
  comment are allowed. Block ends until it's beginnings and endings are
  equally paired.


### Key/value pairs

Both colon `:` and assignment `=` can be used to create key/value pairs and
they can be freely placed (even on separate lines or on same line). Also,
consequtive pairs can be separated with commas but don't have to:
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


### String

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


### Object

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


### Array

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
