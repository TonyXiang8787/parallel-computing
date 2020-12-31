
#define NOMINMAX
#define _USE_MATH_DEFINES

#include <array>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <functional>
#include <memory>
#include <iostream>
#include <limits>
#include <thread>
#include <chrono>
#include <execution>

constexpr size_t N = 16;

#ifdef _DEBUG
constexpr size_t n_array = 100;
constexpr size_t n_batch = 16;
#else
constexpr size_t n_array = 100000;
constexpr size_t n_batch = 128;
#endif

constexpr size_t mutation_stride = 10;
constexpr size_t n_mutation = n_array / mutation_stride;
constexpr double base_mu = 10.0;
constexpr double base_sigma = 1.0;
constexpr double mutate_mu = 100.0;
constexpr double mutate_sigma = 10.0;


using Arr = std::array<double, N>;
using Input = std::vector<Arr>;

struct Result {
	double min, max, mean, sum;
};

struct SingleMutation {
	size_t index;
	Arr value;
};

using Mutation = std::vector<SingleMutation>;
using Mutations = std::vector<Mutation>;
using Results = std::vector<Result>;

struct ResultBatch {
	double time;
	Results results;
};


Arr get_array(double rms) {
	Arr arr{};
	double const m = rms * std::sqrt(2);
	for (size_t i = 0; i != N; ++i) {
		arr[i] = m * std::cos(2 * M_PI / N * i);
	}
	return arr;
}

Input gen_input() {
	Input input(n_array);
	std::mt19937 gen{ std::random_device{}() };
	std::normal_distribution<double> norm{ base_mu, base_sigma };
	std::transform(
		input.cbegin(), input.cend(), 
		input.begin(),
		[&](auto) { return get_array(norm(gen)); }
	);
	return input;
}

Mutations gen_mutation() {
	Mutations mutations(n_batch);
	std::mt19937 gen{ std::random_device{}() };
	std::normal_distribution<double> norm{ mutate_mu, mutate_sigma };
	std::transform(
		mutations.cbegin(), mutations.cend(),
		mutations.begin(),
		[&](auto) { 
			Mutation mutation(n_mutation);
			for (size_t i = 0; i != n_mutation; ++i) {
				mutation[i] = { i * mutation_stride,  get_array(norm(gen)) };
			}
			return mutation;
		}
	);
	return mutations;
}


class Element {
public:
	Element(Arr const& value) :
		value_{ value }
	{}

	void mutate(Arr const& value) {
		value_ = value;
	}

	double rms() const {
		return std::sqrt(
			std::transform_reduce(
				value_.cbegin(), value_.cend(),
			    0.0,
				std::plus{},
				[](double a) { return a * a + std::sin(a) + std::cos(a); }
			) 
			/ value_.size()
		);
	}
private:
	Arr value_;
};


class ValueContainer {
public:
	ValueContainer(Input const& vec) {
		vec_.reserve(vec.size());
		std::transform(
			vec.cbegin(),
			vec.cend(),
			std::back_inserter(vec_),
			[](Arr const& x) { return Element{ x }; }
		);
	}

	Element const& operator[] (size_t i) const { return vec_[i]; }

	size_t size() const { return vec_.size(); }

	void mutate(size_t i, Arr const& value) {
		vec_[i].mutate(value);
	}

private:
	std::vector<Element> vec_;
};

class PtrContainer {
public:
	PtrContainer(Input const& vec) {
		vec_.reserve(vec.size());
		std::transform(
			vec.cbegin(),
			vec.cend(),
			std::back_inserter(vec_),
			[](Arr const& x) { return std::make_shared<Element const>(x); }
		);
	}

	Element const& operator[] (size_t i) const { return *vec_[i]; }

	size_t size() const { return vec_.size(); }

	void mutate(size_t i, Arr const& value) {
		Element e = *vec_[i];
		e.mutate(value);
		vec_[i] = std::make_shared<Element const>(e);
	}

private:
	std::vector<std::shared_ptr<Element const>> vec_;
};




template<class Container>
class Model {
public:
	Model(Input const& vec) :
		container_{ vec }
	{}

	Result calculate() const {
		Result result{
			std::numeric_limits<double>::infinity(),
			-std::numeric_limits<double>::infinity(),
			0.0,
			0.0
		};
		
		for (size_t i = 0; i != container_.size(); ++i) {
			double const rms = container_[i].rms();
			if (rms > result.max) {
				result.max = rms;
			}
			if (rms < result.min) {
				result.min = rms;
			}
			result.sum += rms;
		}
		result.mean = result.sum / container_.size();

		return result;
	}

	void mutate(Mutation const& mutations) {
		std::for_each(
			mutations.cbegin(),
			mutations.cend(),
			[this](SingleMutation mutation) { container_.mutate(mutation.index, mutation.value); }
		);
	}

	template<bool reset_model>
	void calculate_seq(
		Mutations const& mutations,
		std::vector<Result>& results) const {
		Model<Container> model{ *this };
		if constexpr (reset_model) {
			std::transform(
				mutations.cbegin(), mutations.cend(),
				results.begin(),
				[this, &model](Mutation const& mutation) {
					model = *this;
					model.mutate(mutation);
					return model.calculate();
				}
			);
		}
		else {
			std::transform(
				mutations.cbegin(), mutations.cend(),
				results.begin(),
				[this, &model](Mutation const& mutation) {
					model.mutate(mutation);
					return model.calculate();
				}
			);
		}
		
	}

	void calculate_stl_par(
		Mutations const& mutations,
		std::vector<Result>& results) const {
		std::transform(
			std::execution::par_unseq,
			mutations.cbegin(), mutations.cend(),
			results.begin(),
			[this] (Mutation const& mutation) {
				Model<Container> model{ *this };
				model.mutate(mutation);
				return model.calculate();
			}
		);
	}

	template<bool reset_model>
	void calculate_thread_par(
		Mutations const& mutations,
		std::vector<Result>& results
	) {
		size_t const num_threads = std::thread::hardware_concurrency();
		std::vector<std::thread> threads;
		threads.reserve(num_threads);
		for (size_t j = 0; j != num_threads; ++j) {
			threads.emplace_back(
				[this, num_threads, &mutations, &results](size_t jt) {
					Model<Container> model{ *this };
					for (size_t i = jt; i < mutations.size(); i += num_threads) {
						if constexpr (reset_model) {
							model = *this;
						}
						model.mutate(mutations[i]);
						results[i] = model.calculate();
					}
				},
				j
			);
		}
		for (auto& thread : threads) {
			thread.join();
		}
	}

	template<auto func>
	ResultBatch calculate_and_benchmark(
		Mutations const& mutations
	) {
		ResultBatch result_batch{};
		result_batch.results.resize(mutations.size());
		auto const t1 = std::chrono::high_resolution_clock::now();
		(this->*func)(mutations, result_batch.results);
		auto const t2 = std::chrono::high_resolution_clock::now();
		result_batch.time = std::chrono::duration<double>(t2 - t1).count();
		return result_batch;
	}


private:
	Container container_;
};

using ValueModel = Model<ValueContainer>;
using PtrModel = Model<PtrContainer>;


int main()
{
	std::cout << "Number of cores: " << std::thread::hardware_concurrency() << '\n';
	Input const input = gen_input();
	Mutations const mutations = gen_mutation();
	ValueModel value_model{ input };
	PtrModel ptr_model{ input };

	auto const result_seq_value = value_model.calculate_and_benchmark<&ValueModel::calculate_seq<false>>(mutations);
	auto const result_seq_value_reset = value_model.calculate_and_benchmark<&ValueModel::calculate_seq<true>>(mutations);
	auto const result_seq_ptr = ptr_model.calculate_and_benchmark<&PtrModel::calculate_seq<false>>(mutations);
	auto const result_seq_ptr_reset = ptr_model.calculate_and_benchmark<&PtrModel::calculate_seq<true>>(mutations);

	auto const result_stl_par_value_reset = value_model.calculate_and_benchmark<&ValueModel::calculate_stl_par>(mutations);
	auto const result_stl_par_ptr_reset = ptr_model.calculate_and_benchmark<&PtrModel::calculate_stl_par>(mutations);

	auto const result_thread_par_value = value_model.calculate_and_benchmark<&ValueModel::calculate_thread_par<false>>(mutations);
	auto const result_thread_par_value_reset = value_model.calculate_and_benchmark<&ValueModel::calculate_thread_par<true>>(mutations);
	auto const result_thread_par_ptr = ptr_model.calculate_and_benchmark<&PtrModel::calculate_thread_par<false>>(mutations);
	auto const result_thread_par_ptr_reset = ptr_model.calculate_and_benchmark<&PtrModel::calculate_thread_par<true>>(mutations);

	std::cout << "Array: " << n_array << ", batch:" << n_batch << "\n\n";

	std::cout << "Time of sequence calculation of value container without reset: " << result_seq_value.time << '\n';
	std::cout << "Time of sequence calculation of value container with reset: " << result_seq_value_reset.time << '\n';
	std::cout << "Time of sequence calculation of pointer container without reset: " << result_seq_ptr.time << '\n';
	std::cout << "Time of sequence calculation of pointer container with reset: " << result_seq_ptr_reset.time << '\n' << '\n';

	std::cout << "Time of stl parallel calculation of value container with reset: " << result_stl_par_value_reset.time << '\n';
	std::cout << "Time of stl parallel calculation of pointer container with reset: " << result_stl_par_ptr_reset.time << '\n' << '\n';

	std::cout << "Time of thread calculation of value container without reset: " << result_thread_par_value.time << '\n';
	std::cout << "Time of thread calculation of value container with reset: " << result_thread_par_value_reset.time << '\n';
	std::cout << "Time of thread calculation of pointer container without reset: " << result_thread_par_ptr.time << '\n';
	std::cout << "Time of thread calculation of pointer container with reset: " << result_thread_par_ptr_reset.time << '\n' << '\n';
	return 0;
}
