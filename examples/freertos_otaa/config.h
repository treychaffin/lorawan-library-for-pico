/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef _LORA_CONFIG_H
#define _LORA_CONFIG_H

// LoRaWAN region - Set to US915 for North America
#define LORAWAN_REGION_US915

// Device configuration for OTAA
#define LORAWAN_DEVICE_EUI      "dead123456789def"
#define LORAWAN_APP_EUI         "3031323334353637"  
#define LORAWAN_APP_KEY         "1d7179d61a93745acb6c9eb83d19189f"
// For US915, explicitly use sub-band 0 (channels 0-7) and 500kHz downlink channel 0.
// Format: 6x 16-bit masks concatenated (MSB first per 16-bit word):
//   0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001
// Adjust if your network uses a different sub-band.
#define LORAWAN_CHANNEL_MASK    "00FF00000000000000000001"

#endif