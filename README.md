# Minimal Yet Rich Object Notation

*Minimal Yet Rich Object Notation*, or `myron` for short, is an object notation designed to be something of a mix between JSON and CSV.
This combination makes is perfect for complex data structures as well as large data sets.

Core principles of `myron` are:

- Everything is a key-value pair
- Whitespace is purely semantic
- Every character is significant

## Syntax

```txt
#
# Basic syntax (this is a comment)
#

name    "Alice"             # Strings
age     45                  # Numbers

address {                   # Records
    country "UK"
    city    "London"
}

cities [                    # Lists
    "Berlin"
    "New York"
    "Tokyo"
]

people [                    # List of records
    {name "Alice" age 45}
    {name "Bob" age 30}
]

#
# Advanced usage: schemas
#

people (name age) [         # List of records with a schema (record definitions)
    "Alice"     45          # This mimics the layout of CSV
    "Bob"       30
    "Charlie"   17
]

person (name age) {         # Records can have schemas, too!
    "Alice"     45          # But, why?
}

#
# Advanced usage: type definitions
# Idea: type checking
#

Person (
    name String
    age Number
)

people (Person) [
    "Alice" 45
    "Bob"   30
]

```
