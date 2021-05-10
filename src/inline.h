#pragma once

#include <cstdint>

class Endian
{
private:
	static constexpr uint32_t uint32_ = 0x01020304;
	static constexpr uint8_t magic_ = (const uint8_t&)uint32_;
public:
	static constexpr bool little = magic_ == 0x04;
	static constexpr bool middle = magic_ == 0x02;
	static constexpr bool big = magic_ == 0x01;
	static_assert(little || middle || big, "Cannot determine endianness!");
private:
	Endian() = delete;
};

inline uint32_t read4BE(const uint8_t *bytes)
{
	if (Endian::big) return *(uint32_t*)bytes;
	else
	{
		uint32_t n = bytes[0];
		n = (n << 8) | bytes[1];
		n = (n << 8) | bytes[2];
		n = (n << 8) | bytes[3];
		return n;
	}
}
inline uint32_t read4LE(const uint8_t *bytes)
{
	if (Endian::little) return *(uint32_t*)bytes;
	else
	{
		uint32_t n = bytes[3];
		n = (n << 8) | bytes[2];
		n = (n << 8) | bytes[1];
		n = (n << 8) | bytes[0];
		return n;
	}
}
inline uint16_t read2BE(const uint8_t *bytes)
{
	if (Endian::big) return *(uint16_t*)bytes;
	else
	{
		uint16_t n = bytes[0];
		n = (n << 8) | bytes[1];
		return n;
	}
}
inline uint16_t read2LE(const uint8_t *bytes)
{
	if (Endian::little) return *(uint16_t*)bytes;
	else
	{
		uint16_t n = bytes[1];
		n = (n << 8) | bytes[0];
		return n;
	}
}
inline void write4LE(uint8_t *bytes, uint32_t n)
{
	if (Endian::little) (*(uint32_t*)bytes) = n;
	else
	{
		uint8_t *src = (uint8_t*)&n;
		bytes[0] = src[3];
		bytes[1] = src[2];
		bytes[2] = src[1];
		bytes[3] = src[0];
	}
}
inline void write2LE(uint8_t *bytes, uint16_t n)
{
	if (Endian::little) (*(uint16_t*)bytes) = n;
	else
	{
		uint8_t *src = (uint8_t*)&n;
		bytes[0] = src[1];
		bytes[1] = src[0];
	}
}