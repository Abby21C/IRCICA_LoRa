#ifndef INC_CONFIG_WUR_H_
#define INC_CONFIG_WUR_H_


#define SYNCWORD_LENGTH 8 //in bytes
#define PREAMBLE_LENGTH_IN_BIT 50000 //preamble bit length
#define PREAMBLE_DETECTION_LENGTH SX126X_GFSK_PREAMBLE_DETECTOR_MIN_16BITS //!< Preamble detection length /!\ 24bits max for 1ms RX time
#define BR_IN_BPS 50000 //bit rate in bit/second
#define FDEV_IN_HZ 23848 //frequency deivation
#define BW_DSB_PARAM SX126X_GFSK_BW_312000 //bandwidth 
#define RAMP_TIME 0x07 //SX126X_RAMP_?_US
#define POWER 14 //14 max

#endif /* INC_CONFIG_WUR_H_ */
