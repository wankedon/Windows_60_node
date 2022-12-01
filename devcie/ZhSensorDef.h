#pragma once

enum senErrorType {
	SEN_ERR_NONE = 0,                      
	SEN_ERR_NOTIMPLEMENTED = -1,           
	SEN_ERR_UNKNOWN = -2,                 
	SEN_ERR_BUSY = -3                    
};

//enum senArrayMaximums {
//
//	SEN_MAX_GEOGRAPHIC_DATUM = 64,
//	SEN_MAX_SENSOR_NAME = 64, 
//	SEN_MAX_SENSOR_HOSTNAME = 64, 
//	SEN_MAX_APPLICATION_NAME = 64,
//	SEN_MAX_ERROR_STRING = 64, 
//	SEN_MAX_SENSORS = 512,  
//	SEN_MAX_EVENT_MSG_LEN = 81,  
//	SEN_MAX_UNIT = 32,  
//
//	SEN_MAX_FILENAME = 256, 
//	SEN_MAX_SESSION_ID = 256, 
//	SEN_MAX_COMMENT = 256,
//	SEN_MAX_SENSORS_PER_GROUP = 100, 
//	SEN_MAX_GEOLOCATION_SAMPLES = 32768,
//	SEN_MIN_GEOLOCATION_SAMPLES = 256,
//	SEN_MAX_SAMPLES_PER_TRANSFER = 32768
//};

enum senReceiverType {
	senReceiver_noise = 1, //������
	senReceiver_normal = 2,//����
};

enum senAntennaType {
	senAntenna_TestSignal = -4,
	senAntenna_Auto = -3,
	senAntenna_Unknown = -2,
	senAntenna_Terminated = -1,
	senAntenna_1 = 0,
	senAntenna_2 = 1,
	senAntenna_3 = 2,
	senAntenna_4 = 3,
	senAntenna_TestSignal2 = 4,
	senAntenna_Terminated2 = 5,
	senAntenna_Auto2 = 6
};
/*
enum senAverageType {
	senAverage_off,
	senAverage_rms,
	senAverage_peak,
	senAverage_unknown
};

enum senWindowType {
	senWindow_hann,
	senWindow_gaussTop,
	senWindow_flatTop,
	senWindow_uniform,
	senWindow_blackMan,
	senWindow_custom1,
	senWindow_unknown,
};

enum senDataType {
	senDATA_TYPE_NONE = -1,
	senCOMPLEX_32,
	senCOMPLEX_16,
	senCOMPLEX_FLOAT32,
	senREAL_INT8,
	senREAL_INT8_ALAW,
	senREAL_INT8_ULAW,
	senREAL_INT16,
	senREAL_FLOAT32,
	senREAL_FLOAT32_DBM,
};

enum senMonitorMode {
	salMonitorMode_off,
	salMonitorMode_on
};

enum senOverlapType {
	senOverlap_on,
	senOverlap_off,
};

enum senFftDataType {
	senFftData_db,
	senFftData_mag
};
*/

#pragma pack(1)
struct senSensorInfo {
	//char macAddress[SEN_MAX_SENSOR_HOSTNAME];         
	//char modelNumber[SEN_MAX_SENSOR_HOSTNAME];        
	//char serialNumber[SEN_MAX_SENSOR_HOSTNAME];     
	//char hostName[SEN_MAX_SENSOR_HOSTNAME];           
	char ipAddress[64];          
	//char smsAddress[SEN_MAX_SENSOR_HOSTNAME];          
	//char revision[SEN_MAX_SENSOR_HOSTNAME];            
};

struct senLocation {
	double latitude;        
	double longitude;        
	double elevation;       
/*	char geographicDatum[SEN_MAX_GEOGRAPHIC_DATUM];
	double eastVel;         
	double northVel;         
	double upVel;        
	char velocity_unit[SEN_MAX_UNIT];
	unsigned long latitudeType;       
	unsigned long longitudeType;       
	double rotation;  */     
};

struct senSensorStatus {

	//char name[SEN_MAX_SENSOR_NAME];          
	//char hostName[SEN_MAX_SENSOR_HOSTNAME];   
	char ipAddress[64];   
	//char userHostName[SEN_MAX_SENSOR_HOSTNAME];        
	//char userApplicationName[SEN_MAX_APPLICATION_NAME]; 
	//unsigned long lastHeardFrom;   
	//senLocation location;      
	//short currentMode;   
	//short timeSyncAlarms;
	//short systemAlarms; 
	//short integrityAlarms;
};

struct senCalibParam
{
	int32_t antenna_mode; 				  // ����ģʽ 1-�±�Ƶģʽ��2-ֱͨģʽ��3-����ģʽ
	int32_t	gain;						  // ����ֵ����λ��dB�� -30~30dB
	int32_t	downconverter_mode; 		  //�±�Ƶ������ģʽ ����0 ������1 ��ʧ��2
	int32_t	downconverter_if_attenuation; //�±�Ƶ����Ƶ˥�� 0~30dB
	int32_t	receive_mode; 				  // ���ջ�ģʽ 0-����; 1-��������
	int32_t	receive_rf_attenuation; 	  // ���ջ���Ƶ˥�� 0~30dB
	int32_t	receive_if_attenuation; 	  // ���ջ���Ƶ˥�� 0~20dB
	int32_t	attenuation_mode; 			  // ˥��ģʽ 0���Զ���1���ֶ�
};

struct senPanoParms {
	double start_freq;     //��ʼƵ��
	double stop_freq;      //��ֹƵ��
	double IF_bandwidth;   //��Ƶ������λ��Hz��
	int32_t  antenna_mode; //����ģʽ 1-�Ŵ�ģʽ��2-ֱͨģʽ��3-����ģʽ
	int32_t	receive_mode;  //���ջ�ģʽ 1-��������2-����
	int32_t	gain;		   //����ֵ����λ��dB��
	int32_t  interval;     //���ݻش��������λ��ms��
	senCalibParam* calib_param = nullptr; //У׼����
	uint32_t  average_count; //ƽ������[0,128]
};

struct senSweepParms {
	unsigned long    numSweeps;       
	unsigned long    numSegments;      
/*	senWindowType    window;           
	unsigned long    userWorkspace;    
	senDataType      dataType;      
	long             externalCal;       
	long             syncSweepEnable;  
	double           sweepInterval;  
	unsigned long    syncSweepSec;     
	unsigned long    syncSweepNSec;    
	senMonitorMode   monitorMode;     
	double           monitorInterval;  
	unsigned long    reserved; */ 
};

struct senSegmentData {
	//unsigned long      userWorkspace;      
	unsigned long      segmentIndex;    
	unsigned long      sequenceNumber;  
	unsigned long      sweepIndex;     
	unsigned long      timestampSec;  
	unsigned long      timestampNSec; 
	//unsigned long      timeQuality;    
	senLocation    location;     
	//double     startFrequency;
	//double     frequencyStep; 
	unsigned long      numPoints;     
	/*unsigned long      overload;       
	senDataType    dataType;     
	unsigned long      lastSegment;  
	senWindowType  window;             
	senAverageType  averageType;   
	unsigned long      numAverages;       
	double     fftDuration;       
	double     averageDuration;    
	unsigned long      isMonitor;         
	unsigned long sweepFlags;     
	unsigned long      timeAlarms;   
	double         sampleRate;   */   
};

struct senFrequencySegment {
	
	double  startFrequency;
	double  stopFrequency;
	/*senAntennaType  antenna;
	long        preamp;          
	unsigned long       numFftPoints;     
	senAverageType  averageType;     
	unsigned long       numAverages;     
	unsigned long       firstPoint;*/     
	unsigned long       numPoints;        
	/*unsigned long       repeatAverage;
	double      attenuation;   
	double      centerFrequency;
	double      sampleRate;       
	double      duration;        
	senOverlapType  overlapMode;      
	senFftDataType  fftDataType;    
	long        noTunerChange;   
	unsigned long long      levelTriggerControl;*/
} ;

struct senDFParms {
	double start_freq;     //��ʼƵ��
	double stop_freq;      //��ֹƵ��
	double center_freq;    //����Ƶ��
	double IF_bandwidth;   //��Ƶ������λ��Hz��
	double DF_bandwidth;   //���������λ��Hz��
	int32_t  antenna_mode; //����ģʽ 1-�Ŵ�ģʽ��2-ֱͨģʽ��3-����ģʽ
	int32_t	receive_mode;  //���ջ�ģʽ 1-��������2-����
	int32_t	gain;		   //����ֵ����λ��dB��
	int32_t  interval;     //���ݻش��������λ��ms��
	int32_t  mode; 	       //����ģʽ 0-�ֶ���1���Զ�
	int32_t  value; 	   //����ֵ
	int32_t  target_count; //��Դ���� 0~8
};

struct senDFTarget
{
	double	center_frequency;	// Ƶ��
	double	direction;	// Ŀ�귽�򣨵�λ���㣩
	int32_t	confidence;	// ���Ŷ�
	float	amplitude;	// ���ȣ���λ��dBm��
};

struct senIQSegmentData
{
	unsigned long long		sequenceNumber;  /**< ˳��� */
	//unsigned long		segmentIndex;        /**< Ƶ������  */
	//unsigned long		sweepIndex;          /**< ɨ��������� */
	//senDataType		    dataType;		     /**< �������� */
	unsigned long		numSamples;			 /**< �������� */
	//unsigned long		shlBits;			 /**< λƫ�ƣ�16λIQ����Ч�� */
	unsigned long		timestampSeconds;    /**< ʱ���. */
	unsigned long		timestampNSeconds;
	//unsigned long       timeAlarms;			 /**<GPS״̬��Ϣ*/
	//double      scaleToVolts;        /**< ��ѹ������.   */
	double		centerFrequency;     /**< ����Ƶ��. */
	double		sampleRate;          /**< ������.  */
	double		latitude;            /**< λ����Ϣ */
	double		longitude;
	double		elevation;
};

struct senIQParms {
	double start_freq;     //��ʼƵ��
	double stop_freq;      //��ֹƵ��
	double IF_bandwidth;   //��Ƶ������λ��Hz��
	int32_t  antenna_mode; //����ģʽ 1-�Ŵ�ģʽ��2-ֱͨģʽ��3-����ģʽ
	int32_t	receive_mode;  //���ջ�ģʽ 1-��������2-����
	int32_t	gain;		   //����ֵ����λ��dB��
	int32_t  interval;     //���ݻش��������λ��ms��
	int32_t	 points;	   //�������� 0-һֱ������>0�ɼ���Ӧ������ֹͣ����
};

struct IQDataDescribe
{
	double centerFreq;
	double sampleRate;
};

struct senIFParms {
	double start_freq;     //��ʼƵ��
	double stop_freq;      //��ֹƵ��
	double IF_bandwidth;   //��Ƶ������λ��Hz��
	int32_t  antenna_mode; //����ģʽ 1-�Ŵ�ģʽ��2-ֱͨģʽ��3-����ģʽ
	int32_t	receive_mode;  //���ջ�ģʽ 1-��������2-����
	int32_t	gain;		   //����ֵ����λ��dB��
	int32_t  interval;     //���ݻش��������λ��ms��
	int32_t  mode; 	       //����ģʽ 0-�ֶ���1���Զ�
	int32_t  value; 	   //����ֵ
	uint32_t  average_count; //ƽ������[0,128]
};

struct senSignal
{
	double center_frequency;
	double band_width;
	uint32_t modulation_type; 	// �źŵ�����ʽ
	float	amplitude ;			// ���ȣ���λ��dBm��
};
#pragma pack()

using senVecFloat = std::vector<float>;