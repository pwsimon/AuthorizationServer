var http = require('http');
var connect = require('connect');

// parse urlencoded request bodies into req.body 
var bodyParser = require('body-parser');

var app = connect()
    .use(bodyParser.urlencoded({ extended: false }))

    // dieser aufruf ist der CORE und ist letztlich der einzig funktionale part 
    .use('/ping', function(req, res) {
        // Message Headers, https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
        console.log('INFO: headers.authorization: ' + req.headers.authorization);

        res.writeHead(200, {
            'content-type': 'application/xml'
        });
        res.end('<root>ping result</root>');
    })

    // The OAuth 2.0 Authorization Framework, https://tools.ietf.org/html/rfc6749#page-16
    .use('/token', function(req, res) {
        console.log('INFO: grant_type: ' + req.body.grant_type);
        console.log('INFO: refresh_token: ' + req.body.refresh_token);
        console.log('INFO: client_id: ' + req.body.client_id);
        console.log('INFO: client_secret: ' + req.body.client_secret);

        res.writeHead(200, { 'content-type': 'application/json' });
        var tokens = JSON.stringify({ 
            access_token: 'lala',
            refresh_token: 'hurtz',
            token_type: 'bearer',
            expires_in: 3600
        });        
        res.end(tokens);
    });
var port = process.env.port || 1310;
http.createServer(app).listen(port);
