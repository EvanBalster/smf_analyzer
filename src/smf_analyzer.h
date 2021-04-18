#pragma once


#include <cstdint>
#include <vector>


namespace smf_analyzer
{
	using ByteArray = std::vector<uint8_t>;

	/*
		Visitor pattern for MIDI 1.0 and Meta Events.
	*/
	class SMF_Visitor
	{
	public:
		virtual void Meta_Data       (const ByteArray &) = 0;
		virtual void Meta_Text       (const ByteArray &) = 0;
		virtual void Meta_SeqSpecific(const ByteArray &) = 0;
		virtual void Msg_ChannelVoice(const ByteArray &) = 0;
		virtual void Msg_SysEx       (const ByteArray &) = 0;
	};

	

	/*
		
	*/
	class SMF_Analyzer
	{
	public:


	public:
		// Visitor interface
		void Meta_Data       (const ByteArray &) final;
		void Meta_Text       (const ByteArray &) final;
		void Meta_SeqSpecific(const ByteArray &) final;
		void Msg_ChannelVoice(const ByteArray &) final;
		void Msg_SysEx       (const ByteArray &) final;
	};


	class SMF_Parser
	{
	public:
		SMF_Parser
	};
}
