#pragma once
#ifdef TRANSMISSIONSYSTEMRECOGNITION_EXPORTS
#define TRANSMISSIONSYSTEMRECOGNITION_API __declspec(dllexport)
#else
#define TRANSMISSIONSYSTEMRECOGNITION_API __declspec(dllimport)
#endif

namespace TransmissionSystemRecognition
{

	enum TransmissionSystem
	{
		UNKNOWN_TSR,
		GSM,
		CDMA2000,
		WCDMA,
		TD_SCDMA,
		FourG_TDD,
		FourG_FDD,
		FiveG
	};

	TRANSMISSIONSYSTEMRECOGNITION_API void create(uint32_t* handle, double centFreq, double sampleRate);

	TRANSMISSIONSYSTEMRECOGNITION_API TransmissionSystem recognize(uint32_t handle, const float* samples, size_t size);  //size = 4096*100*2

	TRANSMISSIONSYSTEMRECOGNITION_API void destroy(uint32_t handle);
}


