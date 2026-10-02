# Wi-Fi shell command cheat sheet

Replace `<SSID>` and `<PASSWORD>` with your own network. Do not commit real credentials.

```
wifi scan
wifi cred add -s <SSID> -k 1 -p <PASSWORD>
wifi cred auto_connect
wifi connect -s <SSID> -k 1 -p <PASSWORD>
wifi status
```

## Target Wake Time (TWT): 10 s wake-up, 65 ms awake

```
wifi twt quick_setup 65000 10000000
wifi twt teardown_all
```

The AP may round the values (for example it granted 61440 us / 10.01 s in testing).

Tested on an ASUS RT-BE92U (5 GHz): with 30 s and 60 s intervals the link dropped after about 1.5-3.5 minutes
(`cookie response not received` -> `Failed to send SA Query Request` -> `reason=2`). 10 s stayed connected in a
short test (about 3.5 minutes), so use 10 s on that AP and check that the link stays up for longer runs.

`wifi twt setup` needs named options and at least 25 argument tokens, so add `-D 0 -d 0`:

```
wifi twt setup -n 0 -c 0 -t 1 -f 0 -r 0 -T 1 -I 1 -a 1 -w 65000 -p 10000000 -D 0 -d 0
```

## Soft AP (5 GHz)

```
wifi reg_domain NO
wifi ap enable -s nRF7120DK_SAP -c 165 -p mypassword -k 1
net dhcpv4 server start 1 192.168.7.2
```

## zperf

```
zperf udp download 5001
zperf udp upload <peer-ip> 5001 10 1400 80M
zperf tcp download 5001
zperf tcp upload <peer-ip> 5001 10 1K
```

The last argument of `zperf udp upload` is the requested rate, so it caps the result. Set it above what the link can
do. On the RT-BE92U (5 GHz, channel 165, RSSI -35 dBm):

| Packet size | Requested rate | Result | Loss |
|---|---|---|---|
| 1K | 20M | 19.2 Mbps | 0 % |
| 1K | 80M | about 49 Mbps | 0.1-0.4 % |
| 1400 | 80M | 53.9 Mbps | 0.2 % |

Larger packets give higher throughput because there is less per-packet overhead.
