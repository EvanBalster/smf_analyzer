#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>

#include "histogram.h"

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
	using namespace std::string_literals;

	enum TextEncoding { ANSI, ShiftJIS };

	// Defined names for the categories of events we're interested in counting
	const static std::string TXT_BYTES = "Text Meta Events (bytes)";
	const static std::string TXT_CODEPOINTS = "Text Meta Events (codepoints)";
	const static std::string TXT_UTF8BYTES = "Text Meta Events (UTF-8 bytes)";
	const static std::string NONTXT = "Non-Text Meta Events (bytes)";
	const static std::string SYSEX = "Universal SysEx Real-Time Events (bytes)";

	class SMF_Parser
	{
	public:
		// Recursively scan a given directory and its sub-directories for SMF files.
		void read_directory(std::map<std::string, Table>& tables, filesystem::path folder, std::ofstream& errstream);

		// Parse relevant events (their type and length in bytes) from an SMF file and save it into the relevant histogram.
		void parse_file(std::map<std::string, Table>& tables, filesystem::path file, std::ofstream& errstream);

		// Parse lengths from a text meta event (in code points and utf-8 code points) and save it into the relevant histogram.
		void parse_text_event(std::map<std::string, Table>& tables, std::vector<char> text, std::string eventType, TextEncoding encoding, std::ofstream& errstream);

	private:
		// Read a MIDI variable-length value from a filestream
		int read_variable_length(std::ifstream& fstream);
		
		// Possible extensions of standard midi files which are relevant to our analysis
		const std::vector<std::string> SMF_EXTENSIONS = { ".smf", ".mid", ".midi", ".kar" }; 

		// Meta event names (mapped to opcodes) for output readability
		const std::map<int, std::string> META_EVENT_OPCODES = {
			{0x00, "00_Sequence Num."},
			{0x20, "32_Channel Prefix"},
			{0x2F, "47_End of Track"},
			{0x51, "81_Set Tempo"},
			{0x54, "84_SMPTE Offset"},
			{0x58, "88_Time Sig."},
			{0x59, "89_Key Sig."},
			{0x01, "01_Text"},
			{0x02, "02_Copyright"},
			{0x03, "03_Seq./Track"},
			{0x04, "04_Instrument"},
			{0x05, "05_Lyric"},
			{0x06, "06_Marker"},
			{0x07, "07_Cue Point"},
			{0x7F, "127_Seq.Specific"}
		};
	};

	// Exception type for identifying formatting issues in midi files
	class MidiException : public std::exception {
		const std::string message;
	public:
		MidiException(const char* msg) : message(msg) {};
		MidiException(const std::string& msg) : message(msg) {};
		const char * what() const throw () {
			return message.c_str();
		}
	};
}
