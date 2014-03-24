# Acktie Mobile Passbook Scanner (iOS)

## Example

A working example of how to use this module can be found on Github at
[https://github.com/acktie/Acktie-Mobile-Passbook-
Scanner](https://github.com/acktie/Acktie-Mobile-Passbook-Scanner).

## Description

This module allows for a quick integration of a scanner that supports Apple
Passbook's barcode format (QR/Aztec/PDF417). The scanner supports the
detection of all three types in a single scanning window. Additionally, the
scanning window is exposed as a view which can be embedded in various ways in
your iPhone or iPad application. With the scanner as a view, it allow for the
ability of customizable overlays and display information within the scanner
view. **NOTE**: iPhone 4 or higher is supported. Tested and developed with an
iPhone 4/5 and iPad 3

## Accessing the Acktie Mobile Passbook Scanner

To get started, review the [module install instructions](http://docs.appcelera
tor.com/titanium/2.0/#!/guide/Using_Titanium_Modules) on the Appcelerator
website. To access this module from JavaScript, you would do the following:

    
    var passbookScanner = require("com.acktie.mobile.ios.passbook.scanner");

The passbookScanner variable is a reference to the Module object.

## Reference

The following are the Javascript functions you can call with the module. The
module provide callbacks for:

  * success - Called in the event of a successful scan. Callback data - This callback returns the data of the scanned passbook barcode/QR code.
Example:

    
    function success(result){ var scanResult = result.data; }; 

  * cancel - Called if view is closed or scanner is stopped by user.
NOTE: Both cancel does not return data.

# Creating the scanner view

The following syntax is used to create a new scanner view. The scanner module
will create a Titanium compatible view that allow you to attach it where any
view is allowed.

    
    var view = passbookScanner.createScannerView({...});

## createScannerView Arguments

#### enableQR/enableAztec/enablePDF417

These arguments are used to control the scanning ability. In that, this allow
for the app developer to turn off support for a format that will not be used.
By default all supported formats are enabled Example: To only allow for PDF417
you can turn off QR and Aztec. e.g. enabledQR: false, enabledAztec: false,

#### scanInterval

This argument sets the scanning interval for continuous scanning. The scanning
interval is the time between each scan. If an application allows for multiple
scans in a single scanning window set the scanning interval to an appropriate
time where the same barcode is not accidentally scanned twice. In testing, 2-3
seconds seemed to work best. This argument take a float value to allow for
precise values. Any valid float value will be accepted. By default, this
argument is set to 3.0 seconds. Example: Set the scanning interval to 1 and
1/2 seconds. scanInterval: 1.5

#### useFrontCamera

This argument control the camera to use for the view. If true, scanner will
attempt to create a scanner to use the front facing camera. If a front facing
camera is not present than the rear camera is used. By default, this argument
is false. Example: Use the front facing camera useFrontCamera: true **Note:
**At this time all front facing and iPad 2/iPod camera are fixed focus. This
means they lack the ability to auto-focus on an object. As a result, a good
amount of light is needed with a good quality print of the barcode. In our
testing, the barcode/QR code were detected quickly in this environment.
However, it is not recommend using the front camera to scan passbook
barcodes/QR codes from the iPhone. The camera limitation would not detect the
barcode/QR codes.

## createScannerView Functions

#### stop()

This function will stop the active scanner from scanning and close the view.
If an app requires an experience where the user presses a cancel button be
sure to call the stop() function on the scanner view. Calling the stop()
function will trigger the cancel callback.

#### turnLightOn()/turnLightOff()

These functions are used to turn the devices light on for scanning. These
function allow for use in an UI element such as a Switch. However, an app may,
by design, restrict or always turn on the device's light. If the device does
not have a light nothing happens.

Change Log

  * 1.0 Initial Release

## Author

Tony Nuzzi @ Acktie 
Twitter: @Acktie 
Email: support@acktie.com

Code licensed under Apache License v2.0, documentation under CC BY 3.0.

Libaries Used:

Portions of this software utilize the ZXing bar code reader:
  For more information you can go to: http://code.google.com/p/zxing/

Attribution is welcome but not required.