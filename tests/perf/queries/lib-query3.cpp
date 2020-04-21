#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT genre AS type, year AS published, title, name "
		"FROM stories NATURAL JOIN authored "
		"WHERE genre = \"science fiction\" AND year > 1970 AND name != \"Harlan Elison\"",
		stories,
		authored
	>;

int main()
{
	stories s{ sql::load<stories, '\t'>(perf_folder + stories_data) };
	authored a{ sql::load<authored, '\t'>(perf_folder + authored_data) };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (query q{ s, a }; auto const& [g, y, t, n] : q)
		{
			std::cout << g << '\t' << y << '\t' << t << '\t' << n << '\n';
		}
	}

	return 0;
}
