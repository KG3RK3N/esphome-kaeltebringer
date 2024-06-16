# esphome-kaeltebringer

ESPHome component for Kältebringer Climates

# Configuration
```yaml
uart:
  id: uart_bus
  tx_pin: 15
  rx_pin: 13
  baud_rate: 9600
  parity: EVEN

status_led:
    pin: GPIO5

climate:
  - platform: kaeltebringer
    uart_id: uart_bus
    name: "A/C"
```

# Tested devices
- Kältebringer KB34-12000BTU (flashed on original wifi module, esp_air_DIM_tcl_8M_QIO_TLS_1.3.3)

# Thanks
Based on the custom component from [lNikazzzl](https://github.com/lNikazzzl/tcl_ac_esphome/tree/master)