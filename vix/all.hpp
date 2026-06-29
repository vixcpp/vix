/**
 *
 *  @file all.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Complete public aggregation header for Vix.cpp.
 *
 *  Includes every public Vix module entry point. This header is intended
 *  for SDK validation, smoke tests, examples, and quick prototyping.
 *
 *  For production code, prefer including only the modules you need:
 *    #include <vix.hpp>
 *    #include <vix/websocket.hpp>
 *    #include <vix/db.hpp>
 *    #include <vix/kv.hpp>
 *
 *  Usage:
 *    #include <vix/all.hpp>
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_ALL_HPP
#define VIX_ALL_HPP

#include <vix/agent.hpp>
#include <vix/async.hpp>
#include <vix/cache.hpp>
#include <vix/core.hpp>
#include <vix/conversion.hpp>
#include <vix/crypto.hpp>
#include <vix/db.hpp>
#include <vix/env.hpp>
#include <vix/error.hpp>
#include <vix/fs.hpp>
#include <vix/game.hpp>
#include <vix/io.hpp>
#include <vix/json.hpp>
#include <vix/kv.hpp>
#include <vix/log.hpp>
#include <vix/middleware.hpp>
#include <vix/net.hpp>
#include <vix/orm.hpp>
#include <vix/os.hpp>
#include <vix/p2p.hpp>
#include <vix/p2p_http.hpp>
#include <vix/path.hpp>
#include <vix/process.hpp>
#include <vix/reply.hpp>
#include <vix/requests.hpp>
#include <vix/sync.hpp>
#include <vix/template.hpp>
#include <vix/tests.hpp>
#include <vix/threadpool.hpp>
#include <vix/time.hpp>
#include <vix/utils.hpp>
#include <vix/validation.hpp>
#include <vix/webrpc.hpp>
#include <vix/websocket.hpp>

#endif // VIX_ALL_HPP
