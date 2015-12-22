/*
 * This file is part of Adblock Plus <https://adblockplus.org/>,
 * Copyright (C) 2006-2015 Eyeo GmbH
 *
 * Adblock Plus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * Adblock Plus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
 */

(function(global)
{
  function updateFromURL(data)
  {
    if (window.location.search)
    {
      var params = window.location.search.substr(1).split("&");
      for (var i = 0; i < params.length; i++)
      {
        var parts = params[i].split("=", 2);
        if (parts.length == 2 && parts[0] in data)
          data[parts[0]] = decodeURIComponent(parts[1]);
      }
    }
  }

  var params = {
    blockedURLs: "",
    seenDataCorruption: false,
    filterlistsReinitialized: false,
    addSubscription: false,
    filterError: false
  };
  updateFromURL(params);

  var modules = {};
  global.require = function(module)
  {
    return modules[module];
  };

  modules.utils = {
    Utils: {
      getDocLink: function(link)
      {
        return "https://adblockplus.org/redirect?link=" + encodeURIComponent(link);
      },
      get appLocale()
      {
        return parent.ext.i18n.getMessage("@@ui_locale").replace(/_/g, "-");
      }
    }
  };

  modules.prefs = {
    Prefs: {
      "subscriptions_exceptionsurl": "https://easylist-downloads.adblockplus.org/exceptionrules.txt"
    }
  };

  modules.subscriptionClasses = {
    Subscription: function(url)
    {
      this.url = url;
      this.title = "Subscription " + url;
      this.disabled = false;
      this.lastDownload = 1234;
    },

    SpecialSubscription: function(url)
    {
      this.url = url;
      this.disabled = false;
      this.filters = knownFilters.slice();
    }
  };
  modules.subscriptionClasses.Subscription.fromURL = function(url)
  {
    if (/^https?:\/\//.test(url))
      return new modules.subscriptionClasses.Subscription(url);
    else
      return new modules.subscriptionClasses.SpecialSubscription(url);
  };
  modules.subscriptionClasses.DownloadableSubscription = modules.subscriptionClasses.Subscription;

  modules.filterStorage = {
    FilterStorage: {
      get subscriptions()
      {
        var subscriptions = [];
        for (var url in modules.filterStorage.FilterStorage.knownSubscriptions)
          subscriptions.push(modules.filterStorage.FilterStorage.knownSubscriptions[url]);
        return subscriptions;
      },

      get knownSubscriptions()
      {
        return knownSubscriptions;
      },

      addSubscription: function(subscription)
      {
        if (!(subscription.url in modules.filterStorage.FilterStorage.knownSubscriptions))
        {
          knownSubscriptions[subscription.url] = modules.subscriptionClasses.Subscription.fromURL(subscription.url);
          modules.filterNotifier.FilterNotifier.triggerListeners("subscription.added", subscription);
        }
      },

      removeSubscription: function(subscription)
      {
        if (subscription.url in modules.filterStorage.FilterStorage.knownSubscriptions)
        {
          delete knownSubscriptions[subscription.url];
          modules.filterNotifier.FilterNotifier.triggerListeners("subscription.removed", subscription);
        }
      },

      addFilter: function(filter)
      {
        for (var i = 0; i < customSubscription.filters.length; i++)
        {
          if (customSubscription.filters[i].text == filter.text)
            return;
        }
        customSubscription.filters.push(filter);
        modules.filterNotifier.FilterNotifier.triggerListeners("filter.added", filter);
      },

      removeFilter: function(filter)
      {
        for (var i = 0; i < customSubscription.filters.length; i++)
        {
          if (customSubscription.filters[i].text == filter.text)
          {
            customSubscription.filters.splice(i, 1);
            modules.filterNotifier.FilterNotifier.triggerListeners("filter.removed", filter);
            return;
          }
        }
      }
    }
  };

  modules.filterClasses = {
    BlockingFilter: function() {},
    Filter: function(text)
    {
      this.text = text;
      this.disabled = false;
    }
  };
  modules.filterClasses.Filter.fromText = function(text)
  {
    return new modules.filterClasses.Filter(text);
  };

  modules.filterValidation = 
  {
    parseFilter: function(text)
    {
      if (params.filterError)
        return {error: "Invalid filter"};
      return {filter: modules.filterClasses.Filter.fromText(text)};
    },
    parseFilters: function(text)
    {
      if (params.filterError)
        return {errors: ["Invalid filter"]};
      return {
        filters: text.split("\n")
          .filter(function(filter) {return !!filter;})
          .map(modules.filterClasses.Filter.fromText),
        errors: []
      };
    }
  };

  modules.synchronizer = {
    Synchronizer: {}
  };

  modules.matcher = {
    defaultMatcher: {
      matchesAny: function(url, requestType, docDomain, thirdParty)
      {
        var blocked = params.blockedURLs.split(",");
        if (blocked.indexOf(url) >= 0)
          return new modules.filterClasses.BlockingFilter();
        else
          return null;
      }
    }
  };

  modules.cssRules = {
    CSSRules: {
      getRulesForDomain: function(domain) { }
    }
  };

  var notifierListeners = [];
  modules.filterNotifier = {
    FilterNotifier: {
      addListener: function(listener)
      {
        if (notifierListeners.indexOf(listener) < 0)
          notifierListeners.push(listener);
      },

      removeListener: function(listener)
      {
        var index = notifierListeners.indexOf(listener);
        if (index >= 0)
          notifierListeners.splice(index, 1);
      },

      triggerListeners: function()
      {
        var args = Array.prototype.slice.apply(arguments);
        var listeners = notifierListeners.slice();
        for (var i = 0; i < listeners.length; i++)
          listeners[i].apply(null, args);
      }
    }
  };

  modules.info = {
    platform: "gecko",
    platformVersion: "34.0",
    application: "firefox",
    applicationVersion: "34.0",
    addonName: "adblockplus",
    addonVersion: "2.6.7"
  };
  updateFromURL(modules.info);

  global.Services = {
    vc: {
      compare: function(v1, v2)
      {
        return parseFloat(v1) - parseFloat(v2);
      }
    }
  };

  var filters = [
    "@@||alternate.de^$document",
    "@@||der.postillion.com^$document",
    "@@||taz.de^$document",
    "@@||amazon.de^$document",
    "||biglemon.am/bg_poster/banner.jpg",
    "winfuture.de###header_logo_link",
    "###WerbungObenRechts10_GesamtDIV",
    "###WerbungObenRechts8_GesamtDIV",
    "###WerbungObenRechts9_GesamtDIV",
    "###WerbungUntenLinks4_GesamtDIV",
    "###WerbungUntenLinks7_GesamtDIV",
    "###WerbungUntenLinks8_GesamtDIV",
    "###WerbungUntenLinks9_GesamtDIV",
    "###Werbung_Sky",
    "###Werbung_Wide",
    "###__ligatus_placeholder__",
    "###ad-bereich1-08",
    "###ad-bereich1-superbanner",
    "###ad-bereich2-08",
    "###ad-bereich2-skyscrapper"
  ];
  var knownFilters = filters.map(modules.filterClasses.Filter.fromText);

  var subscriptions = [
    "https://easylist-downloads.adblockplus.org/easylistgermany+easylist.txt",
    "https://easylist-downloads.adblockplus.org/exceptionrules.txt",
    "https://easylist-downloads.adblockplus.org/fanboy-social.txt",
    "~user~786254"
  ];
  var knownSubscriptions = Object.create(null);
  for (var subscriptionUrl of subscriptions)
    knownSubscriptions[subscriptionUrl] = modules.subscriptionClasses.Subscription.fromURL(subscriptionUrl);
  var customSubscription = knownSubscriptions["~user~786254"];

  global.seenDataCorruption = params.seenDataCorruption;
  global.filterlistsReinitialized = params.filterlistsReinitialized;

  if (params.addSubscription)
  {
    // We don't know how long it will take for the page to fully load
    // so we'll post the message after one second
    setTimeout(function()
    {
      window.postMessage({
        type: "message",
        payload: {
          title: "Custom subscription",
          url: "http://example.com/custom.txt",
          type: "add-subscription"
        }
      }, "*");
    }, 1000);
  }
})(this);
