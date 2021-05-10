#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <filesystem>

#include "histogram.h"

namespace smf_analyzer
{
	namespace filesystem = std::filesystem;
	using namespace std::string_literals;

	enum TextEncoding { ANSI, ShiftJIS };
	const static std::string TXT_BYTES = "Text Meta Events (bytes)";
	const static std::string TXT_CODEPOINTS = "Text Meta Events (codepoints)";
	const static std::string TXT_UTF8BYTES = "Text Meta Events (UTF-8 bytes)";
	const static std::string NONTXT = "Non-Text Meta Events (bytes)";
	const static std::string SYSEX = "SysEx Events (bytes)";

	/* 
		Recursively parse directory for all .smf, .mid, .midi files (SMF files)
		Parse out meta events and universal sysex events
		Determine, for each event: length, category/type, then amass a count of these in the histogram

		First level function: Reads through file system, finds files, passes them to #2
		Second level function: Reads through files, finds events, passes them to the histogram guy

		We only care about the "contents" of text meta events, because we need to read out the length of them irrespective of their encoding	
			Types from 0x01 to under 0x20 are all "text" events and need to be handled this way.
			Other types are mostly fixed length with the exception of 0x7F "sequencer specific" also.
	*/

	class SMF_Parser
	{
	public:
		// Recursively scan a given directory and its sub-directories for SMF files.
		void read_directory(std::map<std::string, Table>& tables, filesystem::path folder, std::ofstream& errstream);

		// Parse relevant data (events, with their type and length) from an SMF file and load it into our histogram.
		void parse_file(std::map<std::string, Table>& tables, filesystem::path file, std::ofstream& errstream);

		// Parse text event lengths from a text meta event (in bytes, code points, and utf-8 code points).
		void parse_text_event(std::map<std::string, Table>& tables, std::vector<char> text, std::string eventType, std::ofstream& errstream);

	private:
		// Read a MIDI variable-length value from a file
		int read_variable_length(std::ifstream& fstream);

		TextEncoding textEncoding = TextEncoding::ANSI;
		
		// Extensions of standard midi files (lowercase), which are relevant to our analysis.
		const std::vector<std::string> SMF_EXTENSIONS = { ".smf", ".mid", ".midi", ".kar" }; 

		// Map of meta event opcodes for output purposes
		const std::map<int, std::string> META_EVENT_OPCODES = {
			{0x00, "Sequence Number"},
			{0x20, "MIDI Channel Prefix"},
			{0x2F, "End of Track"},
			{0x51, "Set Tempo"},
			{0x54, "SMPTE Offset"},
			{0x58, "Time Signature"},
			{0x59, "Major/Minor Key Signature"},
			{0x01, "Text Event"},
			{0x02, "Copyright Notice"},
			{0x03, "Sequence/Track Name"},
			{0x04, "Instrument Name"},
			{0x05, "Lyric"},
			{0x06, "Marker"},
			{0x07, "Cue Point"},
			{0x7F, "Sequencer Specific"}
		};
	};

	class MidiException : public std::exception {
		const char* message;
	public:
		MidiException(const char* msg) : message(msg) {};
		const char * what() const throw () {
			return message;
		}
	};
}
