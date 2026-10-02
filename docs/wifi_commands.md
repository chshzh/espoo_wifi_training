# Wi-Fi shell command cheat sheet

Replace `<SSID>` and `<PASSWORD>` with your own network. Do not commit real credentials.

```
wifi scan
wifi connect -s <SSID> -k 1 -p <PASSWORD>
wifi status
```

Optional: store the Wi-Fi credentials once, then connect with `wifi cred auto_connect` after a restart.

```
wifi cred add -s <SSID> -k 1 -p <PASSWORD>
wifi cred auto_connect
```

## Target Wake Time (TWT): 10 s wake-up, 65 ms awake

```
wifi twt quick_setup 65000 10000000
wifi twt setup -n 0 -c 0 -t 1 -f 0 -r 0 -T 1 -I 1 -a 1 -w 65000 -p 10000000 -D 0 -d 0
wifi twt teardown_all
```

## zperf

The PC side uses iperf2 (`iperf`, not `iperf3`). `<pc-ip>` is the PC address, `<dk-ip>` is the DK address.

Syntax:

```
zperf udp upload [-S tos -a] <dest ip> [<dest port> <duration> <packet size>[K] <baud rate>[K|M]]
zperf udp download [<port>] [<host>]
zperf tcp upload [-S tos -a -i sec -n] <dest ip> <dest port> <duration> <packet size>[K]
zperf tcp download [<port>] [<host>]
```

### UDP upload (DK to PC)

```
# PC
iperf -s -u -i 1
# DK
zperf udp upload <pc-ip> 5001 10 1K 100M
```

### UDP download (PC to DK)

```
# DK
zperf udp download 5001
# PC
iperf -c <dk-ip> -u -b 100M -l 1K -t 10 -i 1
```

### TCP upload (DK to PC)

```
# PC
iperf -s -i 1
# DK
zperf tcp upload <pc-ip> 5001 10 1K
zperf tcp upload -n <pc-ip> 5001 10 1K
zperf tcp upload -a -i 1 <pc-ip> 5001 10 1K
```

### TCP download (PC to DK)

```
# DK
zperf tcp download 5001
# PC
iperf -c <dk-ip> -t 10 -i 1
```

### Stop the DK servers

```
zperf udp download stop
zperf tcp download stop
```
