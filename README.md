twsgluum
========

These tools connect [InteractiveBroker's TWSAPI][1] to [unserding][2]
networks using the tick data encoders from [uterus][3].

The tool `quo-tws` simply requests market data (via `reqMktData()`) from
a request file in the same format as [twstools][4].

Example:
--------

  Using this as `test.xml`:
```xml
<?xml version="1.0"?>
<TWSXML xmlns="http://www.ga-group.nl/twsxml-0.1">
<request type="contract_details">
  <query>
    <reqContract symbol="EUR" secType="CASH" exchange="IDEALPRO"/>
  </query>
</request>
</TWSXML>
```
  you can subscribe to live quotes via:

    quo-tws --tws-host localhost --tws-port 7474 --tws-client-id 12247 \
      --beef 12247 test.xml

  and have your `unsermon` listen to beef channel 12247 with the
  `svc-uterus` module to decode the tick data.

  [1]: https://github.com/rudimeier/twsapi
  [2]: https://github.com/hroptatyr/unserding
  [2]: https://github.com/hroptatyr/uterus
  [4]: https://github.com/rudimeier/twstools
