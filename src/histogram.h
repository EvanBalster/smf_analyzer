#pragma once


#include <filesystem>
#include <unordered_map>
#include <map>
#include <deque>

#ifndef USING_FILESYSTEM_GHC
#if __cplusplus < 201700 && _MSVC_LANG < 201700
#define USING_FILESYSTEM_GHC 1
#endif
#endif

#if USING_FILESYSTEM_GHC
#include <ghc/filesystem.hpp>
#endif

namespace smf_analyzer
{
#if USING_FILESYSTEM_GHC
	namespace filesystem = ghc::filesystem;
#else
	namespace filesystem = std::filesystem;
#endif
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
		void load(filesystem::path path);

		// Save data to file, skipping sizes not present in any histogram.
		void save(filesystem::path path) const;

		void operator=(const Table&) = delete;
	};
}