var http = require('http');
var connect = require('connect');

// parse urlencoded request bodies into req.body
var bodyParser = require('body-parser');

var app = connect()
    .use(bodyParser.urlencoded({ extended: false }))

/*
* dieser aufruf ist der CORE und ist letztlich der einzig funktionale part.
* Funktional:
* das hier gelieferte "Bearer" token ist letztlich einfach ein (ISO 8601, String) Zeitstempel.
* wenn dieser Zeitstempel aelter als 1 Minute ist liefern wir ein HTTP 401 und erwarten ein "Aktualisieren der Zugriffstoken" (grant_type=refresh_token)
*   https://docs.microsoft.com/de-de/azure/active-directory/develop/active-directory-protocols-oauth-code#refreshing-the-access-tokens
*/
    .use('/ping', function(req, res) {
        // Message Headers, https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
        // console.log('INFO: headers.authorization:', req.headers.authorization);
        var dtBearer = new Date(req.headers.authorization.substring(7));
        // console.log('bearer:', dtBearer);

        res.setHeader('content-type', 'application/xml');
        if(Date.now() < dtBearer.valueOf() + 60 * 1000)
        {
            res.writeHead(200);
            res.end('<root>token valid</root>');
        }
        else
        {
            res.writeHead(401);
            res.end('<root>token expired</root>');
        }
    })

/*
* dieser aufruf liefert UNBEDINGT einen aktualisierten dummy fuer einen aufruf von ping
* Hinweis:
* durch das UNBEDINGT kann ich diesen aufruf auch nutzen das erste/initiale
* TokenResponse-user file zu erzeugen
*/
    // The OAuth 2.0 Authorization Framework, https://tools.ietf.org/html/rfc6749#page-16
    .use('/token', function(req, res) {
        // console.log('INFO: grant_type: ' + req.body.grant_type);
        // console.log('INFO: refresh_token: ' + req.body.refresh_token);
        // console.log('INFO: client_id: ' + req.body.client_id);
        // console.log('INFO: client_secret: ' + req.body.client_secret);

        res.writeHead(200, { 'content-type': 'application/json' });
        var tokens = { 
            access_token: new Date().toISOString(),
            refresh_token: 'hurtz',
            token_type: 'bearer',
            expires_in: 60
        };
        res.end(JSON.stringify(tokens));
    });
var port = process.env.port || 1310;
http.createServer(app).listen(port);
