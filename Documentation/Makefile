

%.html: %.php
	php $< > $@

deploy:
	scp Website/* "root@charcoal-lang.org:/var/www/nginx-charcoal/"
