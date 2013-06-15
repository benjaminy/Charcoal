
website_html := big_four.html concurrency.html example_asynch_exceptions.html example_data_structure.html example_lazy_initialization.html example_multi_dns.html example_signal_handling.html example_stream.html example_tight_loops.html faq.html implementation.html index.html related_work.html short_version.html some_examples.html some_examples_mistake1.html some_examples_mistake2.html some_examples_mistake3.html

WebsiteDeploy/%.html: Website/%.php
	php $< > $@

build: $(addprefix WebsiteDeploy/,$(website_html))

deploy:
	scp Website/* "root@charcoal-lang.org:/var/www/nginx-charcoal/"
