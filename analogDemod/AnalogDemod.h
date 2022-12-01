// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef ANALOGDEMOD_EXPORTS
#define ANALOGDEMOD_API __declspec(dllexport)
#else
#define ANALOGDEMOD_API __declspec(dllimport)
#endif

enum DemodType
{
	Demodulation_none = 0,
	Demodulation_AM = 1,
	Demodulation_FM = 2,
	Demodulation_CW = 3,
	Demodulation_LSB = 4,
	Demodulation_USB = 5,
	Demodulation_DSB = 6,
	Demodulation_ISB = 7
};

class ANALOGDEMOD_API AnalogDemod
{
public:
	AnalogDemod(float sampleRate, int16_t numTransferSamples, DemodType type);
	~AnalogDemod();
public:
	void demod(const std::vector<int32_t>& rawData, std::vector<int16_t>& demodData);
private:
	class Impl;
	std::unique_ptr<Impl> impl;
};