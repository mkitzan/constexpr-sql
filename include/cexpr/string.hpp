#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace cexpr
{

	template <typename CharT, std::size_t N>
	class string
	{
	public:
		using char_type = CharT;

	constexpr string() : size_{ 0 }, string_{ 0 } {}

	constexpr string(const CharT(&s)[N]) : string{}
	{
		for(; s[size_]; ++size_)
		{
			string_[size_] = s[size_];
		}
	}

		constexpr string(cexpr::string<CharT, N> const& s) : string{}
		{
			for (; s[size_]; ++size_)
			{
				string_[size_] = s[size_];
			}
		}

		constexpr string(std::basic_string_view<CharT> const& s) : string{}
		{
			for (; size_ < s.length(); ++size_)
			{
				string_[size_] = s[size_];
			}
		}

		constexpr void fill(const CharT* begin, const CharT* end)
		{
			fill_from(begin, end, begin());
		}

		constexpr void fill_from(const CharT* begin, const CharT* end, CharT* start)
		{
			if (end - begin < N)
			{
				for (auto curr{ start }; begin != end; ++begin, ++curr)
				{
					*curr = *begin;
				}
			}
		}

	constexpr std::size_t capacity() const noexcept
	{ 
		return N - 1;
	}

	constexpr std::size_t size() const noexcept
	{
		return size_;
	}

	constexpr CharT* begin() noexcept
	{
		return string_;
	}
	constexpr const CharT* cbegin() const noexcept
	{
		return string_;
	}

	constexpr CharT* end() noexcept
	{
		return &string_[size_];
	}
	constexpr const CharT* cend() const noexcept
	{
		return &string_[size_];
	}

	constexpr CharT& operator[](std::size_t i) noexcept
	{
		return string_[i];
	}
	constexpr CharT const& operator[](std::size_t i) const noexcept
	{
		return string_[i];
	}

	template <typename OtherCharT, std::size_t OtherN>
	constexpr bool operator==(string<OtherCharT, OtherN> const& other) const
	{
		if constexpr (N != OtherN)
		{
			return false;
		}

		std::size_t i{};
		for (; i < N && string_[i] == other[i]; ++i);

		return i == N;
	}

	template <typename OtherCharT, std::size_t OtherN>
	constexpr bool operator==(const OtherCharT(&other)[OtherN]) const
	{
		if constexpr (N != OtherN)
		{
			return false;
		}

		std::size_t i{};
		for (; i < N && string_[i] == other[i]; ++i);

		return i == N;
	}

	template <typename OtherCharT>
	bool operator==(std::basic_string<OtherCharT> const& other) const
	{
		return other == string_;
	}

	template <typename OtherCharT>
	bool operator!=(std::basic_string<OtherCharT> const& other) const
	{
		return !(other == string_);
	}

	private:
		CharT string_[N];
		std::size_t size_;
	};

	template <typename CharT, std::size_t N>
	string(const CharT[N]) -> string<CharT, N>;

	template <typename CharT, std::size_t N>
	bool operator==(std::basic_string<CharT> const& str, string<CharT, N> const& cstr)
	{
		return cstr == str;
	}

	template <typename CharT, std::size_t N>
	bool operator!=(std::basic_string<CharT> const& str, string<CharT, N> const& cstr)
	{
		return cstr != str;
	}

} // namespace cexpr
