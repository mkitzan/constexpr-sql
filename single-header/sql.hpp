#pragma once

#include <array>
#include <cstddef>
#include <exception>
#include <fstream>
#include <locale>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cexpr
{

	template <typename Char, std::size_t N>
	class string
	{
	public:
		using char_type = Char;

		constexpr string() : size_{ 0 }, string_{ 0 }
		{}

		constexpr string(const Char(&s)[N]) : string{}
		{
			for(; s[size_]; ++size_)
			{
				string_[size_] = s[size_];
			}
		}

		constexpr string(cexpr::string<Char, N> const& s) : string{}
		{
			for (; s[size_]; ++size_)
			{
				string_[size_] = s[size_];
			}
		}

		constexpr string(std::basic_string_view<Char> const& s) : string{}
		{
			for (; size_ < s.length(); ++size_)
			{
				string_[size_] = s[size_];
			}
		}

		constexpr void fill(const Char* begin, const Char* end)
		{
			fill_from(begin, end, begin());
		}

		constexpr void fill_from(const Char* begin, const Char* end, Char* start)
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

		constexpr Char* begin() noexcept
		{
			return string_;
		}
		constexpr const Char* cbegin() const noexcept
		{
			return string_;
		}

		constexpr Char* end() noexcept
		{
			return &string_[size_];
		}
		constexpr const Char* cend() const noexcept
		{
			return &string_[size_];
		}

		constexpr Char& operator[](std::size_t i) noexcept
		{
			return string_[i];
		}
		constexpr Char const& operator[](std::size_t i) const noexcept
		{
			return string_[i];
		}

		template <typename OtherChar, std::size_t OtherN>
		constexpr bool operator==(string<OtherChar, OtherN> const& other) const
		{
			if constexpr (N != OtherN)
			{
				return false;
			}

			std::size_t i{};
			for (; i < N && string_[i] == other[i]; ++i);

			return i == N;
		}

		template <typename OtherChar, std::size_t OtherN>
		constexpr bool operator==(const OtherChar(&other)[OtherN]) const
		{
			if constexpr (N != OtherN)
			{
				return false;
			}

			std::size_t i{};
			for (; i < N && string_[i] == other[i]; ++i);

			return i == N;
		}

		template <typename OtherChar>
		bool operator==(std::basic_string<OtherChar> const& other) const
		{
			return other == string_;
		}

		template <typename OtherChar>
		bool operator!=(std::basic_string<OtherChar> const& other) const
		{
			return !(other == string_);
		}

	private:
		std::size_t size_;
		Char string_[N];
	};

	template <typename Char, std::size_t N>
	string(const Char[N]) -> string<Char, N>;

	template <typename Char, std::size_t N>
	bool operator==(std::basic_string<Char> const& str, string<Char, N> const& cstr)
	{
		return cstr == str;
	}

	template <typename Char, std::size_t N>
	bool operator!=(std::basic_string<Char> const& str, string<Char, N> const& cstr)
	{
		return cstr != str;
	}

} // namespace cexpr

namespace sql
{

	template <cexpr::string Name, typename Type>
	struct column
	{
		static constexpr auto name{ Name };
		
		using type = Type;
	};

} // namespace sql

namespace sql
{

	struct void_row
	{
		static constexpr std::size_t depth{ 0 };
	};

	template <typename Col, typename Next>
	class row
	{
	public:
		using column = Col;
		using next = Next;
		static constexpr std::size_t depth{ 1 + next::depth };

		row() = default;

		template <typename... ColTs>
		row(column::type const& val, ColTs const&... vals) : value_{ val }, next_{ vals... }
		{}

		template <typename... ColTs>
		row(column::type&& val, ColTs&&... vals) : value_{ std::forward<column::type>(val) }, next_{ std::forward<ColTs>(vals)... }
		{}

		inline constexpr next const& tail() const
		{
			return next_;
		}

		inline constexpr next& tail()
		{
			return next_;
		}

		inline constexpr column::type const& head() const
		{
			return value_;
		}

		inline constexpr column::type& head()
		{
			return value_;
		}
	
	private:
		column::type value_;
		next next_;
	};

	template <typename Col, typename... Cols>
	struct variadic_row
	{
	private:
		static inline constexpr auto resolve()
		{
			if constexpr (sizeof...(Cols) != 0)
			{
				return typename variadic_row<Cols...>::row_type{};
			}
			else
			{
				return void_row{};
			}
		}

	public:
		using row_type = row<Col, decltype(resolve())>;
	};

	// user function to query row elements by column name
	template <cexpr::string Name, typename Row>
	constexpr auto const& get(Row const& r)
	{
		static_assert(!std::is_same_v<Row, sql::void_row>, "Name does not match a column name.");

		if constexpr (Row::column::name == Name)
		{
			return r.head();
		}
		else
		{
			return get<Name>(r.tail());
		}
	}

	// compiler function used by structured binding declaration
	template <std::size_t Pos, typename Row>
	constexpr auto const& get(Row const& r)
	{
		static_assert(Pos < Row::depth, "Position is larger than number of row columns.");

		if constexpr (Pos == 0)
		{
			return r.head();
		}
		else
		{
			return get<Pos - 1>(r.tail());
		}
	}

	// function to assign a value to a column's value in a row
	template <cexpr::string Name, typename Row, typename Type>
	constexpr void set(Row& r, Type const& value)
	{
		static_assert(!std::is_same_v<Row, sql::void_row>, "Name does not match a column name.");

		if constexpr (Row::column::name == Name)
		{
			r.head() = value;
		}
		else
		{
			set<Name>(r.tail(), value);
		}
	}

} // namespace sql

// STL injections to allow row to be used in structured binding declarations
namespace std
{

	template <typename Col, typename Next>
	class tuple_size<sql::row<Col, Next>> : public integral_constant<size_t, sql::row<Col, Next>::depth>
	{};

	template <size_t Index, typename Col, typename Next>
	struct tuple_element<Index, sql::row<Col, Next>>
	{
		using type = decltype(sql::get<Index>(sql::row<Col, Next>{}));
	};

} // namespace std

namespace sql
{

	template <cexpr::string... Columns>
	struct index
	{
		template <typename Row>
		struct comparator
		{
			bool operator()(Row const& left, Row const& right) const
			{
				return compare<Columns...>(left, right);
			}
		
		private:
			template <cexpr::string Col, cexpr::string... Cols>
			bool compare(Row const& left, Row const& right) const
			{
				auto const& l{ sql::get<Col>(left) };
				auto const& r{ sql::get<Col>(right) };

				if constexpr (sizeof...(Cols) != 0)
				{
					if (l == r)
					{
						return compare<Cols...>(left, right);
					}					
				}
				
				return l < r;
			}
		};
	};

} // namespace sql

namespace sql
{

	template <cexpr::string Name, typename Index, typename... Cols>
	class schema
	{
	public:
		static constexpr auto name{ Name };

		using row_type = sql::variadic_row<Cols...>::row_type;
		using container = typename
			std::conditional_t<
				std::is_same_v<Index, sql::index<>>,
				std::vector<row_type>,
				std::multiset<row_type, typename Index::template comparator<row_type>>
			>;
		using const_iterator = typename container::const_iterator;
		
		schema() = default;

		template <typename Type, typename... Types>
		schema(std::vector<Type> const& col, Types const&... cols) : schema{}
		{
			insert(col, cols...);
		}

		template <typename Type, typename... Types>
		schema(std::vector<Type>&& col, Types&&... cols) : schema{}
		{
			insert(std::forward<Type>(col), std::forward<Types>(cols)...);
		}

		template <typename... Types>
		inline void emplace(Types const&... vals)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.emplace_back(vals...);
			}
			else
			{
				table_.emplace(vals...);
			}
		}

		template <typename... Types>
		inline void emplace(Types&&... vals)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.emplace_back(vals...);
			}
			else
			{
				table_.emplace(vals...);
			}
		}

		template <typename Type, typename... Types>
		void insert(std::vector<Type> const& col, Types const&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(col[i], cols[i]...);
			}
		}

		template <typename Type, typename... Types>
		void insert(std::vector<Type>&& col, Types&&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(std::forward<Type>(col[i]),std::forward<Types>(cols[i])...);
			}
		}

		void insert(row_type const& row)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.push_back(row);
			}
			else
			{
				table_.insert(row);
			}
		}

		void insert(row_type&& row)
		{
			if constexpr (std::is_same_v<Index, sql::index<>>)
			{
				table_.push_back(std::forward<row_type>(row));
			}
			else
			{
				table_.insert(std::forward<row_type>(row));
			}
		}

		inline const_iterator begin() const
		{
			return table_.begin();
		}

		inline const_iterator end() const
		{
			return table_.end();
		}

	private:
		container table_;
	};

	namespace
	{

		template <typename Row>
		void fill(std::fstream& fstr, Row& row, char delim)
		{
			if constexpr (!std::is_same_v<Row, sql::void_row>)
			{
				if constexpr (std::is_same_v<typename Row::column::type, std::string>)
				{
					if constexpr (std::is_same_v<typename Row::next, sql::void_row>)
					{
						std::getline(fstr, row.head());
					}
					else
					{
						std::getline(fstr, row.head(), delim);
					}
				}
				else
				{
					fstr >> row.head();
				}

				fill<typename Row::next>(fstr, row.tail(), delim);
			}
		}

		template <typename Row>
		void fill(std::fstream& fstr, Row const& row, char delim)
		{
			if constexpr (!std::is_same_v<Row, sql::void_row>)
			{
				fstr << row.head();

				if constexpr (std::is_same_v<typename Row::next, sql::void_row>)
				{
					fstr << '\n';
				}
				else
				{
					fstr << delim;
				}
				
				fill<typename Row::next>(fstr, row.tail(), delim);
			}
		}

	} // namespace

	// helper function for users to load a data into a schema from a file
	template <typename Schema>
	Schema load(std::string const& file, char delim)
	{
		auto fstr{ std::fstream(file, fstr.in) };
		Schema table{};
		typename Schema::row_type row{};

		while (fstr)
		{
			fill<typename Schema::row_type>(fstr, row, delim);
			table.insert(std::move(row));

			// in case last stream extraction did not remove newline
			if (fstr.get() != '\n')
			{
				fstr.unget();
			}
		}

		return table;
	}

	// for compat with previous versions
	template <typename Schema, char Delim>
	Schema load(std::string const& file)
	{
		return load<Schema>(file, Delim);
	}

	// will work with schema and query objects
	template <typename Type>
	void store(Type const& data, std::string const& file, char delim)
	{
		auto fstr{ std::fstream(file, fstr.out) };

		for (auto const& row : data)
		{
			fill<typename Type::row_type>(fstr, row, delim);
		}
	}

	// for devs who want to use the previous format
	template <typename Type, char Delim>
	void store(Type const& data, std::string const& file)
	{
		store<Type>(data, file, Delim);
	}

} // namespace sql

namespace ra
{

	template <typename Input>
	class unary
	{
	public:
		using input_type = std::remove_cvref_t<decltype(Input::next())>;

		template <typename... Inputs>
		static inline void seed(Inputs const&... rs)
		{
			Input::seed(rs...);
		}

		static inline void reset()
		{
			Input::reset();
		}
	};

	template <typename LeftInput, typename RightInput>
	class binary
	{
	public:
		using left_type = std::remove_cvref_t<decltype(LeftInput::next())>;
		using right_type = std::remove_cvref_t<decltype(RightInput::next())>;

		template <typename... Inputs>
		static inline void seed(Inputs const&... rs)
		{
			LeftInput::seed(rs...);
			RightInput::seed(rs...);
		}

		static inline void reset()
		{
			LeftInput::reset();
			RightInput::reset();
		}
	};

} // namespace ra

namespace ra
{

	namespace
	{

		template <typename Left, typename Right>
		constexpr auto recr_merge()
		{
			if constexpr (std::is_same_v<Left, sql::void_row>)
			{
				return Right{};
			}
			else
			{
				using next = decltype(recr_merge<typename Left::next, Right>());

				return sql::row<typename Left::column, next>{};
			}
		}

		template <typename Left, typename Right>
		inline constexpr auto merge()
		{
			if constexpr (Left::column::name == Right::column::name)
			{
				return recr_merge<Left, typename Right::next>();
			}
			else
			{
				return recr_merge<Left, Right>();
			}
		}

		template <typename Dest, typename Row>
		constexpr void recr_copy(Dest& dest, Row const& src)
		{
			if constexpr (std::is_same_v<Row, sql::void_row>)
			{
				return;
			}
			else
			{
				dest.head() = src.head();
				recr_copy(dest.tail(), src.tail());
			}
		}

		template <typename Dest, typename Row>
		inline constexpr void copy(Dest& dest, Row const& src)
		{
			if constexpr (Dest::column::name == Row::column::name)
			{
				recr_copy(dest, src);
			}
			else
			{
				copy(dest.tail(), src);
			}
		}

	} // namespace

	template <typename LeftInput, typename RightInput>
	class join : public ra::binary<LeftInput, RightInput>
	{
		using binary_type = ra::binary<LeftInput, RightInput>;
		using left_type = typename binary_type::left_type;
		using right_type = typename binary_type::right_type;
	public:
		using output_type = decltype(merge<left_type, right_type>());

		template <typename... Inputs>
		static inline void seed(Inputs const&... rs)
		{
			binary_type::seed(rs...);
			copy(output_row, LeftInput::next());
		}

		static inline void reset()
		{
			binary_type::reset();
			copy(output_row, LeftInput::next());
		}

		static output_type output_row;
	};

	template <typename LeftInput, typename RightInput>
	typename join<LeftInput, RightInput>::output_type join<LeftInput, RightInput>::output_row{};

} // namespace ra

namespace ra
{

	struct data_end : std::exception
	{};

	// Id template parameter allows unique ra::relation types to be instantiated from
	//	a single sql::schema type (for queries referencing a schema multiple times).
	template <typename Schema, std::size_t Id>
	class relation
	{
	public:
		using output_type = Schema::row_type&;

		static auto& next()
		{
			if (curr != end)
			{
				return *curr++;
			}
			else
			{
				throw ra::data_end{};
			}
		}
		
		template <typename Input, typename... Inputs>
		static void seed(Input const& r, Inputs const&... rs)
		{
			if constexpr (std::is_same_v<Input, Schema>)
			{
				curr = r.begin();
				begin = r.begin();
				end = r.end();
			}
			else
			{
				seed(rs...);
			}
		}

		static inline void reset()
		{
			curr = begin;
		}

	private:
		static Schema::const_iterator curr;
		static Schema::const_iterator begin;
		static Schema::const_iterator end;
	};

	template <typename Schema, std::size_t Id>
	Schema::const_iterator relation<Schema, Id>::curr{};

	template <typename Schema, std::size_t Id>
	Schema::const_iterator relation<Schema, Id>::begin{};

	template <typename Schema, std::size_t Id>
	Schema::const_iterator relation<Schema, Id>::end{};

} // namespace ra

namespace ra
{

	template <typename LeftInput, typename RightInput>
	class cross : public ra::join<LeftInput, RightInput>
	{
		using join_type = ra::join<LeftInput, RightInput>;
	public:
		using output_type = join_type::output_type;

		static auto&& next()
		{
			try
			{
				copy(join_type::output_row, RightInput::next());
			}
			catch(ra::data_end const& e)
			{
				copy(join_type::output_row, LeftInput::next());
				RightInput::reset();
				copy(join_type::output_row, RightInput::next());
			}

			return std::move(join_type::output_row);
		}
	};

} // namespace ra

namespace ra
{

	template <typename LeftInput, typename RightInput>
	class natural : public ra::join<LeftInput, RightInput>
	{
		using join_type = ra::join<LeftInput, RightInput>;
		using key_type = std::remove_cvref_t<decltype(LeftInput::next().head())>;
		using value_type = std::vector<std::remove_cvref_t<decltype(RightInput::next().tail())>>;
		using map_type = std::unordered_map<key_type, value_type>;
	public:
		using output_type = join_type::output_type;

		template <typename... Inputs>
		static void seed(Inputs const&... rs)
		{
			join_type::seed(rs...);
			
			if (row_cache.empty())
			{
				try
				{
					for (;;)
					{
						auto const& row{ RightInput::next() };
						row_cache[row.head()].push_back(row.tail());
					}
				}
				catch(ra::data_end const& e)
				{
					RightInput::reset();
				}
			}

			auto const& active{ row_cache[join_type::output_row.head()] };
			curr = active.cbegin();
			end = active.cend();
		}

		static auto&& next()
		{
			while (curr == end)
			{
				copy(join_type::output_row, LeftInput::next());
				auto const& active{ row_cache[join_type::output_row.head()] };
				curr = active.cbegin();
				end = active.cend();
			}

			copy(join_type::output_row, *curr++);
			
			return std::move(join_type::output_row);
		}

	private:
		static map_type row_cache;
		static value_type::const_iterator curr;
		static value_type::const_iterator end;
	};

	template <typename LeftInput, typename RightInput>
	typename natural<LeftInput, RightInput>::map_type natural<LeftInput, RightInput>::row_cache{};

	template <typename LeftInput, typename RightInput>
	typename natural<LeftInput, RightInput>::value_type::const_iterator natural<LeftInput, RightInput>::curr;

	template <typename LeftInput, typename RightInput>
	typename natural<LeftInput, RightInput>::value_type::const_iterator natural<LeftInput, RightInput>::end;

} // namespace ra

namespace ra
{

	template <typename Output, typename Input>
	class projection : public ra::unary<Input>
	{
		using input_type =  typename ra::unary<Input>::input_type;
	public:
		using output_type = Output;

		static auto&& next()
		{
			fold<output_type>(output_row, Input::next());
			
			return std::move(output_row);
		}

	private:
		template <typename Dest>
		static inline constexpr void fold(Dest& dest, input_type const& src)
		{
			if constexpr (Dest::depth == 0)
			{
				return;
			}
			else
			{
				dest.head() = sql::get<Dest::column::name>(src);
				fold<typename Dest::next>(dest.tail(), src);	
			}
		}

		static output_type output_row;
	};

	template <typename Output, typename Input>
	typename projection<Output, Input>::output_type projection<Output, Input>::output_row{};

} // namespace ra

namespace ra
{

	template <typename Output, typename Input>
	class rename : public ra::unary<Input>
	{
		using input_type = typename ra::unary<Input>::input_type;
	public:
		using output_type = Output;

		static auto&& next()
		{
			fold<output_type, input_type>(output_row, Input::next());
			
			return std::move(output_row);
		}

	private:
		template <typename Dest, typename Src>
		static inline constexpr void fold(Dest& dest, Src const& src)
		{
			if constexpr (Dest::depth == 0)
			{
				return;
			}
			else
			{
				dest.head() = src.head();
				fold<typename Dest::next, typename Src::next>(dest.tail(), src.tail());
			}
		}

		static output_type output_row;
	};

	template <typename Output, typename Input>
	typename rename<Output, Input>::output_type rename<Output, Input>::output_row{};
	
} // namespace ra

namespace ra
{

	template <typename Predicate, typename Input>
	class selection : public ra::unary<Input>
	{
		using input_type = typename ra::unary<Input>::input_type;
	public:
		using output_type = input_type;		

		static auto&& next()
		{
			output_row = Input::next();

			while(!Predicate::eval(output_row))
			{
				output_row = Input::next();
			}

			return std::move(output_row);
		}

	private:
		static output_type output_row;
	};

	template <typename Output, typename Input>
	typename selection<Output, Input>::output_type selection<Output, Input>::output_row{};

} // namespace ra

namespace sql
{
	namespace
	{
		
		template <typename Char>
		constexpr bool whitespace(Char curr)
		{
			return curr == Char{ ' ' } || curr == Char{ '\t' } || curr == Char{ '\n' };
		}

		template <typename Char>
		constexpr bool syntax(Char curr)
		{
			return curr == Char{ ',' } || curr == Char{ '(' } || curr == Char{ ')' } ||
				curr == Char{ '\'' } || curr == Char{ '\"' } || curr == Char{ '=' };
		}

		template <typename Char>
		constexpr const Char* skip(const Char *curr, const Char *end)
		{
			for (; curr != end && whitespace(*curr); ++curr);
			return curr;
		}

		template <typename Char>
		constexpr const Char* next(const Char *curr, const Char *end)
		{
			Char c{ *curr };

			if (c == Char{ '>' } || c == Char{ '<' } || c == Char{ '!' })
			{
				++curr;

				if (*curr == Char{ '=' } || (c == Char{ '<' } && *curr == Char{ '>' }))
				{
					++curr;
				}
			}
			else if (syntax(c))
			{
				++curr;
			}
			else
			{
				for (; curr != end && !whitespace(*curr) && !syntax(*curr); ++curr);
			}
			
			return curr;
		}
	
	} // namespace

	template <typename Char, std::size_t Count>
	class tokens
	{
	public:
		using token_view = std::basic_string_view<Char>;

		template<std::size_t N>
		constexpr tokens(cexpr::string<Char, N> const& cs) : tokens_{}
		{
			auto curr{ cs.cbegin() }, last{ cs.cbegin() };
			const auto end{ cs.cend() };
			std::size_t i{};

			while (curr < end)
			{
				curr = skip(curr, end);
				last = curr;
				last = next(last, end);

				if (*curr == Char{ '\"' } || *curr == Char{ '\'' })
				{
					tokens_[i++] = token_view{ curr, 1 };
					for (char c{ *curr++ }; last != end && *last != c; ++last);
				}

				auto len{ reinterpret_cast<std::size_t>(last) - reinterpret_cast<std::size_t>(curr) };
				tokens_[i++] = token_view{ curr, len };

				if (*last == Char{ '\"' } || *last == Char{ '\'' })
				{
					tokens_[i++] = token_view{ last, 1 };
					++last;
				}

				curr = last;
			}
		}

		constexpr std::size_t count() const noexcept
		{
			return Count;
		}

		constexpr token_view* begin() noexcept
		{
			return tokens_.begin();
		}
		constexpr const token_view* cbegin() const noexcept
		{
			return tokens_.cbegin();
		}

		constexpr token_view* end() noexcept
		{
			return tokens_.end();
		}
		constexpr const token_view* cend() const noexcept
		{
			return tokens_.cend();
		}

		constexpr token_view& operator[](std::size_t i) noexcept
		{
			return tokens_[i];
		}
		constexpr token_view const& operator[](std::size_t i) const noexcept
		{
			return tokens_[i];
		}

	private:
		std::array<token_view, Count> tokens_;
	};

	template<typename Char, std::size_t N>
	constexpr std::size_t preprocess(cexpr::string<Char, N> const& cs)
	{
		auto begin{ cs.cbegin() };
		const auto end{ cs.cend() };
		std::size_t count{ 1 };

		while (begin < end)
		{
			begin = skip(begin, end);
			begin = next(begin, end);
			++count;
		}

		return count;
	}

} // namespace sql

namespace sql
{

	namespace
	{

		// shim to allow all value types like double or float
		//	to be used as non-type template parameters.
		template <typename Type>
		struct value
		{
			constexpr value(Type v) : val{ v }
			{}

			Type val;
		};

	} // namespace

	template <cexpr::string Op, typename Row, typename Left, typename Right=void>
	struct operation
	{
		static constexpr bool eval(Row const& row)
		{
			if constexpr (Op == "=")
			{
				return Left::eval(row) == Right::eval(row);
			}
			else if constexpr (Op == ">")
			{
				return Left::eval(row) > Right::eval(row);
			}
			else if constexpr(Op == "<")
			{
				return Left::eval(row) < Right::eval(row);
			}
			else if constexpr(Op == ">=")
			{
				return Left::eval(row) >= Right::eval(row);
			}
			else if constexpr(Op == "<=")
			{
				return Left::eval(row) <= Right::eval(row);
			}
			else if constexpr(Op == "!=" || Op == "<>")
			{
				return Left::eval(row) != Right::eval(row);
			}
			else if constexpr(Op == "AND")
			{
				return Left::eval(row) && Right::eval(row);
			}
			else if constexpr(Op == "OR")
			{
				return Left::eval(row) || Right::eval(row);
			}
			else if constexpr(Op == "NOT")
			{
				return !Left::eval(row);
			}
		}
	};

	template <cexpr::string Column, typename Row>
	struct variable
	{
		static constexpr auto eval(Row const& row)
		{
			return sql::get<Column>(row);
		}
	};

	template <auto Const, typename Row>
	struct constant
	{
		static constexpr auto eval(__attribute__((unused)) Row const& row)
		{
			return Const.val;
		}
	};

} // namespace sql

namespace sql
{

	// anonymous namespace to hold helper data structures and functions
	namespace
	{

		template <std::size_t Pos, typename Node>
		struct context
		{
			using node = Node;
			static constexpr std::size_t pos = Pos;
		};

		template <typename Type, std::size_t Name, std::size_t Next>
		struct colinfo
		{
			using type = Type;
			static constexpr std::size_t name = Name;
			static constexpr std::size_t next = Next;
		};

		template <cexpr::string Name, typename Row>
		constexpr bool exists() noexcept
		{
			if constexpr (std::is_same_v<Row, sql::void_row>)
			{
				return false;
			}
			else
			{
				if constexpr (Row::column::name == Name)
				{
					return true;
				}
				else
				{
					return exists<Name, typename Row::next>();
				}
			}
		}

		template <typename Type, typename Char, std::size_t N>
		constexpr value<Type> convert(cexpr::string<Char, N> const& str)
		{
			auto curr{ str.cbegin() }, end{ str.cend() };
			constexpr Char dot{ '.' }, zro{ '0' }, min{ '-' };
			Type acc{}, sign{ 1 }, scalar{ 10 };

			if (*curr == min)
			{
				sign = -1;
				++curr;
			}

			while (curr != end && *curr != dot)
			{
				acc = (acc * scalar) + (*curr - zro);
				++curr;
			}

			if (curr != end && *curr == dot)
			{
				scalar = 1;
				++curr;

				while(curr != end)
				{
					acc += (*curr - zro) * (scalar /= Type{ 10 });
					++curr;
				}
			}

			return value<Type>{ acc * sign };
		}

		inline constexpr bool isquote(std::string_view const& tv) noexcept
		{
			return tv == "\"" || tv == "'";
		}

		inline constexpr bool isor(std::string_view const& tv) noexcept
		{
			return tv == "OR" || tv == "or";
		}

		inline constexpr bool isand(std::string_view const& tv) noexcept
		{
			return tv == "AND" || tv == "and";
		}

		inline constexpr bool isnot(std::string_view const& tv) noexcept
		{
			return tv == "NOT" || tv == "not";
		}

		inline constexpr bool isnatural(std::string_view const& tv) noexcept
		{
			return tv == "NATURAL" || tv == "natural";
		}

		inline constexpr bool isjoin(std::string_view const& tv) noexcept
		{
			return tv == "JOIN" || tv == "join";
		}

		inline constexpr bool iswhere(std::string_view const& tv) noexcept
		{
			return tv == "WHERE" || tv == "where";
		}

		inline constexpr bool isfrom(std::string_view const& tv) noexcept
		{
			return tv == "FROM" || tv == "from";
		}

		inline constexpr bool isas(std::string_view const& tv) noexcept
		{
			return tv == "AS" || tv == "as";
		}

		inline constexpr bool isselect(std::string_view const& tv) noexcept
		{
			return tv == "SELECT" || tv == "select";
		}

		inline constexpr bool iscomma(std::string_view const& tv) noexcept
		{
			return tv == ",";
		}

		constexpr bool isintegral(std::string_view const& tv) noexcept
		{
			bool result{ false };

			for (auto c : tv)
			{
				result |= (c == '.');
			}

			return !result;
		}

		constexpr bool isdigit(char c) noexcept
		{
			return (c >= '0' && c <= '9') || c == '-' || c == '.';
		}

		constexpr bool iscomp(std::string_view const& tv) noexcept
		{
			return tv[0] == '=' || tv[0] == '!' || tv[0] == '<' || tv[0] == '>';
		}

		constexpr bool iscolumn(std::string_view const& tv) noexcept
		{
			return !iscomma(tv) && !isas(tv) && !isfrom(tv);
		}

		constexpr bool isseparator(std::string_view const& tv) noexcept
		{
			return iscomma(tv) || isfrom(tv);
		}

	} // namespace

	// structured binding compliant iterator for query objects
	template <typename Expr>
	class query_iterator
	{
	public:
		using row_type = std::remove_cvref_t<typename Expr::output_type>;

		// seeds row datamember for first dereference
		query_iterator(bool end) : end_{ end }, row_{}
		{
			operator++();
		}

		bool operator==(query_iterator const& it) const
		{
			return end_ == it.end_;
		}

		bool operator!=(query_iterator const& it) const
		{
			return !(*this == it);
		}

		row_type const& operator*() const
		{
			return row_;
		}

		query_iterator& operator++()
		{
			if (!end_)
			{
				try
				{
					row_ = Expr::next();
				}
				catch (ra::data_end const& e)
				{
					end_ = true;
				}
			}

			return *this;
		}

	private:
		bool end_{};
		row_type row_{};
	};

	template <cexpr::string Str, typename... Schemas>
	class query
	{
	private:
		// where predicate terminal parsing 
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_terms() noexcept
		{
			if constexpr (tokens_[Pos] == "(")
			{
				constexpr auto next{ parse_or<Pos + 1, Row>() };

				using node = typename decltype(next)::node;

				static_assert(tokens_[next.pos] == ")", "No closing paranthesis found.");

				return context<next.pos + 1, node>{};
			}
			else if constexpr (isquote(tokens_[Pos]))
			{
				constexpr cexpr::string<char, tokens_[Pos + 1].length() + 1> name{ tokens_[Pos + 1] };

				using str = decltype(name);
				using node = sql::constant<value<str>{ name }, Row>;

				static_assert(isquote(tokens_[Pos + 2]), "No closing quote found.");

				return context<Pos + 3, node>{};
			}
			else if constexpr (isdigit(tokens_[Pos][0]))
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };
				
				using val = decltype(isintegral(tokens_[Pos]) ? std::int64_t{} : double{});
				using node = sql::constant<sql::convert<val>(name), Row>;

				return context<Pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = sql::variable<name, Row>;

				return context<Pos + 1, node>{};
			}
		}

		// parses a single compare operation
		template <typename Left, typename Row>
		static constexpr auto recurse_comparison() noexcept
		{
			if constexpr (!iscomp(tokens_[Left::pos]))
			{
				return Left{};
			}
			else
			{
				constexpr auto next{ parse_terms<Left::pos + 1, Row>() };
				constexpr cexpr::string<char, tokens_[Left::pos].length() + 1> name{ tokens_[Left::pos] };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<name, Row, typename Left::node, ranode>;

				return context<next.pos, node>{};
			}			
		}

		// descend further and attempt to parse a compare operation
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_comparison() noexcept
		{
			using left = decltype(parse_terms<Pos, Row>());
			
			return recurse_comparison<left, Row>();
		}

		// attempt to parse a negation operation then descend further
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_negation() noexcept
		{
			if constexpr (isnot(tokens_[Pos]))
			{
				constexpr auto next{ parse_comparison<Pos + 1, Row>() };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<"NOT", Row, ranode>;

				return context<next.pos, node>{};
			}
			else
			{
				return parse_comparison<Pos, Row>();
			}
		}

		// recursively parse chained AND operations
		template <typename Left, typename Row>
		static constexpr auto recurse_and() noexcept
		{
			if constexpr (!isand(tokens_[Left::pos]))
			{
				return Left{};
			}
			else
			{
				constexpr auto next{ parse_negation<Left::pos + 1, Row>() };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<"AND", Row, typename Left::node, ranode>;

				return recurse_and<context<next.pos, node>, Row>();
			}
		}

		// descend further then attempt to parse AND operations
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_and() noexcept
		{
			using left = decltype(parse_negation<Pos, Row>());
			
			return recurse_and<left, Row>();
		}
		
		// recursively parse chained OR operations
		template <typename Left, typename Row>
		static constexpr auto recurse_or() noexcept
		{
			if constexpr (!isor(tokens_[Left::pos]))
			{
				return Left{};
			}
			else
			{
				constexpr auto next{ parse_and<Left::pos + 1, Row>() };
				
				using ranode = typename decltype(next)::node;
				using node = sql::operation<"OR", Row, typename Left::node, ranode>;

				return recurse_or<context<next.pos, node>, Row>();
			}
		}

		// descend further then attempt to parse OR operations
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_or() noexcept
		{
			using left = decltype(parse_and<Pos, Row>());
			
			return recurse_or<left, Row>();
		}

		// find correct schema for terminal relation
		template <cexpr::string Name, std::size_t Id, typename Schema, typename... Others>
		static constexpr auto recurse_schemas()
		{
			if constexpr (Name == Schema::name)
			{
				return ra::relation<Schema, Id>{};
			}
			else
			{
				static_assert(sizeof...(Others) != 0, "Schema name used in JOIN was not provided.");

				return recurse_schemas<Name, Id, Others...>();
			}
		}

		// wrapper function to determine terminal relation
		template <std::size_t Pos>
		static constexpr auto parse_schema()
		{
			if constexpr (tokens_[Pos] == "(")
			{
				constexpr auto next{ parse_root<Pos + 1>() };
				
				using node = typename decltype(next)::node;

				static_assert(tokens_[next.pos] == ")", "No closing paranthesis found.");

				return context<next.pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = decltype(recurse_schemas<name, Pos, Schemas...>());

				return context<Pos + 1, node>{};
			}
		}

		// stub which will choose the specific join RA node
		template <std::size_t Pos, typename Left, typename Right>
		static constexpr auto choose_join()
		{
			if constexpr (isnatural(tokens_[Pos]))
			{
				return ra::natural<Left, Right>{};
			}
			else
			{
				return ra::cross<Left, Right>{};	
			}
		}

		// parses join colinfo if a join is present else returns the single relation terminal
		template <std::size_t Pos>
		static constexpr auto parse_join() noexcept
		{
			constexpr auto lnext{ parse_schema<Pos>() };

			using lnode = typename decltype(lnext)::node;

			if constexpr (lnext.pos + 2 < tokens_.count() && isjoin(tokens_[lnext.pos + 1]))
			{
				constexpr auto rnext{ parse_schema<lnext.pos + 2>() };

				using rnode = typename decltype(rnext)::node;
				using join = decltype(choose_join<lnext.pos, lnode, rnode>());

				return context<rnext.pos, join>{};
			}
			else
			{
				return context<lnext.pos, lnode>{};
			}
		}

		// starting point to parse everything after the from keyword
		template <std::size_t Pos>
		static constexpr auto parse_from() noexcept
		{
			static_assert(isfrom(tokens_[Pos]), "Expected 'FROM' token not found.");

			constexpr auto next{ parse_join<Pos + 1>() };

			using node = typename decltype(next)::node;

			if constexpr (next.pos < tokens_.count() && iswhere(tokens_[next.pos]))
			{
				using output = std::remove_cvref_t<typename node::output_type>;

				constexpr auto predicate{ parse_or<next.pos + 1, output>() };

				using pnext = typename decltype(predicate)::node;
				using snode = ra::selection<pnext, node>;

				return context<predicate.pos, snode>{};
			}
			else
			{
				return context<next.pos, node>{};	
			}
		}

		// recursively searches all schemas for the a matching column
		template <cexpr::string Name, typename Schema, typename... Others>
		static constexpr auto recurse_types()
		{
			if constexpr (sql::exists<Name, typename Schema::row_type>())
			{
				return decltype(sql::get<Name>(typename Schema::row_type{})){};
			}
			else
			{
				static_assert(sizeof...(Others) != 0, "Column name was not present in any schema.");

				return recurse_types<Name, Others...>();
			}
		}

		// wrapper to determine the type for the the column
		template <std::size_t Pos>
		static constexpr auto column_type() noexcept
		{
			constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

			return recurse_types<name, Schemas...>();
		}

		// asserts token is column separator, and if comma returns one past the comma else start position
		template <std::size_t Pos>
		static constexpr std::size_t next_column() noexcept
		{
			static_assert(isseparator(tokens_[Pos]), "Expected ',' or 'FROM' token following column declaration.");

			if constexpr (iscomma(tokens_[Pos]))
			{
				return Pos + 1;
			}
			else
			{
				return Pos;
			}
		}

		template <std::size_t Pos, bool Rename>
		static constexpr auto parse_colinfo() noexcept
		{
			static_assert(iscolumn(tokens_[Pos]), "Invalid token starting column delcaration.");

			constexpr bool rename{ isas(tokens_[Pos + 1]) && iscolumn(tokens_[Pos + 2]) };

			using col = decltype(column_type<Pos>());

			if constexpr (Rename && rename)
			{
				constexpr auto next{ next_column<Pos + 3>() };

				return colinfo<col, Pos + 2, next>{};
			}
			else if constexpr (rename)
			{
				constexpr auto next{ next_column<Pos + 3>() };

				return colinfo<col, Pos, next>{};
			}
			else
			{
				constexpr auto next{ next_column<Pos + 1>() };

				return colinfo<col, Pos, next>{};
			}
		}

		// recursively parse all columns projected/renamed in the query
		template <std::size_t Pos, bool Rename>
		static constexpr auto recurse_columns() noexcept
		{
			if constexpr (isfrom(tokens_[Pos]))
			{
				return context<Pos, sql::void_row>{};
			}
			else
			{
				constexpr auto info{ parse_colinfo<Pos, Rename>() };
				constexpr cexpr::string<char, tokens_[info.name].length() + 1> name{ tokens_[info.name] };
				constexpr auto child{ recurse_columns<info.next, Rename>() };

				using next = std::remove_const_t<typename decltype(child)::node>;
				using col = std::remove_const_t<decltype(sql::column<name, typename decltype(info)::type>{})>;
				using node = sql::row<col, next>;

				return context<child.pos, node>{};
			}
		}

		// wrapper to parse columns as a projection RA node
		template <std::size_t Pos>
		static constexpr auto parse_projection() noexcept
		{
			constexpr auto proj{ recurse_columns<Pos, false>() };
			constexpr auto next{ parse_from<proj.pos>() };

			using ranode = typename decltype(proj)::node;
			using node = ra::projection<ranode, typename decltype(next)::node>;

			return context<next.pos, node>{};
		}

		// wrapper to parse columns as a rename RA node
		template <std::size_t Pos>
		static constexpr auto parse_rename() noexcept
		{
			constexpr auto next = parse_projection<Pos>();

			using ranode = typename decltype(recurse_columns<Pos, true>())::node;
			using node = ra::rename<ranode, typename decltype(next)::node>;

			return context<next.pos, node>{};
		}

		// attempts to match column rename operation pattern on a column declaration
		template <std::size_t Pos>
		static constexpr bool has_rename() noexcept
		{
			if constexpr (isfrom(tokens_[Pos]) || isfrom(tokens_[Pos + 2]))
			{
				return false;
			}
			else if constexpr (iscolumn(tokens_[Pos]) && isas(tokens_[Pos + 1]) && iscolumn(tokens_[Pos + 2]))
			{
				return true;
			}
			else
			{
				// earlier "isfrom(tokens_[Pos + 2])" will catch FROM token
				static_assert(iscomma(tokens_[Pos + 1]), "Expected ',' or 'FROM' token following column declaration.");

				return has_rename<Pos + 2>();
			}
		}

		// decide RA node to root the expression tree
		template <std::size_t Pos>
		static constexpr auto parse_root() noexcept
		{
			static_assert(isselect(tokens_[Pos]), "Expected 'SELECT' token not found.");

			if constexpr (tokens_[Pos + 1] == "*")
			{
				return parse_from<Pos + 2>();
			}
			else if constexpr (has_rename<Pos + 1>())
			{
				return parse_rename<Pos + 1>();
			}
			else
			{
				return parse_projection<Pos + 1>();
			}
		}

		static constexpr sql::tokens<char, sql::preprocess(Str)> tokens_{ Str };

		using expression = typename decltype(parse_root<0>())::node;

		bool empty_;
	
	public:
		using iterator = query_iterator<expression>;
		using row_type = expression::output_type;

		query(Schemas const&... tables)
		{
			try 
			{
				expression::seed(tables...);
				empty_ = false;
			}
			catch(ra::data_end const& e)
			{
				empty_ = true;
			}
		}

		~query()
		{
			expression::reset();
		}

		iterator begin() const
		{
			return iterator{ empty_ };
		}

		iterator end() const
		{
			return iterator{ true };
		}
	};

} // namespace sql

