from bs4 import BeautifulSoup
from bs4 import NavigableString
from datautil import toTxt

import requests
from nt import link
from os.path import join
import re

def _log(log, indent = 1, tag = ""):
    log = str(log)
    if tag: log = tag + ": " + log
    print("\t" * indent + log)
    
def testGrep():
    if findAsyncBehaviorInAPI("https://developer.mozilla.org/en-US/docs/Web/API/Geolocation"):
        print("ASYNC")
    
def main():
    base_url = "https://developer.mozilla.org/en-US/docs/Web/API"
    page = content(base_url)
    links = getRelevantLinks(page, "/en-US/docs/Web/API")
    
    async_apis = []
    for link in links[5:]: 
        api_url = urlToAPI(base_url, link)
        _log(link, tag = "API")
        
        async_methods = findAsyncBehaviorInAPI(api_url)
        if async_methods:
            async_apis.append((link, async_methods))
    
def findAsyncBehaviorInAPI(api_url):
    methods = getAPIMethods(api_url)
    async_methods = []
    if methods:
        for m in methods: 
            method_url = urlToAPI(api_url, m)
            _log(m, indent = 3, tag = "METHOD")
            method_page = content(method_url)
            if hasAsyncBehavior(method_page):
                async_methods.append(m)
    
    return async_methods
    
def hasAsyncBehavior(page):
    def filterScripts(): 
        for tag in page("script"): 
            tag.extract() 
            
    filterScripts()
    page_str_repr = page.get_text()
    indicators = {"async", "callback", "promise"}
    for i in indicators:
        if i in page_str_repr: 
            _log("Indicator - " + i, indent = 4, tag = "Potential Async")
            return True
    return False

def hasAsyncBehaviorCLARA(page):
    """Does not seem to work... 
    (Won't detect async behavior f not work for geolocation)"""
    page_str_repr = str(page)
    pattern = "<script(.)*</script>"
    new_page = re.sub(pattern,"", page_str_repr, flags=re.DOTALL)
    #print (page_str_repr)
    indicators = {"async", "callback", "promise"} #
    for i in indicators:
        if i in new_page: 
            return True
    return False

def content(url): return BeautifulSoup(requests.get(url).content, "lxml")

def getRelevantLinks(soup, common_url):
    links = []
    refs = [l.get("href") for l in soup.find_all("a") if hasattr(l, "href")]
    for r in refs: 
        if common_url in r:
            links.append(r.split("/")[-1])
            
    return links

def getAPIMethods(api_url):
    api_page = content(api_url)
    method_section = api_page.find(id="Methods")
    base_url = "/en-US/docs/Web/API"
    if method_section: 
        return methodStrategyMIGUEL(method_section, base_url)
    return None

def methodStrategyMIGUEL(method_section, api_url):
    methods = []
    for element in method_section.next_siblings:
        if element.name == "h2":
            break
        
        #Apparently, some API HTML Docs don't have hypelinks in their
        #method section...
        if hasattr(element, "href"):
            for tag in element.descendants:
                if isinstance(tag, NavigableString):
                    continue
                
                if tag.has_attr("href"):
                    href = tag["href"]
                    if api_url in href:
                        methods.append(href.split("/")[-1])       
    return methods
            
def methodStrategyCLARA(method_section):
    methods = []   
    if method_section:
        end_of_methodsection = False
        current = method_section.next_element 
        print (type(current))
        
        while not end_of_methodsection:
            
            if type(current) is 'bs4.element.Tag':
                if current.name == "h2":
                    end_of_methodsection = True  
                
                if current.has_attr("href"):
                    methods.append(a["href"].split("/")[-1])
            
            current = current.next_element
            
    return methods

def urlToAPI(common, api_name): 
    return common + "/" + api_name

if __name__ == "__main__":
    main()