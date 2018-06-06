
#ifdef NS3_TRACI_COMPILATION
# error "Do not include ns3 module aggregator headers from other modules; these are meant only for end user scripts."
#endif

#ifndef NS3_MODULE_MOBILITY

// Module headers:
#include "traci-client.h"
#include "traci-functions.h"
#include "traci-mobility-model.h"
#include "sumo-config.h"
#include "sumo-socket.h"
#include "sumo-storage.h"
#include "sumo-TraCIAPI.h"
#include "sumo-TraCIDefs.h"
#include "sumo-TraCIConstants.h"
#endif
