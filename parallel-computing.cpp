
#include <array>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>
#include <iostream>

constexpr size_t N = 16;

using Arr = std::array<double, N>;

class Element {
public:
	Element(Arr const& value) :
		value_{ value }
	{}

	void mutate(Arr const& value) {
		value_ = value;
	}

	double rms() const {
		return std::transform_reduce(
			value_.cbegin(),
			value_.cend(),
			0.0,
			std::plus{},
			[](double a) { return a * a; }
		) / value_.size();
	}
private:
	Arr value_;
};


class ValueContainer {
public:
	ValueContainer(std::vector<Arr> const& vec) {
		vec_.reserve(vec.size());
		std::transform(
			vec.cbegin(),
			vec.cend(),
			std::back_inserter(vec_),
			[](Arr const& x) { return Element{ x }; }
		);
	}

	Element const& operator[] (size_t i) const { return vec_[i]; }

	void mutate(size_t i, Arr const& value) {
		vec_[i].mutate(value);
	}

private:
	std::vector<Element> vec_;
};

class PtrContainer {
public:
	PtrContainer(std::vector<Arr> const& vec) {
		vec_.reserve(vec.size());
		std::transform(
			vec.cbegin(),
			vec.cend(),
			std::back_inserter(vec_),
			[](Arr const& x) { return std::make_shared<Element const>(x); }
		);
	}

	Element const& operator[] (size_t i) const { return *vec_[i]; }

	void mutate(size_t i, Arr const& value) {
		Element e = *vec_[i];
		e.mutate(value);
		vec_[i] = std::make_shared<Element const>(e);
	}

private:
	std::vector<std::shared_ptr<Element const>> vec_;
};


struct Result {
	double min, max, mean, sum;
};

struct Mutation {
	size_t index;
	Arr value;
};


template<class Container>
class Model {
public:

private:
	Container container_;
};


int main()
{
	std::cout << "Hello CMake." << '\n';
	return 0;
}
