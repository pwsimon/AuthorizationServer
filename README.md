# Authorization Server
handle renew of expired oAuth access_token automaticaly and transparent (independend from control flow of the applications logic).
serialize the access and update of the tokens per DesktopSession so multiple processes can use the (oAuth) service simultanously.

## Details of this solution
clients which deal with oAuth protected API's have to handle renew of it's access_token after it has expired.
any call to an arbitrary API Method can interrupted with an HTTP 401. at this point you have to renew the token.
from an application programmers point of view it would by shieled from doing this oAuth dance because this could be handled automatically.
todo so, simply create an MSXMLHTTPRequest and ask the "Authorization Server" to perform the request an handle renew if necessary.
the renew process is asynchronous so your clients are not blocked.

because a refresh_token can be used only once a syncronization of all processes using the same service is needed.
imagine two independent processes try to exchange the same refresh token one of them will fail and can not continue.
therefore the "Authorization Server" will defere incoming requests without blocking until the first renew is finished.

this project is currently under construction (Version < 1)
i start publishing very early to train/learn how to develop with GitHub.
RoadMap:
- [X] ready to run Server component
- [X] library for easy Client development
- [X] public/cloud [simulator](http://simulatorauthserver-1310.appspot.com/) Service API for quick and easy demo
- [ ] private/cloud [simulator](http://localhost:1310/) Service API for quick and easy demo
- [X] ready to run sample (WTL/MFC) for quick and easy demo
- [ ] tool to perfom UserConsent on real Service APIs
- [ ] redistributable package (.msi merge module)
- [X] installer package

## Server component
this ATL EXE Server component is the one and only controller (single point of control) for the TokenResponse-user file.
the component is build as dll server and configured for instantiation by default surrogate DllHost.exe.
in combination with a FileMoniker the COM Runtime ensure that there is at most one instance in absence of custom code.

## client libraries
the different client libraries brings the capability of detecting expired tokens and the capability to repeat API requests.

### ATL
this client library simply consists of some template classes and is shipped as a set of .h files.

### MFC
this client library simply consists of some base classes.
this classes are shipped as .cpp and .h files not packaged as library.
you have to copy these classes into your project.

## cloud services(s)
for real-live demonstration of the (Client-Side) core components you need a WebService too.
i provide some basic services to start instantly.
all these different implementations below can be used interchangeable.

### public/cloud service
this service is implemented as JavaServlet and hosted on [Google AppEngine (PAAS)](https://console.cloud.google.com/home/).

### private/cloud service
this service is implemented as NodeJS application and the prefered solution for development or investigation.
i started this project using [Visual Studio Code](https://code.visualstudio.com/d?utm_expid=101350005-35.Eg8306GUR6SersZwpBjURQ.3&utm_referrer=https%3A%2F%2Fwww.google.de%2F) instead of [Node.js Tools 1.2 for Visual Studio 2015 (NTVS)](https://marketplace.visualstudio.com/items?itemName=NodejsToolsforVisualStudio.NodejsTools12forVisualStudio2015)

## ready to run sample(s) and services
this repository also contains a SetupProject including a prebuilt .msi for windows systems.
using this installer package and the public/cloud service you get a quick start.

### WTL
this Application is based on ATL and therefore designed to work with template classes.
i suggest to start with the WTL sample which is the most intuitive sample.

### MFC
this Application is based on MFC only and has no other requirements.
