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

Tested on an ASUS RT-BE92U (5 GHz): 10 s and 30 s intervals stay connected. With 60 s the link dropped about
90-120 s after setup (`cookie response not received` -> `Failed to send SA Query Request` -> `reason=2`),
so keep the interval at 30 s or below on that AP.
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
zperf udp upload <peer-ip> 5001 10 1K 20M
zperf tcp download 5001
zperf tcp upload <peer-ip> 5001 10 1K
```
