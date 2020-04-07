#pragma once

#include <string>
#include <type_traits>

#include "sql.hpp"

using books =
	sql::schema<
		sql::index<>,
#ifdef CROSS
		sql::column<"book", std::string>,
#else
		sql::column<"title", std::string>,
#endif
		sql::column<"genre", std::string>,
		sql::column<"year", unsigned>,
		sql::column<"pages", unsigned>
	>;

using stories =
	sql::schema<
		sql::index<>,
#ifdef CROSS
		sql::column<"story", std::string>,
#else
		sql::column<"title", std::string>,
#endif
		sql::column<"genre", std::string>,
		sql::column<"year", unsigned>
	>;

using authored =
	sql::schema<
		sql::index<>,
		sql::column<"title", std::string>,
		sql::column<"name", std::string>
	>;

using collected =
	sql::schema<
		sql::index<>,
		sql::column<"title", std::string>,
		sql::column<"collection", std::string>,
		sql::column<"pages", unsigned>
	>;

const std::string data_folder{ "./data/" };
const std::string bench_folder{ "../data/" };
const std::string books_data{ "books-table.tsv" };
const std::string stories_data{ "stories-table.tsv" };
const std::string authored_data{ "authored-table.tsv" };
const std::string collected_data{ "collected-table.tsv" };

constexpr std::size_t iters{ 1024 };
