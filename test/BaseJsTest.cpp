/*
  2 * This file is part of Adblock Plus <https://adblockplus.org/>,
  3 * Copyright (C) 2006-2015 Eyeo GmbH
  4 *
  5 * Adblock Plus is free software: you can redistribute it and/or modify
  6 * it under the terms of the GNU General Public License version 3 as
  7 * published by the Free Software Foundation.
  8 *
  9 * Adblock Plus is distributed in the hope that it will be useful,
  10 * but WITHOUT ANY WARRANTY; without even the implied warranty of
  11 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  12 * GNU General Public License for more details.
  13 *
  14 * You should have received a copy of the GNU General Public License
  15 * along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.
  16 */

#include "BaseJsTest.h"

AdblockPlus::JsEnginePtr createJsEngine(const AdblockPlus::AppInfo& appInfo)
{
	static AdblockPlus::ScopedV8IsolatePtr isolate = std::make_shared<AdblockPlus::ScopedV8Isolate>();
	return AdblockPlus::JsEngine::New(appInfo, isolate);
}