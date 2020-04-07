#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>

#include "../../data.hpp"

using output_type = std::vector<std::tuple<std::string, std::string, int>>;

output_type query(books_type const& b, authored_type const& a)
{
	using std::get;
	output_type output{};
	
	for (auto const& a_row : a)
	{
		for (auto const& b_row : b)
		{
			if (!(get<1>(b_row) != "science fiction") && get<2>(b_row) > 1970)
			{
				output.emplace_back(get<0>(b_row), get<1>(b_row), get<2>(b_row));
			}
		}
	}

	return output;
}

int main()
{
	books_type b{ books_load<'\t'>() };
	authored_type a{ authored_load<'\t'>() };

	for (std::size_t i{}; i < iters / 128; ++i)
	{
		for (auto data{ query(b, a) }; auto const& [b, g, y] : data)
		{
			std::cout << b << '\t' << g << '\t' << y << '\n';
		}
	}

	return 0;
}
