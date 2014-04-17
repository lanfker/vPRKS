#ifndef SETTINGS_H
#define SETTINGS_H

const uint32_t DEFAULT_PACKET_LENGTH = 300;
const uint32_t DEFAULT_WIFI_DEVICE_INDEX = 0;
const double DEFAULT_EXCLUSION_REGION = -120; //dbm
const uint64_t PAKCET_GENERATION_INTERVAL = 100;// in milliseconds. This is the default interval for safety message.
const double LINK_SELECTION_THRESHOLD = -86; //dBM
const uint16_t DBM_AMPLIFY_TIMES = 300;
const uint8_t DEFAULT_POWER=40; //dBm
const uint32_t MAXIMUM_POWER = 300; //dBm
const int64_t START_PROCESS_TIME = 150; //seconds
const uint32_t DATA_CHANNEL = 1;
const uint32_t CONTROL_CHANNEL = 2;
const double DELIVERY_100_SNR = 14; //dB

const uint32_t MAX_RANDOM_SEED = 100;
const uint32_t MAX_RUN_NUMBER = 200;
const int64_t SLOT_LENGTH = 500; //in terms of micro seconds
const uint32_t FRAME_LENGTH = 100; //slot


const uint16_t _FROM = 2;
const uint16_t _TO = 2;
const uint16_t _ATTENUATION = 2;
const uint16_t _ANGLE = 2;
const uint16_t _BEGIN = 2;
const uint16_t _END = 2;
const uint16_t _ER = 2;

const uint16_t SIGNAL_MAP_ITEM_SIZE = _FROM + _TO + _ATTENUATION + _ANGLE + _BEGIN + _END + _ER;

#endif
