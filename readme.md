# ofxSick is an addon for openFrameworks to interface with SICK laser measurement systems.

ofxSick is written for the SICK [LMS1xx](http://www.sick.com/us/en-us/home/products/product_news/laser_measurement_systems/Pages/lms100.aspx) series, and tested against the LMS111. The LMS1xx datasheet is available [here](https://mysick.com/saqqara/get.aspx?id=im0031331).

## LMS1xx Library

The basic interface protocol is implemented by Konrad Banachowicz' [LMS1xx library](https://github.com/konradb3/libLMS1xx/). Because the library uses sockets, it maybe not be possible to compile on non-POSIX systems like Windows.

Some major modifications have been made to the `LMS1xx::getData()` method in the LMS1xx library in order to support unusual packet fragmentation and unpredictable latency on OSX 10.8.

## Remission and Second Returns

Due to packet fragmentation and unpredictable latency on OSX 10.8, the bandwidth of the data returned by the LMS1xx has been reduced significantly in order to avoid errors. This is done by disabling remission (brightness) and second returns (half-reflected pulses). Right now this can be tweaked by modifying the `targetDataCfg` inside `ofxSickGrabber::connect()`.

## ofxSickGrabber and ofxSickPlayer

The main `ofxSick` class implements the shared features of grabbing and playing LIDAR data. The main `example/` shows a very simple case of using the `ofxSickGrabber`, and the `example-blobs/` shows how to switch between grabbing and playback. `example-blobs/` also works with [ofxCv](https://github.com/kylemcdonald/ofxCv) to do blob detection on objects in the scene.