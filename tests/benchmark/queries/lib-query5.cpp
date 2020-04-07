#include <iostream>
#include "../../data.hpp"

using query =
	sql::query<
		"SELECT story, genre AS type, year AS published, title, collection, pages "
		"FROM T0 CROSS join T1 "
		"WHERE genre != \"science fiction\" OR year >= 1970 OR NOT pages < 300",
		stories,
		collected
	>;

int main()
{
	stories s{ sql::load<stories, '\t'>(bench_folder + stories_data) };
	collected c{ sql::load<collected, '\t'>(bench_folder + collected_data) };

	for (std::size_t i{}; i < iters / 128; ++i)
	{
		for (query q{ s, c }; auto const& [s, g, y, t, c, p] : q)
		{
			std::cout << s << '\t' << g << '\t' << y << '\t' << t << '\t' << c << '\t' << p << '\n';
		}
	}

	return 0;
}
