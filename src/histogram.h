#pragma once


#include <filesystem>
#include <unordered_map>
#include <map>
#include <deque>


namespace smf_analyzer
{
	// 
	class Histogram
	{
	public:
		using FastMap = std::unordered_map<size_t, size_t>;
		//using SortMap = std::map          <size_t, size_t>;

	public:
		FastMap     data;

	public:
		void inc(size_t key)                  {++data[key];}
		void add(size_t key, size_t count)    {data[key] += count;}
	};

	// 
	class Table
	{
	public:
		std::map<std::string, Histogram> columns;

	public:
		// Load data from a file, merging into current data based on column names.
		void load(std::filesystem::path path);

		// Save data to filem skipping sizes not present in any histogram.
		void save(std::filesystem::path path) const;
	};
}