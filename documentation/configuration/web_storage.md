---
layout: page
title: Spectrum 2
---

### Web Storage

Web storage is used to send images and files received by Spectrum 2 to end user. When new image or file is downloaded from the 3rd-party network, it is stored into `service.web_directory` directory
and the link to that file, based on `service.web_url`, is sent to the user.

That means that you should configure the web-server on the machine where you run Spectrum 2 to serve `service.web_directory`.

If you for example use httpd, you can configure `service.web_directory=/var/www/html/spectrum2`. In that case, the matching `service.web_url` value will be `service.web_url=http://domain.tld/spectrum2`.
