# Constexpr SQL

A light weight single header alternative to DBMS

This library was developed during my honors project at the University of Victoria under the supervision of [Bill Bird](https://github.com/billbird). The original development occurred in this [metaprogramming-optimization](https://github.com/mkitzan/metaprogramming-optimization) repository, but was moved into a new, dedicated, home repo. The project was inspired by [Hana Dusíková](https://github.com/hanickadot)'s great [Compile Time Regular Expression](https://github.com/hanickadot/compile-time-regular-expressions) library (CTRE).

## Library Features and Compiler Support

Supported features:

- SQL query syntax for data processing
- `SELECT` data querying
- `AS` column renaming
- `CROSS JOIN` (note: all column names of each relation must be the unique)
- `NATURAL JOIN` (note: natural join will attempt to join on the first column of each relation)
- `WHERE` predicates on numeric and `std::string` types
- Wildcard selection with `*`
- Nested queries
- Uppercase and lowercase SQL keywords
- Contemporary `!=` and legacy `<>` not-equal operator
- Schemas support all default constructable types
- Indexes for schemas (used for sorting the data)
- Range loop and structured binding declaration support
- Loading data from files (no header row)
- Element querying from `sql::row` objects with [`sql::get<"col-name">(Row const&)`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/row.hpp#L81)

Unsupported features (future work):

- `INNER JOIN`, `OUTER JOIN`, `LEFT JOIN`, and `RIGHT JOIN`
- `GROUP BY`, `HAVING`, and `ORDER BY` (using indexes can simulate some of these features)
- `IN` operation within `WHERE` clause
- Helper function for writing query output to file
- Template argument error detection

As of April 2020, Constexpr SQL is only supported by **`GCC 9.0+`**. The compiler support is constrained because of the widespread use of the new `C++20` feature ["Class Types in Non-Type Template Parameters"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf) (proposal `P0732R2`) which is only implemented by `GCC 9.0+`. Library users specify SQL queries and column labels with string literals which are converted into `constexpr` objects which relies on functionality from `P0732R2`.

## Example

The following example shows usage of all class templates a user is expected to define to use the library.

```c++
#include <iostream>
#include <string>

#include "sql.hpp"

using books =
	sql::schema<
		"books", sql::index<"title">,
		sql::column<"title", std::string>,
		sql::column<"genre", std::string>,
		sql::column<"year", unsigned>,
		sql::column<"pages", unsigned>
	>;

using authored =
	sql::schema<
		"authored", sql::index<>,
		sql::column<"title", std::string>,
		sql::column<"name", std::string>
	>;

using query =
	sql::query<
		"SELECT title AS book, name AS author, pages "
		"FROM books NATURAL JOIN (SELECT * FROM authored WHERE name = \"Harlan Ellison\") "
		"WHERE year = 1967 OR year >= 1972",
		books, authored
	>;

int main()
{
	authored a{ sql::load<authored, '\t'>("tests/data/authored-data.tsv") };
	books b{ sql::load<books, '\t'>("tests/data/books-data.tsv") };

	for (query q{ b, a }; auto const& [book, author, pages] : q)
	{
		std::cout << book << '\t' << author << '\t' << pages << '\n';
	}

	return 0;
}
```

[`sql::schema`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/schema.hpp) defines a relation used in a query. [`sql::index`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/index.hpp) defines how an `sql::schema` sorts its data (unsorted if unspecified). [`sql::column`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/column.hpp) types are used to define the rows in an `sql::schema`. [`sql::query`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/query.hpp) wraps an query statement and the `sql::schema` types the query will operate on. [`sql::load`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/schema.hpp#L160) can be used to load data from a file into an `sql::schema`.

The example is from [`example.cpp`](https://github.com/mkitzan/constexpr-sql/blob/master/example.cpp) in the root of the repository, and can be compiled and executed with the following command:

```shell
g++ -std=c++2a -O3 -Isingle-header/ -o example example.cpp && ./example
```

It is strongly recommended to compile with optimizations enabled, otherwise expect template bloat. Use of two object of the same `sql::query` type is considered **undefined behavior** (due to issue involving static members). Instantiating `sql::query` objects should be performed within a guarded scope, like in the example. However, there are no use restrictions to `sql::schema` types. `sql::schema` types may be used multiple times within a single query or in many queries at once. There are more examples and information in [`presentation.pdf`](https://github.com/mkitzan/constexpr-sql/blob/master/presentation.pdf) at the root of the repository.

## Correctness and Performance Testing

The library has a significant testing system which is composed of two script pipelines. All tests use the data from another project of mine called [`Terminus`](https://github.com/mkitzan/terminus) which is a library database shell. The correctness testing pipeline generates nearly 1.5 million test queries, then Constexpr SQL's output is compared against the output of `SQLite3` performing the same queries. The performance testing pipeline executes six different SQL queries implemented using Constexpr SQL and hand coded SQL. The queries are executed over 65 thousand times (256 for `CROSS JOIN` due to computational complexity), and the execution timing is captured using the Linux `time` tool.

The [`runner.sh`](https://github.com/mkitzan/constexpr-sql/blob/master/tests/runner.sh) script in the `tests` directory will execute correctness testing, and the [`runner.sh`](https://github.com/mkitzan/constexpr-sql/tree/master/tests/perf/runner.sh) script in `tests/perf` will execute performance testing.

## Important Class Templates and Implementation Details

The following sections provide a high-level description about how the library is implemented. Hopefully, the sections will provide useful code and document references to others looking to write similar libraries.

### Class Template: `sql::schema`

The `sql::schema` class template represents relational schemas and, when instantiated, SQL tables. The class template is parameterized on three template parameters: `Name`, `Index`, and `Col` variadic pack. `Name` defines the SQL table name which matched against schemas in a query's `FROM` statement. The `Index` template argument is used to support `GROUP BY` statements by using [**SFINAE**](https://en.cppreference.com/w/cpp/language/sfinae) to select the [underlying column data container](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/schema.hpp#L25) (`std::vector` or `std::multiset`). The `Index` template argument, when fully specified, provides the [comparator functor](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/index.hpp#L18) used by the `std::multiset` container. The `Cols` template parameter pack is expanded into the `sql::row` type for the schema. `sql::schema` supports [**structured binding declarations**](https://en.cppreference.com/w/cpp/language/structured_binding) which is facilitated partly through the `sql::schema` API and partly through [`std` namespace injections from `sql::row`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/row.hpp#L119). Reference the example earlier for proper usage of the class template. Notice in the example the string literal as a template argument. String literals are lvalue reference types which are passed as `const` pointers. Normally, pointers can not be used as template arguments. With the new C++20 feature mentioned earlier a `constexpr` string constructor template can be [deduced](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction) to turn the string literal into a `constexpr` object.

### Class Template: `sql::query`

The [`sql::query`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/query.hpp) class template is the user interface to the SQL query parser. The class is templated on a `cexpr::string` the SQL query and a variadic number of `sql::schema` types. At compile time, the SQL query string is parsed into the relational algebra expression tree representing the query's computation. The constructor to a fully specified `sql::query` class takes a variadic pack of `sql::schema` objects which it uses to seed the relational algebra expression tree with iterators to data. The `sql::query` object can then be used in a range loop with structured binding declarations like in the example. The relational algebra expression tree uses static members to hold data, so only one object of a single fully specified `sql::query` class can exist at once in the program. To ensure this the object should be constructed within the range loop specification like in the example. It's worth noting that even though this class template's source file is the largest among the code base, nearly all of it is only live during compilation to parse the SQL query `cexpr::string`. In fact, the [runtime interface](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/query.hpp#L586) is deliberately insubstantial, merely providing an wrapper to support range loops and structured binding declarations. In compliance with range loop syntax, `sql::query` has an associated iterator class [`sql::query_iterator`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/query.hpp#L131). `sql::query_iterator` wraps the type representing the relational algebra expression and handles all of the idiosyncrasies of its usage in favor of the familiar [`forward iterator`](https://en.cppreference.com/w/cpp/named_req/ForwardIterator) interface.

### Class Template: `sql::row`

The [`sql::row`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/row.hpp) class template is a template recursive linked-list (similar to [`std::tuple`](https://en.cppreference.com/w/cpp/utility/tuple)). A template recursive linked-list is a template metaprogramming pattern which expresses a type analogous to a traditional linked-list. `sql::row` implements this pattern with two template parameters `Col` and `Next`. `Col` represents the [`sql::column`](https://github.com/mkitzan/constexpr-sql/blob/master/include/sql/column.hpp) which the node in list holds a data element from. `Next` represents the type of the next node in the list, which is either another `sql::row` type or `sql::void_row`. Because the next node is expressed as a type the template linked-list does not incur the overhead of holding a next pointer nor the run time cost of dereferencing a pointer to iterate (also makes better use of the cache). A quirk to this pattern is that the node data type need not be homogenous across the list, instead the list may be composed of heterogenous data types. Also, template linked-list access is computed at compile time, so the run time cost is constant.

### Relational Algebra Expression Nodes

At the moment, [`ra::projection`](https://github.com/mkitzan/constexpr-sql/blob/master/include/ra/projection.hpp), [`ra::rename`](https://github.com/mkitzan/constexpr-sql/blob/master/include/ra/rename.hpp), [`ra::cross`](https://github.com/mkitzan/constexpr-sql/blob/master/include/ra/join.hpp#L101), [`ra::natural`](https://github.com/mkitzan/constexpr-sql/blob/master/include/ra/join.hpp#L125), and [`ra::relation`](https://github.com/mkitzan/constexpr-sql/blob/master/include/ra/relation.hpp) are the only relational algebra nodes implemented. `ra::projection` and `ra::rename` are unary operators which take a single `sql::row` from their `Input` relational algebra operator and fold their operation over the row before propagating the transformed row to their `Output`. The `fold` is implemented as a template recursive function. `ra::cross` outputs the cross product of two relations. `ra::natural` implements a natural join between two relations using a hash table buffer of the right relation for performance. `ra::relation` is the only terminal node in the expression tree. These operators are composable types and are used to serialize the relational algebra expression tree. Individual objects of each type are not instantiated to compose the expression tree. Instead to ensure the expression tree is a zero overhead abstraction, the types implement a `static` member function `next` used to request data from its input type. The actual `constexpr` template recursive recursive descent SQL parser will serialize these individual nodes together into the appropriate expression tree.

### Constexpr Parsing

As a proof of concept for `constexpr` parsing, two math expression parsers were implemented in old repository: [`cexpr::prefix`](https://github.com/mkitzan/metaprogramming-optimization/blob/master/include/cexpr/prefix.hpp) and [`cexpr::infix`](https://github.com/mkitzan/metaprogramming-optimization/blob/master/include/cexpr/infix.hpp). `cexpr::prefix` demonstrates the fundamental method of `constexpr` parsing an expression tree into a class template. `cexpr::infix` extends this to perform `constepxr` recursive descent parsing. `cexpr::infix` and the SQL query parser are a whole order of magnitude more complex, because there's recursive function template instantiations to many different function templates. The explanation of `constexpr` parsing is illustrated through `cexpr::prefix` for simplicity. The expression tree created while parsing is a template recursive tree which shares similar properties to the template linked-list. A notable benefit to this data structure is that because the tree is composed of types rather than data values, the tree can be used to express computation models (expression trees) rather than just a tree based container. Fundamentally, the parsing is accomplished by calling a template recursive `static constexpr` parsing function member parameterized on the token position which the parser's "cursor" is standing on ([`Pos`](https://github.com/mkitzan/metaprogramming-optimization/blob/master/include/cexpr/prefix.hpp#L39) template parameter). At each "cursor" position the function uses the token to decide the how to proceed. If the token indicates the start of an operation, the parser recurses the immediate left subexpression ("cursor" + 1). On return, the left subexpression will report the depth in the token stream it recursed at which point the right subexpression will pick up at this position. This control flow is expressed in [this line](https://github.com/mkitzan/metaprogramming-optimization/blob/master/include/cexpr/prefix.hpp#L47) of [`cexpr::prefix::parse`](https://github.com/mkitzan/metaprogramming-optimization/blob/master/include/cexpr/prefix.hpp#L40). Once both left and right subexpressions are parsed, the present node's type is formed (`decltype` left and right subexpressions) which is then propagated to the caller. Otherwise, if the token indicates a terminal, then an appropriate terminal node is constructed. It is necessary that the "cursor" position is unique across template instantiations, otherwise template memoization will lead to "infinite" recursion. The few ancillary class templates used to support this parsing and the math node struct templates can be found in the [`templ` namespace](https://github.com/mkitzan/metaprogramming-optimization/tree/master/include/templ) of the old repository. There is also a [driver program](https://github.com/mkitzan/metaprogramming-optimization/blob/master/resources/parser/equation.cpp) using the parsers in the old repository. In the constexpr SQL parser, all of the entities in the `templ` namespace were replaced for more idiomatic structures.
