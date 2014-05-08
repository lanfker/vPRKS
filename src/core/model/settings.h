#ifndef SETTINGS_H
#define SETTINGS_H

const uint32_t DEFAULT_PACKET_LENGTH = 500;
const uint32_t DEFAULT_WIFI_DEVICE_INDEX = 0;
const double DEFAULT_EXCLUSION_REGION = -90.4397; //dbm
const double DEFAULT_EXCLUSION_REGION_WATT = 9.0371e-13;//Watt
const uint64_t PAKCET_GENERATION_INTERVAL = 400;// in milliseconds. This is the default interval for safety message.
const double NOISE = 2.00619e-13; //Watt   -96.976279387 dBm
const double DESIRED_PDR = 0.9;
const double MAX_LINK_DISTANCE = 300.0; // meters
const double EWMA_COEFFICIENT = 0.8;
const uint32_t LINKE_ESTIMATOR_WINDOW_SIZE = 20;
const double LINK_DISTANCE_THRESHOLD = 500;
const uint64_t OBSERVATION_EXPIRATION_TIME = 240; //seconds
const uint32_t MAX_OBSERVATION_ITEMS_PER_LINK = 10;
const uint32_t TX_GAIN = 1;//dB
const uint32_t RX_GAIN = 1;//dB
const double LINK_SELECTION_THRESHOLD = -87; //dBM
const uint16_t DBM_AMPLIFY_TIMES = 300;
const double DEFAULT_POWER=40; //dBm
const uint32_t MAXIMUM_POWER = 300; //dBm
const int64_t START_PROCESS_TIME = 20; //seconds
const uint32_t DATA_CHANNEL = 1;
const uint32_t CONTROL_CHANNEL = 2;
const double DELIVERY_100_SNR = 14; //dB
const double PATH_LOSS_EXPONENT = 3.28;

const uint32_t MAX_RANDOM_SEED = 100;
const uint32_t MAX_RUN_NUMBER = 200;
const int64_t SLOT_LENGTH = 1500; //in terms of micro seconds
const uint32_t FRAME_LENGTH = 100; //slot

const uint16_t _FROM = 2;
const uint16_t _TO = 2;
const uint16_t _ATTENUATION = 2;
const uint16_t _ANGLE = 2;
const uint16_t _BEGIN = 2;
const uint16_t _END = 2;
const uint16_t _ER = 2;
const int64_t SIMULATION_END_TIME = 3000;//seconds

const uint16_t SIGNAL_MAP_ITEM_SIZE = _FROM + _TO + _ATTENUATION + _ANGLE + _BEGIN + _END + _ER;
const double NI_SAMPLE_FILTER = 4;
const double ABSOLUTE_NI_THRESHOLD = 1.0e-09;
const int64_t DATA_INTERFERENCE_SAMPLE_INTERVAL=20; //microseconds

#endif
