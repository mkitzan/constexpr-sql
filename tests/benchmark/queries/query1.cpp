#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../../data.hpp"

books_type query(books_type const& b)
{
	using std::get;
	books_type output{};

	for (auto const& row : b)
	{
		if (!(get<1>(row) != "science fiction") && !(get<2>(row) >= 1970) || get<3>(row) < 300)
		{
			output.push_back(row);
		}
	}

	return output;
}

int main()
{
	books_type b{ books_load<'\t'>() };

	for (std::size_t i{}; i < iters; ++i)
	{
		for (auto data{ query(b) }; auto const& [t, g, y, p] : data)
		{
			std::cout << t << '\t' << g << '\t' << y << '\t' << p << '\n';
		}
	}

	return 0;
}
