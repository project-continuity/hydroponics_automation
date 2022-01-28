# hydroponics_automation

Having someone walk through installing this from scratch in a couple of days, will update this readme depending on the feedback I get from that.

Otherwise, please not the libraries that need to be installed.  I think Git automatically includes them, if not I've included links in the .ino file.

Currently this is just a scheduler.  Once flashed, the IP should appear in the console viewer or the attached display.  Navigate to the page to access.

This is running on an ESP32 and SSD1306 oled display.  I currently have it turnning on a 16 channel relay board and an 8 channel relay board, but it should work with any size relay board as long as you adjust NUM_RELAYS higher if you need more than the five it's currently setup for.

Will be iterating new features into this as I get it tested by other poeple.
