// smf_analyzer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iterator>
#include <algorithm>
#include <locale>
#include <fstream>

#include "smf_analyzer.h"
#include "inline.h"

using namespace smf_analyzer;
using namespace std::string_literals;

/*
	Command line args are:
		1: directory to read from
		2: directory to output histogram files to
*/
int main(int argc, char* argv[])
{
	filesystem::path input = argv[1]; 
	filesystem::path output = argv[2];
	std::ofstream log;
	filesystem::path logpath = output;
	logpath += "/";
	logpath += "log.txt";
	log.open(logpath, std::ios_base::out); // A file in which to log all text event contents for reference and debugging.

	std::map<std::string, Table> myTables;
	SMF_Parser parser;

	parser.read_directory(myTables, input, log);

	std::cout << "Saving histograms to output folder: " << output << std::endl;
	for (std::map<std::string,Table>::iterator it = myTables.begin(); it != myTables.end(); ++it) {
		filesystem::path p = output;
		p += "/";
		p += it->first;
		if (filesystem::exists(p)) {
			p += "_new";
		}
		p += ".csv"; 
		(it->second).save(p);
	}

	std::cout << "Analysis completed." << std::endl;
	return 0;
}


void SMF_Parser::read_directory (std::map<std::string, Table>& tables, filesystem::path folder, std::ofstream& log)
{
	std::cout << "Beginning analysis of files in directory: " << folder.u8string() << std::endl;

	try {
		for (filesystem::recursive_directory_iterator iter(folder); iter != filesystem::recursive_directory_iterator{} ; ++iter)
		{
			if ((*iter).is_regular_file()) // If the file is SMF, analyze it
			{
				// Get file's extension (case-insensitive)
				std::string extension = (*iter).path().extension().u8string();
				for (char& c : extension) c = std::tolower(c, std::locale::classic());

				// Check if the file has one of our SMF file extensions
				if (std::find(std::begin(SMF_EXTENSIONS), std::end(SMF_EXTENSIONS), extension) != std::end(SMF_EXTENSIONS))
				{
					// Send SMF file's path through to the parse_file function for analysis.
					try {
						parse_file(tables, (*iter).path(), log);
					}
					catch (const MidiException& e) { // Exception case for improperly formatted midi files.
						std::cerr << "MIDI FILE ERROR: " << e.what() << std::endl;
					}
					catch (const std::exception& e) { // Catch any other exceptions related to this file, and continue analyzing files.
						std::cerr << "ERROR: " << e.what() << std::endl;
					}
				}
			}
			else // File is somehow unreadable; report this error and continue to next
			{
				std::cerr << "ERROR: Could not read file at " << (*iter).path().u8string() << std::endl;
			}
		}
	}
	catch (const filesystem::filesystem_error& e) { // Exception case for filesystem errors (most likely the directory does not exist)
		std::cerr << "FILESYSTEM ERROR: " << e.what() << std::endl;
		return;
	}

	std::cout << "Finished analysis of files in directory: " << folder.u8string() << std::endl;
}

void SMF_Parser::parse_file(std::map<std::string, Table>& tables, filesystem::path file, std::ofstream& log)
{
	std::ifstream in;
	in.exceptions(std::ios::failbit | std::ios::badbit); // use std exceptions for in
	in.open(file, std::ios_base::binary | std::ios_base::in);

	log << "===== NEW FILE =====" << std::endl; log.flush();
	std::cout << "Analyzing file: " << file.u8string() << std::endl;
	log << "file: " << file.filename().u8string() << std::endl; log.flush();
	TextEncoding encoding = TextEncoding::ANSI; // Read text events as ANSI unless (until) the file tells us that it's in shift-JIS.

	std::vector<char> buffer;
	buffer.resize(5);
	buffer[4] = '\0';
	char* buff = reinterpret_cast<char*>(buffer.data());

	// Read header chunk
	in.read(buff, 4); log << std::string(buff, 4) << " "; log.flush(); // MThd
	if (std::string(buff,4) != "MThd"s) throw MidiException("Improper header format. Skipping file...");
	in.read(buff, 4); // header length
	int hdlength = read4BE(reinterpret_cast<uint8_t*>(buff)); log << hdlength << " "; log.flush();
	in.read(buff, 2); // format (single track, multiple track, multiple song)
	in.read(buff, 2); // number of tracks
	int tracks = read2BE(reinterpret_cast<uint8_t*>(buff)); log << tracks << " "; log.flush();
	in.read(buff, 2); // division

	// Read each track, as per the number of tracks given in the header.
	for (int i = 0; i < tracks; i++)
	{
		// Start of track
		in.read(buff, 4); log << std::string(buff, 4) << " "; // MTrk 
		if (std::string(buff, 4) != "MTrk"s) {
			throw MidiException("Improper track format at track #" + std::to_string(i+1) + ". Skipping remainder of file...");
		}
		in.read(buff, 4); // track length (in bytes past this point)
		int trackLength = read4BE(reinterpret_cast<uint8_t*>(buff)); log << trackLength << " "; log.flush();
		int trackStart = in.tellg();
		int runningStatus = 0;

		// Track events
		while (in.tellg() < trackStart + trackLength)
		{
			int deltatime = read_variable_length(in);

			in.read(buff, 1); // First byte of event message (status byte or event type byte)
			int statusByte = *(reinterpret_cast<uint8_t*>(buff));

			// Check for running status
			if (statusByte & 0x80) { // Store current status as the running status for next event, in case it's needed
				if (statusByte != 0xFF) runningStatus = statusByte; // but ignore this status if it's meta event
			}
			else { // Running status is in use
				// 0xF6 Tune request cannot come with data bytes and thus cannot be used with running status.
				if (runningStatus == 0xF6 || runningStatus == 0xFF || runningStatus == 0xF0) { // Abandon this file, for it is cursed
					throw MidiException("Malformed running status in track #" + std::to_string(i+1) + ". Skipping remainder of file...");
				}
				in.putback(statusByte); // this is not actually what we need for status, so return it to the stream for now
				statusByte = runningStatus; // use running status as statusByte value
			}

			int length = 0;
			if (statusByte == 0xFF) // Meta Event
			{
				in.read(buff, 1); // meta event type
				int opcode = *(reinterpret_cast<uint8_t*>(buff));
				length = read_variable_length(in); // event length
				log << "meta:type:" << opcode << " status:" << runningStatus << " "; log.flush();

				// Resize buffer to the length of the event, and re-align pointer
				buffer.resize(length);
				buff = reinterpret_cast<char*>(buffer.data());

				// Get the name of this meta event, if we know it from our opcode table (otherwise just use the opcode)
				std::string eventName = std::to_string(opcode);
				if (META_EVENT_OPCODES.find(opcode) != META_EVENT_OPCODES.end()) {
					eventName = META_EVENT_OPCODES.at(opcode);
				}

				// Read the event data
				in.read(buff, length); 
				if (opcode > 0x00 && opcode < 0x20) // Opcode range for text meta events
				{
					log << "contents:" << std::string(buff, length) << std::endl; log.flush();
					// Determine if character code set is defined by this message, and set ours if so
					if (std::string(buff, 2) == "{@"s) // Character code set tag begins with {@
					{
						if (std::toupper(buff, std::locale::classic()) == "{@LATIN}"s) {
							encoding = TextEncoding::ANSI;
						}
						else if (std::toupper(buff, std::locale::classic()) == "{@JP}"s) {
							encoding = TextEncoding::ShiftJIS;
						}
						else { // If the codeset is unclear, don't change it and continue on, but report this anomaly
							std::cerr << file << " - Unknown text encoding found: " << std::string(buff, length) << std::endl;
						}
					}

					// Parse length of text event in codepoints & UTF8 bytes, according to codeset, and put into histogram
					parse_text_event(tables, buffer, eventName, encoding, log);

					// Input message type & length in regular bytes to histogram
					tables[TXT_BYTES].columns[eventName].inc(length);
				}
				// Non-text meta event
				else {
					tables[NONTXT].columns[eventName].inc(length);
				}
			}
			else if (statusByte == 0xF0 || statusByte == 0xF7) // Universal SysEx Event
			{
				log << "sysex:time" << std::string(buff, 1) << " "; log.flush();
				length = read_variable_length(in); // event length

				// Resize buffer to the length of the event, and re-align pointer
				buffer.resize(length);
				buff = reinterpret_cast<char*>(buffer.data());

				in.read(buff, length); // event data bytes
				if (buff[0] == 0x7F) // Universal SysEx (realtime)
				{
					// Input values into histogram
					tables[SYSEX].columns["Universal SysEx (realtime)"].inc(length);
				}
				else // Some other kind of SysEx - not used in current data analysis
				{
					// TODO: don't input non-realtime sysex (0x7E) into histogram? should I do something else with them?
				}
			}
			else if (statusByte == 0xF4 || statusByte == 0xF5) { // Unused status codes; abandon this file for it is cursed
				throw MidiException("Malformed status byte in track #" + std::to_string(i+1) + ". Skipping remainder of file...");
			}
			else // Pass over to next event
			{
				// Check status byte to know if this message has additional bytes we need to pass over
				switch (statusByte & 0xF0) {
				case 0x80: // 2-byte long messages
					[[fallthrough]];
				case 0x90:
					[[fallthrough]];
				case 0xE0:
					[[fallthrough]];
				case 0xA0:
					[[fallthrough]];
				case 0xB0:
					in.read(buff, 2);
					break;
				case 0xC0: // 1-byte long messages
					[[fallthrough]];
				case 0xD0:
					in.read(buff, 1);
					break;
				case 0xF0:
					switch (statusByte) {
					case 0xF1:
						in.read(buff, 1);
						break;
					case 0xF2:
						in.read(buff, 2);
						break;
					case 0xF3:
						in.read(buff, 1);
						break;
					default: // all other cases are 0 bytes long
						break;
					}
				default: // 0 byte messages
					break;
				}
			}
		}
	}

	std::cout << " File complete." << std::endl;
	log << "File complete." << std::endl; log.flush();
}

int SMF_Parser::read_variable_length (std::ifstream& ifs) 
{
	ifs.exceptions(std::ios::failbit | std::ios::badbit);
	char c = ifs.get();
	int len = c;

	if (len & 0x80) // If first bit is an escape bit, keep reading
	{
		len &= 0x7F;
		do { 
			len = (len << 7) + ((c = ifs.get()) & 0x7F);
		} while (c & 0x80);
	}
	return len;
}

void SMF_Parser::parse_text_event (std::map<std::string, Table>& tables, std::vector<char> text, std::string eventType, TextEncoding encoding, std::ofstream& log)
{
	int length_utf8bytes = 0;
	int length_codepoints = 0;

	if (encoding == TextEncoding::ShiftJIS) // Text is in Shift-JIS
	{
		//log << "In Shift-JIS" << std::endl;
		// Count lengths according to characters encoded
		for (int j = 0; j < text.size(); j++) {
			switch (text[j] >> 5) {
			case 5: // Half-width katakana character
				[[fallthrough]];
			case 6: // Half-width katakana character (1 codepoint, 3 bytes)
				length_codepoints++;
				length_utf8bytes += 3;
				break;
			case 4: // Two-byte character
				[[fallthrough]];
			case 7: // Two-byte character (1 codepoint, 3 bytes)
				length_codepoints++;
				length_utf8bytes += 3;
				j++;
				break;
			default: // ASCII character (1 codepoint, 1 byte)
				length_codepoints++;
				length_utf8bytes++;
			}
		}
	}
	else // Text is in ANSI
	{
		//log << "In ANSI" << std::endl;
		// Length in codepoints is equal to our length in bytes
		length_codepoints = text.size();

		// Count length in UTF-8 bytes
		for (char c : text) {
			if (c > 127) { // 2 byte extended-ASCII character
				length_utf8bytes += 2;
			}
			else { // 1 byte ASCII character
				length_utf8bytes++;
			}
		}
	}

	// Input into histogram
	tables[TXT_UTF8BYTES].columns[eventType].inc(length_utf8bytes);
	tables[TXT_CODEPOINTS].columns[eventType].inc(length_codepoints);
}
