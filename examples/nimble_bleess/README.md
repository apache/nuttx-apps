# NimBLE Environmental Sensing Peripheral

This example implements a Bluetooth LE Environmental Sensing peripheral with
Apache NimBLE. It advertises the Environmental Sensing service and exposes
readable and notifiable temperature and humidity characteristics.

The standard Bluetooth UUIDs and value formats are:

| Attribute | UUID | Format |
| --- | --- | --- |
| Environmental Sensing service | `0x181a` | Service |
| Temperature characteristic | `0x2a6e` | Signed 16-bit, 0.01 degrees Celsius |
| Humidity characteristic | `0x2a6f` | Unsigned 16-bit, 0.01 percent |

The application generates slowly changing sample values every two seconds.
It can be adapted to a real sensor by passing measured values to
`bleess_gatt_update()`.

Run `bleess` from NSH. An optional numeric argument selects the HCI device:

```text
nsh> bleess 0
```
