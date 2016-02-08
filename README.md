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
- ready to run Server component
- library for easy Client development
- tool to perfom UserConsent and creating config files
- redistributable package (.msi merge module)
