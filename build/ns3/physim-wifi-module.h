
#ifdef NS3_MODULE_COMPILATION
# error "Do not include ns3 module aggregator headers from other modules; these are meant only for end user scripts."
#endif

#ifndef NS3_MODULE_PHYSIM_WIFI
    

// Module headers:
#include "physim-blockinterleaver.h"
#include "physim-channel-estimator.h"
#include "physim-convolutional-encoder.h"
#include "physim-helper.h"
#include "physim-interference-helper.h"
#include "physim-ofdm-symbolcreator.h"
#include "physim-propagation-loss-model.h"
#include "physim-scrambler.h"
#include "physim-signal-detector.h"
#include "physim-vehicular-TDL-channel.h"
#include "physim-vehicular-channel-spec.h"
#include "physim-wifi-channel.h"
#include "physim-wifi-helper.h"
#include "physim-wifi-phy-state-helper.h"
#include "physim-wifi-phy-tag.h"
#include "physim-wifi-phy.h"
#include "physim-wifi-state-checker.h"
#include "physim-wifi-test-suite.h"
#endif
