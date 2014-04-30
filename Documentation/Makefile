
website_examples := example_asynch_exceptions example_data_structure example_lazy_initialization example_multi_dns example_signal_handling example_stream example_tight_loops some_examples_mistake1 some_examples_mistake2 some_examples_mistake3

website_html := $(website_examples) activities_compared big_four concurrency faq implementation index related_work short_version some_examples 

website_other := Activated-carbon.jpg charcoal.css concurrency_concept_map.svg

WebsiteDeploy/%.html: Website/%.php Website/copyright.html Website/code_examples.php
	bash -c "cd Website; php ../$< > ../$@"

build: $(addprefix WebsiteDeploy/,$(addsuffix .html, $(website_html)))
	cp $(addprefix Website/,$(website_other)) WebsiteDeploy

deploy:
	rsync -avz WebsiteDeploy/* "root@charcoal-lang.org:/var/www/nginx-charcoal/"

clean:
	rm -rf WebsiteDeploy/*
