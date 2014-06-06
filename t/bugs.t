#vi:filetype=perl

use lib 'lib';
use Test::Nginx::Socket;

repeat_each(3);

plan tests => repeat_each(1) * blocks();
no_root_location();
no_long_string();
$ENV{TEST_NGINX_SERVROOT} = server_root();
no_shuffle();
run_tests();

#=== TEST 1: multipart, legit file

__DATA__

=== TEST 1: multipart, bad file
--- user_files
>>> foobar
eh yo
--- config
location / {
basic_rule id:1201 gids:UPLOAD mod:on regex:\.\./ msg:"special file charactors" pos:BODY lev:LOG|BLOCK;

basic_rule id:1202 gids:UPLOAD mod:on msg:"uncommon hex encoding, null bytes" pos:BODY lev:LOG|BLOCK;

basic_rule id:1203 gids:UPLOAD mod:on regex:\.jsp|\.php|\.html|\.htm msg:"uncommon filename postfix" pos:BODY  Lev:LOG|BLOCK;

basic_rule id:1204 gids:UPLOAD mod:on msg:"uncommon filename" pos:BODY lev:LOG|BLOCK;
    root $TEST_NGINX_SERVROOT/html/;
    index index.html index.htm;
    error_page 405 = $uri;
}
--- raw_request eval
"POST /foobar HTTP/1.1\r
Host: 127.0.0.1\r
Connection: Close\r
User-Agent: Mozilla/5.0 (iPad; U; CPU OS 3_2 like Mac OS X; en-us) AppleWebKit/531.21.10 (KHTML, like Gecko) Version/4.0.4 Mobile/7B334b Safari/531.21.10\r
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r
Accept-Language: en-us,en;q=0.5\r
Accept-Encoding: gzip, deflate\r
Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r
Referer: http://127.0.0.1/\r
Content-Type: multipart/form-data; boundary=---------------------------1919886344942015258287623957\r
Content-Length: 378\r
\r
-----------------------------1919886344942015258287623957\r
Content-Disposition: form-data; name=\"textline\ \r
\r
valid text and small file\r
-----------------------------1919886344942015258287623957\r
Content-Disposition: form-data; name=\"datafile\"; filename=\"bla.txt\"\r
Content-Type: text/plain\r
\r
buibuibubi
buibuibuib
\r
-----------------------------1919886344942015258287623957--\r
"
--- error_code: 200

