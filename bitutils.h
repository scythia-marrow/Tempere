#ifndef bitutils_h
#define bitutils_h
// TODO: This does not work with types larger than the integer max.
// 	So I will need custom behavior for structs and stuff?
template <typename C> class BitIterator
{
	static const int start = 0;
	static const int finish = sizeof(C);
	int val = start;
	public:
		BitIterator(int _val = 0) : val(_val) {}
		BitIterator& operator++()
		{
			val = finish >= start ? val + 1 : val - 1;
			return *this;
		}
		BitIterator operator++(int) { ++val; return *this;}
		BitIterator begin() { return start; }
		BitIterator end()
		{
			return finish >= start ? finish + 1 : finish - 1;
		}
		bool operator==(BitIterator other) const
		{
			return val == other.val;
		}
		bool operator!=(BitIterator other) const
		{
			return !(*this == other);
		}
		// The actual genius part
		C operator*() { return static_cast<C>(1 << val);}
		// types
		using difference_type = int;
		using value_type = int;
		using pointer = const C*;
		using reference = const C&;
		using iterator_category = std::forward_iterator_tag;
};
#endif
