# Intro to esphttpd developement #

(Thanks to escardin, who wrote this.)


user_main.c and cgi.c provide good examples of setting up the webserver, processing input and output.

## Setting up httpd ##
You need to define a list of urls and callbacks for those urls for processing input and transforming output.
At minimum, it must end with a NULL url.
The list is processed top to bottom, so it is best if wildcards are handled at the bottom.
Example:
HttpdBuiltInUrl builtInUrls[]={
	{"/", cgiRedirect, "/index.tpl"},
	{"/led.tpl", cgiEspFsTemplate, tplLed},
	{"/index.tpl", cgiEspFsTemplate, tplCounter},
	{"/led.cgi", cgiLed, NULL},
	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};

The contents of each field is explained in URLs, CGI Callback and TPL Callback further down.

If you plan on using serial logging, you need to include stdout.c & stdout.h and call stdoutInit() before starting the httpd.

If you plan on serving up any templates or files, you need to call espFsInit. It is recommended to just place the following code unless you know what you're doing:

// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif

You should now call any other initialization routines you may need.

Now, call httpdInit() with the list of urls you created earlier and the port you wish httpd to listen on.

## URLs ##
Urls are relative to the url root and must begin with a /, unless they are *.
A * is a wildcard and will match anything from the * onwards.
The * MUST be the last character in the url pattern to be a wildcard!
Examples:
'*' will match: '/', '/stuff', 'things', '/widgets.tpl'
'/stuff*' will match: '/stuff', '/stuff/', '/stuff/widgets.tpl', '/stuff.html'

## CGI Callback ##
The specified cgi callback is called when the url it is tied to matches. 
It is passed a pointer to the connection (HttpdConnData).
It returns one of HTTPD_CGI_MORE, HTTPD_CGI_DONE, HTTPD_CGI_NOTFOUND, HTTPD_CGI_AUTHENTICATED. If one of these is not returned, it will retry this url again (probably infinitely).
HTTPD_CGI_MORE means it has handled the request, but is not done sending data yet.
HTTPD_CGI_DONE means it has handled the request and is done sending data.
HTTPD_CGI_NOTFOUND and HTTPD_CGI_AUTHENTICATED means that it has not handled the request, and the url matching should continue with the next entry.

## TPL Callback ##
The specified tpl callback is called for every token in the calling tpl file, as well as when the connection is aborted (token will be null).
It is passed a pointer to the connection data (HttpdConnData), a string token, and arguments.
If token is null, assume that the connection has died and it is time to clean up.
A token is the contents between two %. I.e. %ledstate% or %counter%.
It is the responisbility of the callback to write what should replace the token using httpdSend();
The TPL Callback is only used (so far) if you set the CGI Callback to 'cgiEspFsTemplate'.
The return callback for TPL is not currently used, and should return one of HTTPD_CGI_MORE, HTTPD_CGI_DONE, HTTPD_CGI_NOTFOUND, HTTPD_CGI_AUTHENTICATED, though the use of each value is undefined at this time.

## USEFUL BUILTIN FUNCTIONS ##

base64_decode (base64.h): TODO describe how to use this

captdnsInit (captdns.h):
	- Creates a captive dns portal listening on the default ap network

Serving files:
	- espFsInit (espfs.h): Initializes access to the espfs block of flash. See above for basic usage, not recommended to mess around.
	- espFsFlags (espfs.h): returns the flags for an opened file
	- espFsRead (espfs.h): Read len bytes from file into buffer. Returns actual number of bytes read
	- espFsClose (espfs.h): Close the file
	- espFsOpen (espfs.h): open a file and return the file handle

CGI Callbacks
	- cgiEspFsHook (httpdespfs.h): attempts to read out the requested file as is (no template processing)
	- cgiEspFsTemplate (httpdespfs.h): attempts to read out the requested file, with template processing (using the cgiArg tpl callback)
	- cgiRedirect (httpd.h): Redirect to the url specified as a cgiArg (string)
	- cgiRedirectToHostname (httpd.h): Redirect to the given http url, if it doesn't match the request url
	- cgiRedirectApClientToHostname (httpd.h): Same as cgiRedirectToHostname, but only redirects if in the AP ip range
	- authBasic (auth.h): performs http basic auth from the cgiArg callback function
		- int cgiArg(HttpdConnData connData, int userIndex,char *userOut,int userOutLength, char *passOut,int passOutLength)
		- return 1 if an entry at userIndex was found (with the username and password in userOut and passOut respectively)
		- return 0 if no entry at userIndex was found (will cause authBasic to stop processing)
	- OTA Firmware Updating
		- cgiReadFlash (cgiflash.h): returns the first 512Kbyte of firmware
		- cgiGetFirmwareNext (cgiflash.h): Returns which firmware should be uploaded next
		- cgiUploadFirmware (cgiflash.h): replace firmware with POSTed firmware, after some basic validation
		- cgiRebootFirmware (cgiflash.h): reboot after firmware finishes uploading?
	- OTA Wifi Setup
		- cgiWiFiScan (cgiwifi.h): Initiate a scan for wifi access points, and return the status/result as JSON
		- tplWlan (cgiwifi.h): Template processor for the httpd demo template page
		- cgiWiFi (cgiwifi.h): not implemented?
		- cgiWiFiConnect (cgiwifi.h): Connect to the specified essid using the passwd provided
		- cgiWiFiSetMode (cgiwifi.h): Set the esp8266 wifi mode (AP, Station, AP+Station) and reboot
		- cgiWiFiConnStatus (cgiwifi.h): Return the station connection status as JSON

Working with HTTP
	- httpdRedirect (httpd.h): issue a 302 redirect to the given url
	- httpdUrlDecode (httpd.h): decodes a percent encoded url
	- httpdFindArg (httpd.h): finds the specified argument in the get or post data, and returns the percent decoded version of it
	- httpdInit (httpd.h): Starts an http server on the given port for the given urls
	- httpdGetMimetype (httpd.h): returns the mimetype based on the url extension
	- httpdStartResponse (httpd.h): start http response (must be called first, and only once)
	- httpdHeader (httpd.h): sends a particular http response header
	- httpdEndHeaders (httpd.h): completes sending headers (no more headers may be sent)
	- httpdGetHeader (httpd.h): retrieves a specific request header
	- httpdSend (httpd.h): add data to be sent to the user, returns 1 for success, 0 for out of memory
