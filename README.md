# TokenStream

_As applications evolve, new capabilities are added and the dataset must be
enhanced. TokenStream Format is a forward and backward compatible format to
allow applications to add data fields gracefully. It is somewhat analogous to
a binary version of XML._

## Format Basics

Like XML, the key to TokenStreams is simplicity. Each stream consists of one or
more chunks. Each chunk consists of:

- Token - The value of the token is often derived from an enum within the
  structure being serialized.
- Length - The number of bytes of data to follow.
- Data

*For information about the binary format, see [FORMAT.md](FORMAT.md)*

## Types That Support Serialization

Out of the box, `TokenStream` understands how to serialize the following:

- `bool`
- All integer types: `char`, `int8_t`, `uint8_t`, `int16_t`, `uint16_t`,
  `int32_t`, `uint32_t`, `int64_t`, `uint64_t`
- Float types: `float`, `double`
- Any `enum` or `enum class` type
- `const char*`
- `std::string`
- Any type that subclasses `TokenStream::Serializable`
- Any type that implements
  `void WriteToTokenStream(TokenStream::Writer& writer)` and
  `void ReadFromTokenStream(TokenStream::Reader& reader)`
- Any type that has a `TokenStream::Helper<T>`
- `std::vector`, `std::list`, `std::set`, `std::unordered_set`, `std::map`, or
  `std::unordered_map` of any of the above

## Rules For Use

If you want to add elements to a structure, add a new token with a higher value
than the previous highest token value for that structure and write the new
data. If an older version of your application tries to read data created by a
newer version, it can ignore the tokens it does not understand. A newer version
of your application can use data created by an older version without the need
to convert the data to the new format.

Because the format is extensible, you must clear your data structure before
parsing a TokenStream. The data may have come from an older version of your
application that did not have every field. This leads to a nice, cheap
optimization. If a field is set to 0 or NULL, there is no need to write the
field to the stream. Our `TokenSteam::Writer` class checks every element it
writes for a 0 value and skips it if possible. That means that you can add many
switches and rarely used data elements to your structure without taking a
storage hit for each one.

# Typical Usage

There are 5 different ways to read and write `TokenStream` data. Which you
choose will depend on the needs of your application:

1. [Macros to make it easy](#macros-to-make-it-easy)
2. [Override `Read` and `Write`](#override-read-and-write)
3. [Implement `ReadFromTokenStream` and `WriteToTokenStream` to avoid subclassing](#implement-readfromtokenstream-and-writetotokenstream-to-avoid-subclassing)
4. [Use a `TokenStream::Helper<T>` to keep the code out of your class](#use-a-tokenstream-helper-to-keep-the-code-out-of-your-class)
5. [Use `TokenStream::Generic`](#use-tokenstream-generic)

# Macros To Make It Easy

The TokenStream library includes some macros to make most of this work
automatic. Just subclass `TokenStream::Serializable` and add the macros
and your work is done.

**IMPORTANT:** These macros will only work if your class has an
`enum class Token` with names that match your member names as in the below example.

```c++
struct Date : TokenStream::Serializable {
    uint8_t 	day = 0;
    uint8_t 	month = 0;
    uint16_t	year = 0;

    enum class Token {
        day, month, year
    };

    TOKEN_MAP(
        ENUMERATED_TOKEN(day),
        ENUMERATED_TOKEN(month),
        ENUMERATED_TOKEN(year)
    )
};

struct Address : TokenStream::Serializable {
    const char*	address1 = nullptr;
    const char*	address2 = nullptr;
    const char*	city = nullptr;
    const char*	state = nullptr;
    const char*	country = nullptr;
    const char*	zip = nullptr;

    enum class Token {
        address1, address2, city, state,
        country, zip
    };

    TOKEN_MAP(
        ENUMERATED_TOKEN(address1),
        ENUMERATED_TOKEN(address2),
        ENUMERATED_TOKEN(city),
        ENUMERATED_TOKEN(state),
        ENUMERATED_TOKEN(country),
        ENUMERATED_TOKEN(zip)
    )
};

struct Employee : TokenStream::Serializable {
    const char* name = nullptr;
    const char* phone = nullptr;
    uint16_t    extension = 0;
    Date        birthDate;
    Date        hireDate;
    Address     home;
    Address     office;

    enum class Token {
        name, phone, extension,
        birthDate, hireDate, home, office
    };

    TOKEN_MAP(
        ENUMERATED_TOKEN(name),
        ENUMERATED_TOKEN(phone),
        ENUMERATED_TOKEN(extension),
        ENUMERATED_TOKEN(birthDate),
        ENUMERATED_TOKEN(hireDate),
        ENUMERATED_TOKEN(home),
        ENUMERATED_TOKEN(office)
    )
};
```

Now if you have a `TokenStream::Writer` you can simply:

```c++
    TokenStream::MemoryWriter writer;
    Employee employee;
    employee.name = "Joe";
    writer << employee;
```

This will handle writing the entire nested structure, eliminating any default
values. In the above example, all of the values are default except the employee
name so this will only write 5 bytes: `00 03 4A 6F 65`.

### Default Values

The `ENUMERATED_TOKEN` macro can take default values, like this:

```c++
struct Date : TokenStream::Serializable {
    uint8_t     day = 1;
    uint8_t     month = 1;
    uint16_t    year = 1970;

    enum class Token {
        day, month, year
    };

    TOKEN_MAP(
        ENUMERATED_TOKEN(day, 1),
        ENUMERATED_TOKEN(month, 1),
        ENUMERATED_TOKEN(year, 1970)
    )
};
```

Be sure you initialize your members to the same values that you pass in as the
default in the `ENUMERATED_TOKEN` macro.

Once this is done, the default values will not be serialized. This can save a
lot of space, especially for `booleans` that default to `true`.

### Handling Subclasses

When you subclass a type, you may want to extend the serialization. This can be
done with one of two patterns with macros.

The safer method is to use `MAP_BASE_TOKEN`:

```c++
struct DataAndTime : Date {
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    enum class Token {
        base,
        hour,
        minute,
        second
    };

    TOKEN_MAP(
        MAP_BASE_TOKEN(Token::base, Date),
        ENUMERATED_TOKEN(hour),
        ENUMERATED_TOKEN(minute),
        ENUMERATED_TOKEN(second)
    )
};
```

This assigns a token to the base class and wraps the contents of the base class
in its own sub-stream. That means that a few extra bytes will be added so that
we can have a proper envelope on the base data. The big advantage is that it
keeps the tokens for the base and subclasses complete separate.

The less safe method is to use `DERIVED_TOKEN_MAP`:

```c++
struct DataAndTime : Date {
    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    enum class Token {
        hour = 10,
        minute,
        second
    };

    DERIVED_TOKEN_MAP(
        Date,
        ENUMERATED_TOKEN(hour),
        ENUMERATED_TOKEN(minute),
        ENUMERATED_TOKEN(second)
    )
};
```

This version will have only a single envelope but you must ensure that there
is no overlap between the tokens of the 2 types. For well defined types, this
is often a perfectly reasonable choice, especially if you leave a gap in the
token values as I did above.

# Override Read and Write

The [Macros to make it easy](#macros-to-make-it-easy) technique above is
actually overriding the `Read` and `Write` methods of
`TokenStream::Serializable`. You can also just do this yourself, like this:

```c++
struct Date : TokenStream::Serializable {
    uint8_t     day = 0;
    uint8_t     month = 0;
    uint16_t    year = 0;

    enum class Token {
        day, month, year
    };

    void Write(TokenStream::Writer& writer) const override
    {
        writer << Token::day << day
               << Token::month << month
               << Token::year << year;
    }

    void Read(TokenStream::Reader& reader) override
    {
        while(!reader.EOS()) {
            switch(reader.GetToken<Token>()) {
            case Token::day:
                reader >> day;
                break;
    
            case Token::month:
                reader >> month;
                break;
    
            case Token::year:
                reader >> year;
                break;
            }
        }
    }
};
```

If you want to write defaults, you can stream use `ValueWithDefault`:

```c++
    void Write(TokenStream::Writer& writer) const override
    {
        writer << Token::day << TokenStream::ValueWithDefault(day, 1)
               << Token::month << TokenStream::ValueWithDefault(month, 1)
               << Token::year << TokenStream::ValueWithDefault(year, 1);
    }
```

Or you can just use the `Put` methods:

```c++
    void Write(TokenStream::Writer& writer) const override
    {
        writer
            .Put(Token::day, day, 1)
            .Put(Token::month, month, 1)
            .Put(Token::year, year, 1);
    }
```

# Implement ReadFromTokenStream and WriteToTokenStream to avoid subclassing

If you do not want to subclass `TokenStream::Serializable`, you can just
implement a couple of methods to add serialization support. This is often the
best option for very simple types such as `Date` that want to be POD
(Plain Old Data).

```c++
struct Date {
    uint8_t     day = 1;
    uint8_t     month = 1;
    uint16_t    year = 1970;

    enum class Token {
        day, month, year
    };

    void WriteToTokenStream(TokenStream::Writer& writer)
    {
        writer
            .Put(Token::day, day, 1)
            .Put(Token::month, month, 1)
            .Put(Token::year, year, 1);
    }

    void ReadFromTokenStream(TokenStream::Reader& reader)
    {
        while(!reader.EOS()) {
            switch(reader.GetToken<Token>()) {
            case Token::day:
                reader >> day;
                break;
    
            case Token::month:
                reader >> month;
                break;
    
            case Token::year:
                reader >> year;
                break;
            }
        }
    }
};
```

The main downside to this technique is that the methods are not virtual.

# Use a TokenStream Helper to keep the code out of your class

Sometimes you will want to serialize classes without adding any serialization
code to the class. This is often true when either you are unable to modify the
class in question, or when your serialization code needs to live in a separate
library from the class. In these cases, you can create a `TokenStream::Helper`
for the class. For example:

```c++
struct Date {
    uint8_t     day = 1;
    uint8_t     month = 1;
    uint16_t    year = 1970;
};

namespace TokenStream {
    template<> struct Helper<Date> {
        enum class Token {
            day, month, year
        };

        static void Write(Date& o, Writer& writer) {
            writer.Put(Token::day, o.day, 1);
            writer.Put(Token::month, o.month, 1);
            writer.Put(Token::year, o.year, 1);
        }

        static void Read(Date& o, Reader& reader) {
            while(!reader.EOS()) {
                switch(reader.GetToken<Token>()) {
                case Token::day:
                    reader >> o.day;
                    break;
        
                case Token::month:
                    reader >> o.month;
                    break;
        
                case Token::year:
                    reader >> o.year;
                    break;
                }
            }
        }
    };
    static_assert(Reader::has_reader_helper<Date>::value, "Date should have a valid read Helper");
    static_assert(Writer::has_writer_helper<Date>::value, "Date should have a valid write Helper");
}
```

# Use TokenStream Generic

Finally, you can write streams without actually having the structure to
serialize. This can be especially useful when serializing data for runtime
classes that will be generated.

To do this, instantiate `TokenStream::Generic` and use the `Add` method to add
data to the stream. There are `Add` methods for all of the basic types, as well
as for serializing out other `TokenStream::Generic` objects. You can look at
`TokenStreamTest.cpp` for a complete example.
