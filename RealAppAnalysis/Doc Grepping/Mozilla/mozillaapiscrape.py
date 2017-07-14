#!/usr/bin/env python3

from bs4 import BeautifulSoup
from bs4 import NavigableString
from datautil import toTxt

import requests
#from nt import link
from os.path import join
import re
from bs4.element import Tag


def _log(log, indent = 1, tag = ""):
    log = str(log)
    if tag: log = tag + ": " + log
    print("\t" * indent + log)

def testGrep():
    if findAsyncBehaviorInAPI("https://developer.mozilla.org/en-US/docs/Web/API/Document"):
        print("ASYNC")

def main():
    #testGrep()
    #return
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
    artice_section = page.find("article")
    if artice_section:
        indicators = {"async", "callback", "promise"}
        for i in indicators:
            if i in artice_section.text:
                _log("Indicator - " + i, indent = 4, tag = "Potential Async")
                return True
    return False

def content(url): return BeautifulSoup(requests.get(url).content, "html.parser")

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
        return methodStrategy(method_section, base_url)
    return None

def methodStrategy(method_section, api_url):
    methods = []
    for element in method_section.next_siblings:
        if element.name == "h2":
            break
        if type(element) == Tag:
            for tag in element.find_all("dt"):
                for tag2 in tag.find_all("a"):
                    if tag2.has_attr("href"):
                        href = tag2["href"]
                        if api_url in href:
                            methods.append(href.split("/")[-1])

    return methods

def urlToAPI(common, api_name):
    return common + "/" + api_name

if __name__ == "__main__":
    main()
