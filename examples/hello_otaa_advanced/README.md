# LoRaWAN Advanced Confirmed OTAA Example

This example demonstrates advanced LoRaWAN confirmed uplink features including:

## Features Demonstrated

### Core Messaging
- **Confirmed uplinks** with guaranteed delivery
- **Configurable retry count** (1-15 attempts)
- **Acknowledgment status checking** for delivery confirmation

### Network Optimization
- **Adaptive Data Rate (ADR)** for automatic optimization
- **Manual data rate control** (DR0-DR5 for US915)
- **Transmission power management** for battery optimization

### Network Monitoring
- **Link quality checks** with signal margin and gateway count
- **Device time synchronization** requests
- **Real-time transmission parameter reporting**

## Hardware Requirements

- Raspberry Pi Pico or compatible RP2040 board
- Semtech SX1276 LoRa radio module
- Active LoRaWAN network server (TTN, ChirpStack, etc.)

## Configuration

Edit `config.h` with your LoRaWAN network credentials:
- `LORAWAN_DEVICE_EUI`
- `LORAWAN_APP_EUI` 
- `LORAWAN_APP_KEY`
- `LORAWAN_REGION` (set to your region)

## Usage

1. **Build**: `ninja pico_lorawan_hello_otaa_advanced`
2. **Upload**: Copy `pico_lorawan_hello_otaa_advanced.uf2` to your Pico in bootloader mode
3. **Monitor**: Connect to the serial console to see detailed output

## Example Output

```
Pico LoRaWAN - Advanced Confirmed OTAA Example

Initializing LoRaWAN ... success!
Configuring advanced settings:
  ✓ Retry count set to 3
  ✓ ADR enabled
  ✓ Data rate set to DR0
  ✓ TX power set to maximum
Joining LoRaWAN network ... joined successfully!

Sending confirmed message 'Hello #1 DR:0 PWR:0' ... success!
  ✓ Message was acknowledged by server

Requesting link check ... sent!
  Link quality: 15 dB margin, 2 gateways
```

## Advanced Configuration

The example shows how to:

```c
// Configure retry behavior
lorawan_set_confirmed_retry_count(3);

// Enable automatic optimization
lorawan_set_adr_enabled(true);

// Manual parameter control
lorawan_set_datarate(2);        // Set specific data rate
lorawan_set_tx_power(5);        // Reduce power for battery saving

// Check message delivery
if (lorawan_get_last_confirmed_status()) {
    printf("Message delivered successfully\n");
}

// Monitor network quality
lorawan_request_link_check();
uint8_t margin, gateways;
if (lorawan_get_link_check_result(&margin, &gateways) == 0) {
    printf("Signal: %d dB, Gateways: %d\n", margin, gateways);
}
```

## Comparison with Basic Examples

| Feature | hello_otaa | hello_otaa_confirmed | hello_otaa_advanced |
|---------|------------|---------------------|-------------------|
| Message Type | Unconfirmed | Confirmed | Confirmed |
| Retry Control | ❌ | ✅ | ✅ |
| ACK Status | ❌ | ❌ | ✅ |
| ADR Control | ❌ | ❌ | ✅ |
| Power Control | ❌ | ❌ | ✅ |
| Link Quality | ❌ | ❌ | ✅ |
| Real-time Stats | ❌ | ❌ | ✅ |

This example is ideal for production applications requiring reliable messaging with comprehensive monitoring and control capabilities.