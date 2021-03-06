# TODO

## Example

Plotting of CSV can be done for example:

- online [https://plot.ly/create/](https://plot.ly/create/)
- terminal [https://github.com/sgreben/jp](https://github.com/sgreben/jp)
- GUI [https://kst-plot.kde.org/](https://kst-plot.kde.org/)

```bash
$ hobd-log-entry-dump hobd.log

$ tree
├── table_10.csv
└── table_D1.csv
```

Plot CSV file with KST:

```bash
$ kst2 --asciiDelim , --asciiDataStart 2 --asciiFieldNames 1 table_10.csv
```

Plot CSV data in real-time:

```bash
$ cat /dev/ttyUSB0 > /tmp/data.csv

# plot the last 1000 frames as new data comes in
$ kst2 --asciiDelim , --asciiDataStart 2 --asciiFieldNames 1 data.csv -n 1000
```

```bash
[0000]
  type: 0x0020 | HEARTBEAT
  size: 0 bytes
  timestamp: 10330568000 ns

[0001]
  type: 0x0020 | HEARTBEAT
  size: 0 bytes
  timestamp: 14587354000 ns

[0002]
  type: 0x0020 | HEARTBEAT
  size: 0 bytes
  timestamp: 18822744000 ns

[0003]
  type: 0x0020 | HEARTBEAT
  size: 0 bytes
  timestamp: 23059566000 ns

[0004]
  type: 0x0020 | HEARTBEAT
  size: 0 bytes
  timestamp: 27294510000 ns

[0005]
  type: 0x0100 | HOBD-MSG
  size: 4 bytes
  timestamp: 27443230000 ns
  hobd-msg
    type: 0xFE
    size: 4 bytes
    subtype: 0xFF
    checksum: 0xFF
      -> wake-up

[0006]
  type: 0x0100 | HOBD-MSG
  size: 5 bytes
  timestamp: 27493904000 ns
  hobd-msg
    type: 0x72
    size: 5 bytes
    subtype: 0x00
    checksum: 0x99
      -> query

[0007]
  type: 0x0100 | HOBD-MSG
  size: 4 bytes
  timestamp: 27498084000 ns
  hobd-msg
    type: 0x02
    size: 4 bytes
    subtype: 0x00
    checksum: 0xFA
      <- response
        init

[0008]
  type: 0x0100 | HOBD-MSG
  size: 7 bytes
  timestamp: 27581818000 ns
  hobd-msg
    type: 0x72
    size: 7 bytes
    subtype: 0x72
    checksum: 0xF4
      -> query
        table-subgroup
        table: 0x10
        offset: 0x00
        count: 17

[0009]
  type: 0x0100 | HOBD-MSG
  size: 23 bytes
  timestamp: 27604334000 ns
  hobd-msg
    type: 0x02
    size: 23 bytes
    subtype: 0x72
    checksum: 0xD0
      <- response
        table-subgroup
        table: 0x10
        offset: 0x00
        count: 17
```
