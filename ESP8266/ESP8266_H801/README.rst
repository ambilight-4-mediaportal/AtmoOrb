H801 AtmoORB code with status indication
------------------------------------------------------------------------------

The H801 is a very affordable (roughly $10 on Aliexpress) ESP8266 based Wifi
RGB Led controller. It has 5 mosfets accepting input from `5v` to `24v` up to
`4A` with the default configuration. Note that this controller is meant for
dumb RGB(WW) led strips which are _not_ addressable.

The H801 uses `20N06L` mosfets which actually support up to `20A` but I suspect
the rest of the board would burn up if you actually tried it. If you plan to go
beyond the recommend `4A` you should at the very least add some heatsinks to
the mosfets.

The code here makes your H801 behave as a regular AtmoORB but adds a few small
features over the regular ESP8266 code.

1. Since this controller doesn't support addressable leds it drivers the
   mosfets straight away.
2. The controller shows it's status with the 2 status lights on-board.

   1. Whenever the H801 is connected to the wifi network the green led should
      be on.
   2. When the H801 is connecting to the wifi the red led will flash
      periodically (twice per second).
   3. Whenever data is received the red led will flash.

To be able to flash the H801 you will need to solder the headers on the print
yourself. Programming can be done with a regular FTDI programmer but note that
the module _only_ accepts `3.3v` and _will_ burn out if you feed it `5v`.

Some documentation can be found here:
https://eryk.io/2015/10/esp8266-based-wifi-rgb-controller-h801/
