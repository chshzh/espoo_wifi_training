# Wi-Fi shell command cheat sheet

Replace `<SSID>` and `<PASSWORD>` with your own network. Do not commit real credentials.

```
wifi scan
wifi cred add -s <SSID> -k 1 -p <PASSWORD>
wifi cred auto_connect
wifi connect -s <SSID> -k 1 -p <PASSWORD>
wifi status
```

## Target Wake Time (TWT): 60 s wake-up, 65 ms awake

```
wifi twt quick_setup 65000 60000000
wifi twt teardown_all
```

The AP may round the values (for example it granted 61440 us / 60.03 s in testing).
`wifi twt setup` needs named options and at least 25 argument tokens, so add `-D 0 -d 0`:

```
wifi twt setup -n 0 -c 0 -t 1 -f 0 -r 0 -T 1 -I 1 -a 1 -w 65000 -p 60000000 -D 0 -d 0
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
