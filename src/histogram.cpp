#include <locale>
#include <fstream>
#include <sstream>
#include <set>

#include "histogram.h"


using namespace smf_analyzer;


void Table::load(filesystem::path path)
{
	// Use input exceptions and the CSV character classification
	std::ifstream in;
	in.exceptions(std::ios::failbit | std::ios::badbit);
	in.open(path);

	std::string line;
	std::getline(in, line);

	std::vector<Histogram*> order;

	// Read headings
	{
		std::istringstream line_in(line);
		line_in.exceptions(std::ios::failbit | std::ios::badbit);

		bool need_size_heading = true;
		std::string heading;

		while (true)
		{
			heading.clear();
			std::getline(line_in, heading);

			if (need_size_heading)
			{
				if (heading != "size")
					throw std::invalid_argument("Table file's first heading must be 'size'");
				need_size_heading = false;
			}
			else
			{
				auto p = columns.find(heading);
				if (p == columns.end()) p = columns.emplace(heading, Histogram()).first;
				order.push_back(&p->second);
			}
		}
	}

	// Read data
	while (!in.eof())
	{
		line.clear();
		std::getline(in, line);
		if (!line.length()) continue;

		std::istringstream line_in(line);
		line_in.exceptions(std::ios::failbit | std::ios::badbit);

		std::string number;

		static const size_t NEED_SIZE = ~size_t(0);
		size_t size = NEED_SIZE;
		Histogram **col = order.data(), **last = col + order.size();
		while (!line_in.eof() && col < last)
		{
			number.clear();
			std::getline(line_in, number, ',');

			auto value = std::stoull(number);
			if (size == NEED_SIZE)
			{
				// First column is always size
				size = value;
			}
			else
			{
				(*col)->add(size, value);
				++col;
			}
		}
	}
}

void Table::save(filesystem::path path) const
{
	// Use output exceptions
	std::ofstream out;
	out.exceptions(std::ios::failbit | std::ios::badbit);
	out.open(path);

	out << "size,";
	for (auto &col : columns)
		out << col.first << ",";
	out << std::endl;

	// Get all sizes...
	std::set<size_t> sizes;
	for (auto &col : columns) for (auto &entry : col.second.data) sizes.insert(entry.first);

	// Write histogram data
	for (size_t size : sizes)
	{
		out << size << ',';
		for (auto &col : columns)
		{
			auto p = col.second.data.find(size);
			size_t count = ((p == col.second.data.end()) ? 0 : p->second);
			out << count << ',';
		}
		out << std::endl;
	}
	out.close();
}