#pragma once
#ifdef MODULATIONRECOGNITION_EXPORTS
#define MODULATIONRECOGNITION_API __declspec(dllexport)
#else
#define MODULATIONRECOGNITION_API __declspec(dllimport)
#endif

namespace ModulationRecognition
{
	//UNKNOWM 0
	//AM 1 - 10
	//FM 11 - 20
	//ASK 21 - 30
	//FSK 31 - 40
	//PSK 41 - 50
	//QAM 51 - 60
	//CW 70
	//Noise 80 
	enum ModFormat
	{
		UNKNOWM = 0,
		AM = 1,
		AMSC = 2,
		AMTC = 3,
		SSB = 4,
		DSB = 5,
		VSB = 6,
		LSB = 7,
		USB = 8,
		FM = 11,
		ASK = 21,
		ASK2 = 22,
		ASK4 = 23,
		ASK8 = 24,
		ASK16 = 25,
		FSK = 31,
		FSK2 = 32,
		FSK4 = 33,
		FSK8 = 34,
		FSK16 = 35,
		MSK = 36,
		PSK = 41,
		BPSK = 42,
		OQPSK = 43,
		QPSK = 44,
		Pi4QPSK = 45,
		PSK8 = 46,
		PSK16 = 47,
		D8PSK = 48,
		QAM = 51,
		QAM16 = 52,
		QAM32 = 53,
		QAM64 = 54,
		QAM128 = 55,
		QAM256 = 56,
		QAM512 = 57,
		QAM1024 = 58,
		CW = 70,
		BOC = 71,
		Noise = 80
	};

	enum SigFormat
	{
		UN_DETECT = 100,
		NARROW_BAND = 101,
		WIDE_BAND = 102,
		PULSE = 103
	};

	struct SigDescriptor
	{
		SigFormat sigFormat;
		ModFormat modFormat;
		double centFreq;
		double band;
	};

	MODULATIONRECOGNITION_API void create(uint32_t* handle, double centFreq, double sampleRate);

	MODULATIONRECOGNITION_API SigDescriptor recognize(uint32_t handle, const float* samples, size_t size);

	MODULATIONRECOGNITION_API void destroy(uint32_t handle);
}