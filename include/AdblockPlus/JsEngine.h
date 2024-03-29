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

#ifndef ADBLOCK_PLUS_JS_ENGINE_H
#define ADBLOCK_PLUS_JS_ENGINE_H

#include <functional>
#include <map>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <AdblockPlus/AppInfo.h>
#include <AdblockPlus/LogSystem.h>
#include <AdblockPlus/FileSystem.h>
#include <AdblockPlus/JsValue.h>
#include <AdblockPlus/WebRequest.h>

namespace v8
{
  class Arguments;
  class Isolate;
  class Value;
  class Context;
  template<class T> class Handle;
  template<typename T> class FunctionCallbackInfo;
  typedef void(*FunctionCallback)(const FunctionCallbackInfo<v8::Value>& info);
}

namespace AdblockPlus
{
  class JsEngine;

  /**
   * Shared smart pointer to a `JsEngine` instance.
   */
  typedef std::shared_ptr<JsEngine> JsEnginePtr;

  /**
   * Scope based isolate manager. Creates a new isolate instance on
   * constructing and disposes it on destructing.
   */
  class ScopedV8Isolate
  {
  public:
	  ScopedV8Isolate();
	  ~ScopedV8Isolate();
	  v8::Isolate* GetIsolate()
	  {
		  return isolate;
	  }
  protected:
	  v8::Isolate* isolate;
  };

/**
* Shared smart pointer to ScopedV8Isolate instance;
*/
typedef std::shared_ptr<ScopedV8Isolate> ScopedV8IsolatePtr;


  /**
   * JavaScript engine used by `FilterEngine`, wraps v8.
   */
  class JsEngine : public std::enable_shared_from_this<JsEngine>
  {
    friend class JsValue;
    friend class JsContext;

  public:
    /**
     * Event callback function.
     */
    typedef std::function<void(JsValueList& params)> EventCallback;

    /**
     * Maps events to callback functions.
     */
    typedef std::map<std::string, EventCallback> EventMap;

    /**
     * Creates a new JavaScript engine instance.
     * @param appInfo Information about the app.
     * @return New `JsEngine` instance.
     */
	static JsEnginePtr New(const AppInfo& appInfo = AppInfo(), const ScopedV8IsolatePtr& isolate = ScopedV8IsolatePtr());

    /**
     * Registers the callback function for an event.
     * @param eventName Event name. Note that this can be any string - it's a
     *        general purpose event handling mechanism.
     * @param callback Event callback function.
     */
    void SetEventCallback(const std::string& eventName, EventCallback callback);

    /**
     * Removes the callback function for an event.
     * @param eventName Event name.
     */
    void RemoveEventCallback(const std::string& eventName);

    /**
     * Triggers an event.
     * @param eventName Event name.
     * @param params Event parameters.
     */
    void TriggerEvent(const std::string& eventName, JsValueList& params);

    /**
     * Evaluates a JavaScript expression.
     * @param source JavaScript expression to evaluate.
     * @param filename Optional file name for the expression, used in error
     *        messages.
     * @return Result of the evaluated expression.
     */
    JsValuePtr Evaluate(const std::string& source,
        const std::string& filename = "");

    /**
     * Initiates a garbage collection.
     */
    void Gc();

    //@{
    /**
     * Creates a new JavaScript value.
     * @param val Value to convert.
     * @return New `JsValue` instance.
     */
    JsValuePtr NewValue(const std::string& val);
    JsValuePtr NewValue(int64_t val);
    JsValuePtr NewValue(bool val);
    inline JsValuePtr NewValue(const char* val)
    {
      return NewValue(std::string(val));
    }
    inline JsValuePtr NewValue(int val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
#ifdef __APPLE__
    inline JsValuePtr NewValue(long val)
    {
      return NewValue(static_cast<int64_t>(val));
    }
#endif
    //@}

    /**
     * Creates a new JavaScript object.
     * @return New `JsValue` instance.
     */
    JsValuePtr NewObject();

    /**
     * Creates a JavaScript function that invokes a C++ callback.
     * @param callback C++ callback to invoke. The callback receives a
     *        `v8::Arguments` object and can use `FromArguments()` to retrieve
     *        the current `JsEngine`.
     * @return New `JsValue` instance.
     */
	JsValuePtr NewCallback(v8::FunctionCallback callback);

    /**
     * Returns a `JsEngine` instance contained in a `v8::Arguments` object.
     * Use this in callbacks created via `NewCallback()` to retrieve the current
     * `JsEngine`.
     * @param arguments `v8::Arguments` object containing the `JsEngine`
     *        instance.
     * @return `JsEngine` instance from `v8::Arguments`.
     */
	static JsEnginePtr FromArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /**
     * Converts v8 arguments to `JsValue` objects.
     * @param arguments `v8::Arguments` object containing the arguments to
     *        convert.
     * @return List of arguments converted to `JsValue` objects.
     */
	JsValueList ConvertArguments(const v8::FunctionCallbackInfo<v8::Value>& arguments);

    /**
     * @see `SetFileSystem()`.
     */
    FileSystemPtr GetFileSystem();

    /**
     * Sets the `FileSystem` implementation used for all file I/O.
     * Setting this is optional, the engine will use a `DefaultFileSystem`
     * instance by default, which might be sufficient.
     * @param The `FileSystem` instance to use.
     */
    void SetFileSystem(FileSystemPtr val);

    /**
     * @see `SetWebRequest()`.
     */
    WebRequestPtr GetWebRequest();

    /**
     * Sets the `WebRequest` implementation used for XMLHttpRequests.
     * Setting this is optional, the engine will use a `DefaultWebRequest`
     * instance by default, which might be sufficient.
     * @param The `WebRequest` instance to use.
     */
    void SetWebRequest(WebRequestPtr val);

    /**
     * @see `SetLogSystem()`.
     */
    LogSystemPtr GetLogSystem();

    /**
     * Sets the `LogSystem` implementation used for logging (e.g. to handle
     * `console.log()` calls from JavaScript).
     * Setting this is optional, the engine will use a `DefaultLogSystem`
     * instance by default, which might be sufficient.
     * @param The `LogSystem` instance to use.
     */
    void SetLogSystem(LogSystemPtr val);

    /**
     * Sets a global property that can be accessed by all the scripts.
     * @param name Name of the property to set.
     * @param value Value of the property to set.
     */
    void SetGlobalProperty(const std::string& name, AdblockPlus::JsValuePtr value);

	/**
	* Returns a pointer to associated v8::Isolate.
	*/
	v8::Isolate* GetIsolate()
	{
		return isolate->GetIsolate();
	}

  private:
	explicit JsEngine(const ScopedV8IsolatePtr& isolate);
	/// Isolate must be disposed only after disposing of all objects which are
	/// using it.
	ScopedV8IsolatePtr isolate;

    FileSystemPtr fileSystem;
    WebRequestPtr webRequest;
	LogSystemPtr logSystem;
	std::unique_ptr<v8::UniquePersistent<v8::Context>> context;
    EventMap eventCallbacks;
    JsValuePtr globalJsObject;
  };
}

#endif
