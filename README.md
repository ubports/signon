SignOn daemon
=============

The SignOn daemon is a D-Bus service which performs user authentication on
behalf of its clients. There are currently authentication plugins for OAuth 1.0
and 2.0, SASL, Digest-MD5, and plain username/password combination.


License
-------

See COPYING file.


Build instructions
------------------

This project depends on Qt 5. To build it, run
```
qmake
make
make install
```

Resources
---------

[libsignon-qt API reference documentation](http://accounts-sso.gitlab.io/signond/index.html)

[Official source code repository](https://gitlab.com/accounts-sso/signond)
