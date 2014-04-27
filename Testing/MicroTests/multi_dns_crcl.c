#include <charcoal_base.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <charcoal_std_lib.h>

static char *urls[] = {
    "Facebook.com","Twitter.com","Google.com","Youtube.com",
    "Wordpress.org","Adobe.com","Blogspot.com","Wikipedia.org",
    "Linkedin.com","Wordpress.com","Yahoo.com","Amazon.com",
    "Flickr.com","Pinterest.com","Tumblr.com","W3.org",
    "Apple.com","Myspace.com","Vimeo.com","Microsoft.com",
    "Youtu.be","Qq.com","Digg.com","Baidu.com",
    "Stumbleupon.com","Addthis.com","Statcounter.com","Feedburner.com",
    "Miibeian.gov","Delicious.com","Nytimes.com","Reddit.com",
    "vWeebly.com","Bbc.co.uk","Blogger.com","Msn.com",
    "Macromedia.com","Goo.gl","Instagram.com","Gov.uk",
    "Icio.us","Yandex.ru","Cnn.com","Webs.com",
    "Google.de","T.co","Livejournal.com","Imdb.com",
    "Mail.ru","Jimdo.com","Sourceforge.net","Go.com",
    "Tinyurl.com","Vk.com","Google.co.jp","Fc2.com",
    "Free.fr","Joomla.org","Creativecommons.org","Typepad.com",
    "Networkadvertising.org","Technorati.com","Sina.com.cn","Hugedomains.com",
    "About.com","Theguardian.com","Yahoo.co.jp","Nih.gov",
    "Huffingtonpost.com","Google.co.uk","Mozilla.org","51.la",
    "Aol.com","Ebay.com","Ameblo.jp","Wsj.com",
    "Europa.eu","Taobao.com","Bing.com","Rambler.ru",
    "Guardian.co.uk","Tripod.com","Godaddy.com","Issuu.com",
    "Gnu.org","Geocities.com","Slideshare.net","Wix.com",
    "Mapquest.com","Washingtonpost.com","Homestead.com","Reuters.com",
    "163.com","Photobucket.com","Forbes.com","Clickbank.net",
    "Weibo.com","Etsy.com","Amazon.co.uk","Dailymotion.com",
    "Soundcloud.com","Usatoday.com","Yelp.com","Cnet.com",
    "Posterous.com","Telegraph.co.uk","Archive.org","Google.fr",
    "Constantcontact.com","Phoca.cz","Phpbb.com","Latimes.com",
    "E-recht24.de","Rakuten.co.jp","Amazon.de","Opera.com",
    "Miitbeian.gov.cn","Php.net","Scribd.com","Bbb.org",
    "Parallels.com","Ning.com","Dailymail.co.uk","Cdc.gov",
    "Sohu.com","Wikimedia.org","Deviantart.com","Mit.edu",
    "Sakura.ne.jp","Altervista.org","Addtoany.com","Time.com",
    "Google.it","Stanford.edu","Live.com","Alibaba.com",
    "Squidoo.com","Harvard.edu","Gravatar.com","Histats.com",
    "Nasa.gov","Npr.org","Ca.gov","Eventbrite.com",
    "Wired.com","Amazon.co.jp","Nbcnews.com","Blog.com",
    "Amazonaws.com","Bloomberg.com","Narod.ru","Blinklist.com",
    "Imageshack.us","Kickstarter.com","Hatena.ne.jp","Nifty.com",
    "Angelfire.com","Google.es","Ocn.ne.jp","Over-blog.com",
    "Dedecms.com","Google.ca","A8.net","Weather.com",
    "Pbs.org","Ibm.com","Cpanel.net","Prweb.com",
    "Bandcamp.com","Barnesandnoble.com","Mozilla.com","Noaa.gov",
    "Goo.ne.jp","Comsenz.com","Xrea.com","Cbsnews.com",
    "Foxnews.com","Discuz.net","Eepurl.com","Businessweek.com",
    "Berkeley.edu","Newsvine.com","Bluehost.com","Geocities.jp",
    "Loc.gov","Yolasite.com","Apache.org","Mashable.com",
    "Usda.gov","Nationalgeographic.com","Whitehouse.gov","Tripadvisor.com",
    "Ted.com","Sfgate.com","Biglobe.ne.jp","Epa.gov",
    "Vkontakte.ru","Oracle.com","Seesaa.net","Examiner.com",
    "Cornell.edu","Hp.com","Nps.gov","Disqus.com",
    "Alexa.com","Mysql.com","House.gov","Sphinn.com",
    "Boston.com","Techcrunch.com","Un.org","Squarespace.com",
    "Icq.com","Freewebs.com","Ezinearticles.com","Ucoz.ru",
    "Independent.co.uk","Mediafire.com","Xinhuanet.com","Google.nl",
    "Reverbnation.com","Imgur.com","Irs.gov","Webnode.com",
    "Wunderground.com","Bizjournals.com","Who.int","Soup.io",
    "Cloudflare.com","People.com.cn","Ustream.tv","Senate.gov",
    "Cbslocal.com","Ycombinator.com","Opensource.org","Spiegel.de",
    "Oaic.gov.au","Nature.com","Businessinsider.com","Drupal.org",
    "Last.fm","Privacy.gov.au","Skype.com","Wikia.com",
    "About.me","Webmd.com","Youku.com","Gmpg.org",
    "Fda.gov","Redcross.org","Github.com","Cbc.ca",
    "Umich.edu","Jugem.jp","Shinystat.com","Google.com.br",
    "Ifeng.com","Mac.com","Wiley.com","Discovery.com",
    "Topsy.com","Paypal.com","Google.cn","Surveymonkey.com",
    "Moonfruit.com","Dropbox.com","Exblog.jp","Google.pl",
    "Prnewswire.com","Ft.com","Uol.com.br","Behance.net",
    "Goodreads.com","Netvibes.com","Auda.org.au","Marketwatch.com",
    "Ed.gov","Networksolutions.com","State.gov","Sitemeter.com",
    "Liveinternet.ru","Ftc.gov","Census.gov","Quantcast.com",
    "Economist.com","Nydailynews.com","Zdnet.com","Cafepress.com",
    "Ow.ly","Meetup.com","Netscape.com","Chicagotribune.com",
    "Theatlantic.com","Google.com.au","1688.com","Skyrock.com",
    "List-manage.com","Pagesperso-orange.fr","Cdbaby.com","Friendfeed.com",
    "Ehow.com","Patch.com","Upenn.edu","Engadget.com",
    "Diigo.com","Com.com","Slashdot.org","Washington.edu",
    "Columbia.edu","Nhs.uk","Abc.net.au","Elegantthemes.com",
    "Utexas.edu","Yale.edu","Marriott.com","Bigcartel.com",
    "Ucla.edu","Usgs.gov","Jigsy.com","Hexun.com",
    "Hubpages.com","Slate.com","Purevolume.com","Umn.edu",
    "Bloglines.com","So-net.ne.jp","Wikispaces.com","Cargocollective.com",
    "Howstuffworks.com","Plala.or.jp","Infoseek.co.jp","Jiathis.com",
    "Usnews.com","Xing.com","Flavors.me","Desdev.cn",
    "Hc360.com","Usa.gov","Edublogs.org","Lycos.com",
    "Wisc.edu","Thetimes.co.uk","State.tx.us","Example.com",
    "Shareasale.com","Biblegateway.com","Is.gd","Yellowbook.com",
    "Samsung.com","Businesswire.com","G.co","Dion.ne.jp",
    "Dagondesign.com","Theglobeandmail.com","Booking.com","Storify.com",
    "Salon.com","Ucoz.com","Gizmodo.com","Psu.edu",
    "Smh.com.au","Reference.com","Sun.com","Unicef.org",
    "Devhub.com","Artisteer.com","Unesco.org","Istockphoto.com",
    "Answers.com","Trellian.com","Cocolog-nifty.com","I2i.jp",
    "T-online.de","Intel.com","1und1.de","Ebay.co.uk",
    "Sciencedaily.com","Paginegialle.it","Ask.com","Springer.com",
    "Canalblog.com","Timesonline.co.uk","De.vu","Deliciousdays.com",
    "Smugmug.com","Wufoo.com","Globo.com","Cmu.edu",
    "Domainmarket.com","Odnoklassniki.ru","Twitpic.com","Ovh.net",
    "Home.pl","Naver.com","Google.ru","Si.edu",
    "Newyorker.com","Blogs.com","Sciencedirect.com","Hibu.com",
    "Hud.gov","Hhs.gov","Dmoz.org","Dot.gov",
    "Cyberchimps.com","Google.com.hk","Jalbum.net","Craigslist.org",
    "Zimbio.com","Chronoengine.com","Cnbc.com","Uiuc.edu",
    "Vistaprint.com","Symantec.com","Prlog.org","360.cn",
    "Indiatimes.com","Mtv.com","Webeden.co.uk","Java.com",
    "Cisco.com","Japanpost.jp","4shared.com","Github.io",
    "Mayoclinic.com","Studiopress.com","Admin.ch","Virginia.edu",
    "Printfriendly.com","Mlb.com","Omniture.com","Simplemachines.org",
    "Dell.com","Accuweather.com","Princeton.edu","Fotki.com",
    "Comcast.net","Chron.com","Nyu.edu","Wp.com",
    "Merriam-webster.com","Nba.com","Shop-pro.jp","Lulu.com",
    "Furl.net","Indiegogo.com","Buzzfeed.com","Tuttocitta.it",
    "Ox.ac.uk","Mapy.cz","Army.mil","Csmonitor.com",
    "Bravesites.com","Tamu.edu","Rediff.com","Toplist.cz",
    "Yellowpages.com","Va.gov","Tiny.cc","Netlog.com",
    "Elpais.com","Oakley.com","Multiply.com","Tmall.com",
    "Hostgator.com","Nymag.com","Fema.gov","Blogtalkradio.com",
    "China.com.cn","Unblog.fr","Fastcompany.com","Earthlink.net",
    "Vinaora.com","Msu.edu","Aboutads.info","Ucsd.edu",
    "Sogou.com","Seattletimes.com","Dyndns.org","123-reg.co.uk",
    "Sbwire.com","Tinypic.com","Acquirethisname.com","Shutterfly.com",
    "Walmart.com","Pen.io","Arizona.edu","Woothemes.com",
    "Scientificamerican.com","Themeforest.net","Spotify.com","Cam.ac.uk",
    "Unc.edu","Arstechnica.com","Hao123.com","Illinois.edu",
    "Bloglovin.com","Nsw.gov.au","Ihg.com","Pcworld.com",
};

#define BLOCK_SIZE 2

int error_count = 0;

void doit( void *p )
{
    int *idx = (int *)p;
    unsigned i;
    for( i = 0; i < BLOCK_SIZE; ++i )
    {
        const char *name = urls[ *idx + i ];
        struct addrinfo *info;
        int rc = getaddrinfo_crcl( name, NULL, NULL, &info );
        if( rc )
        {
            printf( "ERROR ERROR ERROR %d %d %s\n", rc, i + *idx, name );
            ++error_count;
        }
        else
        {
            printf( "%-20s: %x\n", "flags",     info->ai_flags );
            printf( "%-20s: %d\n", "family",    info->ai_family );
            printf( "%-20s: %d\n", "socktype",  info->ai_socktype );
            printf( "%-20s: %d\n", "protocol",  info->ai_protocol );
            printf( "%-20s: %d\n", "addrlen",   info->ai_addrlen ); /* socklen_t ??? */
            printf( "%-20s: %p\n", "sockaddr",  info->ai_addr ); /* struct sockaddr * ??? */
            printf( "%-20s: %s\n", "canonname", info->ai_canonname );
            printf( "%-20s: %p\n", "next",      info->ai_next );
        }
        printf( "%d\n", rc );
        /* XXX leak! freeaddrinfo( info ); */
    }
}

#define N 40

crcl(application_main)( int argc, char **argv, char **env )
{
    crcl(activity_t) *as = (crcl(activity_t) *)malloc( N * sizeof( as[0] ) );
    size_t url_count = sizeof( urls ) / sizeof( urls[0] );
    printf( "%lu  %lu \n", sizeof( urls ), url_count );
    unsigned i;
    int multi = 1;
    for( i = 0; i < N; i += BLOCK_SIZE )
    {
        if( multi )
        {
            int *i_copy = (int *)malloc( sizeof( i_copy[0] ) );
            *i_copy = i;
            crcl(activate)( &as[i], doit, i_copy );
        }
        else
        {
            doit( &i );
        }
    }
    for( i = 0; i < N; ++i )
    {
        if( multi )
        {
            assert( !crcl(activity_join)( &as[i], NULL ) );
        }
    }
    printf( "\nERROR COUNT: %d\n", error_count );
    return 0;
}
