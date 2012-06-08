/**
 * @file
 * Daemon configuration
 */

/******************************************************************************
 * Copyright 2012, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 ******************************************************************************/
#ifndef _DAEMONCONFIG_H
#define _DAEMONCONFIG_H

#include <vector>

#include <qcc/platform.h>
#include <qcc/XmlElement.h>
#include <qcc/String.h>


/**
 *
 */
namespace ajn {

class DaemonConfig {

  public:

    /**
     * Load a configuration creating the singleton if needed.
     *
     * @param configXml  Character string for the configuration xml
     */
    static DaemonConfig* Load(const char* configXml);

    /**
     * Load a configuration creating the singleton if needed.
     *
     * @param configXml  A source containing the configuration xml
     */
    static DaemonConfig* Load(qcc::Source& configSrc);

    /**
     * Return the configuration singleton
     */
    static DaemonConfig* Access() { return singleton; }

    /**
     * Release the configuration singleton
     */
    static void Release() {
        delete singleton;
        singleton = NULL;
    }

    /**
     * Get an integer configuration value. See Get(const char* key) for more information about the
     * key.
     *
     * @param key         The key is a dotted path to a value in the XML
     * @param defaultVal  The default value if the key is not present
     */
    uint32_t Get(const char* key, uint32_t defaultVal);

    /**
     * Get a string configuration value. The key is a path name to the configuration value
     * expressed as dotted name for the nested tags with an optional attribute specifier
     * at the end separated from the dotted name by a '@' character.
     *
     * Given the configuration XML below Get("foo/value@first") will return "hello" and Get("foo/value@second")
     * returns "world". Note that the outermost tag (in this <config> is implicit and should not be specified.
     *
     * <config>
     *    <foo>
     *       <value first="hello"/>
     *       <value second="world"/>
     *    </foo>
     * </config>
     *
     * @param key   The key is a dotted path (with optional attribute) to a value in the XML
     */
    qcc::String Get(const char* key, const char* defaultVal = NULL);

    /**
     * Get a vector of configuration values that share the same key. The values are the tag
     * contents, attributes are not allowed in this case.
     *
     * @param key   The key is a dotted path to a value in the XML
     */
    std::vector<qcc::String> GetList(const char* key);

    /**
     * Check if the configuration has a specific key.
     */
    bool Has(const char* key) { return !Get(key).empty(); }

  private:

    DaemonConfig();

    ~DaemonConfig();

    qcc::XmlElement* config;

    static DaemonConfig* singleton;

};

};

#endif
