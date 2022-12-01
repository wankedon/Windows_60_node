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
	senReceiver_noise = 1, //低噪声
	senReceiver_normal = 2,//常规
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
	int32_t antenna_mode; 				  // 天线模式 1-下变频模式；2-直通模式、3-测试模式
	int32_t	gain;						  // 增益值（单位：dB） -30~30dB
	int32_t	downconverter_mode; 		  //下变频器工作模式 常规0 低噪声1 低失真2
	int32_t	downconverter_if_attenuation; //下变频器中频衰减 0~30dB
	int32_t	receive_mode; 				  // 接收机模式 0-常规; 1-低噪声；
	int32_t	receive_rf_attenuation; 	  // 接收机射频衰减 0~30dB
	int32_t	receive_if_attenuation; 	  // 接收机中频衰减 0~20dB
	int32_t	attenuation_mode; 			  // 衰减模式 0：自动，1：手动
};

struct senPanoParms {
	double start_freq;     //起始频率
	double stop_freq;      //终止频率
	double IF_bandwidth;   //中频带宽（单位：Hz）
	int32_t  antenna_mode; //天线模式 1-放大模式；2-直通模式、3-测试模式
	int32_t	receive_mode;  //接收机模式 1-低噪声；2-常规
	int32_t	gain;		   //增益值（单位：dB）
	int32_t  interval;     //数据回传间隔（单位：ms）
	senCalibParam* calib_param = nullptr; //校准参数
	uint32_t  average_count; //平均次数[0,128]
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
	double start_freq;     //起始频率
	double stop_freq;      //终止频率
	double center_freq;    //中心频率
	double IF_bandwidth;   //中频带宽（单位：Hz）
	double DF_bandwidth;   //测向带宽（单位：Hz）
	int32_t  antenna_mode; //天线模式 1-放大模式；2-直通模式、3-测试模式
	int32_t	receive_mode;  //接收机模式 1-低噪声；2-常规
	int32_t	gain;		   //增益值（单位：dB）
	int32_t  interval;     //数据回传间隔（单位：ms）
	int32_t  mode; 	       //门限模式 0-手动，1：自动
	int32_t  value; 	   //门限值
	int32_t  target_count; //信源个数 0~8
};

struct senDFTarget
{
	double	center_frequency;	// 频率
	double	direction;	// 目标方向（单位：°）
	int32_t	confidence;	// 置信度
	float	amplitude;	// 幅度（单位：dBm）
};

struct senIQSegmentData
{
	unsigned long long		sequenceNumber;  /**< 顺序号 */
	//unsigned long		segmentIndex;        /**< 频段索引  */
	//unsigned long		sweepIndex;          /**< 扫描次数索引 */
	//senDataType		    dataType;		     /**< 数据类型 */
	unsigned long		numSamples;			 /**< 采样点数 */
	//unsigned long		shlBits;			 /**< 位偏移（16位IQ下有效） */
	unsigned long		timestampSeconds;    /**< 时间戳. */
	unsigned long		timestampNSeconds;
	//unsigned long       timeAlarms;			 /**<GPS状态信息*/
	//double      scaleToVolts;        /**< 电压比例尺.   */
	double		centerFrequency;     /**< 中心频率. */
	double		sampleRate;          /**< 采样率.  */
	double		latitude;            /**< 位置信息 */
	double		longitude;
	double		elevation;
};

struct senIQParms {
	double start_freq;     //起始频率
	double stop_freq;      //终止频率
	double IF_bandwidth;   //中频带宽（单位：Hz）
	int32_t  antenna_mode; //天线模式 1-放大模式；2-直通模式、3-测试模式
	int32_t	receive_mode;  //接收机模式 1-低噪声；2-常规
	int32_t	gain;		   //增益值（单位：dB）
	int32_t  interval;     //数据回传间隔（单位：ms）
	int32_t	 points;	   //采样点数 0-一直工作，>0采集相应点数后停止工作
};

struct IQDataDescribe
{
	double centerFreq;
	double sampleRate;
};

struct senIFParms {
	double start_freq;     //起始频率
	double stop_freq;      //终止频率
	double IF_bandwidth;   //中频带宽（单位：Hz）
	int32_t  antenna_mode; //天线模式 1-放大模式；2-直通模式、3-测试模式
	int32_t	receive_mode;  //接收机模式 1-低噪声；2-常规
	int32_t	gain;		   //增益值（单位：dB）
	int32_t  interval;     //数据回传间隔（单位：ms）
	int32_t  mode; 	       //门限模式 0-手动，1：自动
	int32_t  value; 	   //门限值
	uint32_t  average_count; //平均次数[0,128]
};

struct senSignal
{
	double center_frequency;
	double band_width;
	uint32_t modulation_type; 	// 信号调制样式
	float	amplitude ;			// 幅度（单位：dBm）
};
#pragma pack()

using senVecFloat = std::vector<float>;