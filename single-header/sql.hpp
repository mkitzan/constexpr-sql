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

	template <typename CharT, std::size_t N>
	class string
	{
	public:
		using char_type = CharT;

		constexpr string() : size_{ 0 }, string_{ 0 }
		{}

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
	template <cexpr::string Name, typename Row, typename T>
	constexpr void set(Row& r, T const& value)
	{
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

	template <size_t I, typename Col, typename Next>
	struct tuple_element<I, sql::row<Col, Next>>
	{
		using type = decltype(sql::get<I>(sql::row<Col, Next>{}));
	};

} // namespace std

namespace sql
{

	template <cexpr::string... Columns>
	struct index
	{
		template <typename Row>
		struct comp
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
				std::multiset<row_type, typename Index::template comp<row_type>>
			>;
		using const_iterator = typename container::const_iterator;
		
		schema() = default;

		template <typename T, typename... Ts>
		schema(std::vector<T> const& col, Ts const&... cols) : schema{}
		{
			insert(col, cols...);
		}

		template <typename T, typename... Ts>
		schema(std::vector<T>&& col, Ts&&... cols) : schema{}
		{
			insert(std::forward<T>(col), std::forward<Ts>(cols)...);
		}

		template <typename... Ts>
		inline void emplace(Ts const&... vals)
		{
			table_.emplace_back(vals...);
		}

		template <typename... Ts>
		inline void emplace(Ts&&... vals)
		{
			table_.emplace_back(vals...);
		}

		template <typename T, typename... Ts>
		void insert(std::vector<T> const& col, Ts const&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(col[i], cols[i]...);
			}
		}

		template <typename T, typename... Ts>
		void insert(std::vector<T>&& col, Ts&&... cols)
		{
			for (std::size_t i{}; i < col.size(); ++i)
			{
				emplace(std::forward<T>(col[i]),std::forward<Ts>(cols[i])...);
			}
		}

		void insert(row_type const& row)
		{
			table_.push_back(row);
		}

		void insert(row_type&& row)
		{
			table_.push_back(std::forward<row_type>(row));
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

		template <typename Row, char Delim>
		void fill(std::fstream& file, Row& row)
		{
			if constexpr (!std::is_same_v<Row, sql::void_row>)
			{
				if constexpr (std::is_same_v<typename Row::column::type, std::string>)
				{
					if constexpr (std::is_same_v<typename Row::next, sql::void_row>)
					{
						std::getline(file, row.head());
					}
					else
					{
						std::getline(file, row.head(), Delim);
					}
				}
				else
				{
					file >> row.head();
				}

				fill<typename Row::next, Delim>(file, row.tail());
			}
		}

	} // namespace

	// helper function for users to load a data into a schema from a file
	template <typename Schema, char Delim>
	Schema load(std::string const& data)
	{
		auto file{ std::fstream(data) };
		Schema table{};
		typename Schema::row_type row{};

		while (file)
		{
			fill<typename Schema::row_type, Delim>(file, row);
			table.insert(std::move(row));

			// in case last stream extraction did not remove newline
			if (file.get() != '\n')
			{
				file.unget();
			}
		}

		return table;
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

	struct data_end : std::exception
	{};

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

	template <typename LeftInput, typename RightInput>
	class cross : public ra::join<LeftInput, RightInput>
	{
		using join_type = ra::join<LeftInput, RightInput>;
	public:
		using output_type = join_type::output_type;

		static auto& next()
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

			return join_type::output_row;
		}
	};

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

		static auto& next()
		{
			while (curr == end)
			{
				copy(join_type::output_row, LeftInput::next());
				auto const& active{ row_cache[join_type::output_row.head()] };
				curr = active.cbegin();
				end = active.cend();
			}

			copy(join_type::output_row, *curr++);
			
			return join_type::output_row;
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

		static auto& next()
		{
			fold<output_type>(output_row, Input::next());
			
			return output_row;
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

		static auto& next()
		{
			fold<output_type, input_type>(output_row, Input::next());
			
			return output_row;
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

		static auto& next()
		{
			output_row = Input::next();

			while(!Predicate::eval(output_row))
			{
				output_row = Input::next();
			}

			return output_row;
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
		
		template <typename CharT>
		constexpr bool whitespace(CharT curr)
		{
			return curr == CharT{ ' ' } || curr == CharT{ '\t' } || curr == CharT{ '\n' };
		}

		template <typename CharT>
		constexpr bool syntax(CharT curr)
		{
			return curr == CharT{ ',' } || curr == CharT{ '(' } || curr == CharT{ ')' } ||
				curr == CharT{ '\'' } || curr == CharT{ '\"' } || curr == CharT{ '=' };
		}

		template <typename CharT>
		constexpr const CharT* skip(const CharT *curr, const CharT *end)
		{
			for (; curr != end && whitespace(*curr); ++curr);
			return curr;
		}

		template <typename CharT>
		constexpr const CharT* next(const CharT *curr, const CharT *end)
		{
			CharT c{ *curr };

			if (c == CharT{ '>' } || c == CharT{ '<' } || c == CharT{ '!' })
			{
				++curr;

				if (*curr == CharT{ '=' } || (c == CharT{ '<' } && *curr == CharT{ '>' }))
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

	template <typename CharT, std::size_t Tn>
	class tokens
	{
	public:
		using token_view = std::basic_string_view<CharT>;

		template<std::size_t N>
		constexpr tokens(cexpr::string<CharT, N> const& cs) : tokens_{}
		{
			auto curr{ cs.cbegin() }, last{ cs.cbegin() };
			const auto end{ cs.cend() };
			std::size_t i{};

			while (curr < end)
			{
				curr = skip(curr, end);
				last = curr;
				last = next(last, end);

				if (*curr == CharT{ '\"' } || *curr == CharT{ '\'' })
				{
					tokens_[i++] = token_view{ curr, 1 };
					for (char c{ *curr++ }; last != end && *last != c; ++last);
				}

				auto len{ reinterpret_cast<std::size_t>(last) - reinterpret_cast<std::size_t>(curr) };
				tokens_[i++] = token_view{ curr, len };

				if (*last == CharT{ '\"' } || *last == CharT{ '\'' })
				{
					tokens_[i++] = token_view{ last, 1 };
					++last;
				}

				curr = last;
			}
		}

		constexpr std::size_t count() const noexcept
		{
			return Tn;
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
		std::array<token_view, Tn> tokens_;
	};

	template<typename CharT, std::size_t N>
	constexpr std::size_t preprocess(cexpr::string<CharT, N> const& cs)
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
		template <typename ValT>
		struct value
		{
			constexpr value(ValT v) : val{ v }
			{}

			ValT val;
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
		static constexpr auto eval(Row const& row)
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
		struct TreeNode
		{
			using node = Node;
			static constexpr std::size_t pos = Pos;
		};

		template <typename Type, std::size_t Name, std::size_t Next>
		struct ColInfo
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

		template <typename ValT, typename CharT, std::size_t N>
		constexpr value<ValT> convert(cexpr::string<CharT, N> const& str)
		{
			auto curr{ str.cbegin() }, end{ str.cend() };
			constexpr CharT nul{ '\0' }, dot{ '.' }, zro{ '0' }, min{ '-' };
			ValT acc{}, sign{ 1 }, scalar{ 10 };

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
					acc += (*curr - zro) * (scalar /= ValT{ 10 });
					++curr;
				}
			}

			return value<ValT>{ acc * sign };
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
			return c == '-' || c == '.' || (c >= '0' && c <= '9');
		}

		constexpr bool iscomp(std::string_view const& tv) noexcept
		{
			return tv[0] == '=' || tv[0] == '!' || tv[0] == '<' || tv[0] == '>';
		}

		constexpr bool isor(std::string_view const& tv) noexcept
		{
			return tv == "or" || tv == "OR";
		}

		constexpr bool isand(std::string_view const& tv) noexcept
		{
			return tv == "and" || tv == "AND";
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

				return TreeNode<next.pos + 1, node>{};
			}
			else if constexpr (tokens_[Pos] == "\'" || tokens_[Pos] == "\"")
			{
				constexpr cexpr::string<char, tokens_[Pos + 1].length() + 1> name{ tokens_[Pos + 1] };

				using str = decltype(name);
				using node = sql::constant<value<str>{ name }, Row>;

				return TreeNode<Pos + 3, node>{};
			}
			else if constexpr (isdigit(tokens_[Pos][0]))
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };
				
				using val = decltype(isintegral(tokens_[Pos]) ? std::int64_t{} : double{});
				using node = sql::constant<sql::convert<val>(name), Row>;

				return TreeNode<Pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = sql::variable<name, Row>;

				return TreeNode<Pos + 1, node>{};
			}
		}

		// parses a single compare operation
		template <typename Left, typename Row>
		static constexpr auto recurse_comp() noexcept
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

				return TreeNode<next.pos, node>{};
			}			
		}

		// descend further and attempt to parse a compare operation
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_comp() noexcept
		{
			using left = decltype(parse_terms<Pos, Row>());
			
			return recurse_comp<left, Row>();
		}

		// attempt to parse a negation operation then descend further
		template <std::size_t Pos, typename Row>
		static constexpr auto parse_negation() noexcept
		{
			if constexpr (tokens_[Pos] == "not" || tokens_[Pos] == "NOT")
			{
				constexpr auto next{ parse_comp<Pos + 1, Row>() };

				using ranode = typename decltype(next)::node;
				using node = sql::operation<"NOT", Row, ranode>;

				return TreeNode<next.pos, node>{};
			}
			else
			{
				return parse_comp<Pos, Row>();
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

				return recurse_and<TreeNode<next.pos, node>, Row>();
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

				return recurse_or<TreeNode<next.pos, node>, Row>();
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
				return recurse_schemas<Name, Id, Others...>();
			}
		}

		// wrapper function to determine terminal relation
		template <std::size_t Pos>
		static constexpr auto parse_schema()
		{
			if constexpr (tokens_[Pos] == "(")
			{
				constexpr auto next{ parse_root<Pos + 2>() };
				
				using node = typename decltype(next)::node;

				return TreeNode<next.pos + 1, node>{};
			}
			else
			{
				constexpr cexpr::string<char, tokens_[Pos].length() + 1> name{ tokens_[Pos] };

				using node = decltype(recurse_schemas<name, Pos, Schemas...>());

				return TreeNode<Pos + 1, node>{};
			}
		}

		// stub which will choose the specific join RA node
		template <std::size_t Pos, typename Left, typename Right>
		static constexpr auto choose_join()
		{
			if constexpr (tokens_[Pos] == "natural" || tokens_[Pos] == "NATURAL")
			{
				return ra::natural<Left, Right>{};
			}
			else
			{
				return ra::cross<Left, Right>{};	
			}
		}

		// parses join info if a join is present else returns the single relation terminal
		template <std::size_t Pos>
		static constexpr auto parse_join() noexcept
		{
			constexpr auto lnext{ parse_schema<Pos>() };

			using lnode = typename decltype(lnext)::node;

			if constexpr (lnext.pos + 2 < tokens_.count() && (tokens_[lnext.pos + 1] == "join" || tokens_[lnext.pos + 1] == "JOIN"))
			{
				constexpr auto rnext{ parse_schema<lnext.pos + 2>() };

				using rnode = typename decltype(rnext)::node;
				using join = decltype(choose_join<lnext.pos, lnode, rnode>());

				return TreeNode<rnext.pos, join>{};
			}
			else
			{
				return TreeNode<lnext.pos, lnode>{};
			}
		}

		// starting point to parse everything after the from keyword
		template <std::size_t Pos>
		static constexpr auto parse_from() noexcept
		{
			constexpr auto next{ parse_join<Pos>() };

			using node = typename decltype(next)::node;

			if constexpr (next.pos + 1 < tokens_.count() && (tokens_[next.pos] == "where" || tokens_[next.pos] == "WHERE"))
			{
				using output = std::remove_cvref_t<typename node::output_type>;
				using predicate = typename decltype(parse_or<next.pos + 1, output>())::node;

				return ra::selection<predicate, node>{};
			}
			else
			{
				return node{};	
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

		// searches for the token position of start of the next column in the token array
		template <std::size_t Pos>
		static constexpr auto next_column() noexcept
		{
			if constexpr (tokens_[Pos] == ",")
			{
				return Pos + 1;
			}
			else if constexpr (tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return Pos;
			}
			else
			{
				return next_column<Pos + 1>();
			}
		}

		// search for if the column is renamed, and returns the position of the name change if so
		template <std::size_t Pos, std::size_t Curr>
		static constexpr auto find_rename() noexcept
		{
			if constexpr (tokens_[Pos] == "," || tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return Curr;
			}
			else if constexpr (tokens_[Pos] == "as" || tokens_[Pos] == "AS")
			{
				return Pos + 1;
			}
			else
			{
				return find_rename<Pos + 1, Curr>();
			}
		}

		// parses the column starting from Pos for all it's info (name, type, and pos of next column)
		template <std::size_t Pos, bool Rename>
		static constexpr auto parse_colinfo() noexcept
		{
			constexpr auto next{ next_column<Pos>() };

			if constexpr (Rename)
			{
				constexpr auto offset{ find_rename<Pos, Pos>() };

				using col = decltype(column_type<Pos>());
				
				return ColInfo<col, offset, next>{};
			}
			else
			{
				return ColInfo<std::remove_const_t<decltype(column_type<Pos>())>, Pos, next>{};
			}
		}

		// recursively parse all columns projected/renamed in the query
		template <std::size_t Pos, bool Rename>
		static constexpr auto recurse_columns() noexcept
		{
			if constexpr (tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return TreeNode<Pos, sql::void_row>{};
			}
			else
			{
				constexpr auto info{ parse_colinfo<Pos, Rename>() };
				constexpr cexpr::string<char, tokens_[info.name].length() + 1> name{ tokens_[info.name] };
				constexpr auto child{ recurse_columns<info.next, Rename>() };

				using next = std::remove_const_t<typename decltype(child)::node>;
				using col = std::remove_const_t<decltype(sql::column<name, typename decltype(info)::type>{})>;

				return TreeNode<child.pos, sql::row<col, next>>{};
			}
		}

		// wrapper to parse columns as a projection RA node
		template <std::size_t Pos>
		static constexpr auto parse_projection() noexcept
		{
			constexpr auto projection{ recurse_columns<Pos, false>() };

			using node = typename decltype(projection)::node;
			using next = decltype(parse_from<projection.pos + 1>());

			return ra::projection<node, next>{};
		}

		// wrapper to parse columns as a rename RA node
		template <std::size_t Pos>
		static constexpr auto parse_rename() noexcept
		{
			using node = typename decltype(recurse_columns<Pos, true>())::node;
			using next = decltype(parse_projection<Pos>());

			return ra::rename<node, next>{};
		}

		template <std::size_t Pos>
		static constexpr bool has_rename() noexcept
		{
			if constexpr (tokens_[Pos] == "from" || tokens_[Pos] == "FROM")
			{
				return false;
			}
			else if constexpr (tokens_[Pos] == "as" || tokens_[Pos] == "AS")
			{
				return true;
			}
			else
			{
				return has_rename<Pos + 1>();
			}
		};

		// decide RA node to root the expression tree
		template <std::size_t Pos>
		static constexpr auto parse_root() noexcept
		{
			if constexpr (tokens_[Pos] == "*")
			{
				return parse_from<Pos + 2>();
			}
			else if constexpr (has_rename<Pos + 1>())
			{
				return parse_projection<Pos>();
			}
			else
			{
				return parse_rename<Pos>();
			}
		}

		static constexpr sql::tokens<char, sql::preprocess(Str)> tokens_{ Str };

		using expression = decltype(parse_root<1>());
	
	public:
		using iterator = query_iterator<expression>;

		constexpr query(Schemas const&... tables)
		{
			expression::seed(tables...);
		}

		~query()
		{
			expression::reset();
		}

		constexpr iterator begin()
		{
			return iterator{ false };
		}

		constexpr iterator end()
		{
			return iterator{ true };
		}
	};

} // namespace sql

