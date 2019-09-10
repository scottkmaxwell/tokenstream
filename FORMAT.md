# TokenStream 2.0 Format

TokenStreams have a very simple format for encoding complex data structures
into a flat data stream. This is useful when you need to store data into a file
or when you wish to transmit data over the air. It has the advantage that newer
applications can handle streams from older applications and older applications
can attempt to continue using data from a newer application.

Like XML, the key to TokenStreams is simplicity. Each stream consists of one or
more token/length/data chunks. Each chunk consists of:

- Token - The value of the token is often derived from an enum within a structure.
- Length - The number of bytes of data to follow.
- Data

## TokenStream Length Encoding

TokenStream uses a special length encoding format for both tokens and lengths.
The format is simple and efficient and can handle tokens and lengths of up to
64 bits. All values are written in big-endian format.

- A value less than `0x80` is written as a single byte.
- A value less than `0x7800` is written as `value | 0x8000`, so `0x4007` would
  be written as `C0 07`
- Larger values are written to an 8-byte buffer in big-endian format and then
  leading zeros are removed. For example, the value `0x50000` would require 3
  bytes. Then we write `len | 0xF7`, plus the actual bytes of the value. So
  `0x50000` would be written as `FA 05 00 00`.

## Special handling for lists

The format includes a special optimization for handling lists. While it is fine
to simply encode `token/length/data` repeatedly for a list, this is not quite
as efficient as it could be. Given the length encoding detailed above, an
astute reader may have realized that no length string will ever start with
`0xF8`, since that would indicate a 1-byte length string that could be more
simply encoded. Therefore, we use `0xF8` to indicate a list length. The `0xF8`
will be followed by another length encoded numbers to indicate the number of
elements to follow.

After that, will come the token, followed by `n` `length/data` chunks. So in
the case of a list of integers `1, 2, 3`, with a token of `0x20`, we would
write `F8 03 20 01 01 01 02 01 03`. That is a list of 3 elements, with a token
of `0x20`, each with a 1-byte length and values of `1`, `2`, and `3`.

## Leading Zero Compression For Numeric Types

Integer and floating point types are always written out in big-endian format.
Any leading zeros are trimmed. Therefore a `uint64_t` value of `42` will have
the first 7 bytes trimmed and only write the final byte.

This works surprisingly well for floating point values as well. For instance,
if you have a `double` with a value of `2.5`, that converts to a big-endian hex
string of: `00 00 00 00 00 00 04 40`. As you can see, we can trim the first 6
bytes and write out only the last 2.

## An Example

Let's try an example using this data structure:

```c++
struct Date {
    uint8_t     day = 0;
    uint8_t     month = 0;
    uint16_t    year = 0;

    enum class Token {
        day, month, year,
        /* Always add new tokens to the end of the list */
        lastToken
    };
};

struct Address {
    const char*	address1 = nullptr;
    const char*	address2 = nullptr;
    const char*	city = nullptr;
    const char*	state = nullptr;
    const char*	country = nullptr;
    const char*	zip = nullptr;

    enum class Token {
        address1, address2, city, state,
        country, zip,
        /* Always add new tokens to the end of the list */
        lastToken
    };
};

struct Employee {
    const char* name = nullptr;
    const char* phone = nullptr;
    uint32_t    extension = 0;
    Date        birthDate;
    Date        hireDate;
    Address     home;
    Address     office;

    enum class Token {
        name, phone, extension,
        birthDate, hireDate, home, office,
        /* Always add new tokens to the end of the list */
        lastToken
    };
};
```

Here is a sample stream that corresponds to an Employee record:

    0x00: 000A 4A6F 6520 536D 6974 6800 010F 2838    ..Joe Smith...(8
    0x10: 3030 2920 3535 352D 3132 3132 0002 0201    00) 555-1212....
    0x20: 2C03 0A00 011B 0101 0302 0207 AE04 0A00    ,...........®...
    0x30: 0110 0101 0902 0207 CC05 2E00 0D31 3233    .............123
    0x40: 204D 6169 6E20 5374 2E00 020A 5361 6E20     Main St....San 
    0x50: 4469 6567 6F00 0303 4341 0004 0455 5341    Diego...CA...USA
    0x60: 0005 0639 3230 3230 0006 3C00 0F34 3536    ...92020..<..456
    0x70: 2047 7261 6E64 2041 7665 2E00 010A 5375     Grand Ave....Su
    0x80: 6974 6520 3130 3100 020A 4573 636F 6E64    ite 101...Escond
    0x90: 6964 6F00 0303 4341 0004 0455 5341 0005    ido...CA...USA..
    0xA0: 0639 3230 3237 00                          .92027.

The first byte is 0 so it is `Employee::Token::name`. The following length byte
is `0x0A`. Since this is less than `0x7F`, we know it is a length of 10 bytes. The
next ten bytes are the name "Joe Smith" and a terminating null.

Next is `Employee::Token::phone`, a length of 15 and the phone number. Then
`Employee::Token::extension`, a length of 2 bytes and the number `0x012c`
(300 decimal). Numeric values are always stored in network byte order
(big-endian) format. Note that only 2 bytes were required even though the type
supports up to 4 bytes.

Here is where it gets a bit more interesting. The next token is
`Employee::Token::birthDate` with a length of 10. The next 10 bytes are
actually a sub-stream containing an instance of the `Date` structure. The
first data byte is 0, or `Date::Token::day` with a length of 1 and a value of
`0x1b` (27). Next is `Date::Token::month` with a length of 1 and a value of 3.
Finally we have a two-byte `Date::Token::year` with a value of `0x07ae`
(1966). So the birth date is March 27, 1966. That takes us to the end of our
8-byte birth date. The next byte indicates that we have an 8-byte
`Employee::Token::hireDate` which, of course, contains another `Date`
sub-stream with a date of Sep. 16, 1996.

The last two elements are the home and office addresses. Notice that there is
no `Address::Token::address2` in the first address. Whenever a token is
missing, a 0 or NULL value is implied.

